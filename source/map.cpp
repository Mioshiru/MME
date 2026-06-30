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
#include "gui.h" // loadbar
#include "lua/lua_script_manager.h"
#include "ecs_manager.h"

#include "map.h"

#include <sstream>
#include <format>
#include <vector>

Map::Map() :
	BaseMap(),
	width(512),
	height(512),
	houses(*this),
	has_changed(false),
	unnamed(false),
	waypoints(*this) {
    m_ecs = std::make_unique<RME::Core::ECS::ECSManager>();
	// Earliest version possible
	// Caller is responsible for converting us to proper version
	mapVersion.otbm = MAP_OTBM_1;
	mapVersion.client = CLIENT_VERSION_NONE;
}

Map::~Map() {
	////
}

bool Map::open(const std::string file) {
	if (file == filename) {
		return true; // Do not reopen ourselves!
	}

	tilecount = 0;

	IOMapOTBM maploader(getVersion());

	bool success = maploader.loadMap(*this, wxstr(file));

	mapVersion = maploader.version;

	warnings = maploader.getWarnings();

	if (!success) {
		error = maploader.getError();
		return false;
	}

	has_changed = false;

	wxFileName fn = wxstr(file);
	filename = fn.GetFullPath().mb_str(wxConvUTF8);
	name = fn.GetFullName().mb_str(wxConvUTF8);

	name = fn.GetFullName().mb_str(wxConvUTF8);
	g_luaScripts.emit("mapLoad");

	return true;
}

bool Map::convert(MapVersion to, bool showdialog) {
	if (mapVersion.client == to.client) {
		// Only OTBM version differs
		// No changes necessary
		mapVersion = to;
		return true;
	}

	/* TODO

	if(to.otbm == MAP_OTBM_4 && to.client < CLIENT_VERSION_850)
		return false;

	if(mapVersion.client >= CLIENT_VERSION_760 && to.client < CLIENT_VERSION_760)
		convert(getReplacementMapFrom760To740(), showdialog);

	if(mapVersion.client < CLIENT_VERSION_810 && to.client >= CLIENT_VERSION_810)
		convert(getReplacementMapFrom800To810(), showdialog);

	if(mapVersion.client == CLIENT_VERSION_854_BAD && to.client >= CLIENT_VERSION_854)
		convert(getReplacementMapFrom854To854(), showdialog);
	*/
	mapVersion = to;

	return true;
}

