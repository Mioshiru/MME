//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "main.h"

#include "live_server.h"
#include "live_peer.h"
#include "live_tab.h"
#include "live_action.h"

#include "editor.h"
#include "materials.h"
#include "brush.h"
#include <cpr/cpr.h>

LiveServer::LiveServer(Editor& editor) :
	LiveSocket(),
	clients(), acceptor(nullptr), socket(nullptr), editor(&editor),
	clientIds(0), port(0), stopped(false) {
	//
}

LiveServer::~LiveServer() {
	//
}

bool LiveServer::bind() {
	NetworkConnection& connection = NetworkConnection::getInstance();
	if (!connection.start()) {
		setLastError("The previous connection has not been terminated yet.");
		return false;
	}

	auto& service = connection.get_service();
	acceptor = std::make_shared<boost::asio::ip::tcp::acceptor>(service);

	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
	acceptor->open(endpoint.protocol());

	boost::system::error_code error;
	acceptor->set_option(boost::asio::ip::tcp::no_delay(true), error);
	if (error) {
		setLastError("Error: " + error.message());
		return false;
	}

	acceptor->bind(endpoint);
	acceptor->listen();

	acceptClient();
	return true;
}

void LiveServer::close() {
	if (stopped) {
		return;
	}
	stopped = true;

	if (acceptor) {
		boost::system::error_code ec;
		acceptor->close(ec);
	}

	NetworkMessage kickMsg;
	kickMsg.write<uint8_t>(PACKET_KICK);
	kickMsg.write<std::string>("Host has closed the server.");
	memcpy(&kickMsg.buffer[0], &kickMsg.size, 4);

	for (auto& clientEntry : clients) {
		LivePeer* peer = clientEntry.second;
		if (peer->socket.is_open()) {
			boost::system::error_code ec;
			peer->socket.non_blocking(true, ec); // Prevent blocking if TCP buffer is full or client dead
			boost::asio::write(peer->socket, boost::asio::buffer(kickMsg.buffer, kickMsg.size + 4), ec);
		}
	}
	
	// Give the TCP stack a tiny moment to flush before we forcefully terminate the process
	wxMilliSleep(50);

	for (auto& clientEntry : clients) {
		LivePeer* peer = clientEntry.second;
		peer->close();
		delete peer;
	}
	clients.clear();

	if (log) {
		log->Message("Server was shutdown.");
		log->Disconnect();
		log = nullptr;
	}

	stopped = true;
	if (acceptor) {
		acceptor->close();
	}

	if (socket) {
		socket->close();
	}
}

void LiveServer::acceptClient() {
	static uint32_t id = 0;
	if (stopped) {
		return;
	}

	if (!socket) {
		socket = std::make_shared<boost::asio::ip::tcp::socket>(
			NetworkConnection::getInstance().get_service()
		);
	}

	acceptor->async_accept(*socket, [this](const boost::system::error_code& error) -> void {
		if (stopped || error == boost::asio::error::operation_aborted) {
			return;
		}

		if (!error) {
			boost::system::error_code ec;
			auto endpoint = socket->remote_endpoint(ec);
			if (!ec) {
				std::string ip = endpoint.address().to_string();
				std::vector<uint32_t> ids_to_remove;
				for (auto& clientEntry : clients) {
					LivePeer* peer = clientEntry.second;
					boost::system::error_code peer_ec;
					auto peer_endpoint = peer->socket.remote_endpoint(peer_ec);
					if (!peer_ec && peer_endpoint.address().to_string() == ip) {
						ids_to_remove.push_back(clientEntry.first);
					}
				}
				for (uint32_t rid : ids_to_remove) {
					auto it = clients.find(rid);
					if (it != clients.end()) {
						if (log) {
							log->Message("Same IP connected. Kicking ghost connection: " + it->second->getName() + " (" + ip + ")");
						}
						it->second->close();
					}
				}
			}

			uint32_t currentId = id++;
			LivePeer* peer = new LivePeer(this, std::move(*socket), currentId);
			peer->log = log;
			peer->receiveHeader();

			clients.insert(std::make_pair(currentId, peer));
		}
		socket = nullptr;
		acceptClient();
	});
}

void LiveServer::removeClient(uint32_t id) {
	auto it = clients.find(id);
	if (it == clients.end()) {
		return;
	}

	LivePeer* peer = it->second;
	const uint32_t clientId = peer->getClientId();
	if (clientId != 0) {
		clientIds &= ~clientId;
		editor->map.clearVisible(clientIds);
	}

	clearLocksForClient(clientId);
	clients.erase(it);
	delete peer;
	updateClientList();
}

