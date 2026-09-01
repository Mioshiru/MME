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

#include "live_action.h"
#include "live_peer.h"
#include "live_server.h"
#include "live_tab.h"


#include "editor.h"
#include "radio_player.h"
#include "materials.h"
#include "tileset.h"
#include "brush.h"
#include "live_approval_window.h"

LivePeer::LivePeer(LiveServer *server, boost::asio::ip::tcp::socket socket, uint32_t id)
    : LiveSocket(), readMessage(), server(server), socket(std::move(socket)),
      color(), latency(0), packetLoss(0), lastHeartbeat(0),
      connectionStatus("Connecting"), id(id), clientId(0), connected(false),
      aliveFlag(std::make_shared<std::atomic<bool>>(true)) {
  ASSERT(server != nullptr);
  boost::system::error_code error;
  this->socket.set_option(boost::asio::ip::tcp::no_delay(true), error);
  this->socket.set_option(boost::asio::socket_base::keep_alive(true), error);
  this->socket.set_option(boost::asio::socket_base::send_buffer_size(131072), error);
  this->socket.set_option(boost::asio::socket_base::receive_buffer_size(131072), error);
}

LivePeer::~LivePeer() {
  g_gui.latencies.erase(this);
  if (socket.is_open()) {
    socket.close();
  }
}

void LivePeer::close() {
  // Mark as dead FIRST so any pending CallAfter lambdas bail out safely.
  aliveFlag->store(false);
  {
    std::lock_guard<std::mutex> lock(writeMutex);
    writeQueue.clear();
  }
  if (connected) {
    server->broadcastChat("Server", name + " left the session.");
  }
  server->removeClient(id);
}

bool LivePeer::handleError(const boost::system::error_code &error) {
  if (error) {
    connectionStatus = "Disconnected";
    // handleError may be called from the Asio I/O thread (receive callback).
    // All GUI access and object lifecycle (close/delete) must happen on the main thread.
    // Use aliveFlag so the lambda is a no-op if the peer was already deleted.
    boost::system::error_code err = error;
    auto flag = aliveFlag;
    wxTheApp->CallAfter([this, err, flag]() {
      if (!flag->load()) return; // Peer already deleted - bail out.
      logMessage(wxString() + getHostName() + ": disconnected (" + err.message() + ").");
      close();
    });
    return true;
  }
  return false;
}

std::string LivePeer::getHostName() const {
  boost::system::error_code ec;
  auto endpoint = socket.remote_endpoint(ec);
  if (ec) {
    return "Unknown IP";
  }
  return endpoint.address().to_string();
}

void LivePeer::receiveHeader() {
  if (!socket.is_open()) {
    return;
  }
  readMessage.position = 0;
  boost::asio::async_read(
      socket, boost::asio::buffer(readMessage.buffer, 4),
      [this](const boost::system::error_code &error,
             size_t bytesReceived) -> void {
        if (error) {
          if (!handleError(error)) {
            logMessage(wxString() + getHostName() + ": " + error.message());
          }
        } else if (bytesReceived < 4) {
          logMessage(
              wxString() + getHostName() + ": Could not receive header[size: " +
              std::to_string(bytesReceived) + "], disconnecting client.");
        } else {
          receive(readMessage.read<uint32_t>());
        }
      });
}

void LivePeer::receive(uint32_t packetSize) {
  readMessage.buffer.resize(readMessage.position + packetSize);
  boost::asio::async_read(
      socket,
      boost::asio::buffer(&readMessage.buffer[readMessage.position],
                          packetSize),
      [this](const boost::system::error_code &error,
             size_t bytesReceived) -> void {
        if (error) {
          if (!handleError(error)) {
            logMessage(wxString() + getHostName() + ": " + error.message());
          }
        } else if (bytesReceived < readMessage.buffer.size() - 4) {
          logMessage(
              wxString() + getHostName() + ": Could not receive packet[size: " +
              std::to_string(bytesReceived) + "], disconnecting client.");
        } else {
          NetworkMessage msg = std::move(readMessage);
          readMessage.clear();
          auto flag = aliveFlag;
          wxTheApp->CallAfter([this, msg = std::move(msg), flag]() mutable {
            if (!flag->load()) return;
            try {
              if (connected) {
                parseEditorPacket(msg);
              } else {
                parseLoginPacket(msg);
              }
            } catch (const std::exception& e) {
              logMessage(wxString::Format("Error processing packet from %s: %s", getHostName(), e.what()));
            } catch (...) {
              logMessage(wxString::Format("Unknown error processing packet from %s", getHostName()));
            }
            if (flag->load() && socket.is_open()) {
              receiveHeader();
            }
          });
        }
      });
}