bool Map::convert(const ConversionMap& rm, bool showdialog) {
	if (showdialog) {
		g_gui.CreateLoadBar("Converting map ...");
	}

	uint64_t tiles_done = 0;
	std::vector<uint16_t> id_list;

	// std::ofstream conversions("converted_items.txt");

	for (MapIterator miter = begin(); miter != end(); ++miter) {
		Tile* tile = (*miter)->get();
		ASSERT(tile);

		if (tile->size() == 0) {
			continue;
		}

		// id_list try MTM conversion
		id_list.clear();

		if (tile->ground) {
			id_list.push_back(tile->ground->getID());
		}
		for (ItemVector::const_iterator item_iter = tile->items.begin(); item_iter != tile->items.end(); ++item_iter) {
			if ((*item_iter)->isBorder()) {
				id_list.push_back((*item_iter)->getID());
			}
		}

		std::sort(id_list.begin(), id_list.end());

		ConversionMap::MTM::const_iterator cfmtm = rm.mtm.end();

		while (id_list.size()) {
			cfmtm = rm.mtm.find(id_list);
			if (cfmtm != rm.mtm.end()) {
				break;
			}
			id_list.pop_back();
		}

		// Keep track of how many items have been inserted at the bottom
		size_t inserted_items = 0;

		if (cfmtm != rm.mtm.end()) {
			const std::vector<uint16_t>& v = cfmtm->first;

			if (tile->ground && std::find(v.begin(), v.end(), tile->ground->getID()) != v.end()) {
				delete tile->ground;
				tile->ground = nullptr;
			}

			for (ItemVector::iterator item_iter = tile->items.begin(); item_iter != tile->items.end();) {
				if (std::find(v.begin(), v.end(), (*item_iter)->getID()) != v.end()) {
					delete *item_iter;
					item_iter = tile->items.erase(item_iter);
				} else {
					++item_iter;
				}
			}

			const std::vector<uint16_t>& new_items = cfmtm->second;
			for (std::vector<uint16_t>::const_iterator iit = new_items.begin(); iit != new_items.end(); ++iit) {
				Item* item = Item::Create(*iit);
				if (item->isGroundTile()) {
					tile->ground = item;
				} else {
					tile->items.insert(tile->items.begin(), item);
					++inserted_items;
				}
			}
		}

		if (tile->ground) {
			ConversionMap::STM::const_iterator cfstm = rm.stm.find(tile->ground->getID());
			if (cfstm != rm.stm.end()) {
				uint16_t aid = tile->ground->getActionID();
				uint16_t uid = tile->ground->getUniqueID();
				delete tile->ground;
				tile->ground = nullptr;

				const std::vector<uint16_t>& v = cfstm->second;
				// conversions << "Converted " << tile->getX() << ":" << tile->getY() << ":" << tile->getZ() << " " << id << " -> ";
				for (std::vector<uint16_t>::const_iterator iit = v.begin(); iit != v.end(); ++iit) {
					Item* item = Item::Create(*iit);
					// conversions << *iit << " ";
					if (item->isGroundTile()) {
						item->setActionID(aid);
						item->setUniqueID(uid);
						tile->addItem(item);
					} else {
						tile->items.insert(tile->items.begin(), item);
						++inserted_items;
					}
				}
				// conversions << std::endl;
			}
		}

		for (ItemVector::iterator replace_item_iter = tile->items.begin() + inserted_items; replace_item_iter != tile->items.end();) {
			uint16_t id = (*replace_item_iter)->getID();
			ConversionMap::STM::const_iterator cf = rm.stm.find(id);
			if (cf != rm.stm.end()) {
				// uint16_t aid = (*replace_item_iter)->getActionID();
				// uint16_t uid = (*replace_item_iter)->getUniqueID();
				delete *replace_item_iter;

				replace_item_iter = tile->items.erase(replace_item_iter);
				const std::vector<uint16_t>& v = cf->second;
				for (std::vector<uint16_t>::const_iterator iit = v.begin(); iit != v.end(); ++iit) {
					replace_item_iter = tile->items.insert(replace_item_iter, Item::Create(*iit));
					// conversions << "Converted " << tile->getX() << ":" << tile->getY() << ":" << tile->getZ() << " " << id << " -> " << *iit << std::endl;
					++replace_item_iter;
				}
			} else {
				++replace_item_iter;
			}
		}

		++tiles_done;
		if (showdialog && tiles_done % 0x10000 == 0) {
			g_gui.SetLoadDone(int(tiles_done / double(getTileCount()) * 100.0));
		}
	}

	if (showdialog) {
		g_gui.DestroyLoadBar();
	}

	return true;
}

void Map::cleanInvalidTiles(bool showdialog) {
	if (showdialog) {
		g_gui.CreateLoadBar("Removing invalid tiles...");
	}

	uint64_t tiles_done = 0;

	for (MapIterator miter = begin(); miter != end(); ++miter) {
		Tile* tile = (*miter)->get();
		ASSERT(tile);

		if (tile->size() == 0) {
			continue;
		}

		for (ItemVector::iterator item_iter = tile->items.begin(); item_iter != tile->items.end();) {
			if (g_items.typeExists((*item_iter)->getID())) {
				++item_iter;
			} else {
				delete *item_iter;
				item_iter = tile->items.erase(item_iter);
			}
		}

		++tiles_done;
		if (showdialog && tiles_done % 0x10000 == 0) {
			g_gui.SetLoadDone(int(tiles_done / double(getTileCount()) * 100.0));
		}
	}

	if (showdialog) {
		g_gui.DestroyLoadBar();
	}
}

