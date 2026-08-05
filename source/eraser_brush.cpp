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

#include "brush.h"
#include "sprites.h"
#include "basemap.h"
#include "settings.h"

//=============================================================================
// Eraser brush

EraserBrush::EraserBrush() {
	////
}

EraserBrush::~EraserBrush() {
	////
}

std::string EraserBrush::getName() const {
	return "Eraser";
}

int EraserBrush::getLookID() const {
	return EDITOR_SPRITE_ERASER;
}

bool EraserBrush::canDraw(BaseMap* map, const Position& position) const {
	return true;
}

void EraserBrush::undraw(BaseMap* map, Tile* tile) {
	draw(map, tile, nullptr);
}

void EraserBrush::draw(BaseMap* map, Tile* tile, void* parameter) {
	if (!tile) return;
	tile->unsetMapFlags(TILESTATE_PROTECTIONZONE | TILESTATE_NOPVP | TILESTATE_NOLOGOUT | TILESTATE_PVPZONE);

	bool leave_ground = g_settings.getInteger(Config::ERASER_LEAVE_GROUND);

	for (ItemVector::iterator item_iter = tile->items.begin(); item_iter != tile->items.end();) {
		Item* item = *item_iter;
		bool is_complex_protected = item->isComplex() && g_settings.getInteger(Config::ERASER_LEAVE_UNIQUE);
		bool is_border_protected = leave_ground && (item->isBorder() || g_items[item->getID()].isOptionalBorder);

		if (is_complex_protected || is_border_protected) {
			++item_iter;
		} else {
			delete item;
			item_iter = tile->items.erase(item_iter);
		}
	}
	if (leave_ground) {
		// Keep ground tile intact
		return;
	}
	if (tile->ground) {
		if (g_settings.getInteger(Config::ERASER_LEAVE_UNIQUE) && tile->ground->isComplex()) {
			// Leave unique complex ground if setting explicitly enabled
		} else {
			delete tile->ground;
			tile->ground = nullptr;
		}
	}
}