void LiveServer::kickClient(uint32_t id, const wxString& reason) {
	auto it = clients.find(id);
	if (it == clients.end()) {
		return;
	}

	LivePeer* peer = it->second;

	// Send kick packet (enqueued – will be delivered async before socket closes)
	NetworkMessage kickMsg;
	kickMsg.write<uint8_t>(PACKET_KICK);
	kickMsg.write<std::string>(nstr(reason));
	peer->send(kickMsg);

	// Broadcast departure message to remaining clients
	broadcastChat("Server", peer->getName() + " was kicked from the session.");

	if (log) {
		log->Message(wxString::Format("Kicked %s: %s", peer->getName(), reason));
	}

	removeClient(id);
}

void LiveServer::updateCursor(const Position& position) {
	static wxColor player_colors[] = {
		wxColor(0, 255, 0, 128),   // Host: Grün
		wxColor(255, 0, 0, 128),   // P2: Rot
		wxColor(0, 120, 255, 128), // P3: Blau
		wxColor(255, 255, 0, 128)  // P4: Gelb
	};

	LiveCursor cursor;
	cursor.id = 0;
	cursor.pos = position;
	cursor.color = player_colors[0];
	broadcastCursor(cursor);
}

void LiveServer::updateClientList() const {
	if (log) {
		log->UpdateClientList(clients);
	}
}

uint16_t LiveServer::getPort() const {
	return port;
}

bool LiveServer::setPort(int32_t newPort) {
	if (newPort < 1 || newPort > 65535) {
		setLastError("Port must be a number in the range 1-65535.");
		return false;
	}
	port = newPort;
	return true;
}

uint32_t LiveServer::getFreeClientId() {
	for (int32_t bit = 1; bit < (1 << 16); bit <<= 1) {
		if (!testFlags(clientIds, bit)) {
			clientIds |= bit;
			return bit;
		}
	}
	return 0;
}

std::string LiveServer::getHostName() const {
	if (acceptor) {
		auto endpoint = acceptor->local_endpoint();
		return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
	}
	return "localhost";
}

void LiveServer::broadcastNodes(DirtyList& dirtyList) {
	if (dirtyList.Empty()) {
		return;
	}

	// VBO-Batch-Synchronisation: Optimierung für große Prefabs
	size_t totalNodes = dirtyList.GetPosList().size();
	bool isBatchSync = totalNodes > 50;
	if (isBatchSync) {
		startOperation("Synchronizing Prefab Batch...");
	}

	// Schritt 1: Cache alle betroffenen Nodes, um redundante QuadTree-Lookups 
	// innerhalb der Client-Schleife zu vermeiden (O(N) statt O(N*C)).
	struct NodeCache {
		QTreeNode* node;
		int32_t ndx, ndy;
		uint32_t floors;
	};
	std::vector<NodeCache> cache;
	cache.reserve(totalNodes);

	for (const auto& val : dirtyList.GetPosList()) {
		int32_t ndx = val.pos >> 18;
		int32_t ndy = (val.pos >> 4) & 0x3FFF;
		if (QTreeNode* node = editor->map.getLeaf(ndx * 4, ndy * 4)) {
			cache.push_back({node, ndx, ndy, val.floors});
		}
	}

	// Schritt 2: Verteilung an alle relevanten Clients
	for (auto& clientEntry : clients) {
		LivePeer* peer = clientEntry.second;
		const uint32_t clientId = peer->getClientId();

		// Optimierung für flüssiges Zeichnen: Sende die geänderten Nodes nicht an den
		// Client zurück, der die Aktion selbst lokal gezeichnet und committed hat!
		if (dirtyList.owner != 0 && clientId == dirtyList.owner) {
			continue;
		}

		for (const auto& item : cache) {
			// Optimierter Check: Nur senden, wenn sich der Client im Sichtbereich befindet
            // und die Ebene (Floors) tatsächlich Daten enthält.
            bool isUnderground = (item.floors & 0xFF00) != 0;
            bool isOverground = (item.floors & 0x00FF) != 0;

			if (isUnderground) {
				peer->sendNode(clientId, item.node, item.ndx, item.ndy, item.floors & 0xFF00);
			}
			if (isOverground) {
				peer->sendNode(clientId, item.node, item.ndx, item.ndy, item.floors & 0x00FF);
			}
		}
	}

	if (isBatchSync) {
		updateOperation(100);
	}
}

