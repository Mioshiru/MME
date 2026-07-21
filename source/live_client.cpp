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

#include "live_client.h"
#include "live_tab.h"
#include "live_action.h"
#include "map_tab.h"
#include "editor.h"

#include <chrono>
#include <new>
#include <thread>
#include <wx/event.h>

LiveClient::LiveClient() :
	LiveSocket(),
	readMessage(), queryNodeList(), currentOperation(),
	resolver(nullptr), socket(nullptr), mapEditor(nullptr), reconnectAddress(),
	reconnectPort(0), latency(0), packetLossPercent(0), reconnectAttempts(0),
	pingsSent(0), pingsMissed(0), lastPingTimestamp(0), waitingForPong(false),
	reconnectScheduled(false), kickedByServer(false), connectionStatus("Disconnected"), stopped(false) 
{
}

LiveClient::~LiveClient() {
	g_gui.latencies.erase(this);
}

bool LiveClient::connect(const std::string& address, uint16_t port) {
	reconnectAddress = address;
	reconnectPort = port;
	stopped = false;
	connectionStatus = reconnectAttempts > 0 ? wxString::Format("Reconnecting (%u/3)", reconnectAttempts) : wxString("Connecting");

	NetworkConnection& connection = NetworkConnection::getInstance();
	if (!connection.start()) {
		setLastError("The previous connection has not been terminated yet.");
		return false;
	}

	auto& service = connection.get_service();
	if (!resolver) {
		resolver = std::make_shared<boost::asio::ip::tcp::resolver>(service);
	}

	socket = std::make_shared<boost::asio::ip::tcp::socket>(service);

	resolver->async_resolve(address, std::to_string(port), [this](const boost::system::error_code& error, boost::asio::ip::tcp::resolver::results_type results) -> void {
		if (error) {
			if (!scheduleReconnect("Name resolution failed: " + wxstr(error.message()))) {
				logMessage("Error: " + error.message());
			}
		} else {
			tryConnect(results);
		}
	});

	return true;
}

void LiveClient::tryConnect(const boost::asio::ip::tcp::resolver::results_type& results) {
	if (stopped) {
		return;
	}

	logMessage("Connecting to server...");

	boost::asio::async_connect(*socket, results, [this](boost::system::error_code error, const boost::asio::ip::tcp::endpoint& endpoint) -> void {
		if (error) {
			if (handleError(error)) {
				//
			} else if (scheduleReconnect(wxString::Format("Connection failed: %s", error.message()))) {
				//
			} else {
				wxTheApp->CallAfter([this]() {
					close();
					g_gui.CloseLiveEditors(this);
				});
			}
		} else {
			socket->set_option(boost::asio::ip::tcp::no_delay(true), error);
			if (error) {
				wxTheApp->CallAfter([this]() {
					close();
				});
				return;
			}
			reconnectScheduled = false;
			reconnectAttempts = 0;
			connectionStatus = "Connected";
			kickedByServer = false;
			sendHello();
			receiveHeader();
		}
	});
}

void LiveClient::close() {
	g_gui.latencies.erase(this);
	if (resolver) {
		resolver->cancel();
	}

	if (socket) {
		socket->close();
	}

	if (log) {
		log->Message("Disconnected from server.");
		log->Disconnect();
		log = nullptr;
	}

	connectionStatus = "Disconnected";
	reconnectScheduled = false;
	stopped = true;
}

bool LiveClient::handleError(const boost::system::error_code& error) {
	if (error == boost::asio::error::eof || error == boost::asio::error::connection_reset) {
		wxTheApp->CallAfter([this, error]() {
			if (error == boost::asio::error::connection_reset || !scheduleReconnect(wxString() + getHostName() + ": disconnected.")) {
				if (log) {
					log->Message(wxString() + getHostName() + ": disconnected.");
				}
				close();
				g_gui.CloseLiveEditors(this);
				mapEditor = nullptr;
			}
		});
		return true;
	} else if (error == boost::asio::error::connection_aborted) {
		logMessage("You have left the server.");
		connectionStatus = "Disconnected";
		return true;
	}
	return false;
}

