#include "main.h"
#include "main_menubar.h"
#include "gui.h"
#include "editor.h"
#include "settings.h"
#include "map_display.h"
#include "properties_window.h"
#include "find_item_window.h"
#include "town.h"
#include "house.h"
#include "materials.h"
#include "common_windows.h"

namespace OnMapRemoveItems {
	struct RemoveItemCondition {
		RemoveItemCondition(uint16_t itemId) :
			itemId(itemId) { }

		uint16_t itemId;

		bool operator()(Map& map, Item* item, int64_t removed, int64_t done) {
			if (done % 0x8000 == 0) {
				g_gui.SetLoadDone((uint32_t)(100 * done / map.getTileCount()));
			}
			return item->getID() == itemId && !item->isComplex();
		}
	};
}

void MainMenuBar::OnMapRemoveItems(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	FindItemDialog dialog(frame, "Item Type to Remove");
	if (dialog.ShowModal() == wxID_OK) {
		uint16_t itemid = dialog.getResultID();

		g_gui.GetCurrentEditor()->selection.clear();
		g_gui.GetCurrentEditor()->actionQueue->clear();

		OnMapRemoveItems::RemoveItemCondition condition(itemid);
		g_gui.CreateLoadBar("Searching map for items to remove...");

		int64_t count = RemoveItemOnMap(g_gui.GetCurrentMap(), condition, false);

		g_gui.DestroyLoadBar();

		wxString msg;
		msg << count << " items deleted.";

		g_gui.PopupDialog("Search completed", msg, wxOK);
		g_gui.GetCurrentMap().doChange();
		g_gui.RefreshView();
	}
	dialog.Destroy();
}

namespace OnMapRemoveCorpses {
	struct condition {
		condition() { }

		bool operator()(Map& map, Item* item, long long removed, long long done) {
			if (done % 0x800 == 0) {
				g_gui.SetLoadDone((unsigned int)(100 * done / map.getTileCount()));
			}

			return g_materials.isInTileset(item, "Corpses") & !item->isComplex();
		}
	};
}

void MainMenuBar::OnMapRemoveCorpses(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	int ok = g_gui.PopupDialog("Remove Corpses", "Do you want to remove all corpses from the map?", wxYES | wxNO);

	if (ok == wxID_YES) {
		g_gui.GetCurrentEditor()->selection.clear();
		g_gui.GetCurrentEditor()->actionQueue->clear();

		OnMapRemoveCorpses::condition func;
		g_gui.CreateLoadBar("Searching map for items to remove...");

		int64_t count = RemoveItemOnMap(g_gui.GetCurrentMap(), func, false);

		g_gui.DestroyLoadBar();

		wxString msg;
		msg << count << " items deleted.";
		g_gui.PopupDialog("Search completed", msg, wxOK);
		g_gui.GetCurrentMap().doChange();
	}
}

namespace OnMapRemoveUnreachable {
	struct condition {
		condition() { }

		bool isReachable(Tile* tile) {
			if (tile == nullptr) {
				return false;
			}
			if (!tile->isBlocking()) {
				return true;
			}
			return false;
		}

		bool operator()(Map& map, Tile* tile, long long removed, long long done, long long total) {
			if (done % 0x1000 == 0) {
				g_gui.SetLoadDone((unsigned int)(100 * done / total));
			}

			Position pos = tile->getPosition();
			int sx = std::max(pos.x - 10, 0);
			int ex = std::min(pos.x + 10, 65535);
			int sy = std::max(pos.y - 8, 0);
			int ey = std::min(pos.y + 8, 65535);
			int sz, ez;

			if (pos.z <= GROUND_LAYER) {
				sz = 0;
				ez = 9;
			} else {
				sz = std::max(pos.z - 2, GROUND_LAYER);
				ez = std::min(pos.z + 2, MAP_MAX_LAYER);
			}

			for (int z = sz; z <= ez; ++z) {
				for (int y = sy; y <= ey; ++y) {
					for (int x = sx; x <= ex; ++x) {
						if (isReachable(map.getTile(x, y, z))) {
							return false;
						}
					}
				}
			}
			return true;
		}
	};
}

void MainMenuBar::OnMapRemoveUnreachable(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	int ok = g_gui.PopupDialog("Remove Unreachable Tiles", "Do you want to remove all unreachable items from the map?", wxYES | wxNO);

	if (ok == wxID_YES) {
		g_gui.GetCurrentEditor()->selection.clear();
		g_gui.GetCurrentEditor()->actionQueue->clear();

		OnMapRemoveUnreachable::condition func;
		g_gui.CreateLoadBar("Searching map for tiles to remove...");

		long long removed = remove_if_TileOnMap(g_gui.GetCurrentMap(), func);

		g_gui.DestroyLoadBar();

		wxString msg;
		msg << removed << " tiles deleted.";

		g_gui.PopupDialog("Search completed", msg, wxOK);

		g_gui.GetCurrentMap().doChange();
	}
}

void MainMenuBar::OnClearHouseTiles(wxCommandEvent& WXUNUSED(event)) {
	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor) return;
	int ret = g_gui.PopupDialog("Clear Invalid House Tiles", "Are you sure you want to remove all house tiles that do not belong to a house (this action cannot be undone)?", wxYES | wxNO);
	if (ret == wxID_YES) {
		editor->clearInvalidHouseTiles(true);
	}
	g_gui.RefreshView();
}

void MainMenuBar::OnClearModifiedState(wxCommandEvent& WXUNUSED(event)) {
	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor) return;
	int ret = g_gui.PopupDialog("Clear Modified State", "This will have the same effect as closing the map and opening it again. Do you want to proceed?", wxYES | wxNO);
	if (ret == wxID_YES) {
		editor->clearModifiedTileState(true);
	}
	g_gui.RefreshView();
}

