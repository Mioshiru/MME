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

#ifndef LIVE_PACKETS_H
#define LIVE_PACKETS_H

enum LivePacketType {
	PACKET_HELLO_FROM_CLIENT = 0x10,
	PACKET_READY_CLIENT = 0x11,

	PACKET_REQUEST_NODES = 0x20,
	PACKET_CHANGE_LIST = 0x21,
	PACKET_ADD_HOUSE = 0x23,
	PACKET_EDIT_HOUSE = 0x24,
	PACKET_REMOVE_HOUSE = 0x25,
	PACKET_TOWN_LIST = 0x26,
	PACKET_ADD_TOWN = 0x27,
	PACKET_EDIT_TOWN = 0x28,
	PACKET_REMOVE_TOWN = 0x29,
	PACKET_WORLD_PALETTE = 0x2A,

	PACKET_CLIENT_TALK = 0x30,
	PACKET_CLIENT_UPDATE_CURSOR = 0x31,
	PACKET_LOCK_ENTITY = 0x32,
	PACKET_UNLOCK_ENTITY = 0x33,
	PACKET_PING_LOCATION = 0x34,
	PACKET_ADD_ANNOTATION = 0x35,
	PACKET_REMOVE_ANNOTATION = 0x36,
	PACKET_UPDATE_STATUS = 0x37,
	PACKET_APPROVAL_REQUEST = 0x38,
	PACKET_CHECKLIST_SYNC = 0x39,
	PACKET_CHECKLIST_ADD = 0x3A,
	PACKET_CHECKLIST_TOGGLE = 0x3B,
	PACKET_CHECKLIST_DELETE = 0x3C,
	PACKET_CHECKLIST_CLEAR_COMPLETED = 0x3D,

	PACKET_HELLO_FROM_SERVER = 0x80,
	PACKET_KICK = 0x81,
	PACKET_ACCEPTED_CLIENT = 0x82,
	PACKET_CHANGE_CLIENT_VERSION = 0x83,
	PACKET_SERVER_TALK = 0x84,

	PACKET_NODE = 0x90,
	PACKET_CURSOR_UPDATE = 0x91,
	PACKET_START_OPERATION = 0x92,
	PACKET_UPDATE_OPERATION = 0x93,
	PACKET_CHAT_MESSAGE = 0x94,
	PACKET_LOCK_BROADCAST = 0x95,
	PACKET_LOCK_REJECT = 0x96,
	PACKET_APPROVAL_RESPONSE = 0x97,
};

#endif