bool LiveClient::scheduleReconnect(const wxString& reason) {
	if (kickedByServer || reconnectScheduled || reconnectAddress.empty()) {
		return false;
	}

	if (reconnectAttempts >= 3) {
		return false;
	}

	if (resolver) {
		resolver->cancel();
	}
	if (socket && socket->is_open()) {
		boost::system::error_code ignored;
		socket->close(ignored);
	}

	reconnectScheduled = true;
		connectionStatus = wxString::Format("Reconnecting (%u/3)", reconnectAttempts + 1);
	if (log) {
		log->Message(reason);
		log->Message(connectionStatus + "...");
	}

	std::thread([this]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(1500));
		wxTheApp->CallAfter([this]() {
			attemptReconnect();
		});
	}).detach();
	return true;
}

void LiveClient::attemptReconnect() {
	if (kickedByServer || stopped) {
		return;
	}

	reconnectScheduled = false;
		++reconnectAttempts;
	resetConnectionMetrics();
	connect(reconnectAddress, reconnectPort);
}

void LiveClient::resetConnectionMetrics() {
	latency = 0;
	packetLossPercent = 0;
	pingsSent = 0;
	pingsMissed = 0;
	lastPingTimestamp = 0;
	waitingForPong = false;
	g_gui.latencies[this] = 0;
}

std::string LiveClient::getHostName() const {
	if (!socket) {
		return "not connected";
	}
	return socket->remote_endpoint().address().to_string();
}

void LiveClient::receiveHeader() {
	if (stopped || !socket || !socket->is_open()) {
		return;
	}
	readMessage.position = 0;
	boost::asio::async_read(*socket, boost::asio::buffer(readMessage.buffer, 4), [this](const boost::system::error_code& error, size_t bytesReceived) -> void {
		if (error) {
			if (!handleError(error)) {
				logMessage(wxString() + getHostName() + ": " + error.message());
			}
		} else if (bytesReceived < 4) {
			logMessage(wxString() + getHostName() + ": Could not receive header[size: " + std::to_string(bytesReceived) + "], disconnecting client.");
		} else {
			receive(readMessage.read<uint32_t>());
		}
	});
}

void LiveClient::receive(uint32_t packetSize) {
	readMessage.buffer.resize(readMessage.position + packetSize);
	boost::asio::async_read(*socket, boost::asio::buffer(&readMessage.buffer[readMessage.position], packetSize), [this](const boost::system::error_code& error, size_t bytesReceived) -> void {
		if (error) {
			if (!handleError(error)) {
				logMessage(wxString() + getHostName() + ": " + error.message());
			}
		} else if (bytesReceived < readMessage.buffer.size() - 4) {
			logMessage(wxString() + getHostName() + ": Could not receive packet[size: " + std::to_string(bytesReceived) + "], disconnecting client.");
		} else {
			NetworkMessage msg = std::move(readMessage);
			readMessage.clear();
			wxTheApp->CallAfter([this, msg = std::move(msg)]() mutable {
				parsePacket(msg);
				receiveHeader();
			});
		}
	});
}

void LiveClient::send(NetworkMessage& message) {
	memcpy(&message.buffer[0], &message.size, 4);
	boost::asio::async_write(*socket, boost::asio::buffer(message.buffer, message.size + 4), [this](const boost::system::error_code& error, size_t bytesTransferred) -> void {
		if (error) {
			logMessage(wxString() + getHostName() + ": " + error.message());
		}
	});
}

void LiveClient::updateCursor(const Position& position) {
	LiveCursor cursor;
	cursor.id = 77; // Unimportant, server fixes it for us
	cursor.pos = position;
	cursor.color = wxColor(
		g_settings.getInteger(Config::CURSOR_RED),
		g_settings.getInteger(Config::CURSOR_GREEN),
		g_settings.getInteger(Config::CURSOR_BLUE),
		g_settings.getInteger(Config::CURSOR_ALPHA)
	);

	NetworkMessage message;
	message.write<uint8_t>(PACKET_CLIENT_UPDATE_CURSOR);
	writeCursor(message, cursor);

	send(message);
}

LiveLogTab* LiveClient::createLogWindow(wxWindow* parent) {
	MapTabbook* mtb = dynamic_cast<MapTabbook*>(parent);
	ASSERT(mtb);

	log = newd LiveLogTab(mtb, this);
	log->Message("New Live mapping session started.");

	return log;
}

MapTab* LiveClient::createEditorWindow() {
	MapTabbook* mtb = dynamic_cast<MapTabbook*>(g_gui.tabbook);
	ASSERT(mtb);

	MapTab* edit = newd MapTab(mtb, mapEditor);
	edit->OnSwitchEditorMode(g_gui.IsSelectionMode() ? SELECTION_MODE : DRAWING_MODE);

	return edit;
}

