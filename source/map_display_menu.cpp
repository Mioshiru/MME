#include "main.h"
#include "map_display.h"
#include "gui.h"
#include "editor.h"
#include "settings.h"
#include "live_socket.h"
#include "live_client.h"
#include "live_server.h"
#include "live_peer.h"
#include "tileset_window.h"
#include "old_properties_window.h"
#include "properties_window.h"
#include "common_windows.h"
#include "lua/lua_script_manager.h"
#include "doodad_brush.h"
#include "house_brush.h"
#include "wall_brush.h"
#include "spawn_brush.h"
#include "creature_brush.h"
#include "ground_brush.h"
#include "raw_brush.h"
#include "carpet_brush.h"
#include "table_brush.h"
#include "brush.h"
#include "complexitem.h"
#include "application.h"
#include "materials.h"
#include "spawn.h"
#include "live_approval_window.h"

uint16_t getItemUseSwitchID(Item* item) {
	if (!item || item->getID() == 0) return 0;
	uint16_t id = item->getID();

	// 1. Direct explicit table of known Tibia light & interactive pairs
	static const std::map<uint16_t, uint16_t> switch_pairs = {
		// Street lamps & Lamps
		{ 1479, 1480 }, { 1480, 1479 },
		{ 2041, 2042 }, { 2042, 2041 },
		{ 2043, 2044 }, { 2044, 2043 },
		{ 2045, 2046 }, { 2046, 2045 },
		{ 2047, 2048 }, { 2048, 2047 },
		{ 2049, 2050 }, { 2050, 2049 },
		{ 2057, 2058 }, { 2058, 2057 },
		// Coal basins
		{ 1484, 1485 }, { 1485, 1484 }, { 1486, 1485 },
		// Braziers & Amphoras
		{ 1481, 1482 }, { 1482, 1481 }, { 1487, 1488 }, { 1488, 1487 },
		// Torches
		{ 2050, 2051 }, { 2051, 2050 }, { 2052, 2050 }, { 2053, 2050 }, { 2054, 2050 }, { 2055, 2050 },
		// Wall Torches
		{ 1492, 1493 }, { 1493, 1492 }, { 1494, 1495 }, { 1495, 1494 },
		{ 1496, 1497 }, { 1497, 1496 }, { 1498, 1499 }, { 1499, 1498 },
		// Campfires & Chimneys
		{ 1421, 1422 }, { 1422, 1421 }, { 1423, 1424 }, { 1424, 1423 },
		{ 1425, 1426 }, { 1426, 1425 }, { 1427, 1428 }, { 1428, 1427 },
		{ 1780, 1781 }, { 1781, 1780 }, { 1782, 1783 }, { 1783, 1782 },
		{ 7131, 7132 }, { 7132, 7131 },
		// Levers & Switches
		{ 1945, 1946 }, { 1946, 1945 }, { 1947, 1948 }, { 1948, 1947 },
		// Chests & Boxes
		{ 1738, 1739 }, { 1739, 1738 }, { 1740, 1741 }, { 1741, 1740 },
		{ 1746, 1747 }, { 1747, 1746 }, { 1748, 1749 }, { 1749, 1748 },
		{ 1750, 1751 }, { 1751, 1750 }, { 1752, 1753 }, { 1753, 1752 }
	};

	auto itPair = switch_pairs.find(id);
	if (itPair != switch_pairs.end()) {
		if (g_items.typeExists(itPair->second)) {
			return itPair->second;
		}
	}

	const ItemType& it = g_items.getItemType(id);
	std::string name = it.name;
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);
	bool has_light = (it.sprite && it.sprite->hasLight());

	// Clean name helper (strips "lit", "unlit", "burning", "active", "off", "on", "extinguished")
	auto cleanName = [](std::string s) -> std::string {
		std::vector<std::string> words = { "lit ", " unlit", "unlit ", "burning ", " active", "active ", " extinguished", " (lit)", " (off)", " (on)", " (burning)" };
		for (const auto& w : words) {
			size_t pos = s.find(w);
			if (pos != std::string::npos) s.erase(pos, w.length());
		}
		return s;
	};

	std::string rootName = cleanName(name);

	// Helper to check if candidate is a compatible light/interactive toggle
	auto isCandidate = [&](int target_id) -> bool {
		if (target_id <= 0 || target_id > 65535) return false;
		if (!g_items.typeExists(target_id)) return false;

		const ItemType& tit = g_items.getItemType(target_id);
		std::string tname = tit.name;
		std::transform(tname.begin(), tname.end(), tname.begin(), ::tolower);
		std::string trootName = cleanName(tname);

		bool thas_light = (tit.sprite && tit.sprite->hasLight());

		if (!rootName.empty() && rootName == trootName) {
			if (has_light != thas_light || name.find("lever") != std::string::npos || name.find("switch") != std::string::npos || name.find("chest") != std::string::npos) {
				return true;
			}
		}

		// Partial keyword matching for fire / light items
		static const std::vector<std::string> keywords = {
			"basin", "lamp", "torch", "candle", "brazier", "campfire", "fire", "chimney", "lantern", "candelabr", "amphora", "lever", "switch", "chest"
		};
		for (const auto& kw : keywords) {
			if (name.find(kw) != std::string::npos && tname.find(kw) != std::string::npos) {
				if (has_light != thas_light || name.find("lever") != std::string::npos || name.find("switch") != std::string::npos) {
					return true;
				}
			}
		}
		return false;
	};

	const int offsets[] = { 1, -1, 2, -2 };
	for (int offset : offsets) {
		int target_id = (int)id + offset;
		if (isCandidate(target_id)) {
			return (uint16_t)target_id;
		}
	}

	return 0;
}