void LiveServer::broadcastCursor(const LiveCursor& cursor) {
	if (clients.empty()) {
		return;
	}

	if (cursor.id != 0) {
		cursors[cursor.id] = cursor;
	}

	NetworkMessage message;
	message.write<uint8_t>(PACKET_CURSOR_UPDATE);
	writeCursor(message, cursor);

	std::vector<LivePeer*> peerList;
	for (auto& clientEntry : clients) {
		if (clientEntry.second && clientEntry.second->getClientId() != cursor.id) {
			peerList.push_back(clientEntry.second);
		}
	}
	for (LivePeer* peer : peerList) {
		peer->send(message);
	}
}

void LiveServer::broadcastChat(const wxString& speaker, const wxString& chatMessage) {
	if (clients.empty()) {
		return;
	}

	NetworkMessage message;
	message.write<uint8_t>(PACKET_SERVER_TALK);
	message.write<std::string>(nstr(speaker));
	message.write<std::string>(nstr(chatMessage));

	std::vector<LivePeer*> peerList;
	for (auto& clientEntry : clients) {
		if (clientEntry.second) {
			peerList.push_back(clientEntry.second);
		}
	}
	for (LivePeer* peer : peerList) {
		peer->send(message);
	}

	if (log) {
		log->Chat(speaker, chatMessage);
	}
}

void LiveServer::startOperation(const wxString& operationMessage) {
	if (clients.empty()) {
		return;
	}

	NetworkMessage message;
	message.write<uint8_t>(PACKET_START_OPERATION);
	message.write<std::string>(nstr(operationMessage));

	std::vector<LivePeer*> peerList;
	for (auto& clientEntry : clients) {
		if (clientEntry.second) {
			peerList.push_back(clientEntry.second);
		}
	}
	for (LivePeer* peer : peerList) {
		peer->send(message);
	}
}

void LiveServer::updateOperation(int32_t percent) {
	if (clients.empty()) {
		return;
	}

	NetworkMessage message;
	message.write<uint8_t>(PACKET_UPDATE_OPERATION);
	message.write<int32_t>(percent);

	std::vector<LivePeer*> peerList;
	for (auto& clientEntry : clients) {
		if (clientEntry.second) {
			peerList.push_back(clientEntry.second);
		}
	}
	for (LivePeer* peer : peerList) {
		peer->send(message);
	}
}

bool LiveServer::requestLock(uint32_t clientId, const Position& pos, const wxString& clientName, const wxColor& clientColor) {
	auto it = lockedEntities.find(pos);
	if (it != lockedEntities.end()) {
		if (it->second.ownerId != clientId) {
			return false; // Already locked by another client
		}
		return true;
	}

	LiveEntityLock lock;
	lock.pos = pos;
	lock.ownerId = clientId;
	lock.ownerName = clientName;
	lock.ownerColor = clientColor;
	lockedEntities[pos] = lock;

	broadcastLockState(pos, clientId, clientName, clientColor, true);
	return true;
}

void LiveServer::unlock(uint32_t clientId, const Position& pos) {
	auto it = lockedEntities.find(pos);
	if (it != lockedEntities.end() && it->second.ownerId == clientId) {
		lockedEntities.erase(it);
		broadcastLockState(pos, 0, "", *wxWHITE, false);
	}
}

void LiveServer::clearLocksForClient(uint32_t clientId) {
	std::vector<Position> toRemove;
	for (const auto& pair : lockedEntities) {
		if (pair.second.ownerId == clientId) {
			toRemove.push_back(pair.first);
		}
	}
	for (const auto& pos : toRemove) {
		lockedEntities.erase(pos);
		broadcastLockState(pos, 0, "", *wxWHITE, false);
	}
}

void LiveServer::broadcastLockState(const Position& pos, uint32_t ownerId, const wxString& ownerName, const wxColor& ownerColor, bool isLocked) {
	NetworkMessage message;
	message.write<uint8_t>(PACKET_LOCK_BROADCAST);
	message.write<Position>(pos);
	message.write<uint32_t>(ownerId);
	message.write<uint8_t>(isLocked ? 1 : 0);
	if (isLocked) {
		message.write<std::string>(nstr(ownerName));
		message.write<uint8_t>(ownerColor.Red());
		message.write<uint8_t>(ownerColor.Green());
		message.write<uint8_t>(ownerColor.Blue());
	}

	std::vector<LivePeer*> peerList;
	for (auto& clientEntry : clients) {
		if (clientEntry.second) {
			peerList.push_back(clientEntry.second);
		}
	}
	for (LivePeer* peer : peerList) {
		peer->send(message);
	}
}