void LiveClient::sendHello() {
	NetworkMessage message;
	message.write<uint8_t>(PACKET_HELLO_FROM_CLIENT);
	message.write<uint32_t>(__RME_VERSION_ID__);
	message.write<uint32_t>(__LIVE_NET_VERSION__);
	message.write<uint32_t>(g_gui.GetCurrentVersionID());
	message.write<std::string>(nstr(name));
	message.write<std::string>(nstr(password));

	send(message);
}

void LiveClient::sendNodeRequests() {
	static uint64_t last_ping_time = 0;
	uint64_t now = wxGetLocalTimeMillis().GetValue();
	if (now - last_ping_time > 2000) {
		if (waitingForPong && lastPingTimestamp != 0 && now - lastPingTimestamp > 4000) {
			++pingsMissed;
			packetLossPercent = pingsSent == 0 ? 0 : (pingsMissed * 100U) / pingsSent;
			connectionStatus = "Unstable";
			waitingForPong = false;
		}

		NetworkMessage msg;
		msg.write<uint8_t>(PACKET_PING);
		msg.write<uint64_t>(now);
		msg.write<uint32_t>(g_gui.latencies[this]);
		msg.write<uint32_t>(packetLossPercent);
		send(msg);
		++pingsSent;
		lastPingTimestamp = now;
		waitingForPong = true;
		last_ping_time = now;
	}

	if (queryNodeList.empty()) {
		return;
	}

	NetworkMessage message;
	message.write<uint8_t>(PACKET_REQUEST_NODES);

	message.write<uint32_t>(queryNodeList.size());
	for (uint32_t node : queryNodeList) {
		message.write<uint32_t>(node);
	}

	send(message);
	queryNodeList.clear();
}

void LiveClient::requestTileChange(Action* action) {
	mapWriter.reset();
	mapWriter.addNode(0x00); // Root container node
	for (Change* change : action->getChanges()) {
		if (change->getType() == CHANGE_TILE) {
			Tile* newtile = reinterpret_cast<Tile*>(change->getData());
			if (newtile) {
				const Position& position = newtile->getPosition();
				sendTile(mapWriter, newtile, &position);
			}
		}
	}
	mapWriter.endNode();

	NetworkMessage message;
	message.write<uint8_t>(PACKET_CHANGE_LIST);

	std::string data(reinterpret_cast<const char*>(mapWriter.getMemory()), mapWriter.getSize());
	message.write<std::string>(data);

	send(message);
}

void LiveClient::requestBatchChange(BatchAction* batchAction) {
	mapWriter.reset();
	mapWriter.addNode(0x00); // Root container node
	for (Action* action : batchAction->getActions()) {
		for (Change* change : action->getChanges()) {
			if (change->getType() == CHANGE_TILE) {
				Tile* newtile = reinterpret_cast<Tile*>(change->getData());
				if (newtile) {
					const Position& position = newtile->getPosition();
					sendTile(mapWriter, newtile, &position);
				}
			}
		}
	}
	mapWriter.endNode();

	NetworkMessage message;
	message.write<uint8_t>(PACKET_CHANGE_LIST);

	std::string data(reinterpret_cast<const char*>(mapWriter.getMemory()), mapWriter.getSize());
	message.write<std::string>(data);

	send(message);
}

void LiveClient::sendChanges(DirtyList& dirtyList) {
	ChangeList& changeList = dirtyList.GetChanges();
	if (changeList.empty()) {
		return;
	}

	mapWriter.reset();
	mapWriter.addNode(0x00); // Root container node
	for (Change* change : changeList) {
		switch (change->getType()) {
			case CHANGE_TILE: { 
				const Position& position = static_cast<Tile*>(change->getData())->getPosition();
				Tile* tile = mapEditor->map.getTile(position);
				if (tile) {
					sendTile(mapWriter, tile, &position);
				}
				break;
			}
			default:
				break;
		}
	}
	mapWriter.endNode();

	NetworkMessage message;
	message.write<uint8_t>(PACKET_CHANGE_LIST);

	std::string data(reinterpret_cast<const char*>(mapWriter.getMemory()), mapWriter.getSize());
	message.write<std::string>(data);

	send(message);
}

void LiveClient::sendChat(const wxString& message) {
	if (!socket || !socket->is_open()) {
		return;
	}

	NetworkMessage msg;
	msg.write<uint8_t>(PACKET_CLIENT_TALK);
	msg.write<std::string>(nstr(message));
	send(msg);
}

