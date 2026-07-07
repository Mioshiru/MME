#include "main.h"
#include "map_display.h"
#include "gui.h"
#include "editor.h"
#include "settings.h"
#include "tileset_window.h"
#include "old_properties_window.h"
#include "properties_window.h"
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

void MapCanvas::OnSelectCreatureBrush(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor.selection.getSelectedTile();
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

	if (item) {
		w = newd TilesetWindow(g_gui.root, &editor.map, new_tile, item);
	} else {
		return;
	}

	int ret = w->ShowModal(); // unique_ptr verwaltet dies jetzt
	if (ret != 0) {
		Action* action = editor.actionQueue->createAction(ACTION_CHANGE_PROPERTIES);
		action->addChange(newd Change(new_tile));
		editor.addAction(action);

		g_gui.RebuildPalettes();
	} else {
		// Cancel!
		delete new_tile;
	}
	w->Destroy();
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

	Tile* new_tile = tile->deepCopy(editor.map);

	wxDialog* w = nullptr; 

	if (new_tile->spawn && g_settings.getInteger(Config::SHOW_SPAWNS)) {
		w = newd OldPropertiesWindow(g_gui.root, &editor.map, new_tile, new_tile->spawn);
	} else if (new_tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)) {
		w = newd OldPropertiesWindow(g_gui.root, &editor.map, new_tile, new_tile->creature);
	} else if (item) {
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
			w = new PropertiesWindow(g_gui.root, &editor.map, new_tile, new_item); 
		}
	}

	if (!w) {
		delete new_tile;
		return;
	}

	int ret = w->ShowModal();
	if (ret != 0) {
		Action* action = editor.actionQueue->createAction(ACTION_CHANGE_PROPERTIES);
		action->addChange(newd Change(new_tile));
		editor.addAction(action);
	} else {
		// Cancel!
		delete new_tile;
	}
	w->Destroy();
}

MapPopupMenu::MapPopupMenu(MapEditor& map_editor_ref) :
	wxMenu("Map Context"), editor(map_editor_ref) {
}

MapPopupMenu::~MapPopupMenu() {
	////
}

void MapPopupMenu::Update() {
	SetTitle("Map Context");

	// Clear the menu of all items
	while (GetMenuItemCount() != 0) {
		wxMenuItem* m_item = FindItemByPosition(0);
		Delete(m_item);
	}

	bool anything_selected = editor.selection.size() != 0;

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

				if (topSelectedItem && topSelectedItem->isRoteable()) {
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

				if (hasTable) {
					Append(MAP_POPUP_MENU_SELECT_TABLE_BRUSH, "Select Tablebrush", "Uses the current item as a tablebrush");
				}

				if (topSelectedItem && topSelectedItem->getDoodadBrush() && topSelectedItem->getDoodadBrush()->visibleInPalette()) {
					Append(MAP_POPUP_MENU_SELECT_DOODAD_BRUSH, "Select Doodadbrush", "Use this doodad brush");
				}

				if (topSelectedItem && topSelectedItem->isBrushDoor() && topSelectedItem->getDoorBrush()) {
					Append(MAP_POPUP_MENU_SELECT_DOOR_BRUSH, "Select Doorbrush", "Use this door brush");
				}

				if (tile->hasGround() && tile->getGroundBrush() && tile->getGroundBrush()->visibleInPalette()) {
					Append(MAP_POPUP_MENU_SELECT_GROUND_BRUSH, "Select Groundbrush", "Uses the current item as a groundbrush");
				}

				if (hasCollection || (topSelectedItem && topSelectedItem->hasCollectionBrush()) || (tile->getGroundBrush() && tile->getGroundBrush()->hasCollection())) {
					Append(MAP_POPUP_MENU_SELECT_COLLECTION_BRUSH, "Select Collection", "Use this collection");
				}

				AppendSeparator();
				Append(MAP_POPUP_MENU_PROPERTIES, "&Properties", "Properties for the current object");
			} else {
				bool has_items_added = false;
				if (topCreature) {
					Append(MAP_POPUP_MENU_SELECT_CREATURE_BRUSH, "Select Creature", "Uses the current creature as a creature brush");
					has_items_added = true;
				}

				if (topSpawn) {
					Append(MAP_POPUP_MENU_SELECT_SPAWN_BRUSH, "Select Spawn", "Select the spawn brush");
					has_items_added = true;
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

				if (tile->hasGround() || topCreature || topSpawn) {
					AppendSeparator();
					Append(MAP_POPUP_MENU_PROPERTIES, "&Properties", "Properties for the current object");
				}
			}

			AppendSeparator();

			wxMenuItem* browseTile = Append(MAP_POPUP_MENU_BROWSE_TILE, "Browse Field", "Navigate from tile items");
			browseTile->Enable(anything_selected);

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
void MapCanvas::OnRotateItem(wxCommandEvent& WXUNUSED(event)) {
	if (editor.selection.size() == 0) {
		Tile* tile = editor.map.getTile(last_click_map_x, last_click_map_y, floor);
		if (tile && (!tile->empty() || tile->ground)) {
			Item* top = tile->getTopItem();
			if (top && top->isRoteable()) {
				ItemType& it = g_items[top->getID()];
				if (it.rotateTo != 0) {
					BatchAction* batch = editor.actionQueue->createBatch(ACTION_DRAW);
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
						new_top->setID(it.rotateTo);
					}
					action->addChange(newd Change(new_tile));
					batch->addAndCommitAction(action);
					editor.addBatch(batch, 2);
					Refresh();
				}
			}
		}
	}
}
void MapCanvas::OnSwitchDoor(wxCommandEvent& WXUNUSED(event)) {}
void MapCanvas::OnSelectRAWBrush(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) return;
	Item* top = tile->getTopItem();
	if (!top) top = tile->ground;
	if (top && top->getRAWBrush()) {
		g_gui.SelectBrush(top->getRAWBrush(), TILESET_RAW);
	}
}
void MapCanvas::OnSelectGroundBrush(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor.selection.getSelectedTile();
	if (!tile || !tile->ground) return;
	if (tile->getGroundBrush()) {
		g_gui.SelectBrush(tile->getGroundBrush(), TILESET_TERRAIN);
	}
}
void MapCanvas::OnSelectDoodadBrush(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) return;
	Item* top = tile->getTopItem();
	if (top && top->getDoodadBrush()) {
		g_gui.SelectBrush(top->getDoodadBrush(), TILESET_DOODAD);
	}
}
void MapCanvas::OnSelectDoorBrush(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) return;
	Item* top = tile->getTopItem();
	if (top && top->getDoorBrush()) {
		g_gui.SelectBrush(top->getDoorBrush(), TILESET_TERRAIN);
	}
}
void MapCanvas::OnSelectWallBrush(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor.selection.getSelectedTile();
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
	if (!tile) return;
	Item* top = tile->getTopItem();
	if (!top) top = tile->ground;
	if (top && g_items[top->getID()].collection_brush) {
		g_gui.SelectBrush(g_items[top->getID()].collection_brush, TILESET_COLLECTION);
	}
}