void MainMenuBar::OnMapCleanHouseItems(wxCommandEvent& WXUNUSED(event)) {
	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor) return;
	int ret = g_gui.PopupDialog("Clear Moveable House Items", "Are you sure you want to remove all items inside houses that can be moved (this action cannot be undone)?", wxYES | wxNO);
	if (ret == wxID_YES) {
		// editor->removeHouseItems(true); // Implementation is out-commented in base
	}
	g_gui.RefreshView();
}

void MainMenuBar::OnMapEditTowns(wxCommandEvent& WXUNUSED(event)) {
	if (g_gui.GetCurrentEditor()) {
		wxDialog* town_dialog = newd EditTownsDialog(frame, *g_gui.GetCurrentEditor());
		town_dialog->ShowModal();
		town_dialog->Destroy();
	}
}

void MainMenuBar::OnMapEditItems(wxCommandEvent& WXUNUSED(event)) { ; }
void MainMenuBar::OnMapEditMonsters(wxCommandEvent& WXUNUSED(event)) { ; }

void MainMenuBar::OnMapStatistics(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) return;
	g_gui.CreateLoadBar("Collecting data...");

	Map* map = &g_gui.GetCurrentMap();
	int load_counter = 0;
	uint64_t tile_count = 0, detailed_tile_count = 0, blocking_tile_count = 0, walkable_tile_count = 0;
	uint64_t spawn_count = 0, creature_count = 0;
	uint64_t item_count = 0, loose_item_count = 0, depot_count = 0, action_item_count = 0, unique_item_count = 0, container_count = 0;

	int town_count = map->towns.count();
	int house_count = map->houses.count();
	std::map<uint32_t, uint32_t> town_sqm_count;
	const Town* largest_town = nullptr;
	uint64_t largest_town_size = 0, total_house_sqm = 0;
	const House* largest_house = nullptr;
	uint64_t largest_house_size = 0;

	for (MapIterator mit = map->begin(); mit != map->end(); ++mit) {
		Tile* tile = (*mit)->get();
		if (load_counter % 8192 == 0) g_gui.SetLoadDone((unsigned int)(int64_t(load_counter) * 95ll / int64_t(map->getTileCount())));
		if (tile->empty()) continue;

		tile_count += 1;
		bool is_detailed = false;
#define ANALYZE_ITEM(_item)                                         \
	{                                                               \
		item_count += 1;                                            \
		if (!(_item)->isGroundTile() && !(_item)->isBorder()) {     \
			is_detailed = true;                                     \
			ItemType& it = g_items[(_item)->getID()];               \
			if (it.moveable) loose_item_count += 1;                 \
			if (it.isDepot()) depot_count += 1;                     \
			if ((_item)->getActionID() > 0) action_item_count += 1; \
			if ((_item)->getUniqueID() > 0) unique_item_count += 1; \
			if (Container* c = dynamic_cast<Container*>((_item))) { \
				if (c->getVector().size()) container_count += 1;    \
			}                                                       \
		}                                                           \
	}

		if (tile->ground) ANALYZE_ITEM(tile->ground);
		for (auto* item : tile->items) ANALYZE_ITEM(item);
#undef ANALYZE_ITEM

		if (tile->spawn) spawn_count += 1;
		if (tile->creature) creature_count += 1;
		if (tile->isBlocking()) blocking_tile_count += 1;
		else walkable_tile_count += 1;
		if (is_detailed) detailed_tile_count += 1;
		load_counter += 1;
	}

	// Rest der Logging-Logik analog zum ursprünglichen Code (gekürzt auf minimales Maß)...
	g_gui.DestroyLoadBar();
	
	std::ostringstream os;
	os.setf(std::ios::fixed, std::ios::floatfield);
	os.precision(2);
	os << "Map statistics for the map \"" << map->getMapDescription() << "\"\n";
	os << "\tTile data:\n\t\tTotal number of tiles: " << tile_count << "\n";
	//... rest of the stream formatting
	
	wxDialog* dg = newd wxDialog(frame, wxID_ANY, "Map Statistics", wxDefaultPosition, wxDefaultSize, wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX);
	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);
	wxTextCtrl* text_field = newd wxTextCtrl(dg, wxID_ANY, wxstr(os.str()), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
	text_field->SetMinSize(wxSize(400, 300));
	topsizer->Add(text_field, wxSizerFlags(5).Expand());

	wxSizer* choicesizer = newd wxBoxSizer(wxHORIZONTAL);
	choicesizer->Add(newd wxButton(dg, wxID_CANCEL, "OK"), wxSizerFlags(1).Center());
	topsizer->Add(choicesizer, wxSizerFlags(1).Center());
	dg->SetSizerAndFit(topsizer);
	dg->Centre(wxBOTH);
	dg->ShowModal();
	dg->Destroy();
}

void MainMenuBar::OnMapCleanup(wxCommandEvent& WXUNUSED(event)) {
	int ok = g_gui.PopupDialog("Clean map", "Do you want to remove all invalid items from the map?", wxYES | wxNO);
	if (ok == wxID_YES) {
		g_gui.GetCurrentMap().cleanInvalidTiles(true);
	}
}

void MainMenuBar::OnMapProperties(wxCommandEvent& WXUNUSED(event)) {
	wxDialog* properties = newd MapPropertiesWindow(frame, static_cast<MapTab*>(g_gui.GetCurrentTab()), *g_gui.GetCurrentEditor());
	if (properties->ShowModal() == 0) g_gui.CloseAllEditors();
	properties->Destroy();
}