void LiveClient::sendReady() {
	NetworkMessage message;
	message.write<uint8_t>(PACKET_READY_CLIENT);
	send(message);
}

void LiveClient::queryNode(int32_t ndx, int32_t ndy, bool underground) {
	uint32_t nd = 0;
	nd |= ((ndx >> 2) << 18);
	nd |= ((ndy >> 2) << 4);
	nd |= (underground ? 1 : 0);
	queryNodeList.insert(nd);
}

void LiveClient::parsePacket(NetworkMessage message) {
	bool needsRefresh = false;
	uint8_t packetType;
	while (message.position < message.buffer.size()) {
		packetType = message.read<uint8_t>();
		switch (packetType) {
			case PACKET_HELLO_FROM_SERVER:
				parseHello(message);
				break;
			case PACKET_KICK:
				parseKick(message);
				break;
			case PACKET_ACCEPTED_CLIENT:
				parseClientAccepted(message);
				break;
			case PACKET_CHANGE_CLIENT_VERSION:
				parseChangeClientVersion(message);
				break;
			case PACKET_SERVER_TALK:
				parseServerTalk(message);
				break;
			case PACKET_NODE:
				parseNode(message);
				needsRefresh = true;
				break;
			case PACKET_CURSOR_UPDATE:
				parseCursorUpdate(message);
				needsRefresh = true;
				break;
			case PACKET_START_OPERATION:
				parseStartOperation(message);
				break;
			case PACKET_UPDATE_OPERATION:
				parseUpdateOperation(message);
				break;
			case PACKET_LOCK_BROADCAST:
				parseLockBroadcast(message);
				needsRefresh = true;
				break;
			case PACKET_LOCK_REJECT:
				parseLockReject(message);
				break;
			case PACKET_PING_LOCATION:
				parsePingLocation(message);
				needsRefresh = true;
				break;
			case PACKET_ADD_ANNOTATION:
				parseAddAnnotation(message);
				needsRefresh = true;
				break;
			case PACKET_REMOVE_ANNOTATION:
				parseRemoveAnnotation(message);
				needsRefresh = true;
				break;
			case PACKET_PONG: {
				uint64_t timestamp = message.read<uint64_t>();
				latency = (uint32_t)(wxGetLocalTimeMillis().GetValue() - timestamp);
				g_gui.latencies[this] = latency;
				packetLossPercent = pingsSent == 0 ? 0 : (pingsMissed * 100U) / pingsSent;
				waitingForPong = false;
				connectionStatus = packetLossPercent > 20 ? "Unstable" : "Connected";
				break;
			}
			default: {
				if (log) {
					log->Message(wxString::Format("Unknown packet received: 0x%02X", packetType));
				}
				close();
				return; // Stop processing after close
			}
		}
	}

	if (needsRefresh) {
		g_gui.RefreshView();
		g_gui.UpdateMinimap();
	}
}

void LiveClient::parseHello(NetworkMessage& message) {
	logMessage("Connected to server.");
	if (!mapEditor) {
		mapEditor = newd Editor(g_gui.copybuffer, this);
	} else {
		mapEditor->selection.clear();
		mapEditor->map.~Map();
		new (&mapEditor->map) Map();
	}

	Map& map = mapEditor->map;
	map.setName("Live Map - " + message.read<std::string>());
	map.setWidth(message.read<uint16_t>());
	map.setHeight(message.read<uint16_t>());
	Position focusPos = message.read<Position>();
	map.clearChanges();

	MapVersion ver;
	ver.otbm = g_gui.GetCurrentVersion().getPrefferedMapVersionID();
	ver.client = g_gui.GetCurrentVersionID();
	map.convert(ver);
	this->mapVersion = VirtualIOMap(ver);

	if (reconnectAttempts == 0) {
		MapTab* tab = createEditorWindow();
		if (tab) {
			tab->SetScreenCenterPosition(focusPos);
		}
	} else {
		g_gui.SetScreenCenterPosition(focusPos);
		g_gui.RefreshView();
		g_gui.UpdateMinimap();
	}
}

void LiveClient::parseKick(NetworkMessage& message) {
	const std::string& kickMessage = message.read<std::string>();
	kickedByServer = true;
	connectionStatus = "Disconnected";
	close();
	g_gui.CloseLiveEditors(this);
	mapEditor = nullptr;

	g_gui.PopupDialog("Disconnected", wxstr(kickMessage), wxOK);
}