void Map::convertHouseTiles(uint32_t fromId, uint32_t toId) {
	g_gui.CreateLoadBar("Converting house tiles...");
	uint64_t tiles_done = 0;

	for (MapIterator miter = begin(); miter != end(); ++miter) {
		Tile* tile = (*miter)->get();
		ASSERT(tile);

		uint32_t houseId = tile->getHouseID();
		if (houseId == 0 || houseId != fromId) {
			continue;
		}

		tile->setHouseID(toId);
		++tiles_done;
		if (tiles_done % 0x10000 == 0) {
			g_gui.SetLoadDone(int(tiles_done / double(getTileCount()) * 100.0));
		}
	}

	g_gui.DestroyLoadBar();
}

MapVersion Map::getVersion() const {
	return mapVersion;
}

TileLocation* Map::createTileL(int x, int y, int z) {
	QTreeNode* node = root.getLeafForce(x, y);
	return node->createFloor(this, x, y, z)->locs + ((x & 3) * 4 + (y & 3));
}

void Map::beginBatchChange() {
	batch_depth++;
}

void Map::endBatchChange() {
	if (--batch_depth <= 0) {
		batch_depth = 0;
		root.markDirty(-1); // Rebuild VBOs once batch is finished
	}
}

bool Map::hasChanged() const {
	return has_changed;
}

bool Map::doChange() {
	has_changed = true;
	root.markDirty(-1);
	return true;
}

bool Map::clearChanges() {
	bool doupdate = has_changed;
	has_changed = false;
	return doupdate;
}

std::vector<uint8_t> Map::createRegionSnapshot(int x, int y, int width, int height) {
    std::vector<uint8_t> buffer;
    buffer.reserve(width * height * 8);
    auto writeU16 = [&](uint16_t val) {
        buffer.push_back(val & 0xFF);
        buffer.push_back((val >> 8) & 0xFF);
    };

    for (int map_z = 0; map_z < MAP_LAYERS; ++map_z) {
        for (int map_y = y; map_y < y + height; ++map_y) {
            for (int map_x = x; map_x < x + width; ++map_x) {
                Tile* tile = this->getTile(map_x, map_y, map_z);
                if (tile && !tile->empty()) {
                    // Header: Relative Koordinaten und Item-Anzahl
                    buffer.push_back(static_cast<uint8_t>(map_x - x));
                    buffer.push_back(static_cast<uint8_t>(map_y - y));
                    buffer.push_back(static_cast<uint8_t>(map_z));
                    
                    writeU16(tile->getMapFlags()); // Speichere Tile-Flags (PZ etc.)

                    uint16_t itemCount = (tile->ground ? 1 : 0) + static_cast<uint16_t>(tile->items.size());
                    writeU16(itemCount);

                    // Ground Item
                    if (tile->ground) {
                        writeU16(tile->ground->getID());
                        writeU16(tile->ground->getActionID());
                        writeU16(tile->ground->getUniqueID());
                    }

                    // Top Items
                    for (Item* item : tile->items) {
                        writeU16(item->getID());
                        writeU16(item->getActionID()); // Jetzt mit Metadaten
                        writeU16(item->getUniqueID());
                    }
                }
            }
        }
    }
    // Terminator
    buffer.push_back(0xFF); 
    return buffer;
}

void Map::restoreRegionSnapshot(int x, int y, const std::vector<uint8_t>& snapshot) {
    if (snapshot.empty()) return;
    
    beginBatchChange(); // Unterdrückt einzelne VBO-Updates
    size_t offset = 0;
    auto readU16 = [&]() -> uint16_t {
        if (offset + 2 > snapshot.size()) return 0;
        uint16_t val = snapshot[offset] | (snapshot[offset + 1] << 8);
        offset += 2;
        return val;
    };

    while (offset + 3 <= snapshot.size() && snapshot[offset] != 0xFF) {
        int tx = x + snapshot[offset++];
        int ty = y + snapshot[offset++];
        int tz = snapshot[offset++];
        
        uint16_t flags = readU16();
        uint16_t itemCount = readU16();

        Tile* tile = this->createTile(tx, ty, tz);
        if (tile) {
            // Bestehende Items entfernen (da clear() kein Member von Tile ist)
            for (Item* item : tile->items) delete item;
            tile->items.clear();
            if (tile->ground) { delete tile->ground; tile->ground = nullptr; }
            
            tile->setMapFlags(flags);

            for (uint16_t i = 0; i < itemCount; ++i) {
                uint16_t id = readU16();
                uint16_t aid = readU16();
                uint16_t uid = readU16();

                Item* item = Item::Create(id);
                if (item) {
                    item->setActionID(aid);
                    item->setUniqueID(uid);
                    if (item->isGroundTile() && !tile->ground) {
                        tile->ground = item;
                    } else {
                        tile->addItem(item);
                    }
                }
            }
        } else {
            // Überspringe Items im Buffer, falls Kachel ungültig ist
            for (uint16_t i = 0; i < itemCount; ++i) {
                readU16(); readU16(); readU16();
            }
        }
    }
    
    // Signalisiere dem Renderer, dass ein großer Block fertig ist.
    // Das Vulkan-Backend kann nun einen dedizierten Transfer-Job starten.
    endBatchChange(); 
	root.markDirty(-1); 
	g_gui.SetStatusText("Prefab restored and VBOs synchronized.");
}