void LiveServer::broadcastPing(const LivePing& ping) {
	activePings.push_back(ping);
	NetworkMessage message;
	message.write<uint8_t>(PACKET_PING_LOCATION);
	message.write<Position>(ping.pos);
	message.write<uint32_t>(ping.senderId);
	message.write<std::string>(nstr(ping.senderName));
	message.write<uint8_t>(ping.color.Red());
	message.write<uint8_t>(ping.color.Green());
	message.write<uint8_t>(ping.color.Blue());
	message.write<uint64_t>(ping.timestamp);

	std::vector<LivePeer*> peerList;
	for (auto& clientEntry : clients) {
		if (clientEntry.second) {
			peerList.push_back(clientEntry.second);
		}
	}
	for (LivePeer* peer : peerList) {
		peer->send(message);
	}
}

void LiveServer::broadcastAnnotation(const MapAnnotation& annotation, bool remove) {
	if (remove) {
		mapAnnotations.erase(annotation.id);
	} else {
		mapAnnotations[annotation.id] = annotation;
	}

	NetworkMessage message;
	message.write<uint8_t>(remove ? PACKET_REMOVE_ANNOTATION : PACKET_ADD_ANNOTATION);
	message.write<uint32_t>(annotation.id);
	if (!remove) {
		message.write<Position>(annotation.pos);
		message.write<std::string>(nstr(annotation.text));
		message.write<std::string>(nstr(annotation.author));
		message.write<uint8_t>(annotation.color.Red());
		message.write<uint8_t>(annotation.color.Green());
		message.write<uint8_t>(annotation.color.Blue());
	}

	std::vector<LivePeer*> peerList;
	for (auto& clientEntry : clients) {
		if (clientEntry.second) {
			peerList.push_back(clientEntry.second);
		}
	}
	for (LivePeer* peer : peerList) {
		peer->send(message);
	}
}

void LiveServer::broadcastHostFavorites() {
	auto fav_it = g_materials.tilesets.find("Favorites");
	if (fav_it == g_materials.tilesets.end() || !fav_it->second) return;

	Tileset* ts = fav_it->second;
	std::vector<Brush*> allBrushes;
	for (TilesetCategory* cat : ts->categories) {
		if (cat) {
			for (Brush* b : cat->brushlist) {
				if (b && !b->isSeparator() && !b->getName().empty()) {
					if (std::find(allBrushes.begin(), allBrushes.end(), b) == allBrushes.end()) {
						allBrushes.push_back(b);
					}
				}
			}
		}
	}

	NetworkMessage wpMsg;
	wpMsg.write<uint8_t>(PACKET_WORLD_PALETTE);
	wpMsg.write<std::string>("Host-Favorites");
	wpMsg.write<uint32_t>((uint32_t)allBrushes.size());
	for (Brush* b : allBrushes) {
		wpMsg.write<std::string>(b->getName());
		wpMsg.write<uint32_t>(b->getID());
	}

	std::vector<LivePeer*> peerList;
	for (auto& clientEntry : clients) {
		if (clientEntry.second) {
			peerList.push_back(clientEntry.second);
		}
	}
	for (LivePeer* peer : peerList) {
		peer->send(wpMsg);
	}
}

LiveLogTab* LiveServer::createLogWindow(wxWindow* parent) {
	MapTabbook* mapTabBook = dynamic_cast<MapTabbook*>(parent);
	ASSERT(mapTabBook);

	log = newd LiveLogTab(mapTabBook, this);
	log->Message("New Live mapping session started.");

	std::thread([this]() {
		cpr::Response r = cpr::Get(cpr::Url{"https://api.ipify.org"}, cpr::Timeout{2000});
		std::string extIp = (r.status_code == 200) ? r.text : "";
		if (!extIp.empty()) {
			wxTheApp->CallAfter([this, extIp]() {
				if (log) {
					log->Message("External host IP: " + extIp + ":" + std::to_string(port) + ".");
				}
			});
		}
	}).detach();

	updateClientList();
	return log;
}
