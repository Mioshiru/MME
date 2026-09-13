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
#include "live_socket.h"
#include "map_region.h"
#include "iomap_otbm.h"
#include "live_tab.h"
#include "editor.h"
#include <zlib.h>

LiveSocket::LiveSocket() :
	cursors(), mapReader(nullptr, 0), mapWriter(),
	mapVersion(MapVersion(MAP_OTBM_4, CLIENT_VERSION_NONE)), log(nullptr),
	name("User"), password(""),
	callbackAlive(std::make_shared<std::atomic<bool>>(true)) {
	//
}

LiveSocket::~LiveSocket() {
	invalidateCallbacks();
}

wxString LiveSocket::getName() const {
	return name;
}

bool LiveSocket::setName(const wxString& newName) {
	if (newName.empty()) {
		setLastError("Must provide a name.");
		return false;
	} else if (newName.length() > 32) {
		setLastError("Name is too long.");
		return false;
	}
	name = newName;
	return true;
}

wxString LiveSocket::getPassword() const {
	return password;
}

bool LiveSocket::setPassword(const wxString& newPassword) {
	if (newPassword.length() > 32) {
		setLastError("Password is too long.");
		return false;
	}
	password = newPassword;
	return true;
}

wxString LiveSocket::getLastError() const {
	return lastError;
}

void LiveSocket::setLastError(const wxString& error) {
	lastError = error;
}

std::string LiveSocket::getHostName() const {
	return "?";
}

std::vector<LiveCursor> LiveSocket::getCursorList() const {
	std::vector<LiveCursor> cursorList;
	for (auto& cursorEntry : cursors) {
		cursorList.push_back(cursorEntry.second);
	}
	return cursorList;
}

void LiveSocket::logMessage(const wxString& message) {
	auto alive = callbackAlive;
	wxTheApp->CallAfter([this, message, alive]() {
		if (!alive->load()) return;
		if (log) {
			log->Message(message);
		}
	});
}

void LiveSocket::receiveNode(NetworkMessage& message, MapEditor& editor, Action* action, int32_t ndx, int32_t ndy, bool underground) {
	// Use getLeafForce to create the node if it doesn't exist yet (client-side nodes
	// are not pre-allocated, they are created on demand when the server sends data).
	QTreeNode* node = editor.map.createLeaf(ndx * 4, ndy * 4);
	if (!node) {
		if (log) {
			log->Message("Warning: Could not create tile node (" + std::to_string(ndx * 4) + "/" + std::to_string(ndy * 4) + "/" + (underground ? "true" : "false") + ")");
		}
		return;
	}

	node->setRequested(underground, false);
	node->setVisible(underground, true);

	uint16_t floorBits = message.read<uint16_t>();
	if (floorBits == 0) {
		return;
	}

	for (uint_fast8_t z = 0; z < MAP_LAYERS; ++z) {
		if (testFlags(floorBits, static_cast<uint64_t>(1) << z)) {
			receiveFloor(message, editor, action, ndx, ndy, z, node, node->getFloor(z));
		}
	}
}

void LiveSocket::sendNode(uint32_t clientId, QTreeNode* node, int32_t ndx, int32_t ndy, uint32_t floorMask) {
	bool underground;
	if (floorMask & 0xFF00) {
		if (floorMask & 0x00FF) {
			underground = false;
		} else {
			underground = true;
		}
	} else {
		underground = false;
	}

	node->setVisible(clientId, underground, true);

	// Send message
	NetworkMessage message;
	message.write<uint8_t>(PACKET_NODE);
	message.write<uint32_t>((ndx << 18) | (ndy << 4) | ((floorMask & 0xFF00) ? 1 : 0));

	if (!node) {
		message.write<uint16_t>(0x0000);
	} else {
		Floor** floors = node->getFloors();

		uint16_t sendMask = 0;
		for (uint32_t z = 0; z < MAP_LAYERS; ++z) {
			uint32_t bit = 1 << z;
			if (floors[z] && testFlags(floorMask, bit)) {
				sendMask |= bit;
			}
		}

		message.write<uint16_t>(sendMask);
		for (uint32_t z = 0; z < MAP_LAYERS; ++z) {
			if (testFlags(sendMask, static_cast<uint64_t>(1) << z)) {
				sendFloor(message, floors[z]);
			}
		}
	}

	send(message);
}