void Map::exportOTClient(const std::string& directory) {
    // OTClient Minimap Export Logik
    // Erstellt 256x256 Tiles PNGs (Format: x_y_z.png)
    g_gui.CreateLoadBar("Exporting OTClient Minimap...");
    
    int min_x = 0xFFFF, min_y = 0xFFFF, max_x = 0, max_y = 0;
    for (auto it = begin(); it != end(); ++it) {
        Position p = (*it)->getPosition();
        min_x = std::min(min_x, p.x); min_y = std::min(min_y, p.y);
        max_x = std::max(max_x, p.x); max_y = std::max(max_y, p.y);
    }

    if (size() == 0 || max_x < min_x || max_y < min_y) {
        g_gui.DestroyLoadBar();
        return;
    }

    int chunk_size = 256;
    int total_chunks = ((max_x - min_x) / chunk_size + 1) * ((max_y - min_y) / chunk_size + 1) * MAP_LAYERS;
    int current_chunk = 0;

    for (int z = 0; z < MAP_LAYERS; ++z) {
        for (int cx = min_x / chunk_size; cx <= max_x / chunk_size; ++cx) {
            for (int cy = min_y / chunk_size; cy <= max_y / chunk_size; ++cy) {
                wxImage chunk(chunk_size, chunk_size);
                bool has_data = false;

                for (int y = 0; y < chunk_size; ++y) {
                    for (int x = 0; x < chunk_size; ++x) {
                        Tile* t = getTile(cx * chunk_size + x, cy * chunk_size + y, z);
                        if (t) {
                            uint8_t c = t->getMiniMapColor();
                            RGBQuad color = minimap_color[c];
                            chunk.SetRGB(x, y, color.red, color.green, color.blue);
                            has_data = true;
                        } else {
                            chunk.SetRGB(x, y, 0, 0, 0);
                        }
                    }
                }
                if (has_data) {
                    std::string path = std::format("{}/{}_{}_{}.png", directory, cx * chunk_size, cy * chunk_size, z);
                    chunk.SaveFile(wxString::FromUTF8(path), wxBITMAP_TYPE_PNG);
                }
                g_gui.SetLoadDone((++current_chunk * 100) / total_chunks);
            }
        }
    }
    g_gui.DestroyLoadBar();
}

void Map::clearDirtyFlags() {
    // Performance: Wir setzen alle Layer gleichzeitig zurück, um Cache-Misses 
    // beim rekursiven Baum-Traversieren zu minimieren.
	root.clearAllDirty(-1); // -1 signalisiert 'alle Layer'
}

bool Map::hasFile() const {
	return filename != "";
}

void Map::setWidth(int new_width) {
	if (new_width > 65000) {
		width = 65000;
	} else if (new_width < 64) {
		width = 64;
	} else {
		width = new_width;
	}
}

void Map::setHeight(int new_height) {
	if (new_height > 65000) {
		height = 65000;
	} else if (new_height < 64) {
		height = 64;
	} else {
		height = new_height;
	}
}
void Map::setMapDescription(const std::string& new_description) {
	description = new_description;
}

