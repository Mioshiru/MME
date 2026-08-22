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
#include "iomap_sec.h"
#include "map.h"
#include "tile.h"
#include "item.h"
#include "items.h"
#include "creatures.h"
#include "spawn.h"
#include "gui.h"

#include <wx/dir.h>
#include <wx/filename.h>
#include <sstream>
#include <iomanip>
#include <algorithm>

IOMapSEC::IOMapSEC() {
	setupDefaultLegacyMappings();
}

IOMapSEC::~IOMapSEC() {
}

void IOMapSEC::setupDefaultLegacyMappings() {
	// Standard 7.4 / 7.72 default identity mappings for basic items
	for (uint16_t id = 100; id < 4000; ++id) {
		server_to_client[id] = id;
		client_to_server[id] = id;
	}
}

bool IOMapSEC::CoordinateToSector(int x, int y, int z, int& sec_x, int& sec_y, int& sec_z) {
	if (x < 0 || y < 0 || z < 0 || z > 15) return false;
	sec_x = x / 32;
	sec_y = y / 32;
	sec_z = z;
	return true;
}

void IOMapSEC::SectorToCoordinate(int sec_x, int sec_y, int sec_z, int& min_x, int& min_y, int& z) {
	min_x = sec_x * 32;
	min_y = sec_y * 32;
	z = sec_z;
}

std::string IOMapSEC::MakeSectorFilename(int sec_x, int sec_y, int sec_z) {
	std::ostringstream ss;
	ss << std::setfill('0') << std::setw(3) << sec_x
	   << std::setfill('0') << std::setw(3) << sec_y
	   << std::setfill('0') << std::setw(2) << sec_z
	   << ".sec";
	return ss.str();
}

bool IOMapSEC::ParseSectorFilename(const std::string& filename, int& sec_x, int& sec_y, int& sec_z) {
	// Expected format: 10010007.sec (8 digits + .sec)
	wxFileName fn(wxstr(filename));
	std::string name = nstr(fn.GetName());
	if (name.length() != 8) return false;

	for (char c : name) {
		if (!isdigit(static_cast<unsigned char>(c))) return false;
	}

	try {
		sec_x = std::stoi(name.substr(0, 3));
		sec_y = std::stoi(name.substr(3, 3));
		sec_z = std::stoi(name.substr(6, 2));
		return true;
	} catch (...) {
		return false;
	}
}

uint16_t IOMapSEC::ServerToClientId(uint16_t server_id) const {
	auto it = server_to_client.find(server_id);
	if (it != server_to_client.end()) return it->second;
	return server_id;
}

uint16_t IOMapSEC::ClientToServerId(uint16_t client_id) const {
	auto it = client_to_server.find(client_id);
	if (it != client_to_server.end()) return it->second;
	return client_id;
}

const CipObjectDef* IOMapSEC::GetObjectDef(uint16_t server_id) const {
	auto it = objects_map.find(server_id);
	if (it != objects_map.end()) return &it->second;
	return nullptr;
}

bool IOMapSEC::loadObjectsSrv(const std::string& filepath) {
	std::ifstream file(filepath);
	if (!file.is_open()) return false;

	objects_map.clear();
	server_to_client.clear();
	client_to_server.clear();

	std::string line;
	CipObjectDef current_def;
	bool in_type = false;

	while (std::getline(file, line)) {
		// Trim whitespace
		line.erase(0, line.find_first_not_of(" \t\r\n"));
		line.erase(line.find_last_not_of(" \t\r\n") + 1);

		if (line.empty() || line[0] == '#') continue;

		if (line.rfind("TypeID", 0) == 0) {
			if (in_type && current_def.server_id > 0) {
				objects_map[current_def.server_id] = current_def;
				server_to_client[current_def.server_id] = current_def.client_id ? current_def.client_id : current_def.server_id;
				client_to_server[current_def.client_id ? current_def.client_id : current_def.server_id] = current_def.server_id;
			}
			current_def = CipObjectDef();
			in_type = true;

			size_t eq = line.find('=');
			if (eq != std::string::npos) {
				current_def.server_id = std::stoi(line.substr(eq + 1));
				current_def.client_id = current_def.server_id;
			}
		} else if (in_type) {
			size_t eq = line.find('=');
			if (eq != std::string::npos) {
				std::string key = line.substr(0, eq);
				std::string val = line.substr(eq + 1);
				key.erase(0, key.find_first_not_of(" \t"));
				key.erase(key.find_last_not_of(" \t") + 1);
				val.erase(0, val.find_first_not_of(" \t\""));
				val.erase(val.find_last_not_of(" \t\"") + 1);

				if (key == "Name") {
					current_def.name = val;
				} else if (key == "ClientID") {
					current_def.client_id = std::stoi(val);
				} else if (key == "Weight") {
					current_def.weight = std::stoi(val);
				} else if (key == "Flags") {
					if (val.find("Bank") != std::string::npos) current_def.is_ground = true;
					if (val.find("Clip") != std::string::npos) current_def.is_blocking = true;
					if (val.find("Chest") != std::string::npos || val.find("Container") != std::string::npos) current_def.is_container = true;
					if (val.find("Cumulative") != std::string::npos) current_def.is_stackable = true;
					if (val.find("Take") != std::string::npos) current_def.is_pickupable = true;
					if (val.find("Unpass") != std::string::npos) current_def.is_unpassable = true;
				} else {
					current_def.attributes[key] = val;
				}
			}
		}
	}

	if (in_type && current_def.server_id > 0) {
		objects_map[current_def.server_id] = current_def;
		server_to_client[current_def.server_id] = current_def.client_id ? current_def.client_id : current_def.server_id;
		client_to_server[current_def.client_id ? current_def.client_id : current_def.server_id] = current_def.server_id;
	}

	return !objects_map.empty();
}