void MapCanvas::OnSelectCreatureBrush(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) {
		tile = editor.map.getTile(last_click_map_x, last_click_map_y, floor);
	}
	if (!tile) {
		return;
	}

	if (tile->creature) {
		g_gui.SelectBrush(tile->creature->getBrush(), TILESET_CREATURE);
	}
}

void MapCanvas::OnSelectSpawnBrush(wxCommandEvent& WXUNUSED(event)) {
	g_gui.SelectBrush(g_gui.spawn_brush, TILESET_CREATURE);
}

void MapCanvas::OnSelectMoveTo(wxCommandEvent& WXUNUSED(event)) {
	if (editor.selection.size() != 1) {
		return;
	}

	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) {
		return;
	}
	ASSERT(tile->isSelected());
	Tile* new_tile = tile->deepCopy(editor.map);

	wxDialog* w = nullptr; // unique_ptr verwaltet dies jetzt

	ItemVector selected_items = new_tile->getSelectedItems();

	Item* item = nullptr;
	for (Item* it : selected_items) {
		if (it->isSelected()) {
			item = it;
			break;
		}
	}

	int ret = 0;
	if (item) {
		TilesetWindow w(g_gui.root, &editor.map, new_tile, item);
		ret = w.ShowModal();
	} else {
		delete new_tile;
		return;
	}

	if (ret != 0) {
		Action* action = editor.actionQueue->createAction(ACTION_CHANGE_PROPERTIES);
		action->addChange(newd Change(new_tile));
		editor.addAction(action);

		g_gui.RebuildPalettes();
	} else {
		// Cancel!
		delete new_tile;
	}
}

void MapCanvas::OnProperties(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = nullptr;
	Item* item = nullptr;

	if (editor.selection.size() == 1) {
		tile = editor.selection.getSelectedTile();
		if (tile) {
			ItemVector selected_items = tile->getSelectedItems();
			for (Item* it : selected_items) {
				if (it->isSelected()) {
					item = it;
					break;
				}
			}
		}
	} else if (editor.selection.size() == 0) {
		tile = editor.map.getTile(last_click_map_x, last_click_map_y, floor);
		if (tile) {
			item = tile->getTopItem();
		}
	}

	if (!tile || (!item && !tile->spawn && !tile->creature)) {
		return;
	}

	if (editor.IsLive()) {
		Position pos = tile->getPosition();
		auto& live = editor.GetLive();
		auto it = live.lockedEntities.find(pos);
		if (it != live.lockedEntities.end()) {
			if (editor.IsLiveClient() && it->second.ownerId != 0) {
				g_gui.SetStatusText(wxString::Format("Access denied: Position is currently being edited by '%s'!", it->second.ownerName));
				wxMessageBox(wxString::Format("This position/property is currently being edited by '%s'!", it->second.ownerName), "Lock Active", wxOK | wxICON_WARNING);
				return;
			}
		}
	}

	Tile* new_tile = tile->deepCopy(editor.map);

	int ret = 0;
	Item* new_item = nullptr;

	if (new_tile->spawn && g_settings.getInteger(Config::SHOW_SPAWNS)) {
		OldPropertiesWindow w(g_gui.root, &editor.map, new_tile, new_tile->spawn);
		ret = w.ShowModal();
	} else if (new_tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)) {
		OldPropertiesWindow w(g_gui.root, &editor.map, new_tile, new_tile->creature);
		ret = w.ShowModal();
	} else if (item) {
		if (item == tile->ground) {
			new_item = new_tile->ground;
		} else {
			for (size_t i = 0; i < tile->items.size() && i < new_tile->items.size(); ++i) {
				if (tile->items[i] == item) {
					new_item = new_tile->items[i];
					break;
				}
			}
		}
		if (new_item) {
			PropertiesWindow w(g_gui.root, &editor.map, new_tile, new_item);
			ret = w.ShowModal();
		} else {
			delete new_tile;
			return;
		}
	} else {
		delete new_tile;
		return;
	}

	if (ret == wxID_OK || ret == 1) {
		Action* action = editor.actionQueue->createAction(ACTION_CHANGE_PROPERTIES);
		action->addChange(newd Change(new_tile));
		editor.addAction(action);
	} else {
		// Cancel!
		delete new_tile;
	}
}

MapPopupMenu::MapPopupMenu(MapEditor& map_editor_ref) :
	wxMenu(), editor(map_editor_ref) {
}

MapPopupMenu::~MapPopupMenu() {
	////
}