void LiveSocket::sendBatchNodesZlib(uint32_t clientId, const std::vector<BatchNodePayload>& batch) {
	if (batch.empty()) {
		return;
	}

	NetworkMessage rawMsg;
	rawMsg.write<uint32_t>(static_cast<uint32_t>(batch.size()));

	for (const auto& item : batch) {
		int32_t ndx = item.ndx;
		int32_t ndy = item.ndy;
		uint32_t floorMask = item.floorMask;
		QTreeNode* node = item.node;

		bool underground = (floorMask & 0xFF00) && !(floorMask & 0x00FF);
		uint32_t ind = (ndx << 18) | (ndy << 4) | (underground ? 1 : 0);
		rawMsg.write<uint32_t>(ind);

		if (!node) {
			rawMsg.write<uint16_t>(0);
			continue;
		}

		node->setVisible(clientId, underground, true);
		Floor** floors = node->getFloors();

		uint16_t sendMask = 0;
		for (uint32_t z = 0; z < MAP_LAYERS; ++z) {
			uint32_t bit = 1 << z;
			if (floors[z] && testFlags(floorMask, bit)) {
				sendMask |= bit;
			}
		}

		rawMsg.write<uint16_t>(sendMask);
		for (uint32_t z = 0; z < MAP_LAYERS; ++z) {
			if (testFlags(sendMask, static_cast<uint64_t>(1) << z)) {
				Floor* floor = floors[z];
				uint16_t tileBits = 0;
				for (uint_fast8_t x = 0; x < 4; ++x) {
					for (uint_fast8_t y = 0; y < 4; ++y) {
						uint_fast8_t index = (x * 4) + y;
						Tile* tile = floor->locs[index].get();
						if (tile && tile->size() > 0) {
							tileBits |= (1 << index);
						}
					}
				}

				rawMsg.write<uint16_t>(tileBits);
				if (tileBits != 0) {
					mapWriter.reset();
					mapWriter.addNode(0x00);
					for (uint_fast8_t x = 0; x < 4; ++x) {
						for (uint_fast8_t y = 0; y < 4; ++y) {
							uint_fast8_t index = (x * 4) + y;
							if (testFlags(tileBits, static_cast<uint64_t>(1) << index)) {
								sendTile(mapWriter, floor->locs[index].get(), nullptr);
							}
						}
					}
					mapWriter.endNode();

					std::string stream(
						reinterpret_cast<const char*>(mapWriter.getMemory()),
						mapWriter.getSize()
					);
					rawMsg.write<std::string>(stream);
				}
			}
		}
	}

	size_t rawDataSize = rawMsg.size;
	if (rawDataSize == 0) {
		return;
	}

	uLongf maxCompressed = compressBound(static_cast<uLong>(rawDataSize));
	std::vector<uint8_t> compressed(maxCompressed);

	// buffer has an initial 4-byte offset in NetworkMessage
	const uint8_t* rawPtr = rawMsg.buffer.data() + 4;
	int res = compress(compressed.data(), &maxCompressed, rawPtr, static_cast<uLong>(rawDataSize));
	if (res != Z_OK) {
		if (log) {
			log->Message("Warning: ZLIB compression failed for node batch.");
		}
		return;
	}

	NetworkMessage packet;
	packet.write<uint8_t>(PACKET_BATCH_NODES_ZLIB);
	packet.write<uint32_t>(static_cast<uint32_t>(rawDataSize));
	packet.write<uint32_t>(static_cast<uint32_t>(maxCompressed));
	
	packet.expand(maxCompressed);
	memcpy(&packet.buffer[packet.position], compressed.data(), maxCompressed);
	packet.position += maxCompressed;

	send(packet);
}

