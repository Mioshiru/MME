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

#ifndef RME_MAP_REGION_H
#define RME_MAP_REGION_H

#include "position.h"
#include <vector>
#include <span>

typedef std::vector<uint32_t> HouseExitList;

struct RenderBatch {
    uint32_t texture_id;
    uint32_t first;
    uint32_t count;
};

struct alignas(32) MapVertex {
    float x, y;
    float u, v;
    uint8_t r, g, b, a;
    float anim_frames; // Anzahl der Animationsframes
    float anim_speed;  // Geschwindigkeit/Offset
    float padding;     // Alignment auf 32 Byte
};

class Tile;
class Floor;
class BaseMap;
class QTreeNode;

class TileLocation {
	TileLocation();

public:
	~TileLocation();

	TileLocation(const TileLocation&) = delete;
	TileLocation& operator=(const TileLocation&) = delete;

protected:
	Tile* tile;
	Position position;
	size_t spawn_count;
	size_t waypoint_count;
	size_t town_count;
	HouseExitList* house_exits; // Any house exits pointing here
public: // Made public for easier access in map.cpp/tile.cpp
	QTreeNode* qtree_node;

public:
	// Access tile
	// Can't set directly since that does not update tile count
	Tile* get() {
		return tile;
	}
	const Tile* get() const {
		return tile;
	}

	int size() const;
	bool empty() const;

	Position getPosition() const {
		return position;
	}

	int getX() const {
		return position.x;
	}
	int getY() const {
		return position.y;
	}
	int getZ() const {
		return position.z;
	}

	size_t getSpawnCount() const {
		return spawn_count;
	}
	void increaseSpawnCount() {
		spawn_count++;
	}
	void decreaseSpawnCount() {
		spawn_count--;
	}
	size_t getWaypointCount() const {
		return waypoint_count;
	}
	void increaseWaypointCount() {
		waypoint_count++;
	}
	void decreaseWaypointCount() {
		waypoint_count--;
	}
	size_t getTownCount() const {
		return town_count;
	}
	void increaseTownCount() {
		town_count++;
	}
	void decreaseTownCount() {
		town_count--;
	}
	HouseExitList* createHouseExits() {
		if (house_exits) {
			return house_exits;
		}
		return house_exits = newd HouseExitList;
	}
	HouseExitList* getHouseExits() {
		return house_exits;
	}
	friend class Floor;
	friend class QTreeNode;
	friend class Waypoints;
};
 
class Floor {
public:
	Floor(QTreeNode* parent_node, int sx, int sy, int z);
    ~Floor();

	TileLocation locs[MAP_LAYERS];

    std::span<TileLocation> getLocations() { return {locs, MAP_LAYERS}; }
    std::span<const TileLocation> getLocations() const { return {locs, MAP_LAYERS}; }
    
    // REDUX RENDERING DATA
    uint32_t vbo_id = 0;
    std::vector<RenderBatch> batches;
    bool is_empty = true;
    bool has_opaque_ground = false;
    uint64_t last_rebuild_tick = 0;
};

// This is not a QuadTree, but a HexTree (16 child nodes to every node), so the name is abit misleading
class QTreeNode {
public:
	QTreeNode(BaseMap& map);
	virtual ~QTreeNode();

	QTreeNode(const QTreeNode&) = delete;
	QTreeNode& operator=(const QTreeNode&) = delete;

	QTreeNode* getLeaf(int x, int y); // Might return nullptr
	QTreeNode* getLeafForce(int x, int y); // Will never return nullptr, it will create the node if it's not there

	// Coordinates are NOT relative
	TileLocation* createTile(int x, int y, int z);
	TileLocation* getTile(int x, int y, int z);
	Tile* setTile(int x, int y, int z, Tile* tile);
	void clearTile(int x, int y, int z);

	void markDirty(int z) { dirty_flags |= (1 << z); }
	bool isDirty(int z) const { return (dirty_flags & (1 << z)) != 0; }
	void clearDirty(int z) { dirty_flags &= ~(1 << z); }
    void clearAllDirty(int z);

	Floor* createFloor(BaseMap* map, int x, int y, int z);
	Floor* getFloor(uint32_t z) {
		ASSERT(isLeaf);
		return array[z];
	}
    std::span<Floor*> getFloorsSpan() {
        ASSERT(isLeaf);
        return {array, MAP_LAYERS};
    }

	Floor** getFloors() {
		return array;
	}

	void setVisible(bool overground, bool underground);
	void setVisible(uint32_t client, bool underground, bool value);
	bool isVisible(uint32_t client, bool underground);
	void clearVisible(uint32_t client);

	void setRequested(bool underground, bool r);
	bool isVisible(bool underground);
	bool isRequested(bool underground);

protected:
	BaseMap& map;
	uint32_t visible;
	uint16_t dirty_flags;

	bool isLeaf;
	union {
		QTreeNode* child[MAP_LAYERS];
		Floor* array[MAP_LAYERS];
		/*
		#if 16 != MAP_LAYERS
		#    error "You need to rewrite the QuadTree in order to handle more or less than 16 floors"
		#endif
		*/
	};

	friend class BaseMap;
	friend class MapIterator;
};

#endif