void MapPopupMenu::Update() {
	// Clear the menu of all items
	while (GetMenuItemCount() != 0) {
		wxMenuItem* m_item = FindItemByPosition(0);
		Delete(m_item);
	}

	bool anything_selected = editor.selection.size() != 0;

	if (anything_selected) {
		Append(MAP_POPUP_MENU_CUT, "Cut\tCtrl+X", "Cut selected tiles/items");
		Append(MAP_POPUP_MENU_COPY, "Copy\tCtrl+C", "Copy selected tiles/items");
	}
	if (editor.copybuffer.canPaste()) {
		Append(MAP_POPUP_MENU_PASTE, "Paste\tCtrl+V", "Paste copied tiles/items");
	}
	if (anything_selected || editor.copybuffer.canPaste()) {
		AppendSeparator();
	}

	if (anything_selected) {
		if (editor.selection.size() == 1) {
			Tile* tile = editor.selection.getSelectedTile();
			ItemVector selected_items = tile->getSelectedItems();

			bool hasWall = false;
			bool hasCarpet = false;
			bool hasTable = false;
			bool hasCollection = false;
			Item* topItem = nullptr;
			Item* topSelectedItem = (selected_items.size() == 1 ? selected_items.back() : nullptr);
			Creature* topCreature = tile->creature;
			Spawn* topSpawn = tile->spawn;

			for (auto* item : tile->items) {
				if (item->isWall()) {
					Brush* wb = item->getWallBrush();
					if (wb && wb->visibleInPalette()) {
						hasWall = true;
						hasCollection = hasCollection || wb->hasCollection();
					}
				}
				if (item->isTable()) {
					Brush* tb = item->getTableBrush();
					if (tb && tb->visibleInPalette()) {
						hasTable = true;
						hasCollection = hasCollection || tb->hasCollection();
					}
				}
				if (item->isCarpet()) {
					Brush* cb = item->getCarpetBrush();
					if (cb && cb->visibleInPalette()) {
						hasCarpet = true;
						hasCollection = hasCollection || cb->hasCollection();
					}
				}
				if (Brush* db = item->getDoodadBrush()) {
					hasCollection = hasCollection || db->hasCollection();
				}
				if (item->isSelected()) {
					topItem = item;
				}
			}
			if (!topItem) {
				topItem = tile->ground;
			}

			if (topSelectedItem || topCreature || topItem) {
				bool has_items_added = false;

				Item* rotatableItem = topSelectedItem ? topSelectedItem : topItem;
				Append(MAP_POPUP_MENU_CHANGE, "&Change...\tAlt+C", "Change connected elements to a different brush or type via Palette");
				if (rotatableItem && rotatableItem->isRoteable()) {
					Append(MAP_POPUP_MENU_ROTATE, "&Rotate item", "Rotate this item");
					has_items_added = true;
				}

				if (topCreature) {
					if (has_items_added) AppendSeparator();
					Append(MAP_POPUP_MENU_SELECT_CREATURE_BRUSH, "Select Creature", "Uses the current creature as a creature brush");
					has_items_added = true;
				}

				if (topSpawn) {
					if (has_items_added) AppendSeparator();
					Append(MAP_POPUP_MENU_SELECT_SPAWN_BRUSH, "Select Spawn", "Select the spawn brush");
					has_items_added = true;
				}

				if (topItem) {
					if (has_items_added) AppendSeparator();
					Append(MAP_POPUP_MENU_SELECT_RAW_BRUSH, "Select RAW", "Uses the top item as a RAW brush");
					has_items_added = true;
				}

				if (hasWall) {
					Append(MAP_POPUP_MENU_SELECT_WALL_BRUSH, "Select Wallbrush", "Uses the current item as a wallbrush");
				}

				if (hasCarpet) {
					Append(MAP_POPUP_MENU_SELECT_CARPET_BRUSH, "Select Carpetbrush", "Uses the current item as a carpetbrush");
				}

				Brush* foundDoodadBrush = nullptr;
				if (topSelectedItem && topSelectedItem->getDoodadBrush()) {
					foundDoodadBrush = topSelectedItem->getDoodadBrush();
				} else {
					for (auto it = tile->items.rbegin(); it != tile->items.rend(); ++it) {
						if ((*it)->getDoodadBrush()) {
							foundDoodadBrush = (*it)->getDoodadBrush();
							break;
						}
					}
				}
				if (foundDoodadBrush && foundDoodadBrush->visibleInPalette()) {
					Append(MAP_POPUP_MENU_SELECT_DOODAD_BRUSH, "Select Doodadbrush", "Use this doodad brush");
				}

				Brush* foundDoorBrush = nullptr;
				if (topSelectedItem && topSelectedItem->isBrushDoor() && topSelectedItem->getDoorBrush()) {
					foundDoorBrush = topSelectedItem->getDoorBrush();
				} else {
					for (auto it = tile->items.rbegin(); it != tile->items.rend(); ++it) {
						if ((*it)->isBrushDoor() && (*it)->getDoorBrush()) {
							foundDoorBrush = (*it)->getDoorBrush();
							break;
						}
					}
				}
				if (foundDoorBrush) {
					Append(MAP_POPUP_MENU_SELECT_DOOR_BRUSH, "Select Doorbrush", "Use this door brush");
				}

				bool can_use = false;
				if (topSelectedItem) {
					if (topSelectedItem->isBrushDoor() || getItemUseSwitchID(topSelectedItem) != 0 || topSelectedItem->isContainer()) {
						can_use = true;
					}
				} else if (topItem) {
					if (topItem->isBrushDoor() || getItemUseSwitchID(topItem) != 0 || topItem->isContainer()) {
						can_use = true;
					}
				}
				if (can_use) {
					Append(MAP_POPUP_MENU_SWITCH_DOOR, "Use", "Use, toggle light, or interact with this item");
				}

				if (tile->hasGround() && tile->getGroundBrush() && tile->getGroundBrush()->visibleInPalette()) {
					Append(MAP_POPUP_MENU_SELECT_GROUND_BRUSH, "Select Groundbrush", "Uses the current item as a groundbrush");
				}

				if (hasCollection || (topSelectedItem && topSelectedItem->hasCollectionBrush()) || (tile->getGroundBrush() && tile->getGroundBrush()->hasCollection())) {
					Append(MAP_POPUP_MENU_SELECT_COLLECTION_BRUSH, "Select Collection", "Use this collection");
				}

				if (tile->hasGround() || topSelectedItem || topItem || topCreature || topSpawn) {
					AppendSeparator();
					Append(MAP_POPUP_MENU_ADD_FAVORITE, "⭐ Add to Favorites", "Add this brush/item directly to your Favorites");
					Append(MAP_POPUP_MENU_PROPERTIES, "&Attributes\tAlt+Enter", "Edit attributes and properties for the current object");
				}
			} else {
				bool has_items_added = false;
				if (topCreature) {
					Append(MAP_POPUP_MENU_SELECT_CREATURE_BRUSH, "Select Creature", "Uses the current creature as a creature brush");
					has_items_added = true;
				}

				if (topSpawn) {
					if (has_items_added) AppendSeparator();
					Append(MAP_POPUP_MENU_SELECT_SPAWN_BRUSH, "Select Spawn", "Select the spawn brush");
					has_items_added = true;
				}

				if (tile->hasGround() || !tile->empty()) {
					Append(MAP_POPUP_MENU_CHANGE, "&Change...\tAlt+C", "Change connected elements to a different brush or type via Palette");
				}
				if (hasWall) {
					Append(MAP_POPUP_MENU_SELECT_WALL_BRUSH, "Select Wallbrush", "Uses the current item as a wallbrush");
				}
				if (tile->hasGround() && tile->getGroundBrush() && tile->getGroundBrush()->visibleInPalette()) {
					Append(MAP_POPUP_MENU_SELECT_GROUND_BRUSH, "Select Groundbrush", "Uses the current tile as a groundbrush");
				}

				if (hasCollection || (tile->getGroundBrush() && tile->getGroundBrush()->hasCollection())) {
					Append(MAP_POPUP_MENU_SELECT_COLLECTION_BRUSH, "Select Collection", "Use this collection");
				}

				Town* clicked_town = nullptr;
				Position click_pos = tile->getPosition();
				for (const auto& pair : editor.map.towns) {
					if (pair.second->getTemplePosition() == click_pos) {
						clicked_town = pair.second;
						break;
					}
				}

				AppendSeparator();
				if (clicked_town) {
					Append(MAP_POPUP_MENU_EDIT_TOWN, "Edit Town", "Edit this town");
				} else {
					Append(MAP_POPUP_MENU_CREATE_TOWN, "Create Town", "Create a town here");
				}

				if (tile->hasGround() || topSelectedItem || topItem || topCreature || topSpawn) {
					AppendSeparator();
					Append(MAP_POPUP_MENU_ADD_FAVORITE, "⭐ Add to Favorites", "Add this brush/item directly to your Favorites");
					Append(MAP_POPUP_MENU_PROPERTIES, "&Attributes\tAlt+Enter", "Edit attributes and properties for the current object");
				}
			}

			bool rotatable_items_selected = false;
			if (editor.selection.size() > 1) {
				for (Tile* tile : editor.selection.getTiles()) {
					ItemVector selected_items = tile->getSelectedItems();
					for (Item* item : selected_items) {
						if (item->isRoteable()) {
							rotatable_items_selected = true;
							break;
						}
					}
					if (rotatable_items_selected) break;
				}
			}
			if (rotatable_items_selected) {
				Append(MAP_POPUP_MENU_ROTATE, "&Rotate items", "Rotate the selected items");
			}

			// Add Lua Context Menu Items
			const auto& menuItems = g_luaScripts.getContextMenuItems();
			if (!menuItems.empty()) {
				AppendSeparator();

				for (size_t i = 0; i < menuItems.size(); ++i) {
					int id = MAP_POPUP_MENU_SCRIPT_FIRST + i;
					if (id > MAP_POPUP_MENU_SCRIPT_LAST) {
						break;
					}

					Append(id, wxString::FromUTF8(menuItems[i].label));
				}
			}
		}
	}
}