void LiveSocket::receiveBatchNodesZlib(NetworkMessage& message, MapEditor& editor, Action* action) {
	uint32_t rawDataSize = message.read<uint32_t>();
	uint32_t compressedSize = message.read<uint32_t>();

	if (message.position + compressedSize > message.buffer.size()) {
		if (log) {
			log->Message("Error: Truncated compressed node batch received.");
		}
		return;
	}

	const uint8_t* compressedPtr = &message.buffer[message.position];
	message.position += compressedSize;

	std::vector<uint8_t> rawBuffer(rawDataSize + 4, 0);
	uLongf destLen = static_cast<uLongf>(rawDataSize);

	int res = uncompress(rawBuffer.data() + 4, &destLen, compressedPtr, static_cast<uLong>(compressedSize));
	if (res != Z_OK || destLen != rawDataSize) {
		if (log) {
			log->Message("Error: Failed to decompress ZLIB node batch.");
		}
		return;
	}

	NetworkMessage reader;
	reader.buffer = std::move(rawBuffer);
	reader.position = 4;
	reader.size = rawDataSize;

	uint32_t nodeCount = reader.read<uint32_t>();
	Map& map = editor.map;

	for (uint32_t i = 0; i < nodeCount; ++i) {
		uint32_t ind = reader.read<uint32_t>();
		int32_t ndx = ind >> 18;
		int32_t ndy = (ind >> 4) & 0x3FFF;
		bool underground = ind & 1;

		uint16_t sendMask = reader.read<uint16_t>();
		if (sendMask == 0) {
			continue;
		}

		QTreeNode* node = map.createLeaf(ndx * 4, ndy * 4);
		if (!node) {
			continue;
		}

		node->setRequested(underground, false);
		node->setVisible(underground, true);

		for (uint_fast8_t z = 0; z < MAP_LAYERS; ++z) {
			if (testFlags(sendMask, static_cast<uint64_t>(1) << z)) {
				uint16_t tileBits = reader.read<uint16_t>();
				if (tileBits == 0) {
					continue;
				}

				const std::string& data = reader.read<std::string>();
				mapReader.assign(reinterpret_cast<const uint8_t*>(data.data()), data.size());

				BinaryNode* rootNode = mapReader.getRootNode();
				if (!rootNode) {
					continue;
				}
				BinaryNode* tileNode = rootNode->getChild();

				Position position(0, 0, z);
				for (uint_fast8_t x = 0; x < 4; ++x) {
					for (uint_fast8_t y = 0; y < 4; ++y) {
						position.x = (ndx * 4) + x;
						position.y = (ndy * 4) + y;

						if (testFlags(tileBits, static_cast<uint64_t>(1) << ((x * 4) + y))) {
							if (tileNode) {
								Tile* tile = readTile(tileNode, editor, &position);
								if (tile) {
									map.setTile(position.x, position.y, position.z, tile);
								}
								tileNode = tileNode->advance();
							}
						}
					}
				}
				mapReader.close();
			}
		}
	}
}

void LiveSocket::receiveFloor(NetworkMessage& message, MapEditor& editor, Action* action, int32_t ndx, int32_t ndy, int32_t z, QTreeNode* node, Floor* floor) {
	Map& map = editor.map;

	uint16_t tileBits = message.read<uint16_t>();
	if (tileBits == 0) {
		return;
	}

	// -1 on address since we skip the first START_NODE when sending
	const std::string& data = message.read<std::string>(); // Liest die String-Daten
	mapReader.assign(reinterpret_cast<const uint8_t*>(data.data()), data.size()); // Korrekte Zuweisung der Rohdaten

	BinaryNode* rootNode = mapReader.getRootNode();
	if (!rootNode) {
		return;
	}
	BinaryNode* tileNode = rootNode->getChild();

	Position position(0, 0, z);
	for (uint_fast8_t x = 0; x < 4; ++x) {
		for (uint_fast8_t y = 0; y < 4; ++y) {
			position.x = (ndx * 4) + x;
			position.y = (ndy * 4) + y;

			if (testFlags(tileBits, static_cast<uint64_t>(1) << ((x * 4) + y))) {
				if (tileNode) {
					Tile* tile = readTile(tileNode, editor, &position);
					if (tile) {
						map.setTile(position.x, position.y, position.z, tile);
					}
					tileNode = tileNode->advance();
				}
			}
		}
	}
	mapReader.close();
}

void LiveSocket::sendFloor(NetworkMessage& message, Floor* floor) {
	uint16_t tileBits = 0;
	for (uint_fast8_t x = 0; x < 4; ++x) {
		for (uint_fast8_t y = 0; y < 4; ++y) {
			uint_fast8_t index = (x * 4) + y;

			Tile* tile = floor->locs[index].get();
			if (tile && tile->size() > 0) {
				tileBits |= (1 << index);
			}
		}
	}

	message.write<uint16_t>(tileBits);
	if (tileBits == 0) {
		return;
	}

	mapWriter.reset();
	mapWriter.addNode(0x00); // Root container node
	for (uint_fast8_t x = 0; x < 4; ++x) {
		for (uint_fast8_t y = 0; y < 4; ++y) {
			uint_fast8_t index = (x * 4) + y;
			if (testFlags(tileBits, static_cast<uint64_t>(1) << index)) {
				sendTile(mapWriter, floor->locs[index].get(), nullptr);
			}
		}
	}
	mapWriter.endNode();

	std::string stream(
		reinterpret_cast<char*>(mapWriter.getMemory()),
		mapWriter.getSize()
	);
	message.write<std::string>(stream);
}

void LiveSocket::receiveTile(BinaryNode* node, MapEditor& editor, Action* action, const Position* position) {
	ASSERT(node != nullptr);

	Tile* tile = readTile(node, editor, position);
	if (tile) {
		action->addChange(newd Change(tile));
	}
}