bool IOMapSEC::loadMonsterDb(Map& map, const std::string& filepath) {
	std::ifstream file(filepath);
	if (!file.is_open()) return false;

	spawns_list.clear();
	std::string line;

	while (std::getline(file, line)) {
		line.erase(0, line.find_first_not_of(" \t\r\n"));
		line.erase(line.find_last_not_of(" \t\r\n") + 1);
		if (line.empty() || line[0] == '#') continue;

		// Format: "Demon" 32345 32221 7 1 60 2
		// or Name X Y Z Count RespawnTime Radius
		std::istringstream ss(line);
		std::string name;
		char first;
		ss >> std::ws;
		if (ss.peek() == '"') {
			ss >> first; // consume opening quote
			std::getline(ss, name, '"');
		} else {
			ss >> name;
		}

		int x = 0, y = 0, z = 0, count = 1, respawn = 60, radius = 1;
		if (ss >> x >> y >> z) {
			if (!(ss >> count)) count = 1;
			if (!(ss >> respawn)) respawn = 60;
			if (!(ss >> radius)) radius = 1;

			CipSpawnEntry entry;
			entry.monster_name = name;
			entry.pos = Position(x, y, z);
			entry.count = count;
			entry.respawn_time = respawn;
			entry.spawn_radius = radius;
			spawns_list.push_back(entry);

			// Add creature to map
			Tile* tile = map.getTile(entry.pos);
			if (!tile) {
				tile = map.allocator(map.createTileL(entry.pos.x, entry.pos.y, entry.pos.z));
			}
			if (tile) {
				CreatureType* ct = g_creatures[entry.monster_name];
				if (!ct) {
					ct = g_creatures.addMissingCreatureType(entry.monster_name, false);
				}
				if (ct) {
					Creature* c = new Creature(ct);
					tile->creature = c;
				}
			}
		}
	}
	return true;
}

bool IOMapSEC::saveMonsterDb(Map& map, const std::string& filepath) {
	std::ofstream file(filepath);
	if (!file.is_open()) return false;

	file << "# RealOTS / CipSoft monster.db exported by MME\n";
	file << "# Format: \"MonsterName\" X Y Z Count RespawnTime Radius\n\n";

	for (const auto& entry : spawns_list) {
		file << "\"" << entry.monster_name << "\" "
		     << entry.pos.x << " " << entry.pos.y << " " << entry.pos.z << " "
		     << entry.count << " " << entry.respawn_time << " " << entry.spawn_radius << "\n";
	}

	return true;
}

bool IOMapSEC::loadSectorFile(Map& map, const std::string& filepath, int sec_x, int sec_y, int sec_z) {
	std::ifstream file(filepath, std::ios::binary);
	if (!file.is_open()) return false;

	int min_x, min_y, z;
	SectorToCoordinate(sec_x, sec_y, sec_z, min_x, min_y, z);

	// CipSoft .sec sector files contain a 32x32 array of tiles
	// Each tile starts with count of items or tile data block
	for (int dy = 0; dy < 32; ++dy) {
		for (int dx = 0; dx < 32; ++dx) {
			int x = min_x + dx;
			int y = min_y + dy;

			uint16_t item_count = 0;
			if (!file.read(reinterpret_cast<char*>(&item_count), sizeof(uint16_t))) {
				return true; // Reached EOF or end of sector data gracefully
			}

			if (item_count == 0) continue;

			Tile* tile = map.getTile(x, y, z);
			if (!tile) {
				tile = map.allocator(map.createTileL(x, y, z));
			}

			for (uint16_t i = 0; i < item_count; ++i) {
				uint16_t server_id = 0;
				if (!file.read(reinterpret_cast<char*>(&server_id), sizeof(uint16_t))) break;

				uint16_t client_id = ServerToClientId(server_id);
				ItemType& it = g_items.getItemIdByClientID(client_id);
				uint16_t rme_id = (it.id > 0) ? it.id : client_id;

				Item* item = Item::Create(rme_id);
				if (item) {
					// Check if item has attributes or count
					if (item->isStackable()) {
						uint8_t count = 1;
						file.read(reinterpret_cast<char*>(&count), sizeof(uint8_t));
						item->setSubtype(count);
					}
					tile->addItem(item);
				}
			}
		}
	}
	return true;
}