void Map::setHouseFilename(const std::string& new_housefile) {
	housefile = new_housefile;
	unnamed = false;
}

void Map::setSpawnFilename(const std::string& new_spawnfile) {
	spawnfile = new_spawnfile;
	unnamed = false;
}

bool Map::addSpawn(Tile* tile) {
	Spawn* spawn = tile->spawn;
	if (spawn) {
		int z = tile->getZ();
		int start_x = tile->getX() - spawn->getSize();
		int start_y = tile->getY() - spawn->getSize();
		int end_x = tile->getX() + spawn->getSize();
		int end_y = tile->getY() + spawn->getSize();

		for (int y = start_y; y <= end_y; ++y) {
			for (int x = start_x; x <= end_x; ++x) {
				TileLocation* ctile_loc = createTileL(x, y, z);
				ctile_loc->increaseSpawnCount();
				if (ctile_loc->qtree_node && !isBatching()) { 
					ctile_loc->qtree_node->markDirty(z); 
				}
			}
		}
		g_luaScripts.emit("spawnChange", "add", tile);
		return true;
	}
	return false;
}

void Map::removeSpawnInternal(Tile* tile) {
	Spawn* spawn = tile->spawn;
	ASSERT(spawn);

	int z = tile->getZ();
	int start_x = tile->getX() - spawn->getSize();
	int start_y = tile->getY() - spawn->getSize();
	int end_x = tile->getX() + spawn->getSize();
	int end_y = tile->getY() + spawn->getSize();

	for (int y = start_y; y <= end_y; ++y) {
		for (int x = start_x; x <= end_x; ++x) {
			TileLocation* ctile_loc = getTileL(x, y, z);
			if (ctile_loc != nullptr && ctile_loc->getSpawnCount() > 0) {
				ctile_loc->decreaseSpawnCount();
				if (ctile_loc->qtree_node && !isBatching()) { 
					ctile_loc->qtree_node->markDirty(z); 
				}
			}
		}
	}
}

void Map::removeSpawn(Tile* tile) {
	if (tile->spawn) {
		removeSpawnInternal(tile);
		spawns.removeSpawn(tile);
		if (tile->getLocation() && tile->getLocation()->qtree_node) { // qtree_node ist jetzt public
			tile->getLocation()->qtree_node->markDirty(tile->getZ()); 
		}
		g_luaScripts.emit("spawnChange", "remove", tile);
	}
}

SpawnList Map::getSpawnList(Tile* where) {
	SpawnList list;
	TileLocation* tile_loc = where->getLocation();
	if (tile_loc) {
		if (tile_loc->getSpawnCount() > 0) {
			uint32_t found = 0;
			if (where->spawn) {
				++found;
				list.push_back(where->spawn);
			}

			// Scans the border tiles in an expanding square around the original spawn
			int z = where->getZ();
			int start_x = where->getX() - 1, end_x = where->getX() + 1;
			int start_y = where->getY() - 1, end_y = where->getY() + 1;
			while (found != tile_loc->getSpawnCount()) {
				for (int x = start_x; x <= end_x; ++x) {
					Tile* tile = getTile(x, start_y, z);
					if (tile && tile->spawn) {
						list.push_back(tile->spawn);
						++found;
					}
					tile = getTile(x, end_y, z);
					if (tile && tile->spawn) {
						list.push_back(tile->spawn);
						++found;
					}
				}

				for (int y = start_y + 1; y < end_y; ++y) {
					Tile* tile = getTile(start_x, y, z);
					if (tile && tile->spawn) {
						list.push_back(tile->spawn);
						++found;
					}
					tile = getTile(end_x, y, z);
					if (tile && tile->spawn) {
						list.push_back(tile->spawn);
						++found;
					}
				}
				--start_x, --start_y;
				++end_x, ++end_y;
			}
		}
	}
	return list;
}

