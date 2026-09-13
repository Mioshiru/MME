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
#include "creature.h"
#include "complexitem.h"
#include "waypoints.h"
#include "world_map_markers.h"
#include "checklist_manager.h"
#include "style_manager.h"
#include <wx/clipbrd.h>
#include <sstream>
#include <iomanip>
#include <map>
#include <vector>
#include <algorithm>

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
		g_gui.RefreshMinimapPanel();
	}
}

void MainMenuBar::OnMapEditItems(wxCommandEvent& WXUNUSED(event)) { ; }
void MainMenuBar::OnMapEditMonsters(wxCommandEvent& WXUNUSED(event)) { ; }

void MainMenuBar::OnMapStatistics(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) return;
	g_gui.CreateLoadBar("Analyzing Map Statistics...");

	Map* map = &g_gui.GetCurrentMap();
	int load_counter = 0;
	uint64_t tile_count = 0, detailed_tile_count = 0, blocking_tile_count = 0, walkable_tile_count = 0;
	uint64_t pz_tile_count = 0, nopz_tile_count = 0, pvp_tile_count = 0, nolog_tile_count = 0;
	uint64_t spawn_count = 0, total_spawn_radius = 0;
	uint64_t creature_count = 0, monster_count = 0, npc_count = 0;
	uint64_t item_count = 0, loose_item_count = 0, depot_count = 0, action_item_count = 0, unique_item_count = 0, container_count = 0;
	uint64_t door_count = 0, teleporter_count = 0, ground_count = 0, border_count = 0;

	uint64_t floor_tiles[16] = { 0 };
	uint64_t floor_creatures[16] = { 0 };
	uint64_t floor_spawns[16] = { 0 };

	std::map<std::string, int> monster_freq;
	std::map<std::string, int> npc_freq;
	std::map<uint32_t, uint32_t> house_sqm_map;
	std::map<uint32_t, uint32_t> town_sqm_map;

	for (MapIterator mit = map->begin(); mit != map->end(); ++mit) {
		Tile* tile = (*mit)->get();
		if (load_counter % 8192 == 0) g_gui.SetLoadDone((unsigned int)(int64_t(load_counter) * 95ll / int64_t(map->getTileCount())));
		if (tile->empty()) continue;

		tile_count += 1;
		int z = tile->getZ();
		if (z >= 0 && z < 16) {
			floor_tiles[z] += 1;
		}

		uint16_t mapflags = tile->getMapFlags();
		if (mapflags & TILESTATE_PROTECTIONZONE) pz_tile_count += 1;
		if (mapflags & TILESTATE_NOPVP) nopz_tile_count += 1;
		if (mapflags & TILESTATE_PVPZONE) pvp_tile_count += 1;
		if (mapflags & TILESTATE_NOLOGOUT) nolog_tile_count += 1;

		if (tile->isHouseTile() && tile->getHouseID() > 0) {
			house_sqm_map[tile->getHouseID()] += 1;
			House* h = map->houses.getHouse(tile->getHouseID());
			if (h && h->townid > 0) {
				town_sqm_map[h->townid] += 1;
			}
		}

		bool is_detailed = false;
#define ANALYZE_ITEM(_item)                                         \
		{                                                               \
			item_count += 1;                                            \
			if ((_item)->isGroundTile()) ground_count += 1;             \
			else if ((_item)->isBorder()) border_count += 1;            \
			else {                                                      \
				is_detailed = true;                                     \
				ItemType& it = g_items[(_item)->getID()];               \
				if (it.moveable) loose_item_count += 1;                 \
				if (it.isDepot()) depot_count += 1;                     \
				if (it.isDoor()) door_count += 1;                       \
				if (it.isTeleport()) teleporter_count += 1;             \
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

		if (tile->spawn) {
			spawn_count += 1;
			total_spawn_radius += tile->spawn->getSize();
			if (z >= 0 && z < 16) floor_spawns[z] += 1;
		}

		if (tile->creature) {
			creature_count += 1;
			if (z >= 0 && z < 16) floor_creatures[z] += 1;
			if (tile->creature->isNpc()) {
				npc_count += 1;
				npc_freq[tile->creature->getName()] += 1;
			} else {
				monster_count += 1;
				monster_freq[tile->creature->getName()] += 1;
			}
		}

		if (tile->isBlocking()) blocking_tile_count += 1;
		else walkable_tile_count += 1;

		if (is_detailed) detailed_tile_count += 1;
		load_counter += 1;
	}

	g_gui.DestroyLoadBar();

	int town_count = map->towns.count();
	int house_count = map->houses.count();
	uint64_t total_house_sqm = 0;
	int guildhall_count = 0;
	const House* largest_house = nullptr;
	uint64_t largest_house_size = 0;
	uint64_t total_rent = 0;

	for (auto hit = map->houses.begin(); hit != map->houses.end(); ++hit) {
		const House* h = hit->second;
		if (!h) continue;
		if (h->guildhall) guildhall_count += 1;
		total_rent += h->rent;
		uint32_t sqms = 0;
		auto it = house_sqm_map.find(h->getID());
		if (it != house_sqm_map.end()) sqms = it->second;
		else sqms = (uint32_t)h->size();
		total_house_sqm += sqms;
		if (sqms > largest_house_size) {
			largest_house_size = sqms;
			largest_house = h;
		}
	}

	std::ostringstream os;
	os << "======================================================================\n";
	os << "                      MAP STATISTICS OVERVIEW                         \n";
	os << "======================================================================\n\n";

	os << "[MAP INFORMATION]\n";
	os << "  Description : " << (map->getMapDescription().empty() ? "Untitled Map" : map->getMapDescription()) << "\n";
	os << "  Dimensions  : " << map->getWidth() << " x " << map->getHeight() << " (Tiles in Memory: " << tile_count << ")\n\n";

	os << "[TOWNS & CITIES (" << town_count << " Total)]\n";
	if (town_count == 0) {
		os << "  No towns registered on this map.\n";
	} else {
		for (auto tit = map->towns.begin(); tit != map->towns.end(); ++tit) {
			const Town* t = tit->second;
			if (!t) continue;
			Position tpos = t->getTemplePosition();
			uint32_t t_sqm = town_sqm_map[t->getID()];
			os << "  - ID " << std::setw(3) << t->getID() << " | " << std::left << std::setw(24) << t->getName()
			   << " | Temple: (" << tpos.x << ", " << tpos.y << ", " << tpos.z << ")"
			   << " | House Area: " << t_sqm << " sqm\n";
		}
	}
	os << "\n";

	os << "[HOUSES & GUILDHALLS]\n";
	os << "  Total Houses/Estates : " << house_count << " (Regular: " << (house_count - guildhall_count) << ", Guildhalls: " << guildhall_count << ")\n";
	os << "  Total House Area     : " << total_house_sqm << " sqm";
	if (house_count > 0) {
		os << " (Average: " << std::fixed << std::setprecision(1) << (double)total_house_sqm / house_count << " sqm/house)";
	}
	os << "\n";
	os << "  Total Monthly Rent   : " << total_rent << " gold\n";
	if (largest_house) {
		os << "  Largest House        : \"" << largest_house->name << "\" (ID: " << largest_house->getID() << ") - " << largest_house_size << " sqm\n";
	}
	os << "\n";

	os << "[CREATURES & SPAWNS]\n";
	os << "  Total Creatures      : " << creature_count << " (Monsters: " << monster_count << ", NPCs: " << npc_count << ")\n";
	os << "  Unique Monster Types : " << monster_freq.size() << "\n";
	os << "  Unique NPC Types     : " << npc_freq.size() << "\n";
	os << "  Total Spawns         : " << spawn_count;
	if (spawn_count > 0) {
		os << " (Average Radius: " << std::fixed << std::setprecision(1) << (double)total_spawn_radius / spawn_count << ")";
	}
	os << "\n";

	if (!monster_freq.empty()) {
		std::vector<std::pair<std::string, int>> sorted_monsters(monster_freq.begin(), monster_freq.end());
		std::sort(sorted_monsters.begin(), sorted_monsters.end(), [](const auto& a, const auto& b) {
			return a.second > b.second;
		});
		os << "  Top Monsters:\n";
		int shown = 0;
		for (const auto& kv : sorted_monsters) {
			os << "    * " << std::left << std::setw(20) << kv.first << " : " << kv.second << "\n";
			if (++shown >= 8) break;
		}
	}

	if (!npc_freq.empty()) {
		os << "  NPCs on Map:\n";
		int count = 0;
		os << "    ";
		for (const auto& kv : npc_freq) {
			os << kv.first << " (" << kv.second << ")  ";
			if (++count % 4 == 0) os << "\n    ";
		}
		os << "\n";
	}
	os << "\n";

	os << "[ITEMS & MECHANICS]\n";
	os << "  Total Items          : " << item_count << "\n";
	os << "  Ground & Borders     : " << ground_count << " grounds, " << border_count << " borders\n";
	os << "  Loose / Moveable     : " << loose_item_count << "\n";
	os << "  Containers & Chests  : " << container_count << "\n";
	os << "  Depot Lockers        : " << depot_count << "\n";
	os << "  Doors & Gates        : " << door_count << "\n";
	os << "  Teleporters          : " << teleporter_count << "\n";
	os << "  Action IDs Configured: " << action_item_count << "\n";
	os << "  Unique IDs Configured: " << unique_item_count << "\n\n";

	os << "[Z-AXIS FLOOR DISTRIBUTION]\n";
	os << "  Floor | SQM Tiles | Creatures | Spawns\n";
	os << "  ------+-----------+-----------+-------\n";
	for (int z = 0; z <= 15; ++z) {
		if (floor_tiles[z] > 0 || floor_creatures[z] > 0 || floor_spawns[z] > 0) {
			std::string layerName = "";
			if (z == 7) layerName = " (Surface Ground)";
			else if (z < 7) layerName = " (Above Ground +" + std::to_string(7 - z) + ")";
			else layerName = " (Underground -" + std::to_string(z - 7) + ")";

			os << "  Z=" << std::setw(2) << z << "  | " << std::setw(9) << floor_tiles[z] << " | "
			   << std::setw(9) << floor_creatures[z] << " | " << std::setw(6) << floor_spawns[z]
			   << layerName << "\n";
		}
	}
	os << "\n";

	os << "[NAVIGATION, MARKERS & TASKS]\n";
	os << "  Waypoints Defined    : " << map->waypoints.waypoints.size() << "\n";
	os << "  World Map Markers    : " << WorldMapMarkerManager::GetInstance().GetMarkers().size() << "\n";
	os << "  Checklist Quests     : " << ChecklistManager::getInstance().getActiveCount() << " active, "
	   << ChecklistManager::getInstance().getCompletedCount() << " completed\n";
	os << "======================================================================\n";

	std::string stats_text = os.str();

	wxDialog* dg = newd wxDialog(frame, wxID_ANY, "Map Statistics", wxDefaultPosition, wxSize(640, 560), wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX);
	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);
	wxTextCtrl* text_field = newd wxTextCtrl(dg, wxID_ANY, wxstr(stats_text), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP);
	wxFont monoFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	text_field->SetFont(monoFont);
	topsizer->Add(text_field, wxSizerFlags(1).Expand().Border(wxALL, 8));

	wxSizer* choicesizer = newd wxBoxSizer(wxHORIZONTAL);
	wxButton* copyBtn = newd wxButton(dg, wxID_ANY, "Copy to Clipboard");
	copyBtn->Bind(wxEVT_BUTTON, [stats_text](wxCommandEvent&) {
		if (wxTheClipboard->Open()) {
			wxTheClipboard->SetData(newd wxTextDataObject(wxstr(stats_text)));
			wxTheClipboard->Close();
			g_gui.SetStatusText("Map statistics copied to clipboard.");
		}
	});
	choicesizer->Add(copyBtn, wxSizerFlags(0).Center().Border(wxRIGHT, 10));

	wxButton* okBtn = newd wxButton(dg, wxID_OK, "Close");
	choicesizer->Add(okBtn, wxSizerFlags(0).Center());

	topsizer->Add(choicesizer, wxSizerFlags(0).Right().Border(wxRIGHT | wxBOTTOM, 10));
	dg->SetSizer(topsizer);
	dg->Centre(wxBOTH);

	RME::UI::StyleManager::ApplyThemeRecursively(dg, RME::UI::StyleManager::GetTheme());

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