void MapCanvas::OnScriptMenu(wxCommandEvent& event) {
	int index = event.GetId() - MAP_POPUP_MENU_SCRIPT_FIRST;
	const auto& menuItems = g_luaScripts.getContextMenuItems();

	if (index >= 0 && index < static_cast<int>(menuItems.size())) {
		try {
			// Call the callback
			const auto& item = menuItems[index];
			if (item.callback.valid()) {
				// Pass the clicked tile/position to the callback
				Tile* tile = editor.map.getTile(last_click_map_x, last_click_map_y, floor);
				if (tile) {
					item.callback(tile);
				} else {
					item.callback(Position(last_click_map_x, last_click_map_y, floor));
				}
			}
		} catch (const sol::error& e) {
			wxMessageBox(wxString("Script execution error: ") + e.what(), "Lua Error", wxOK | wxICON_ERROR);
		}
	}
}

void MapCanvas::OnCut(wxCommandEvent& WXUNUSED(event)) { g_gui.DoCut(); }
void MapCanvas::OnCopy(wxCommandEvent& WXUNUSED(event)) { g_gui.DoCopy(); }
void MapCanvas::OnPaste(wxCommandEvent& WXUNUSED(event)) { g_gui.PreparePaste(); }
void MapCanvas::OnDelete(wxCommandEvent& WXUNUSED(event)) {
	if (editor.selection.size() == 0) {
		Tile* tile = editor.map.getTile(last_click_map_x, last_click_map_y, floor);
		if (tile && (!tile->empty() || tile->ground)) {
			Item* top = tile->getTopItem();
			if (top) {
				BatchAction* batch = editor.actionQueue->createBatch(ACTION_DELETE_TILES);
				Action* action = editor.actionQueue->createAction(batch);
				Tile* new_tile = tile->deepCopy(editor.map);
				Item* new_top = nullptr;
				if (top == tile->ground) new_top = new_tile->ground;
				else {
					for (size_t i = 0; i < tile->items.size() && i < new_tile->items.size(); ++i) {
						if (tile->items[i] == top) { new_top = new_tile->items[i]; break; }
					}
				}
				if (new_top) {
					if (new_top == new_tile->ground) {
						delete new_tile->ground;
						new_tile->ground = nullptr;
					} else {
						for (auto it = new_tile->items.begin(); it != new_tile->items.end(); ++it) {
							if (*it == new_top) {
								delete *it;
								new_tile->items.erase(it);
								break;
							}
						}
					}
				}
				action->addChange(newd Change(new_tile));
				batch->addAndCommitAction(action);
				editor.addBatch(batch, 2);
				Refresh();
			}
		}
	} else {
		editor.destroySelection();
	}
}
void MapCanvas::OnCopyPosition(wxCommandEvent& WXUNUSED(event)) {}
void MapCanvas::OnCopyServerId(wxCommandEvent& WXUNUSED(event)) {}
void MapCanvas::OnCopyClientId(wxCommandEvent& WXUNUSED(event)) {}
void MapCanvas::OnCopyName(wxCommandEvent& WXUNUSED(event)) {}
void MapCanvas::OnBrowseTile(wxCommandEvent& WXUNUSED(event)) {}
void MapCanvas::OnGotoDestination(wxCommandEvent& WXUNUSED(event)) {}
void MapCanvas::OnChangeConnected(wxCommandEvent& WXUNUSED(event)) {
	int click_x = last_click_map_x;
	int click_y = last_click_map_y;
	int click_z = floor;

	if (editor.selection.size() == 1) {
		Tile* sel_tile = editor.selection.getSelectedTile();
		if (sel_tile) {
			click_x = sel_tile->getX();
			click_y = sel_tile->getY();
			click_z = sel_tile->getZ();
		}
	}

	Tile* start_tile = editor.map.getTile(click_x, click_y, click_z);
	if (!start_tile) {
		return;
	}

	// Determine what is clicked: wall, carpet, table, doodad, raw item, or ground
	Item* target_item = nullptr;
	if (editor.selection.size() == 1) {
		ItemVector selected_items = start_tile->getSelectedItems();
		for (Item* it : selected_items) {
			if (it->isSelected()) {
				target_item = it;
				break;
			}
		}
	}
	if (!target_item) {
		target_item = start_tile->getTopItem();
		if (!target_item) {
			target_item = start_tile->ground;
		}
	}

	const int map_width = editor.map.getWidth();
	const int map_height = editor.map.getHeight();

	auto get_ground_id = [](Tile* t) -> uint32_t {
		if (!t) return 0;
		if (t->getGroundBrush()) return t->getGroundBrush()->getID();
		if (t->ground) return t->ground->getID();
		return 0;
	};

	bool match_ground = (target_item == nullptr || target_item == start_tile->ground || start_tile->empty());
	uint32_t target_ground_id = get_ground_id(start_tile);
	uint16_t target_item_id = target_item ? target_item->getID() : 0;
	WallBrush* target_wall_brush = target_item ? target_item->getWallBrush() : nullptr;
	CarpetBrush* target_carpet_brush = target_item ? target_item->getCarpetBrush() : nullptr;
	TableBrush* target_table_brush = target_item ? target_item->getTableBrush() : nullptr;
	Brush* target_doodad_brush = target_item ? target_item->getDoodadBrush() : nullptr;

	int min_x = 1, min_y = 1, max_x = map_width - 1, max_y = map_height - 1;
	int screen_w = 0, screen_h = 0;
	GetClientSize(&screen_w, &screen_h);
	if (screen_w > 0 && screen_h > 0) {
		int vis_min_x = 0, vis_min_y = 0, vis_max_x = 0, vis_max_y = 0;
		ScreenToMap(0, 0, &vis_min_x, &vis_min_y);
		ScreenToMap(screen_w, screen_h, &vis_max_x, &vis_max_y);

		int v_left = std::min(vis_min_x, vis_max_x);
		int v_right = std::max(vis_min_x, vis_max_x);
		int v_top = std::min(vis_min_y, vis_max_y);
		int v_bottom = std::max(vis_min_y, vis_max_y);

		min_x = std::max(1, v_left - 10);
		min_y = std::max(1, v_top - 10);
		max_x = std::min(map_width - 1, v_right + 10);
		max_y = std::min(map_height - 1, v_bottom + 10);
	}

	min_x = std::min(min_x, click_x);
	max_x = std::max(max_x, click_x);
	min_y = std::min(min_y, click_y);
	max_y = std::max(max_y, click_y);

	const size_t range_w = static_cast<size_t>(max_x - min_x + 1);
	const size_t range_h = static_cast<size_t>(max_y - min_y + 1);
	const size_t max_fill_tiles = range_w * range_h;

	std::queue<Position> queue;
	std::vector<uint8_t> visited(range_w * range_h, 0);
	std::vector<std::pair<Tile*, Item*>> flooded_elements;

	auto is_visited = [&](int x, int y) -> bool {
		return visited[static_cast<size_t>(y - min_y) * range_w + static_cast<size_t>(x - min_x)] != 0;
	};
	auto set_visited = [&](int x, int y) {
		visited[static_cast<size_t>(y - min_y) * range_w + static_cast<size_t>(x - min_x)] = 1;
	};

	Position start_pos(click_x, click_y, click_z);
	queue.push(start_pos);
	set_visited(click_x, click_y);

	while (!queue.empty() && flooded_elements.size() < max_fill_tiles) {
		Position current = queue.front();
		queue.pop();

		Tile* tile = editor.map.getTile(current);
		if (!tile) continue;

		Item* matching_item = nullptr;
		if (match_ground) {
			if (tile->ground && get_ground_id(tile) == target_ground_id) {
				matching_item = tile->ground;
			}
		} else if (target_wall_brush) {
			for (Item* it : tile->items) {
				if (it && (it->getWallBrush() == target_wall_brush || target_wall_brush->hasWall(it))) {
					matching_item = it;
					break;
				}
			}
		} else if (target_carpet_brush) {
			for (Item* it : tile->items) {
				if (it && it->getCarpetBrush() == target_carpet_brush) {
					matching_item = it;
					break;
				}
			}
		} else if (target_table_brush) {
			for (Item* it : tile->items) {
				if (it && it->getTableBrush() == target_table_brush) {
					matching_item = it;
					break;
				}
			}
		} else if (target_doodad_brush) {
			for (Item* it : tile->items) {
				if (it && it->getDoodadBrush() == target_doodad_brush) {
					matching_item = it;
					break;
				}
			}
		} else if (target_item_id > 0) {
			for (Item* it : tile->items) {
				if (it && it->getID() == target_item_id) {
					matching_item = it;
					break;
				}
			}
		}

		if (!matching_item) {
			continue;
		}

		flooded_elements.push_back({tile, matching_item});

		static const int dx[4] = {-1, 1, 0, 0};
		static const int dy[4] = {0, 0, -1, 1};
		for (int i = 0; i < 4; ++i) {
			int nx = current.x + dx[i];
			int ny = current.y + dy[i];
			if (nx >= min_x && nx <= max_x && ny >= min_y && ny <= max_y) {
				if (!is_visited(nx, ny)) {
					set_visited(nx, ny);
					queue.push(Position(nx, ny, current.z));
				}
			}
		}
	}

	if (flooded_elements.empty()) {
		return;
	}

	editor.selection.clear();
	editor.selection.start(Selection::INTERNAL);
	for (const auto& elem : flooded_elements) {
		Tile* t = elem.first;
		Item* it = elem.second;
		if (t && it) {
			editor.selection.add(t, it);
		}
	}
	editor.selection.finish(Selection::INTERNAL);

	g_gui.SetPendingChangeMode(true);
	g_gui.SetStatusText("Change Mode: Select replacement brush from Palette to replace connected elements (or press Esc/click canvas to cancel)");
	markDirty();
	Refresh();
}