void LiveClient::parseClientAccepted(NetworkMessage& message) {
	sendReady();

	// Resync active map view nodes on reconnect
	int map_x = 0, map_y = 0;
	MapTab* activeTab = dynamic_cast<MapTab*>(g_gui.GetCurrentTab());
	if (activeTab) {
		Position centerPos = activeTab->GetScreenCenterPosition();
		map_x = centerPos.x;
		map_y = centerPos.y;
	}
	int min_ndx = std::max(0, (map_x - 100) / 4);
	int max_ndx = (map_x + 100) / 4;
	int min_ndy = std::max(0, (map_y - 100) / 4);
	int max_ndy = (map_y + 100) / 4;

	for (int ndx = min_ndx; ndx <= max_ndx; ++ndx) {
		for (int ndy = min_ndy; ndy <= max_ndy; ++ndy) {
			queryNode(ndx, ndy, false);
			queryNode(ndx, ndy, true);
		}
	}
	sendNodeRequests();
}

void LiveClient::parseChangeClientVersion(NetworkMessage& message) {
	ClientVersionID clientVersion = static_cast<ClientVersionID>(message.read<uint32_t>());
	wxString versionName = i2ws(clientVersion);
	if (message.position < message.buffer.size()) {
		versionName = wxstr(message.read<std::string>());
	}
	if (!g_gui.CloseAllEditors()) {
		close();
		return;
	}
	mapEditor = nullptr;

	wxString error;
	wxArrayString warnings;
	if (!g_gui.LoadVersion(clientVersion, error, warnings)) {
		close();
		g_gui.PopupDialog("Version Mismatch", "The host requires client version '" + versionName + "', but the assets could not be loaded.\n\n" + error, wxOK);
		return;
	}
	if (!warnings.empty()) {
		g_gui.ListDialog("Version Warnings", warnings);
	}

	MapVersion ver;
	ver.otbm = g_gui.GetCurrentVersion().getPrefferedMapVersionID();
	ver.client = clientVersion;
	this->mapVersion = VirtualIOMap(ver);

	sendReady();
}

void LiveClient::parseServerTalk(NetworkMessage& message) {
	const std::string& speaker = message.read<std::string>();
	const std::string& chatMessage = message.read<std::string>();
	if (log) {
		log->Chat(
			wxstr(speaker),
			wxstr(chatMessage)
		);
		if (speaker == "Server") {
			log->Message(wxstr(chatMessage));
		}
	}
	g_gui.AddChatMessage(speaker, chatMessage);
}

void LiveClient::parseNode(NetworkMessage& message) {
	uint32_t ind = message.read<uint32_t>();

	// Extract node position
	int32_t ndx = ind >> 18;
	int32_t ndy = (ind >> 4) & 0x3FFF;
	bool underground = ind & 1;

	if (!mapEditor) {
		if (log) { log->Message("Warning: Received node before map was initialized."); }
		return;
	}

	Action* action = mapEditor->actionQueue->createAction(ACTION_REMOTE);
	receiveNode(message, *mapEditor, action, ndx, ndy, underground);
	mapEditor->actionQueue->addAction(action);
}

void LiveClient::parseCursorUpdate(NetworkMessage& message) {
	LiveCursor cursor = readCursor(message);
	cursors[cursor.id] = cursor;

	if (followClientId != 0 && cursor.id == followClientId) {
		g_gui.SetScreenCenterPosition(cursor.pos);
	}
}

void LiveClient::parseStartOperation(NetworkMessage& message) {
	const std::string& operation = message.read<std::string>();

	currentOperation = wxstr(operation);
	g_gui.SetStatusText("Server Operation in Progress: " + currentOperation + "... (0%)");
}

void LiveClient::parseUpdateOperation(NetworkMessage& message) {
	int32_t percent = message.read<uint32_t>();
	if (percent >= 100) {
		g_gui.SetStatusText("Server Operation Finished.");
	} else {
		g_gui.SetStatusText("Server Operation in Progress: " + currentOperation + "... (" + std::to_string(percent) + "%)");
	}
}

void LiveClient::requestLock(const Position& pos) {
	NetworkMessage message;
	message.write<uint8_t>(PACKET_LOCK_ENTITY);
	message.write<Position>(pos);
	send(message);
}

void LiveClient::sendUnlock(const Position& pos) {
	NetworkMessage message;
	message.write<uint8_t>(PACKET_UNLOCK_ENTITY);
	message.write<Position>(pos);
	send(message);
}

void LiveClient::sendPing(const Position& pos) {
	NetworkMessage message;
	message.write<uint8_t>(PACKET_PING_LOCATION);
	message.write<Position>(pos);
	send(message);
}

