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
#include "radio_player.h"
#include "materials.h"
#include "tileset.h"
#include "brush.h"
#include "map.h"

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
	resetConnectionMetrics();

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
			socket->set_option(boost::asio::socket_base::keep_alive(true), error);
			socket->set_option(boost::asio::socket_base::send_buffer_size(131072), error);
			socket->set_option(boost::asio::socket_base::receive_buffer_size(131072), error);
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
	stopped = true;
	{
		std::lock_guard<std::mutex> lock(writeMutex);
		writeQueue.clear();
	}
	g_gui.latencies.erase(this);
	if (resolver) {
		boost::system::error_code ec;
		resolver->cancel();
	}

	if (socket) {
		boost::system::error_code ec;
		socket->cancel(ec);
		socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		socket->close(ec);
	}

	if (log) {
		log->Message("Disconnected from server.");
		log->Disconnect();
		log = nullptr;
	}

	connectionStatus = "Disconnected";
	reconnectScheduled = false;
	hasCreatedEditorTab = false;
}

bool LiveClient::handleError(const boost::system::error_code& error) {
	if (error) {
		wxTheApp->CallAfter([this, error]() {
			if (!scheduleReconnect(wxString::Format("%s: disconnected (%s).", getHostName(), error.message()))) {
				if (log) {
					log->Message(wxString::Format("%s: disconnected (%s).", getHostName(), error.message()));
				}
				close();
				g_gui.CloseLiveEditors(this);
				mapEditor = nullptr;
			}
		});
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
	boost::system::error_code ec;
	auto endpoint = socket->remote_endpoint(ec);
	if (ec) {
		return "Unknown IP";
	}
	return endpoint.address().to_string();
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
				if (stopped) return;
				try {
					parsePacket(msg);
				} catch (const std::exception& e) {
					logMessage(wxString::Format("Error parsing packet from server: %s", e.what()));
				} catch (...) {
					logMessage("Unknown error parsing packet from server.");
				}
				if (!stopped && socket && socket->is_open()) {
					receiveHeader();
				}
			});
		}
	});
}

void LiveClient::send(NetworkMessage& message) {
	if (!socket || !socket->is_open() || stopped) {
		return;
	}
	memcpy(&message.buffer[0], &message.size, 4);

	std::lock_guard<std::mutex> lock(writeMutex);
	writeQueue.emplace_back(message.buffer.begin(), message.buffer.begin() + message.size + 4);

	if (!isWriting) {
		isWriting = true;
		doWrite();
	}
}

void LiveClient::doWrite() {
	// Caller or completion handler ensures isWriting is true when calling doWrite.
	if (writeQueue.empty() || !socket || !socket->is_open() || stopped) {
		isWriting = false;
		return;
	}

	auto buffer = std::make_shared<std::vector<uint8_t>>(std::move(writeQueue.front()));
	writeQueue.pop_front();

	boost::asio::async_write(
		*socket,
		boost::asio::buffer(*buffer),
		[this, buffer](const boost::system::error_code& error, size_t /*bytesTransferred*/) {
			if (error) {
				{
					std::lock_guard<std::mutex> lock(writeMutex);
					writeQueue.clear();
					isWriting = false;
				}
				boost::system::error_code err = error;
				wxTheApp->CallAfter([this, err]() {
					if (!handleError(err)) {
						logMessage(wxString() + getHostName() + ": " + err.message());
					}
				});
			} else {
				std::lock_guard<std::mutex> lock(writeMutex);
				if (!writeQueue.empty() && socket && socket->is_open() && !stopped) {
					doWrite();
				} else {
					isWriting = false;
				}
			}
		});
}