void LivePeer::send(NetworkMessage &message) {
  if (!socket.is_open()) {
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

void LivePeer::doWrite() {
  // Caller or completion handler ensures isWriting is true when calling doWrite.
  if (writeQueue.empty() || !socket.is_open()) {
    isWriting = false;
    return;
  }

  auto buffer = std::make_shared<std::vector<uint8_t>>(std::move(writeQueue.front()));
  writeQueue.pop_front();

  auto flag = aliveFlag;
  boost::asio::async_write(
      socket, boost::asio::buffer(*buffer),
      [this, buffer, flag](const boost::system::error_code &error, size_t /*bytesTransferred*/) {
        if (!flag->load()) return;
        if (error) {
          {
            std::lock_guard<std::mutex> lock(writeMutex);
            writeQueue.clear();
            isWriting = false;
          }
          // Dispatch disconnect handling to the wxWidgets main thread.
          boost::system::error_code err = error;
          wxTheApp->CallAfter([this, err, flag]() {
            if (!flag->load()) return; // Peer already deleted.
            if (!handleError(err)) {
              logMessage(wxString() + getHostName() + ": " + err.message());
            }
          });
        } else {
          std::lock_guard<std::mutex> lock(writeMutex);
          if (!writeQueue.empty() && socket.is_open()) {
            doWrite();
          } else {
            isWriting = false;
          }
        }
      });
}

void LivePeer::parseLoginPacket(NetworkMessage message) {
  uint8_t packetType;
  while (message.position < message.buffer.size()) {
    if (connected) {
      parseEditorPacket(message);
      return;
    }
    packetType = message.read<uint8_t>();
    switch (packetType) {
    case PACKET_HELLO_FROM_CLIENT:
      parseHello(message);
      break;
    case PACKET_READY_CLIENT:
      parseReady(message);
      break;
    default: {
      logMessage("Invalid login packet received, connection severed.");
      close();
      return;
    }
    }
  }
}

void LivePeer::parseEditorPacket(NetworkMessage message) {
  uint8_t packetType;
  while (message.position < message.buffer.size()) {
    packetType = message.read<uint8_t>();
    switch (packetType) {
    case PACKET_REQUEST_NODES:
      parseNodeRequest(message);
      break;
    case PACKET_CHANGE_LIST:
      parseReceiveChanges(message);
      break;
    case PACKET_ADD_HOUSE:
      parseAddHouse(message);
      break;
    case PACKET_EDIT_HOUSE:
      parseEditHouse(message);
      break;
    case PACKET_REMOVE_HOUSE:
      parseRemoveHouse(message);
      break;
    case PACKET_CLIENT_UPDATE_CURSOR:
      parseCursorUpdate(message);
      break;
    case PACKET_CLIENT_TALK:
      parseChatMessage(message);
      break;
    case PACKET_LOCK_ENTITY:
      parseLockEntity(message);
      break;
    case PACKET_UNLOCK_ENTITY:
      parseUnlockEntity(message);
      break;
    case PACKET_PING_LOCATION:
      parsePingLocation(message);
      break;
    case PACKET_ADD_ANNOTATION:
      parseAddAnnotation(message);
      break;
    case PACKET_REMOVE_ANNOTATION:
      parseRemoveAnnotation(message);
      break;
    case PACKET_UPDATE_STATUS:
      parseUpdateStatus(message);
      break;
    case PACKET_APPROVAL_REQUEST: {
      uint32_t reqId = message.read<uint32_t>();
      uint8_t reqType = message.read<uint8_t>();
      Position pos = message.read<Position>();
      uint32_t reqVal = message.read<uint32_t>();
      std::string details = message.read<std::string>();

      LiveApprovalRequest req;
      req.reqId = reqId;
      req.clientId = getClientId();
      req.requesterName = getClientName();
      req.type = (LiveApprovalType)reqType;
      req.pos = pos;
      req.requestedValue = reqVal;
      req.details = wxstr(details);
      req.timestamp = g_gui.gfx.getElapsedTime();

      wxTheApp->CallAfter([req]() {
        LiveApprovalWindow::AddPendingRequest(req);
      });
      break;
    }
    case PACKET_PING: {
      uint64_t timestamp = message.read<uint64_t>();
      uint32_t reported_latency = message.read<uint32_t>();
      uint32_t reported_packet_loss = 0;
      if (message.position < message.buffer.size()) {
        reported_packet_loss = message.read<uint32_t>();
      }
      g_gui.latencies[this] = reported_latency;
      latency = reported_latency;
      packetLoss = reported_packet_loss;
      lastHeartbeat = wxGetLocalTimeMillis().GetValue();
      connectionStatus = packetLoss > 20 ? "Unstable" : "Connected";
      if (log) {
        log->Message(wxString::Format("[Server] Received Ping from %s, reported latency: %u ms", name, latency));
      }
      server->updateClientList();

      NetworkMessage out;
      out.write<uint8_t>(PACKET_PONG);
      out.write<uint64_t>(timestamp);
      send(out);
      break;
    }
    default: {
      logMessage("Invalid editor packet received, connection severed.");
      close();
      break;
    }
    }
  }
}

void LivePeer::parseHello(NetworkMessage &message) {
  if (connected) {
    close();
    return;
  }

  connectionStatus = "Negotiating";

  uint32_t rmeVersion = message.read<uint32_t>();
  if (rmeVersion != __RME_VERSION_ID__) {
    NetworkMessage outMessage;
    outMessage.write<uint8_t>(PACKET_KICK);
    outMessage.write<std::string>(
        "Editor version mismatch. Host uses " + __RME_VERSION__ +
        ", client uses version id " + i2s(rmeVersion) + ".");

    send(outMessage);
    close();
    return;
  }

  uint32_t netVersion = message.read<uint32_t>();
  if (netVersion != __LIVE_NET_VERSION__) {
    NetworkMessage outMessage;
    outMessage.write<uint8_t>(PACKET_KICK);
    outMessage.write<std::string>(
        "Multiplayer protocol mismatch. Host uses protocol " +
        i2s(__LIVE_NET_VERSION__) + ", client uses protocol " +
        i2s(netVersion) + ".");

    send(outMessage);
    close();
    return;
  }

  uint32_t clientVersion = message.read<uint32_t>();
  std::string nickname = message.read<std::string>();
  std::string password = message.read<std::string>();

  name = wxString(nickname.c_str(), wxConvUTF8);
  g_gui.SetStatusText(name + " joined the session!");
  logMessage(name + " (" + getHostName() + ") connected.");

  MapVersion ver = server->getEditor()->map.getVersion();
  this->mapVersion = VirtualIOMap(ver);

  NetworkMessage outMessage;
  if (static_cast<ClientVersionID>(clientVersion) !=
      g_gui.GetCurrentVersionID()) {
    outMessage.write<uint8_t>(PACKET_CHANGE_CLIENT_VERSION);
    outMessage.write<uint32_t>(g_gui.GetCurrentVersionID());
    ClientVersion* version = ClientVersion::get(g_gui.GetCurrentVersionID());
    outMessage.write<std::string>(version ? version->getName() : std::string("Unknown"));
  } else {
    outMessage.write<uint8_t>(PACKET_ACCEPTED_CLIENT);
  }
  send(outMessage);
}

void LivePeer::parseReady(NetworkMessage &message) {
  if (connected) {
    close();
    return;
  }

  connected = true;
  connectionStatus = "Connected";
  lastHeartbeat = wxGetLocalTimeMillis().GetValue();

  // Find free client id
  clientId = server->getFreeClientId();
  if (clientId == 0) {
    NetworkMessage outMessage;
    outMessage.write<uint8_t>(PACKET_KICK);
    outMessage.write<std::string>("Server is full.");

    send(outMessage);
    close();
    return;
  }

  server->updateClientList();

  // Step 1: Send HELLO first so the client creates its map/editor
  Map &map = server->getEditor()->map;

  NetworkMessage outMessage;
  outMessage.write<uint8_t>(PACKET_HELLO_FROM_SERVER);
  outMessage.write<std::string>(map.getName());
  outMessage.write<uint16_t>(map.getWidth());
  outMessage.write<uint16_t>(map.getHeight());

  Position focusPos(map.getWidth() / 2, map.getHeight() / 2, 7);
  MapTab* hostTab = g_gui.GetCurrentMapTab();
  if (hostTab) {
    focusPos = hostTab->GetScreenCenterPosition();
  }
  outMessage.write<Position>(focusPos);

  // Pack host active View Settings
  uint32_t viewFlags = 0;
  if (g_settings.getBoolean(Config::SHOW_ALL_FLOORS)) viewFlags |= (1 << 0);
  if (g_settings.getBoolean(Config::SHOW_CREATURES)) viewFlags |= (1 << 1);
  if (g_settings.getBoolean(Config::SHOW_SPAWNS)) viewFlags |= (1 << 2);
  if (g_settings.getBoolean(Config::SHOW_HOUSES)) viewFlags |= (1 << 3);
  if (g_settings.getBoolean(Config::SHOW_SHADE)) viewFlags |= (1 << 4);
  if (g_settings.getBoolean(Config::SHOW_SPECIAL_TILES)) viewFlags |= (1 << 5);
  if (g_settings.getBoolean(Config::SHOW_ITEMS)) viewFlags |= (1 << 6);
  if (g_settings.getBoolean(Config::SHOW_BLOCKING)) viewFlags |= (1 << 7);
  if (g_settings.getBoolean(Config::SHOW_TOOLTIPS)) viewFlags |= (1 << 8);
  if (g_settings.getBoolean(Config::SHOW_WALL_HOOKS)) viewFlags |= (1 << 9);
  if (g_settings.getBoolean(Config::SHOW_AS_MINIMAP)) viewFlags |= (1 << 10);
  if (g_settings.getBoolean(Config::SHOW_ONLY_TILEFLAGS)) viewFlags |= (1 << 11);
  if (g_settings.getBoolean(Config::TRANSPARENT_FLOORS)) viewFlags |= (1 << 13);
  if (g_settings.getBoolean(Config::TRANSPARENT_ITEMS)) viewFlags |= (1 << 14);
  if (g_settings.getBoolean(Config::HIGHLIGHT_ITEMS)) viewFlags |= (1 << 15);
  if (g_settings.getBoolean(Config::HIGHLIGHT_LOCKED_DOORS)) viewFlags |= (1 << 16);
  if (g_settings.getBoolean(Config::SHOW_MINIMAP_HUD)) viewFlags |= (1 << 17);
  if (g_settings.getBoolean(Config::SHOW_GRID)) viewFlags |= (1 << 18);
  if (g_settings.getBoolean(Config::SHOW_TECHNICAL_ITEMS)) viewFlags |= (1 << 19);
  if (g_settings.getBoolean(Config::SHOW_WAYPOINTS)) viewFlags |= (1 << 20);
  if (g_settings.getBoolean(Config::SHOW_TOWNS)) viewFlags |= (1 << 21);
  if (g_settings.getBoolean(Config::ALWAYS_SHOW_ZONES)) viewFlags |= (1 << 22);
  if (RadioPlayerWindow::IsDocked() || RadioPlayerWindow::GetInstance() != nullptr) viewFlags |= (1 << 23);

  outMessage.write<uint32_t>(viewFlags);
  send(outMessage);

  // Step 2: Send chat messages (after HELLO, so the client is ready)
  // Broadcast join message to everyone
  server->broadcastChat("Server", name + " joined the session!");

  // Send current users list to the newly joined client
  wxString userList = "Connected clients: Host";
  for (const auto& clientEntry : server->clients) {
    LivePeer* peer = clientEntry.second;
    if (peer != this && peer->isConnected()) {
      userList += ", " + peer->getName();
    }
  }
  NetworkMessage listMsg;
  listMsg.write<uint8_t>(PACKET_SERVER_TALK);
  listMsg.write<std::string>("Server");
  listMsg.write<std::string>(nstr(userList));
  send(listMsg);

  // Step 2.5: Synchronize Towns container to Remote Client
  NetworkMessage townListMsg;
  townListMsg.write<uint8_t>(PACKET_TOWN_LIST);
  townListMsg.write<uint32_t>((uint32_t)map.towns.count());
  for (const auto& pair : map.towns) {
    Town* t = pair.second;
    if (t) {
      townListMsg.write<uint32_t>(t->getID());
      townListMsg.write<std::string>(t->getName());
      townListMsg.write<Position>(t->getTemplePosition());
    }
  }
  send(townListMsg);

  // Step 2.6: Synchronize World Palette (Corporate Design) to Remote Client
  auto wp_it = g_materials.tilesets.find("World Palette");
  if (wp_it != g_materials.tilesets.end() && wp_it->second) {
    Tileset* ts = wp_it->second;
    std::vector<Brush*> allBrushes;
    for (TilesetCategory* cat : ts->categories) {
      if (cat) {
        for (Brush* b : cat->brushlist) {
          if (b) allBrushes.push_back(b);
        }
      }
    }
    NetworkMessage wpMsg;
    wpMsg.write<uint8_t>(PACKET_WORLD_PALETTE);
    wpMsg.write<std::string>("World Palette");
    wpMsg.write<uint32_t>((uint32_t)allBrushes.size());
    for (Brush* b : allBrushes) {
      wpMsg.write<std::string>(b->getName());
      wpMsg.write<uint32_t>(b->getID());
    }
    send(wpMsg);
  }

  // Step 3: Initial Map Sync — send ALL non-empty nodes to this client
  // Send start operation so the client displays a smooth loading bar and avoids redraw lag
  NetworkMessage startOpMsg;
  startOpMsg.write<uint8_t>(PACKET_START_OPERATION);
  startOpMsg.write<std::string>("Downloading Map from Host...");
  send(startOpMsg);

  int nodesSent = 0;
  std::vector<QTreeNode::VisibleNode> visibleNodes;
  map.root.getVisibleLeaves(0, 0, -1, 0, 0, 65535, 65535, visibleNodes);
  size_t totalVisible = visibleNodes.size();
  int lastPercent = 0;

  for (size_t i = 0; i < totalVisible; ++i) {
    const auto& vn = visibleNodes[i];
    QTreeNode* node = vn.node;
    if (!node) {
      continue;
    }
    
    int ndx = vn.map_x / 4;
    int ndy = vn.map_y / 4;

    // Check if this node has any floor data at all
    Floor** floors = node->getFloors();
    bool hasOverground = false;
    bool hasUnderground = false;

    for (int z = 0; z < MAP_LAYERS; ++z) {
      if (floors[z]) {
        if (z <= GROUND_LAYER) {
          hasOverground = true;
        } else {
          hasUnderground = true;
        }
      }
    }

    if (hasOverground) {
      sendNode(clientId, node, ndx, ndy, 0x00FF);
      ++nodesSent;
    }
    if (hasUnderground) {
      sendNode(clientId, node, ndx, ndy, 0xFF00);
      ++nodesSent;
    }

    int currentPercent = totalVisible > 0 ? static_cast<int>((i * 100) / totalVisible) : 100;
    if (currentPercent >= lastPercent + 10 && currentPercent < 100) {
      lastPercent = currentPercent;
      NetworkMessage progressMsg;
      progressMsg.write<uint8_t>(PACKET_UPDATE_OPERATION);
      progressMsg.write<uint32_t>(currentPercent);
      send(progressMsg);
    }
  }

  // Signal completion of map synchronization
  NetworkMessage endOpMsg;
  endOpMsg.write<uint8_t>(PACKET_UPDATE_OPERATION);
  endOpMsg.write<uint32_t>(100);
  send(endOpMsg);

  logMessage(wxString::Format("%s: Initial sync complete (%d nodes sent).", name, nodesSent));
}

void LivePeer::parseNodeRequest(NetworkMessage &message) {
  Map &map = server->getEditor()->map;
  for (uint32_t nodes = message.read<uint32_t>(); nodes != 0; --nodes) {
    uint32_t ind = message.read<uint32_t>();

    int32_t ndx = ind >> 18;
    int32_t ndy = (ind >> 4) & 0x3FFF;
    bool underground = ind & 1;

    QTreeNode *node = map.createLeaf(ndx * 4, ndy * 4);
    if (node) {
      sendNode(clientId, node, ndx, ndy, underground ? 0xFF00 : 0x00FF);
    }
  }
}

void LivePeer::parseReceiveChanges(NetworkMessage &message) {
  MapEditor &editor = *server->getEditor();

  const std::string &data = message.read<std::string>();
  mapReader.assign(reinterpret_cast<const uint8_t *>(data.data()), data.size());

  BinaryNode *rootNode = mapReader.getRootNode();
  if (!rootNode) {
    return;
  }
  BinaryNode *tileNode = rootNode->getChild();

  NetworkedAction *action = static_cast<NetworkedAction *>(
      editor.actionQueue->createAction(ACTION_REMOTE));
  action->owner = clientId;

  if (tileNode) {
    do {
      Tile *tile = readTile(tileNode, editor, nullptr);
      if (tile) {
        action->addChange(newd Change(tile));
      }
    } while (tileNode->advance());
  }
  mapReader.close();

  editor.actionQueue->addAction(action);

  g_gui.RefreshView();
}

void LivePeer::parseAddHouse(NetworkMessage &message) {}

void LivePeer::parseEditHouse(NetworkMessage &message) {}

void LivePeer::parseRemoveHouse(NetworkMessage &message) {}

void LivePeer::parseCursorUpdate(NetworkMessage &message) {
  LiveCursor cursor = readCursor(message);
  cursor.id = clientId;

  // Distinct color assignment for up to 6 players (Host + 5 Clients)
  if (color == wxColor()) {
    static wxColor available[] = {
        wxColor(255, 50, 50, 128),   // Rot (P2)
        wxColor(0, 150, 255, 128),  // Cyan (P3)
        wxColor(255, 215, 0, 128),  // Gold (P4)
        wxColor(200, 0, 255, 128),  // Violett (P5)
        wxColor(255, 128, 0, 128)   // Orange (P6)
    };
    uint32_t colorIdx = (clientId == 0) ? 0 : ((clientId - 1) % 5);
    setUsedColor(available[colorIdx]);
  }
  cursor.color = color;

  if (cursor.color != color) {
    setUsedColor(cursor.color);
    server->updateClientList();
  }

  server->broadcastCursor(cursor);
  g_gui.RefreshView();
}

void LivePeer::parseChatMessage(NetworkMessage &message) {
  const std::string &chatMessage = message.read<std::string>();
  server->broadcastChat(name, wxstr(chatMessage));
  g_gui.AddChatMessage(nstr(name), chatMessage);
}

void LivePeer::parseLockEntity(NetworkMessage &message) {
  Position pos = message.read<Position>();
  bool success = server->requestLock(clientId, pos, name, color);
  if (!success) {
    NetworkMessage reply;
    reply.write<uint8_t>(PACKET_LOCK_REJECT);
    reply.write<Position>(pos);
    auto it = server->lockedEntities.find(pos);
    if (it != server->lockedEntities.end()) {
      reply.write<std::string>(nstr(it->second.ownerName));
    } else {
      reply.write<std::string>("Another user");
    }
    send(reply);
  }
}

void LivePeer::parseUnlockEntity(NetworkMessage &message) {
  Position pos = message.read<Position>();
  server->unlock(clientId, pos);
}

void LivePeer::parsePingLocation(NetworkMessage &message) {
  Position pos = message.read<Position>();
  LivePing ping;
  ping.pos = pos;
  ping.senderId = clientId;
  ping.senderName = name;
  ping.color = color;
  ping.timestamp = wxGetLocalTimeMillis().GetValue();
  server->broadcastPing(ping);
  g_gui.RefreshView();
}

void LivePeer::parseAddAnnotation(NetworkMessage &message) {
  MapAnnotation annotation;
  annotation.id = message.read<uint32_t>();
  annotation.pos = message.read<Position>();
  annotation.text = wxstr(message.read<std::string>());
  annotation.author = name;
  annotation.color = color;
  server->broadcastAnnotation(annotation, false);
  g_gui.RefreshView();
}

void LivePeer::parseRemoveAnnotation(NetworkMessage &message) {
  uint32_t annotationId = message.read<uint32_t>();
  MapAnnotation annotation;
  annotation.id = annotationId;
  server->broadcastAnnotation(annotation, true);
  g_gui.RefreshView();
}

void LivePeer::parseUpdateStatus(NetworkMessage &message) {
  UserStatus status = static_cast<UserStatus>(message.read<uint8_t>());
  auto it = server->cursors.find(clientId);
  if (it != server->cursors.end()) {
    it->second.status = status;
    server->broadcastCursor(it->second);
  }
}