void MapCanvas::OnRotateItem(wxCommandEvent& WXUNUSED(event)) {
	if (editor.selection.empty()) {
		Brush* brush = g_gui.GetCurrentBrush();
		if (brush) {
			if (brush->isRaw()) {
				RAWBrush* raw_brush = static_cast<RAWBrush*>(brush);
				const ItemType& itemtype = g_items[raw_brush->getItemID()];
				if (itemtype.rotateTo != 0) {
					const ItemType& rotated_type = g_items[itemtype.rotateTo];
					if (rotated_type.raw_brush) {
						g_gui.SelectBrush(rotated_type.raw_brush);
					} else {
						raw_brush->setItemID(itemtype.rotateTo);
					}
					Refresh();
					return;
				}
			} else if (brush->isDoodad()) {
				DoodadBrush* doodad = static_cast<DoodadBrush*>(brush);
				if (doodad->getMaxVariation() > 1) {
					int current_var = g_gui.GetBrushVariation();
					int next_var = (current_var + 1) % doodad->getMaxVariation();
					g_gui.SetBrushVariation(next_var);
					Refresh();
					return;
				}
			}
		}
	}

	BatchAction* batch = editor.actionQueue->createBatch(ACTION_DRAW);
	Action* action = editor.actionQueue->createAction(batch);
	bool rotated_any = false;

	if (editor.selection.size() > 0) {
		for (Tile* tile : editor.selection.getTiles()) {
			ItemVector selected_items = tile->getSelectedItems();
			if (selected_items.empty()) {
				if (tile->ground && tile->ground->isRoteable()) {
					selected_items.push_back(tile->ground);
				} else {
					Item* top = tile->getTopItem();
					if (top && top->isRoteable()) {
						selected_items.push_back(top);
					}
				}
			}

			Tile* new_tile = nullptr;
			for (Item* item : selected_items) {
				if (item->isRoteable()) {
					ItemType& it = g_items[item->getID()];
					if (it.rotateTo != 0) {
						if (!new_tile) {
							new_tile = tile->deepCopy(editor.map);
						}
						Item* new_item = nullptr;
						if (item == tile->ground) {
							new_item = new_tile->ground;
						} else {
							for (size_t i = 0; i < tile->items.size() && i < new_tile->items.size(); ++i) {
								if (tile->items[i] == item) {
									new_item = new_tile->items[i];
									break;
								}
							}
						}
						if (new_item) {
							new_item->setID(it.rotateTo);
							rotated_any = true;
						}
					}
				}
			}
			if (new_tile) {
				action->addChange(newd Change(new_tile));
			}
		}
	} else {
		int target_x = last_click_map_x;
		int target_y = last_click_map_y;
		int target_z = last_click_map_z != -1 ? last_click_map_z : floor;

		int mouse_map_x, mouse_map_y;
		ScreenToMap(cursor_x, cursor_y, &mouse_map_x, &mouse_map_y);
		Tile* hover_tile = editor.map.getTile(mouse_map_x, mouse_map_y, floor);
		if (hover_tile && (!hover_tile->empty() || hover_tile->ground)) {
			target_x = mouse_map_x;
			target_y = mouse_map_y;
			target_z = floor;
		}

		Tile* tile = editor.map.getTile(target_x, target_y, target_z);
		if (tile && (!tile->empty() || tile->ground)) {
			Item* target = nullptr;
			Item* top = tile->getTopItem();
			if (top && top->isRoteable()) {
				target = top;
			} else if (tile->ground && tile->ground->isRoteable()) {
				target = tile->ground;
			} else {
				for (Item* item : tile->items) {
					if (item->isRoteable()) {
						target = item;
						break;
					}
				}
			}

			if (target && target->isRoteable()) {
				ItemType& it = g_items[target->getID()];
				if (it.rotateTo != 0) {
					Tile* new_tile = tile->deepCopy(editor.map);
					Item* new_target = nullptr;
					if (target == tile->ground) {
						new_target = new_tile->ground;
					} else {
						for (size_t i = 0; i < tile->items.size() && i < new_tile->items.size(); ++i) {
							if (tile->items[i] == target) {
								new_target = new_tile->items[i];
								break;
							}
						}
					}
					if (new_target) {
						new_target->setID(it.rotateTo);
						rotated_any = true;
					}
					action->addChange(newd Change(new_tile));
				}
			}
		}
	}

	if (rotated_any) {
		batch->addAndCommitAction(action);
		editor.addBatch(batch, 2);
		Refresh();
	} else {
		delete action;
		delete batch;
	}
}
void MapCanvas::OnSwitchDoor(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor.map.getTile(last_click_map_x, last_click_map_y, floor);
	if (!tile) return;

	Item* target_item = nullptr;
	bool is_container_only = false;
	for (Item* item : tile->items) {
		if (item->isContainer()) {
			target_item = item;
			is_container_only = true;
			break;
		}
	}
	if (!target_item) {
		for (Item* item : tile->items) {
			if (getItemUseSwitchID(item) != 0) {
				target_item = item;
				is_container_only = false;
				break;
			}
		}
	}
	if (!target_item) {
		for (Item* item : tile->items) {
			if (item->isBrushDoor()) {
				target_item = item;
				is_container_only = false;
				break;
			}
		}
	}
	if (!target_item && tile->ground) {
		if (tile->ground->isContainer()) {
			target_item = tile->ground;
			is_container_only = true;
		} else if (getItemUseSwitchID(tile->ground) != 0 || tile->ground->isBrushDoor()) {
			target_item = tile->ground;
			is_container_only = false;
		}
	}

	if (is_container_only) {
		// Just open properties window
		wxCommandEvent empty_event;
		OnProperties(empty_event);
		return;
	}

	if (target_item) {
		Tile* new_tile = tile->deepCopy(editor.map);
		Item* new_item = nullptr;
		if (target_item == tile->ground) {
			new_item = new_tile->ground;
		} else {
			for (size_t i = 0; i < tile->items.size(); ++i) {
				if (tile->items[i] == target_item) {
					new_item = new_tile->items[i];
					break;
				}
			}
		}

		if (new_item) {
			uint16_t container_switch_id = getItemUseSwitchID(new_item);
			if (container_switch_id != 0) {
				new_item->setID(container_switch_id);
			} else if (new_item->isBrushDoor()) {
				DoorBrush::switchDoor(new_item);
			}

			new_tile->deselect();

			Action* action = editor.actionQueue->createAction(ACTION_CHANGE_PROPERTIES);
			action->addChange(newd Change(new_tile));
			editor.addAction(action);
			
			editor.selection.start(Selection::INTERNAL);
			editor.selection.clear();
			editor.selection.finish(Selection::INTERNAL);
			
			Refresh();
		} else {
			delete new_tile;
		}
	}
}
void MapCanvas::OnSelectRAWBrush(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) tile = editor.map.getTile(last_click_map_x, last_click_map_y, floor);
	if (!tile) return;
	Item* top = tile->getTopItem();
	if (!top) top = tile->ground;
	if (top && top->getRAWBrush()) {
		g_gui.SelectBrush(top->getRAWBrush(), TILESET_RAW);
	}
}
void MapCanvas::OnSelectGroundBrush(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) tile = editor.map.getTile(last_click_map_x, last_click_map_y, floor);
	if (!tile || !tile->ground) return;
	GroundBrush* gb = tile->getGroundBrush();
	if (!gb) {
		ItemType& it = g_items[tile->ground->getID()];
		if (it.brush && it.brush->isGround()) {
			gb = it.brush->asGround();
		}
	}
	if (gb) {
		g_gui.SelectBrush(gb, TILESET_TERRAIN);
	} else if (tile->ground->getRAWBrush()) {
		g_gui.SelectBrush(tile->ground->getRAWBrush(), TILESET_RAW);
	}
}
void MapCanvas::OnSelectDoodadBrush(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) tile = editor.map.getTile(last_click_map_x, last_click_map_y, floor);
	if (!tile) return;
	Item* found = nullptr;
	for (auto* item : tile->items) {
		if (item->isSelected() && item->getDoodadBrush()) {
			found = item;
			break;
		}
	}
	if (!found) {
		for (auto it = tile->items.rbegin(); it != tile->items.rend(); ++it) {
			if ((*it)->getDoodadBrush()) {
				found = *it;
				break;
			}
		}
	}
	if (found && found->getDoodadBrush()) {
		g_gui.SelectBrush(found->getDoodadBrush(), TILESET_DOODAD);
	}
}
void MapCanvas::OnSelectDoorBrush(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) tile = editor.map.getTile(last_click_map_x, last_click_map_y, floor);
	if (!tile) return;
	Item* found = nullptr;
	for (auto* item : tile->items) {
		if (item->isSelected() && item->isBrushDoor() && item->getDoorBrush()) {
			found = item;
			break;
		}
	}
	if (!found) {
		for (auto it = tile->items.rbegin(); it != tile->items.rend(); ++it) {
			if ((*it)->isBrushDoor() && (*it)->getDoorBrush()) {
				found = *it;
				break;
			}
		}
	}
	if (found && found->getDoorBrush()) {
		g_gui.SelectBrush(found->getDoorBrush(), TILESET_TERRAIN);
	}
}
void MapCanvas::OnSelectWallBrush(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) tile = editor.map.getTile(last_click_map_x, last_click_map_y, floor);
	if (!tile) return;
	for (auto* item : tile->items) {
		if (item->isWall() && item->getWallBrush()) {
			g_gui.SelectBrush(item->getWallBrush(), TILESET_TERRAIN);
			break;
		}
	}
}
void MapCanvas::OnSelectCarpetBrush(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) tile = editor.map.getTile(last_click_map_x, last_click_map_y, floor);
	if (!tile) return;
	for (auto* item : tile->items) {
		if (item->isCarpet() && item->getCarpetBrush()) {
			g_gui.SelectBrush(item->getCarpetBrush(), TILESET_TERRAIN);
			break;
		}
	}
}
void MapCanvas::OnSelectTableBrush(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) tile = editor.map.getTile(last_click_map_x, last_click_map_y, floor);
	if (!tile) return;
	for (auto* item : tile->items) {
		if (item->isTable() && item->getTableBrush()) {
			g_gui.SelectBrush(item->getTableBrush(), TILESET_TERRAIN);
			break;
		}
	}
}
void MapCanvas::OnSelectHouseBrush(wxCommandEvent& WXUNUSED(event)) {}
void MapCanvas::OnSelectCollectionBrush(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) tile = editor.map.getTile(last_click_map_x, last_click_map_y, floor);
	if (!tile) return;
	Item* top = tile->getTopItem();
	if (!top) top = tile->ground;
	if (top && g_items[top->getID()].collection_brush) {
		g_gui.SelectBrush(g_items[top->getID()].collection_brush, TILESET_COLLECTION);
	}
}

