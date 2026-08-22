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

#ifndef RME_IOMAP_SEC_H_
#define RME_IOMAP_SEC_H_

#include "iomap.h"
#include "position.h"
#include <map>
#include <vector>
#include <string>
#include <fstream>

class Map;
class Tile;
class Item;

struct CipObjectDef {
	uint16_t server_id = 0;
	uint16_t client_id = 0;
	std::string name;
	uint32_t flags = 0;
	uint32_t weight = 0;
	bool is_ground = false;
	bool is_blocking = false;
	bool is_container = false;
	bool is_stackable = false;
	bool is_unpassable = false;
	bool is_pickupable = false;
	std::map<std::string, std::string> attributes;
};

struct CipSpawnEntry {
	std::string monster_name;
	Position pos;
	int count = 1;
	int respawn_time = 60; // in seconds
	int spawn_radius = 1;
};

class IOMapSEC : public IOMap {
public:
	IOMapSEC();
	virtual ~IOMapSEC();

	virtual bool loadMap(Map& map, const FileName& identifier) override;
	virtual bool saveMap(Map& map, const FileName& identifier) override;

	// Load individual sector file (e.g. 10010007.sec)
	bool loadSectorFile(Map& map, const std::string& filepath, int sector_x, int sector_y, int sector_z);
	// Save individual sector file
	bool saveSectorFile(Map& map, const std::string& filepath, int sector_x, int sector_y, int sector_z);

	// Parse objects.srv
	bool loadObjectsSrv(const std::string& filepath);
	// Parse monster.db
	bool loadMonsterDb(Map& map, const std::string& filepath);
	// Save monster.db
	bool saveMonsterDb(Map& map, const std::string& filepath);

	// ID Translation
	uint16_t ServerToClientId(uint16_t server_id) const;
	uint16_t ClientToServerId(uint16_t client_id) const;
	const CipObjectDef* GetObjectDef(uint16_t server_id) const;

	const std::map<uint16_t, CipObjectDef>& GetObjects() const { return objects_map; }
	const std::vector<CipSpawnEntry>& GetSpawns() const { return spawns_list; }

	static bool CoordinateToSector(int x, int y, int z, int& sec_x, int& sec_y, int& sec_z);
	static void SectorToCoordinate(int sec_x, int sec_y, int sec_z, int& min_x, int& min_y, int& z);
	static std::string MakeSectorFilename(int sec_x, int sec_y, int sec_z);
	static bool ParseSectorFilename(const std::string& filename, int& sec_x, int& sec_y, int& sec_z);

private:
	std::map<uint16_t, CipObjectDef> objects_map;
	std::map<uint16_t, uint16_t> server_to_client;
	std::map<uint16_t, uint16_t> client_to_server;
	std::vector<CipSpawnEntry> spawns_list;

	void setupDefaultLegacyMappings();
};

#endif // RME_IOMAP_SEC_H_
