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

#include "raw_brush.h"
#include "settings.h"
#include "items.h"
#include "basemap.h"

//=============================================================================
// RAW brush

RAWBrush::RAWBrush(uint16_t itemid) :
	Brush() {
	ItemType& it = g_items[itemid];
	if (it.id == 0) {
		itemtype = nullptr;
	} else {
		itemtype = &it;
	}
}

RAWBrush::~RAWBrush() {
	////
}

int RAWBrush::getLookID() const {
	if (itemtype) {
		return itemtype->id;
	}
	return 0;
}

uint16_t RAWBrush::getItemID() const {
	return itemtype->id;
}

std::string RAWBrush::getName() const {
	if (!itemtype) {
		return "RAWBrush";
	}

	if (itemtype->hookSouth) {
		return i2s(itemtype->id) + " - " + itemtype->name + " (Hook South)";
	} else if (itemtype->hookEast) {
		return i2s(itemtype->id) + " - " + itemtype->name + " (Hook East)";
	}

	return i2s(itemtype->id) + " - " + itemtype->name + itemtype->editorsuffix;
}

void RAWBrush::undraw(BaseMap* map, Tile* tile) {
	if (tile->ground && tile->ground->getID() == itemtype->id) {
		delete tile->ground;
		tile->ground = nullptr;
	}
	for (ItemVector::iterator iter = tile->items.begin(); iter != tile->items.end();) {
		Item* item = *iter;
		if (item->getID() == itemtype->id) {
			delete item;
			iter = tile->items.erase(iter);
		} else {
			++iter;
		}
	}
}

#include "brush_enums.h"

void RAWBrush::setItemID(uint16_t id) {
	ItemType& it = g_items[id];
	if (it.id != 0) {
		itemtype = &it;
	}
}

static void autoAlignHangableItem(BaseMap* map, Tile* tile, Item* item) {
	if (!item || !tile) return;
	const ItemType& it = g_items[item->getID()];
	if (it.rotateTo == 0) return;
	if (!it.isHangable && !it.hookEast && !it.hookSouth) return;

	bool is_horizontal_wall = false;
	bool is_vertical_wall = false;

	auto checkTileWalls = [&](Tile* t) {
		if (!t) return;
		for (Item* tile_item : t->items) {
			if (tile_item->isWall()) {
				BorderType bt = tile_item->getWallAlignment();
				if (bt == NORTH_HORIZONTAL || bt == SOUTH_HORIZONTAL || bt == WALL_HORIZONTAL) {
					is_horizontal_wall = true;
				} else if (bt == EAST_HORIZONTAL || bt == WEST_HORIZONTAL || bt == WALL_VERTICAL) {
					is_vertical_wall = true;
				}
			}
		}
	};

	checkTileWalls(tile);
	if (!is_horizontal_wall && !is_vertical_wall && map && tile->getPosition().isValid()) {
		Position p = tile->getPosition();
		if (p.y > 0) checkTileWalls(map->getTile(p.x, p.y - 1, p.z));
		if (p.x > 0) checkTileWalls(map->getTile(p.x - 1, p.y, p.z));
	}

	const ItemType& rot_it = g_items[it.rotateTo];

	if (is_horizontal_wall && !is_vertical_wall) {
		if (it.hookEast || (!it.hookSouth && rot_it.hookSouth)) {
			item->setID(it.rotateTo);
		}
	} else if (is_vertical_wall && !is_horizontal_wall) {
		if (it.hookSouth || (!it.hookEast && rot_it.hookEast)) {
			item->setID(it.rotateTo);
		}
	}
}

void RAWBrush::draw(BaseMap* map, Tile* tile, void* parameter) {
	if (!itemtype) {
		return;
	}

	bool b = parameter ? *reinterpret_cast<bool*>(parameter) : false;
	if ((g_settings.getInteger(Config::RAW_LIKE_SIMONE) && !b) && itemtype->alwaysOnBottom && itemtype->alwaysOnTopOrder == 2) {
		for (ItemVector::iterator iter = tile->items.begin(); iter != tile->items.end();) {
			Item* item = *iter;
			if (item->getTopOrder() == itemtype->alwaysOnTopOrder) {
				delete item;
				iter = tile->items.erase(iter);
			} else {
				++iter;
			}
		}
	}
	Item* new_item = Item::Create(itemtype->id);
	autoAlignHangableItem(map, tile, new_item);
	tile->addItem(new_item);
}

bool RAWBrush::isWall() const {
	return itemtype && itemtype->isWall;
}