void LiveClient::sendAddAnnotation(const Position& pos, const wxString& text) {
	static uint32_t nextId = 1;
	NetworkMessage message;
	message.write<uint8_t>(PACKET_ADD_ANNOTATION);
	message.write<uint32_t>(nextId++);
	message.write<Position>(pos);
	message.write<std::string>(nstr(text));
	send(message);
}

void LiveClient::sendRemoveAnnotation(uint32_t id) {
	NetworkMessage message;
	message.write<uint8_t>(PACKET_REMOVE_ANNOTATION);
	message.write<uint32_t>(id);
	send(message);
}

void LiveClient::sendStatusUpdate(UserStatus status) {
	localStatus = status;
	NetworkMessage message;
	message.write<uint8_t>(PACKET_UPDATE_STATUS);
	message.write<uint8_t>(static_cast<uint8_t>(status));
	send(message);
}

void LiveClient::updateActivity() {
	lastUserActivityTime = wxGetLocalTimeMillis().GetValue();
	if (localStatus == USER_STATUS_AFK) {
		sendStatusUpdate(USER_STATUS_ACTIVE);
	}
}

void LiveClient::parseLockBroadcast(NetworkMessage& message) {
	Position pos = message.read<Position>();
	uint32_t ownerId = message.read<uint32_t>();
	bool isLocked = message.read<uint8_t>() != 0;
	if (isLocked) {
		LiveEntityLock lock;
		lock.pos = pos;
		lock.ownerId = ownerId;
		lock.ownerName = wxstr(message.read<std::string>());
		uint8_t r = message.read<uint8_t>();
		uint8_t g = message.read<uint8_t>();
		uint8_t b = message.read<uint8_t>();
		lock.ownerColor = wxColor(r, g, b, 255);
		lockedEntities[pos] = lock;
	} else {
		lockedEntities.erase(pos);
	}
}

void LiveClient::parseLockReject(NetworkMessage& message) {
	Position pos = message.read<Position>();
	std::string ownerName = message.read<std::string>();
	g_gui.SetStatusText(wxString::Format("Zugriff verweigert: Diese Position wird gerade von '%s' bearbeitet!", wxstr(ownerName)));
	wxMessageBox(wxString::Format("Diese Position/Eigenschaft wird derzeit von '%s' bearbeitet!", wxstr(ownerName)), "Sperre aktiv", wxOK | wxICON_WARNING);
}

void LiveClient::parsePingLocation(NetworkMessage& message) {
	Position pos = message.read<Position>();
	uint32_t senderId = message.read<uint32_t>();
	std::string senderName = message.read<std::string>();
	uint8_t r = message.read<uint8_t>();
	uint8_t g = message.read<uint8_t>();
	uint8_t b = message.read<uint8_t>();
	uint64_t timestamp = message.read<uint64_t>();

	LivePing ping;
	ping.pos = pos;
	ping.senderId = senderId;
	ping.senderName = wxstr(senderName);
	ping.color = wxColor(r, g, b, 255);
	ping.timestamp = timestamp;
	activePings.push_back(ping);
}

void LiveClient::parseAddAnnotation(NetworkMessage& message) {
	MapAnnotation annotation;
	annotation.id = message.read<uint32_t>();
	annotation.pos = message.read<Position>();
	annotation.text = wxstr(message.read<std::string>());
	annotation.author = wxstr(message.read<std::string>());
	uint8_t r = message.read<uint8_t>();
	uint8_t g = message.read<uint8_t>();
	uint8_t b = message.read<uint8_t>();
	annotation.color = wxColor(r, g, b, 255);
	mapAnnotations[annotation.id] = annotation;
}

void LiveClient::touchActivity() {
	lastUserActivityTime = wxGetLocalTimeMillis().GetValue();
	if (localStatus == USER_STATUS_AFK) {
		sendStatusUpdate(USER_STATUS_ACTIVE);
	}
}

void LiveClient::checkInactivity() {
	if (localStatus == USER_STATUS_ACTIVE) {
		uint64_t now_ms = wxGetLocalTimeMillis().GetValue();
		if (lastUserActivityTime > 0 && (now_ms - lastUserActivityTime > 300000)) {
			sendStatusUpdate(USER_STATUS_AFK);
		}
	}
}

void LiveClient::parseRemoveAnnotation(NetworkMessage& message) {
	uint32_t id = message.read<uint32_t>();
	mapAnnotations.erase(id);
}