void LiveClient::updateCursor(const Position& position) {
	static Position last_sent_pos(-1, -1, -1);
	static uint64_t last_sent_time = 0;
	uint64_t now = wxGetLocalTimeMillis().GetValue();

	if (position == last_sent_pos && (now - last_sent_time < 500)) {
		return;
	}
	if (now - last_sent_time < 20) {
		return;
	}
	last_sent_pos = position;
	last_sent_time = now;

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
	if (!mtb) {
		return nullptr;
	}

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

void LiveClient::sendHeartbeat() {
	if (!socket || !socket->is_open()) return;
	static uint64_t last_ping_time = 0;
	uint64_t now = wxGetLocalTimeMillis().GetValue();
	if (now - last_ping_time >= 1000) {
		if (waitingForPong && lastPingTimestamp != 0 && now - lastPingTimestamp > 3000) {
			++pingsMissed;
			packetLossPercent = pingsSent == 0 ? 0 : (pingsMissed * 100U) / pingsSent;
			connectionStatus = "Unstable";
			waitingForPong = false;
		}

		NetworkMessage msg;
		msg.write<uint8_t>(PACKET_PING);
		msg.write<uint64_t>(now);
		msg.write<uint32_t>(latency);
		msg.write<uint32_t>(packetLossPercent);
		send(msg);
		if (log) {
			log->Message(wxString::Format("[Client] Sending Ping, current latency: %u ms", latency));
		}
		++pingsSent;
		lastPingTimestamp = now;
		waitingForPong = true;
		last_ping_time = now;
	}
}

void LiveClient::sendNodeRequests() {
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
				if (currentOperation.empty()) {
					needsRefresh = true;
				}
				break;
			case PACKET_CURSOR_UPDATE:
				parseCursorUpdate(message);
				if (currentOperation.empty()) {
					needsRefresh = true;
				}
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
			case PACKET_TOWN_LIST:
				parseTownList(message);
				needsRefresh = true;
				break;
			case PACKET_WORLD_PALETTE:
				parseWorldPalette(message);
				break;
			case PACKET_APPROVAL_RESPONSE:
				parseApprovalResponse(message);
				needsRefresh = true;
				break;
			case PACKET_PONG: {
				uint64_t timestamp = message.read<uint64_t>();
				latency = (uint32_t)(wxGetLocalTimeMillis().GetValue() - timestamp);
				g_gui.latencies[this] = latency;
				if (log) {
					log->Message(wxString::Format("[Client] Received Pong, computed latency: %u ms", latency));
				}
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
	if (mapEditor) {
		delete mapEditor;
	}
	mapEditor = newd Editor(g_gui.copybuffer, this);

	Map& map = mapEditor->map;
	map.setName("Live Map - " + message.read<std::string>());
	map.setWidth(message.read<uint16_t>());
	map.setHeight(message.read<uint16_t>());
	pendingFocusPos = message.read<Position>();

	uint32_t hostViewFlags = 0;
	bool hasHostViewFlags = false;
	if (message.position + sizeof(uint32_t) <= message.buffer.size()) {
		hostViewFlags = message.read<uint32_t>();
		hasHostViewFlags = true;
	}

	map.clearChanges();

	MapVersion ver;
	ver.otbm = g_gui.GetCurrentVersion().getPrefferedMapVersionID();
	ver.client = g_gui.GetCurrentVersionID();
	map.convert(ver);
	this->mapVersion = VirtualIOMap(ver);

	g_settings.setInteger(Config::USE_AUTOMAGIC, 1);
	if (g_gui.root) {
		g_gui.root->UpdateMenubar();
	}

	if (hasHostViewFlags) {
		std::string srvKey = getHostName();
		if (srvKey.empty() || srvKey == "Unknown IP" || srvKey == "not connected") {
			if (!reconnectAddress.empty()) srvKey = reconnectAddress;
		}

		std::string adoptedList = g_settings.getString(Config::MULTIPLAYER_ADOPTED_SERVERS);
		bool alreadyAnswered = false;
		int savedDecision = -1; // 1 = adopt, 0 = keep

		if (!adoptedList.empty()) {
			wxArrayString entries = wxSplit(wxString::FromUTF8(adoptedList), ';');
			for (size_t i = 0; i < entries.size(); ++i) {
				wxString entry = entries[i];
				int sep = entry.Find(':');
				if (sep != wxNOT_FOUND) {
					std::string entryIp = nstr(entry.Left(sep));
					if (entryIp == srvKey || (!reconnectAddress.empty() && entryIp == reconnectAddress)) {
						alreadyAnswered = true;
						savedDecision = wxAtoi(entry.Mid(sep + 1));
						break;
					}
				}
			}
		}

		auto applyHostSettings = [hostViewFlags]() {
			g_settings.setInteger(Config::SHOW_ALL_FLOORS, (hostViewFlags & (1 << 0)) ? 1 : 0);
			g_settings.setInteger(Config::SHOW_CREATURES, (hostViewFlags & (1 << 1)) ? 1 : 0);
			g_settings.setInteger(Config::SHOW_SPAWNS, (hostViewFlags & (1 << 2)) ? 1 : 0);
			g_settings.setInteger(Config::SHOW_HOUSES, (hostViewFlags & (1 << 3)) ? 1 : 0);
			g_settings.setInteger(Config::SHOW_SHADE, (hostViewFlags & (1 << 4)) ? 1 : 0);
			g_settings.setInteger(Config::SHOW_SPECIAL_TILES, (hostViewFlags & (1 << 5)) ? 1 : 0);
			g_settings.setInteger(Config::SHOW_ITEMS, (hostViewFlags & (1 << 6)) ? 1 : 0);
			g_settings.setInteger(Config::SHOW_BLOCKING, (hostViewFlags & (1 << 7)) ? 1 : 0);
			g_settings.setInteger(Config::SHOW_TOOLTIPS, (hostViewFlags & (1 << 8)) ? 1 : 0);
			g_settings.setInteger(Config::SHOW_WALL_HOOKS, (hostViewFlags & (1 << 9)) ? 1 : 0);
			g_settings.setInteger(Config::SHOW_AS_MINIMAP, (hostViewFlags & (1 << 10)) ? 1 : 0);
			g_settings.setInteger(Config::TRANSPARENT_FLOORS, (hostViewFlags & (1 << 13)) ? 1 : 0);
			g_settings.setInteger(Config::TRANSPARENT_ITEMS, (hostViewFlags & (1 << 14)) ? 1 : 0);
			g_settings.setInteger(Config::HIGHLIGHT_ITEMS, (hostViewFlags & (1 << 15)) ? 1 : 0);
			g_settings.setInteger(Config::HIGHLIGHT_LOCKED_DOORS, (hostViewFlags & (1 << 16)) ? 1 : 0);
			g_settings.setInteger(Config::SHOW_MINIMAP_HUD, (hostViewFlags & (1 << 17)) ? 1 : 0);
			g_settings.setInteger(Config::SHOW_GRID, (hostViewFlags & (1 << 18)) ? 1 : 0);
			g_settings.setInteger(Config::SHOW_TECHNICAL_ITEMS, (hostViewFlags & (1 << 19)) ? 1 : 0);
			g_settings.setInteger(Config::SHOW_WAYPOINTS, (hostViewFlags & (1 << 20)) ? 1 : 0);
			g_settings.setInteger(Config::SHOW_TOWNS, (hostViewFlags & (1 << 21)) ? 1 : 0);
			g_settings.setInteger(Config::ALWAYS_SHOW_ZONES, (hostViewFlags & (1 << 22)) ? 1 : 0);

			if (hostViewFlags & (1 << 23)) {
				if (!RadioPlayerWindow::IsDocked() && !RadioPlayerWindow::GetInstance()) {
					RadioPlayerWindow::ShowDocked(true);
				}
			}

			if (g_gui.root) {
				g_gui.root->UpdateMenubar();
			}
			g_gui.RefreshView();
			g_gui.RefreshPalettes();
			g_gui.RefreshMinimapPanel();
		};

		if (alreadyAnswered) {
			if (savedDecision == 1) {
				applyHostSettings();
			}
		} else {
			wxMessageDialog dlg(g_gui.root,
				"The Host is sharing their active View & Workspace Settings (Overlays, Layers, Minimap, Docked Radio Player, Grid, etc.).\n\nWould you like to adopt the Host's View Settings, or keep your own?",
				"Adopt Host View Settings?",
				wxYES_NO | wxICON_QUESTION);
			dlg.SetYesNoLabels("Adopt Host Settings", "Keep My Settings");
			int res = dlg.ShowModal();
			int decision = (res == wxID_YES) ? 1 : 0;
			if (decision == 1) {
				applyHostSettings();
			}
			std::string newEntry = srvKey + ":" + std::to_string(decision);
			if (!adoptedList.empty()) adoptedList += ";";
			adoptedList += newEntry;
			g_settings.setString(Config::MULTIPLAYER_ADOPTED_SERVERS, adoptedList);
		}
	}

	if (reconnectAttempts > 0) {
		g_gui.SetScreenCenterPosition(pendingFocusPos);
		g_gui.RefreshView();
		g_gui.UpdateMinimap();
	}

	// Now that the server has set connected=true and is in parseEditorPacket mode,
	// we can safely send PACKET_REQUEST_NODES for the visible area.
	int map_x = pendingFocusPos.x;
	int map_y = pendingFocusPos.y;
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
	if (!queryNodeList.empty()) {
		sendNodeRequests();
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
	// Signal to the server that we are ready to receive map data.
	// Do NOT send node requests here - the server's parseReady() sets connected=true
	// and only then can it handle PACKET_REQUEST_NODES in parseEditorPacket.
	// Node requests will be sent after we receive PACKET_HELLO_FROM_SERVER.
	sendReady();
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
		LogErrorToFile("[LiveClient] Warning: Received node before map was initialized.");
		if (log) { log->Message("Warning: Received node before map was initialized."); }
		return;
	}

	receiveNode(message, *mapEditor, nullptr, ndx, ndy, underground);
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

	// Only show popup loading window during initial map download
	if (!hasCreatedEditorTab) {
		g_gui.CreateLoadBar(currentOperation);
	}
	g_gui.SetStatusText("Server Operation: " + currentOperation);
}

void LiveClient::parseUpdateOperation(NetworkMessage& message) {
	int32_t percent = message.read<uint32_t>();
	if (percent >= 100) {
		if (!hasCreatedEditorTab) {
			g_gui.SetLoadDone(100);
			g_gui.DestroyLoadBar();
		}
		currentOperation.clear();
		g_gui.SetStatusText("Server Operation Finished.");

		if (!hasCreatedEditorTab && mapEditor) {
			hasCreatedEditorTab = true;
			MapTab* tab = createEditorWindow();
			if (tab) {
				tab->SetScreenCenterPosition(pendingFocusPos);
			}
			g_gui.UpdateTitle();
			g_gui.RefreshPalettes();
			if (g_gui.root) {
				g_gui.root->UpdateMenubar();
				g_gui.root->Refresh();
			}
		}

		g_gui.RefreshView();
		g_gui.UpdateMinimap();
	} else {
		if (!hasCreatedEditorTab) {
			g_gui.SetLoadDone(percent, currentOperation + wxString::Format(" (%d%%)", percent));
		}
		g_gui.SetStatusText("Server Operation: " + currentOperation + wxString::Format(" (%d%%)", percent));
	}
}

void LiveClient::parseTownList(NetworkMessage& message) {
	if (!mapEditor) return;
	Map& map = mapEditor->map;
	uint32_t count = message.read<uint32_t>();
	map.towns.clear();
	for (uint32_t i = 0; i < count; ++i) {
		uint32_t tid = message.read<uint32_t>();
		std::string tname = message.read<std::string>();
		Position tpos = message.read<Position>();
		Town* t = new Town(tid);
		t->setName(tname);
		t->setTemplePosition(tpos);
		map.towns.addTown(t);
	}
	g_gui.RefreshMinimapPanel();
}

void LiveClient::parseWorldPalette(NetworkMessage& message) {
	std::string tsName = message.read<std::string>();
	uint32_t brushCount = message.read<uint32_t>();

	Tileset* tileset = nullptr;
	auto it = g_materials.tilesets.find(tsName);
	if (it != g_materials.tilesets.end()) {
		tileset = it->second;
		tileset->clear();
	} else {
		tileset = new Tileset(g_brushes, tsName);
		g_materials.tilesets[tsName] = tileset;
	}

	for (uint32_t i = 0; i < brushCount; ++i) {
		std::string brushName = message.read<std::string>();
		uint32_t brushId = message.read<uint32_t>();
		Brush* b = g_brushes.getBrush(brushName);
		if (b) {
			TilesetCategoryType catType = TILESET_RAW;
			if (b->isGround()) catType = TILESET_TERRAIN;
			else if (b->isDoodad()) catType = TILESET_DOODAD;
			else if (b->isWall()) catType = TILESET_TERRAIN;
			else if (b->isCreature()) catType = TILESET_CREATURE;
			else if (b->isHouse()) catType = TILESET_HOUSE;

			TilesetCategory* cat = tileset->getCategory(catType);
			if (cat && !cat->containsBrush(b)) {
				cat->brushlist.push_back(b);
			}
		}
	}

	g_gui.RefreshPalettes();
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
	g_gui.SetStatusText(wxString::Format("Access denied: This position is currently being edited by '%s'!", wxstr(ownerName)));
	wxMessageBox(wxString::Format("This position/property is currently being edited by '%s'!", wxstr(ownerName)), "Lock Active", wxOK | wxICON_WARNING);
	if (g_gui.activePropertiesWindow) {
		g_gui.activePropertiesWindow->EndModal(wxID_CANCEL);
	}
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

void LiveClient::sendApprovalRequest(uint8_t reqType, const Position& pos, uint32_t reqValue, const wxString& details) {
	static uint32_t s_localReqId = 1;
	NetworkMessage message;
	message.write<uint8_t>(PACKET_APPROVAL_REQUEST);
	message.write<uint32_t>(s_localReqId++);
	message.write<uint8_t>(reqType);
	message.write<Position>(pos);
	message.write<uint32_t>(reqValue);
	message.write<std::string>(nstr(details));
	send(message);

	g_gui.SetStatusText("Approval request submitted to Multiplayer Host...");
}

void LiveClient::parseApprovalResponse(NetworkMessage& message) {
	uint32_t reqId = message.read<uint32_t>();
	uint8_t approved = message.read<uint8_t>();
	uint32_t assignedValue = message.read<uint32_t>();
	std::string reason = message.read<std::string>();

	if (approved != 0) {
		g_gui.SetStatusText(wxString::Format("✅ Host approved request (Assigned ID: %u)", assignedValue));
	} else {
		g_gui.SetStatusText(wxString::Format("❌ Host rejected request: %s", wxstr(reason)));
		wxMessageBox(wxString::Format("Host rejected the creation/ID request:\n%s", wxstr(reason)), "Approval Rejected", wxOK | wxICON_INFORMATION);
	}
}