static Brush* FindBestItemBrush(Item* item) {
	if (!item) return nullptr;
	if (item->getDoodadBrush()) return item->getDoodadBrush();
	if (item->getWallBrush()) return item->getWallBrush();
	if (item->getCarpetBrush()) return item->getCarpetBrush();
	if (item->getTableBrush()) return item->getTableBrush();
	if (item->getDoorBrush()) return item->getDoorBrush();
	if (item->getGroundBrush()) return item->getGroundBrush();
	if (item->getBrush() && !item->getBrush()->isRaw()) return item->getBrush();

	return item->getRAWBrush();
}

void MapCanvas::OnAddFavorite(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) tile = editor.map.getTile(last_click_map_x, last_click_map_y, floor);
	if (!tile) return;

	Brush* target_brush = nullptr;

	// 1. Check selected items first
	for (auto* item : tile->items) {
		if (item && item->isSelected()) {
			target_brush = FindBestItemBrush(item);
			if (target_brush && !target_brush->isRaw()) break;
		}
	}

	// 2. If nothing selected or only raw found, inspect items top to bottom
	if (!target_brush || target_brush->isRaw()) {
		for (auto it = tile->items.rbegin(); it != tile->items.rend(); ++it) {
			if (!*it) continue;
			Brush* b = FindBestItemBrush(*it);
			if (b) {
				target_brush = b;
				if (!b->isRaw()) break;
			}
		}
	}

	// 3. Check creature / spawn
	if (!target_brush || target_brush->isRaw()) {
		if (tile->creature && tile->creature->getBrush()) {
			target_brush = tile->creature->getBrush();
		} else if (tile->spawn) {
			target_brush = g_gui.spawn_brush;
		}
	}

	// 4. Check ground
	if (!target_brush || target_brush->isRaw()) {
		if (tile->ground) {
			if (tile->getGroundBrush()) {
				target_brush = tile->getGroundBrush();
			} else if (tile->ground->getBrush() && !tile->ground->getBrush()->isRaw()) {
				target_brush = tile->ground->getBrush();
			} else if (tile->ground->getRAWBrush()) {
				target_brush = tile->ground->getRAWBrush();
			}
		}
	}

	if (target_brush) {
		g_materials.addFavoriteBrush(target_brush);
		g_gui.RefreshFavoritesBox();
		g_gui.SetStatusText(wxString::Format("Added '%s' to Favorites!", target_brush->getName()));
	}
}