void LiveSocket::sendTile(MemoryNodeFileWriteHandle& writer, Tile* tile, const Position* position) {
	if (!tile) {
		return;
	}
	writer.addNode(tile->isHouseTile() ? OTBM_HOUSETILE : OTBM_TILE);
	if (position) {
		writer.addU16(position->x);
		writer.addU16(position->y);
		writer.addU8(position->z);
	}

	if (tile->isHouseTile()) {
		writer.addU32(tile->getHouseID());
	}

	if (tile->getMapFlags()) {
		writer.addByte(OTBM_ATTR_TILE_FLAGS);
		writer.addU32(tile->getMapFlags());
	}

	Item* ground = tile->ground;
	if (ground) {
		if (ground->isComplex()) {
			ground->serializeItemNode_OTBM(mapVersion, writer);
		} else {
			writer.addByte(OTBM_ATTR_ITEM);
			ground->serializeItemCompact_OTBM(mapVersion, writer);
		}
	}

	for (Item* item : tile->items) {
		item->serializeItemNode_OTBM(mapVersion, writer);
	}

	writer.endNode();
}

Tile* LiveSocket::readTile(BinaryNode* node, MapEditor& editor, const Position* position) {
	if (!node) {
		return nullptr;
	}

	Map& map = editor.map;

	uint8_t tileType = 0;
	if (!node->getByte(tileType)) {
		return nullptr;
	}

	if (tileType != OTBM_TILE && tileType != OTBM_HOUSETILE) {
		return nullptr;
	}

	Position pos;
	if (position) {
		pos = *position;
	} else {
		uint16_t x = 0;
		if (!node->getU16(x)) return nullptr;
		pos.x = x;
		uint16_t y = 0;
		if (!node->getU16(y)) return nullptr;
		pos.y = y;
		uint8_t z = 0;
		if (!node->getU8(z)) return nullptr;
		pos.z = z;
	}

	TileLocation* location = map.createTileL(pos.x, pos.y, pos.z);
	if (!location) {
		return nullptr;
	}
	Tile* tile = newd Tile(*location);

	if (tileType == OTBM_HOUSETILE) {
		uint32_t houseId = 0;
		if (!node->getU32(houseId)) {
			delete tile;
			return nullptr;
		}

		if (houseId) {
			House* house = map.houses.getHouse(houseId);
			if (house) {
				tile->setHouse(house);
			}
		}
	}

	uint8_t attribute = 0;
	while (node->getU8(attribute)) {
		switch (attribute) {
			case OTBM_ATTR_TILE_FLAGS: {
				uint32_t flags = 0;
				if (node->getU32(flags)) {
					tile->setMapFlags(flags);
				}
				break;
			}
			case OTBM_ATTR_ITEM: {
				Item* item = Item::Create_OTBM(mapVersion, node);
				if (item) {
					tile->addItem(item);
				}
				break;
			}
			default:
				break;
		}
	}

	for (BinaryNode* itemNode = node->getChild(); itemNode != nullptr; itemNode = itemNode->advance()) {
		uint8_t itemType = 0;
		if (!itemNode->getByte(itemType)) {
			delete tile;
			return nullptr;
		}

		if (itemType == OTBM_ITEM) {
			Item* item = Item::Create_OTBM(mapVersion, itemNode);
			if (item) {
				item->unserializeItemNode_OTBM(mapVersion, itemNode);
				tile->addItem(item);
			}
		}
	}

	return tile;
}

LiveCursor LiveSocket::readCursor(NetworkMessage& message) {
	LiveCursor cursor;
	cursor.id = message.read<uint32_t>();

	uint8_t r = message.read<uint8_t>();
	uint8_t g = message.read<uint8_t>();
	uint8_t b = message.read<uint8_t>();
	uint8_t a = message.read<uint8_t>();
	cursor.color = wxColor(r, g, b, a);

	cursor.pos = message.read<Position>();
	cursor.status = static_cast<UserStatus>(message.read<uint8_t>());
	return cursor;
}

void LiveSocket::writeCursor(NetworkMessage& message, const LiveCursor& cursor) {
	message.write<uint32_t>(cursor.id);
	message.write<uint8_t>(cursor.color.Red());
	message.write<uint8_t>(cursor.color.Green());
	message.write<uint8_t>(cursor.color.Blue());
	message.write<uint8_t>(cursor.color.Alpha());
	message.write<Position>(cursor.pos);
	message.write<uint8_t>(static_cast<uint8_t>(cursor.status));
}