bool Map::exportMinimap(FileName filename, int floor /*= GROUND_LAYER*/, bool displaydialog) {
	uint8_t* pic = nullptr;

	try {
		int min_x = 0x10000, min_y = 0x10000;
		int max_x = 0x00000, max_y = 0x00000;

		if (size() == 0) {
			return true;
		}

		for (MapIterator mit = begin(); mit != end(); ++mit) {
			if ((*mit)->get() == nullptr || (*mit)->empty()) {
				continue;
			}

			Position pos = (*mit)->getPosition();

			if (pos.x < min_x) {
				min_x = pos.x;
			}

			if (pos.y < min_y) {
				min_y = pos.y;
			}

			if (pos.x > max_x) {
				max_x = pos.x;
			}

			if (pos.y > max_y) {
				max_y = pos.y;
			}
		}

		int minimap_width = max_x - min_x + 1;
		int minimap_height = max_y - min_y + 1;

		pic = newd uint8_t[minimap_width * minimap_height]; // 1 byte per pixel

		memset(pic, 0, minimap_width * minimap_height);

		int tiles_iterated = 0;
		for (MapIterator mit = begin(); mit != end(); ++mit) {
			Tile* tile = (*mit)->get();
			++tiles_iterated;
			if (tiles_iterated % 8192 == 0 && displaydialog) {
				g_gui.SetLoadDone(int(tiles_iterated / double(tilecount) * 90.0));
			}

			if (tile->empty() || tile->getZ() != floor) {
				continue;
			}

			// std::cout << "Pixel : " << (tile->getY() - min_y) * width + (tile->getX() - min_x) << std::endl;
			uint32_t pixelpos = (tile->getY() - min_y) * minimap_width + (tile->getX() - min_x);
			uint8_t& pixel = pic[pixelpos];

			for (ItemVector::const_reverse_iterator item_iter = tile->items.rbegin(); item_iter != tile->items.rend(); ++item_iter) {
				if ((*item_iter)->getMiniMapColor()) {
					pixel = (*item_iter)->getMiniMapColor();
					break;
				}
			}
			if (pixel == 0) {
				// check ground too
				if (tile->hasGround()) {
					pixel = tile->ground->getMiniMapColor();
				}
			}
		}

		// Create a file for writing
		FileWriteHandle fh(nstr(filename.GetFullPath()));

		if (!fh.isOpen()) {
			delete[] pic;
			return false;
		}
		// Store the magic number
		fh.addRAW("BM");

		// Store the file size
		// We need to predict how large it will be
		uint32_t file_size = 14 // header
			+ 40 // image data header
			+ 256 * 4 // color palette
			+ ((minimap_width + 3) / 4 * 4) * minimap_height; // pixels
		fh.addU32(file_size);

		// Two values reserved, must always be 0.
		fh.addU16(0);
		fh.addU16(0);

		// Bitmapdata offset
		fh.addU32(14 + 40 + 256 * 4);

		// Header size
		fh.addU32(40);

		// Header width/height
		fh.addU32(minimap_width);
		fh.addU32(minimap_height);

		// Color planes
		fh.addU16(1);

		// bits per pixel, OT map format is 8
		fh.addU16(8);

		// compression type, 0 is no compression
		fh.addU32(0);

		// image size, 0 is valid if we use no compression
		fh.addU32(0);

		// horizontal/vertical resolution in pixels / meter
		fh.addU32(4000);
		fh.addU32(4000);

		// Number of colors
		fh.addU32(256);
		// Important colors, 0 is all
		fh.addU32(0);

		// Write the color palette
		for (int i = 0; i < 256; ++i) {
			fh.addU32(uint32_t(minimap_color[i]));
		}

		// Bitmap width must be divisible by four, calculate how much padding we need
		int padding = ((minimap_width & 3) != 0 ? 4 - (minimap_width & 3) : 0);
		// Bitmap rows are saved in reverse order
		for (int y = minimap_height - 1; y >= 0; --y) {
			fh.addRAW(pic + y * minimap_width, minimap_width);
			for (int i = 0; i < padding; ++i) {
				fh.addU8(0);
			}
			if (y % 100 == 0 && displaydialog) {
				g_gui.SetLoadDone(90 + int((minimap_height - y) / double(minimap_height) * 10.0));
			}
		}

		delete[] pic;
		// fclose(file);
		fh.close();
	} catch (...) {
		delete[] pic;
	}

	return true;
}
