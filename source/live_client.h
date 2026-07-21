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

#ifndef _RME_LIVE_CLIENT_H_
#define _RME_LIVE_CLIENT_H_

#include "live_socket.h"
#include "net_connection.h"

#include <set>

class DirtyList;
class MapTab;
class MapEditor;
class Action;
class BatchAction;

class LiveClient : public LiveSocket {
public:
	LiveClient();
	~LiveClient();

	//
	bool connect(const std::string& address, uint16_t port);
	void tryConnect(const boost::asio::ip::tcp::resolver::results_type& results);

	void close();
	bool handleError(const boost::system::error_code& error);
	uint32_t getLatency() const {
		return latency;
	}
	uint32_t getPacketLoss() const {
		return packetLossPercent;
	}
	wxString getConnectionStatus() const {
		return connectionStatus;
	}

	//
	std::string getHostName() const;

	//
	void receiveHeader();
	void receive(uint32_t packetSize);
	void send(NetworkMessage& message);

	//
	void updateCursor(const Position& position);

	LiveLogTab* createLogWindow(wxWindow* parent);
	MapTab* createEditorWindow();

	// send packets
	void sendHello();
	void sendNodeRequests();
	void sendChanges(DirtyList& dirtyList);
	void requestTileChange(Action* action);
	void requestBatchChange(BatchAction* batchAction);
	void sendChat(const wxString& chatMessage);
	void sendReady();

	// Flags a node as queried and stores it, need to call SendNodeRequest to send it to server
	void queryNode(int32_t ndx, int32_t ndy, bool underground);

protected:
	void parsePacket(NetworkMessage message);

	// parse packets
	void parseHello(NetworkMessage& message);
	void parseKick(NetworkMessage& message);
	void parseClientAccepted(NetworkMessage& message);
	void parseChangeClientVersion(NetworkMessage& message);
	void parseServerTalk(NetworkMessage& message);
	void parseNode(NetworkMessage& message);
	void parseCursorUpdate(NetworkMessage& message);
	void parseStartOperation(NetworkMessage& message);
	void parseUpdateOperation(NetworkMessage& message);
	bool scheduleReconnect(const wxString& reason);
	void attemptReconnect();
	void resetConnectionMetrics();

	//
	NetworkMessage readMessage;

	std::set<uint32_t> queryNodeList;
	wxString currentOperation;

	std::shared_ptr<boost::asio::ip::tcp::resolver> resolver;
	std::shared_ptr<boost::asio::ip::tcp::socket> socket;

	MapEditor* mapEditor;
	std::string reconnectAddress;
	uint16_t reconnectPort;
	uint32_t latency;
	uint32_t packetLossPercent;
	uint32_t reconnectAttempts;
	uint32_t pingsSent;
	uint32_t pingsMissed;
	uint64_t lastPingTimestamp;
	bool waitingForPong;
	bool reconnectScheduled;
	bool kickedByServer;
	wxString connectionStatus;

	bool stopped;
};

#endif