void MapCanvas::OnCreateTown(wxCommandEvent& WXUNUSED(event)) {
	Position click_pos(last_click_map_x, last_click_map_y, floor);

	if (editor.IsLiveClient()) {
		if (editor.GetLiveClient()) {
			editor.GetLiveClient()->sendApprovalRequest(APPROVAL_TOWN, click_pos, 0, "New Town");
		}
		g_gui.SetStatusText("Town creation request sent to Host for approval...");
		return;
	}

	uint32_t max_id = 0;
	for (const auto& pair : editor.map.towns) {
		if (pair.second->getID() > max_id) {
			max_id = pair.second->getID();
		}
	}
	Town* new_town = newd Town(max_id + 1);
	new_town->setName("Unnamed Town");
	new_town->setTemplePosition(click_pos);
	editor.map.towns.addTown(new_town);
	Tile* tile = editor.map.getOrCreateTile(click_pos);
	if (tile) {
		tile->getLocation()->increaseTownCount();
	}
	editor.map.doChange();

	LiveServer* server = editor.GetLiveServer();
	if (server) {
		NetworkMessage townListMsg;
		townListMsg.write<uint8_t>(PACKET_TOWN_LIST);
		townListMsg.write<uint32_t>((uint32_t)editor.map.towns.count());
		for (const auto& pair : editor.map.towns) {
			Town* town = pair.second;
			if (town) {
				townListMsg.write<uint32_t>(town->getID());
				townListMsg.write<std::string>(town->getName());
				townListMsg.write<Position>(town->getTemplePosition());
			}
		}
		for (auto& clientEntry : server->getClients()) {
			if (clientEntry.second) clientEntry.second->send(townListMsg);
		}
	}
	g_gui.RefreshMinimapPanel();

	uint32_t town_id = new_town->getID();
	wxWindow* parent_win = static_cast<wxWindow*>(GetParent());
	CallAfter([this, parent_win, town_id]() {
		wxDialog* town_dialog = newd EditTownsDialog(parent_win, editor, town_id);
		town_dialog->ShowModal();
		town_dialog->Destroy();
		Refresh();
	});
}

void MapCanvas::OnEditTown(wxCommandEvent& WXUNUSED(event)) {
	Position click_pos(last_click_map_x, last_click_map_y, floor);
	Town* clicked_town = nullptr;
	for (const auto& pair : editor.map.towns) {
		if (pair.second->getTemplePosition() == click_pos) {
			clicked_town = pair.second;
			break;
		}
	}
	if (clicked_town) {
		clicked_town->setTemplePosition(click_pos);
		uint32_t town_id = clicked_town->getID();
		wxWindow* parent_win = static_cast<wxWindow*>(GetParent());
		CallAfter([this, parent_win, town_id]() {
			wxDialog* town_dialog = newd EditTownsDialog(parent_win, editor, town_id);
			town_dialog->ShowModal();
			town_dialog->Destroy();
			Refresh();
		});
	}
}