bool IOMapSEC::saveSectorFile(Map& map, const std::string& filepath, int sec_x, int sec_y, int sec_z) {
	std::ofstream file(filepath, std::ios::binary);
	if (!file.is_open()) return false;

	int min_x, min_y, z;
	SectorToCoordinate(sec_x, sec_y, sec_z, min_x, min_y, z);

	bool has_any_tile = false;

	for (int dy = 0; dy < 32; ++dy) {
		for (int dx = 0; dx < 32; ++dx) {
			int x = min_x + dx;
			int y = min_y + dy;

			Tile* tile = map.getTile(x, y, z);
			if (!tile || (tile->size() == 0 && !tile->ground)) {
				uint16_t zero_count = 0;
				file.write(reinterpret_cast<const char*>(&zero_count), sizeof(uint16_t));
				continue;
			}

			has_any_tile = true;
			std::vector<Item*> items_to_save;
			if (tile->ground) items_to_save.push_back(tile->ground);
			for (Item* item : tile->items) {
				if (item) items_to_save.push_back(item);
			}

			uint16_t item_count = static_cast<uint16_t>(items_to_save.size());
			file.write(reinterpret_cast<const char*>(&item_count), sizeof(uint16_t));

			for (Item* item : items_to_save) {
				uint16_t client_id = item->getClientID();
				uint16_t server_id = ClientToServerId(client_id);
				file.write(reinterpret_cast<const char*>(&server_id), sizeof(uint16_t));

				if (item->isStackable()) {
					uint8_t count = static_cast<uint8_t>(item->getCount());
					file.write(reinterpret_cast<const char*>(&count), sizeof(uint8_t));
				}
			}
		}
	}

	return has_any_tile;
}

bool IOMapSEC::loadMap(Map& map, const FileName& identifier) {
	wxString path = identifier.GetFullPath();
	wxFileName fn(path);

	// Check if directory of .sec files or single .sec file
	wxString dir_path = fn.IsDir() ? path : fn.GetPath();

	// Check if objects.srv and monster.db exist in the directory
	wxString obj_path = dir_path + "/objects.srv";
	if (wxFileExists(obj_path)) {
		loadObjectsSrv(nstr(obj_path));
	}

	wxString mon_path = dir_path + "/monster.db";
	if (wxFileExists(mon_path)) {
		loadMonsterDb(map, nstr(mon_path));
	}

	if (!fn.IsDir() && fn.GetExt().Lower() == "sec") {
		int sx = 0, sy = 0, sz = 0;
		if (ParseSectorFilename(nstr(fn.GetFullName()), sx, sy, sz)) {
			return loadSectorFile(map, nstr(path), sx, sy, sz);
		}
	}

	// Iterate all .sec files in the directory
	wxDir dir(dir_path);
	if (!dir.IsOpened()) {
		error("Failed to open sector directory: " + dir_path);
		return false;
	}

	wxString filename;
	bool cont = dir.GetFirst(&filename, "*.sec", wxDIR_FILES);
	int loaded_sectors = 0;

	while (cont) {
		int sx = 0, sy = 0, sz = 0;
		if (ParseSectorFilename(nstr(filename), sx, sy, sz)) {
			wxString full_sec_path = dir_path + "/" + filename;
			if (loadSectorFile(map, nstr(full_sec_path), sx, sy, sz)) {
				loaded_sectors++;
			}
		}
		cont = dir.GetNext(&filename);
	}

	if (loaded_sectors == 0) {
		warning("No valid .sec sector files found in directory.");
		return false;
	}

	return true;
}

bool IOMapSEC::saveMap(Map& map, const FileName& identifier) {
	wxString path = identifier.GetFullPath();
	wxFileName fn(path);
	wxString dir_path = fn.IsDir() ? path : fn.GetPath();

	if (!wxDirExists(dir_path)) {
		wxMkdir(dir_path);
	}

	// Calculate map bounding box in sectors
	// Tibia standard map coordinates: 0..65535, typical active: 31000..34000
	std::set<std::tuple<int, int, int>> active_sectors;

	for (int z = 0; z <= 15; ++z) {
		// Sample populated tiles in the map to find active sectors
		for (auto it = map.begin(); it != map.end(); ++it) {
			TileLocation* loc = *it;
			if (loc) {
				Tile* t = loc->get();
				if (t && (t->ground || t->size() > 0)) {
					Position pos = loc->getPosition();
					int sx, sy, sz;
					if (CoordinateToSector(pos.x, pos.y, pos.z, sx, sy, sz)) {
						active_sectors.insert(std::make_tuple(sx, sy, sz));
					}
				}
			}
		}
	}

	for (const auto& sec : active_sectors) {
		int sx = std::get<0>(sec);
		int sy = std::get<1>(sec);
		int sz = std::get<2>(sec);

		std::string sec_fname = MakeSectorFilename(sx, sy, sz);
		std::string full_sec_path = nstr(dir_path) + "/" + sec_fname;
		saveSectorFile(map, full_sec_path, sx, sy, sz);
	}

	// Save monster.db
	std::string mon_path = nstr(dir_path) + "/monster.db";
	saveMonsterDb(map, mon_path);

	return true;
}
