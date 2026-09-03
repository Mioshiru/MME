#include "main.h"
#include "command_palette_dialog.h"
#include "main_menubar.h"
#include "gui_ids.h"
#include "style_manager.h"
#include "brush.h"
#include "creature_brush.h"
#include "items.h"
#include "creatures.h"
#include "raw_brush.h"
#include "gui.h"
#include "editor.h"
#include "map.h"
#include <algorithm>
#include <cctype>

enum {
	CMD_PALETTE_SEARCH = wxID_HIGHEST + 450,
	CMD_PALETTE_LIST
};

BEGIN_EVENT_TABLE(CommandPaletteDialog, wxDialog)
EVT_TEXT(CMD_PALETTE_SEARCH, CommandPaletteDialog::OnSearchText)
EVT_LISTBOX_DCLICK(CMD_PALETTE_LIST, CommandPaletteDialog::OnListDClick)
EVT_BUTTON(wxID_OK, CommandPaletteDialog::OnClickOK)
EVT_CHAR_HOOK(CommandPaletteDialog::OnKeyDown)
END_EVENT_TABLE()

CommandPaletteDialog::CommandPaletteDialog(wxWindow* parent) :
	wxDialog(parent, wxID_ANY, "Spotlight Universal Search (Befehle, Items, Brushes, Monster, Häuser)", wxDefaultPosition, wxSize(560, 420), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {

	SetBackgroundColour(wxColour(16, 28, 48));
	SetForegroundColour(wxColour(240, 245, 255));

	wxBoxSizer* main_sizer = newd wxBoxSizer(wxVERTICAL);

	search_field = newd wxTextCtrl(this, CMD_PALETTE_SEARCH, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	search_field->SetBackgroundColour(wxColour(10, 20, 35));
	search_field->SetForegroundColour(wxColour(240, 245, 255));
	search_field->SetHint("Suche nach Item-ID, Name, Brush, Monster, Preset, Town oder Befehl...");
	main_sizer->Add(search_field, 0, wxEXPAND | wxALL, 8);

	results_list = newd wxListBox(this, CMD_PALETTE_LIST, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE);
	results_list->SetBackgroundColour(wxColour(10, 20, 35));
	results_list->SetForegroundColour(wxColour(240, 245, 255));
	main_sizer->Add(results_list, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

	SetSizer(main_sizer);
	PopulateCommands();
	FilterCommands();

	RME::UI::StyleManager::ApplyThemeRecursively(this, RME::UI::StyleManager::GetTheme());

	search_field->SetFocus();
}

CommandPaletteDialog::~CommandPaletteDialog() {
}

void CommandPaletteDialog::PopulateCommands() {
	all_commands.clear();

	// 1. Presets & Ansichten
	all_commands.push_back({ "Preset: Mapper Focus (Gitter, Spawns, Ghosting, volle Hilfen)", "Ansichts-Preset", PALETTE_ITEM_ACTION, MenuBar::PRESET_VIEW_MAPPER_FOCUS });
	all_commands.push_back({ "Preset: Ingame Pure (Originale Spieloptik, saubere Sicht)", "Ansichts-Preset", PALETTE_ITEM_ACTION, MenuBar::PRESET_VIEW_INGAME_PURE });
	all_commands.push_back({ "Preset: Performance Mode (Max. FPS für riesige Maps)", "Ansichts-Preset", PALETTE_ITEM_ACTION, MenuBar::PRESET_VIEW_PERFORMANCE_MODE });

	// 2. Tools & Generatoren
	all_commands.push_back({ "Map Diagnostic & Health Scanner", "Tools", PALETTE_ITEM_ACTION, TOOLS_MAP_DIAGNOSTIC });
	all_commands.push_back({ "Procedural Terrain Generator (Noise)", "Tools", PALETTE_ITEM_ACTION, TOOLS_PROCEDURAL_GENERATOR });
	all_commands.push_back({ "Map Diff Tool (Kartenvergleich)", "Tools", PALETTE_ITEM_ACTION, TOOLS_MAP_DIFF });
	all_commands.push_back({ "Prefab & Stamp Library", "Tools", PALETTE_ITEM_ACTION, TOOLS_PREFAB_LIBRARY });
	all_commands.push_back({ "Tileset & Custom Brush Studio", "Tools", PALETTE_ITEM_ACTION, MenuBar::TOOLS_TILESET_MANAGER });
	all_commands.push_back({ "Monster Editor & Outfit Designer", "Tools", PALETTE_ITEM_ACTION, MenuBar::TOOLS_MONSTER_EDITOR });
	all_commands.push_back({ "Creature Wiki & Bestiary", "Tools", PALETTE_ITEM_ACTION, MenuBar::TOOLS_CREATURE_WIKI });
	all_commands.push_back({ "Item Editor & Sprite Inspector", "Tools", PALETTE_ITEM_ACTION, MenuBar::TOOLS_ITEM_EDITOR });
	all_commands.push_back({ "NPC Dialogue & Shop Wizard", "Tools", PALETTE_ITEM_ACTION, MenuBar::WIZARD_NPC });
	all_commands.push_back({ "Special Objects & Quest Chest Wizard", "Tools", PALETTE_ITEM_ACTION, MenuBar::WIZARD_SPECIAL_OBJECTS });
	all_commands.push_back({ "TFS Quest Generator", "Tools", PALETTE_ITEM_ACTION, MenuBar::TFS_QUEST_GENERATOR });
	all_commands.push_back({ "TFS Key & Locked Door Manager", "Tools", PALETTE_ITEM_ACTION, MenuBar::TFS_KEY_MANAGER });
	all_commands.push_back({ "Radio Player", "Tools", PALETTE_ITEM_ACTION, MenuBar::TOOLS_RADIO_PLAYER });
	all_commands.push_back({ "RealOTS / CipSoft Sector Converter", "Tools", PALETTE_ITEM_ACTION, MenuBar::TOOLS_REALOTS_CONVERTER });

	// 3. Häufige Aktionen & Zonen
	all_commands.push_back({ "House Wizard (Haus erstellen/bearbeiten)", "Häuser", PALETTE_ITEM_ACTION, PALETTE_HOUSE_ADD_HOUSE });
	all_commands.push_back({ "Optional Border Tool", "Modus", PALETTE_ITEM_ACTION, PALETTE_TERRAIN_OPTIONAL_BORDER_TOOL });
	all_commands.push_back({ "Eraser Tool (Radiergummi)", "Werkzeug", PALETTE_ITEM_ACTION, PALETTE_TERRAIN_ERASER });
	all_commands.push_back({ "Protection Zone Tool (PZ)", "Zonen", PALETTE_ITEM_ACTION, PALETTE_TERRAIN_PZ_TOOL });
	all_commands.push_back({ "No PVP Zone Tool", "Zonen", PALETTE_ITEM_ACTION, PALETTE_TERRAIN_NOPVP_TOOL });
	all_commands.push_back({ "No Logout Zone Tool", "Zonen", PALETTE_ITEM_ACTION, PALETTE_TERRAIN_NOLOGOUT_TOOL });
	all_commands.push_back({ "Toggle Auto-Bordering (Automagic)", "Modus", PALETTE_ITEM_ACTION, MenuBar::AUTOMAGIC });
	all_commands.push_back({ "Borderize Selection", "Bearbeiten", PALETTE_ITEM_ACTION, MenuBar::BORDERIZE_SELECTION });
	all_commands.push_back({ "Randomize Selection", "Bearbeiten", PALETTE_ITEM_ACTION, MenuBar::RANDOMIZE_SELECTION });
	all_commands.push_back({ "Map Cleanup (Ungültige Tiles löschen)", "Cleanups", PALETTE_ITEM_ACTION, MenuBar::MAP_CLEANUP });
	all_commands.push_back({ "Remove all Corpses", "Cleanups", PALETTE_ITEM_ACTION, MenuBar::MAP_REMOVE_CORPSES });
	all_commands.push_back({ "Remove all Unreachable Tiles", "Cleanups", PALETTE_ITEM_ACTION, MenuBar::MAP_REMOVE_UNREACHABLE_TILES });

	// 4. Brushes (Terrain, Walls, Doors, Carpets, etc.)
	const auto& brushMap = g_brushes.getMap();
	for (const auto& kv : brushMap) {
		Brush* b = kv.second;
		if (b && !b->getName().empty()) {
			PaletteCommand cmd;
			cmd.name = b->getName();
			cmd.category = "Brush";
			cmd.type = PALETTE_ITEM_BRUSH;
			cmd.brush = b;
			all_commands.push_back(cmd);
		}
	}

	// 5. Creatures (Monster & NPCs)
	for (auto it = g_creatures.begin(); it != g_creatures.end(); ++it) {
		CreatureType* ct = it->second;
		if (ct && ct->brush) {
			PaletteCommand cmd;
			cmd.name = ct->name + (ct->isNpc ? " (NPC)" : " (Monster)");
			cmd.category = ct->isNpc ? "NPC" : "Monster";
			cmd.type = PALETTE_ITEM_BRUSH;
			cmd.brush = static_cast<Brush*>(ct->brush);
			all_commands.push_back(cmd);
		}
	}

	// 6. Towns & Houses aus aktueller Map
	if (g_gui.GetCurrentEditor()) {
		Map* map = &g_gui.GetCurrentEditor()->map;
		if (map) {
			for (const auto& kv : map->towns) {
				Town* town = kv.second;
				if (town && town->getTemplePosition().isValid()) {
					PaletteCommand cmd;
					cmd.name = town->getName() + " (Temple Teleport)";
					cmd.category = "Town";
					cmd.type = PALETTE_ITEM_TELEPORT_POS;
					cmd.target_pos = town->getTemplePosition();
					all_commands.push_back(cmd);
				}
			}

			for (const auto& kv : map->houses) {
				House* house = kv.second;
				if (house && house->getExit().isValid()) {
					PaletteCommand cmd;
					cmd.name = house->name + " (House #" + std::to_string(house->getID()) + ")";
					cmd.category = "House";
					cmd.type = PALETTE_ITEM_TELEPORT_POS;
					cmd.target_pos = house->getExit();
					all_commands.push_back(cmd);
				}
			}
		}
	}
}

void CommandPaletteDialog::FilterCommands() {
	results_list->Clear();
	filtered_commands.clear();

	std::string raw_query = search_field->GetValue().ToStdString();
	std::string query = raw_query;
	std::transform(query.begin(), query.end(), query.begin(), ::tolower);

	// Wenn eine numerische ID eingegeben wurde, direkt passendes RAW Item oben anzeigen
	bool is_numeric = !query.empty() && std::all_of(query.begin(), query.end(), ::isdigit);
	if (is_numeric) {
		try {
			int target_id = std::stoi(query);
			if (g_items.typeExists(target_id)) {
				ItemType& it = g_items[target_id];
				if (it.raw_brush) {
					PaletteCommand cmd;
					cmd.name = "ID " + std::to_string(target_id) + " - " + (it.name.empty() ? "Item" : it.name);
					cmd.category = "Item ID";
					cmd.type = PALETTE_ITEM_BRUSH;
					cmd.brush = it.raw_brush;
					filtered_commands.push_back(cmd);
					results_list->Append(wxstr("[" + cmd.category + "] " + cmd.name));
				}
			}
		} catch (...) {}
	}

	// 1. Suche in statischen / dynamischen Commands (Tools, Presets, Brushes, Creatures, Towns)
	for (const auto& cmd : all_commands) {
		std::string name_lower = cmd.name;
		std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);

		if (query.empty() || name_lower.find(query) != std::string::npos) {
			filtered_commands.push_back(cmd);
			results_list->Append(wxstr("[" + cmd.category + "] " + cmd.name));
			if (filtered_commands.size() >= 100) break; // Schnelle Begrenzung für flüssige UI
		}
	}

	// 2. Suche in Items nach Namen (falls query mindestens 2 Zeichen lang)
	if (query.length() >= 2 && filtered_commands.size() < 100) {
		uint16_t max_id = g_items.getMaxID();
		for (uint16_t id = 100; id <= max_id && filtered_commands.size() < 100; ++id) {
			if (g_items.typeExists(id)) {
				const ItemType& it = g_items[id];
				if (!it.name.empty() && it.raw_brush) {
					std::string iname = it.name;
					std::string iname_lower = iname;
					std::transform(iname_lower.begin(), iname_lower.end(), iname_lower.begin(), ::tolower);
					if (iname_lower.find(query) != std::string::npos) {
						PaletteCommand cmd;
						cmd.name = iname + " (ID: " + std::to_string(id) + ")";
						cmd.category = "Item";
						cmd.type = PALETTE_ITEM_BRUSH;
						cmd.brush = it.raw_brush;
						filtered_commands.push_back(cmd);
						results_list->Append(wxstr("[" + cmd.category + "] " + cmd.name));
					}
				}
			}
		}
	}

	if (results_list->GetCount() > 0) {
		results_list->SetSelection(0);
	}
}

void CommandPaletteDialog::OnSearchText(wxCommandEvent& evt) {
	FilterCommands();
}

void CommandPaletteDialog::OnKeyDown(wxKeyEvent& evt) {
	int key = evt.GetKeyCode();
	if (key == WXK_DOWN) {
		int sel = results_list->GetSelection();
		if (sel + 1 < (int)results_list->GetCount()) {
			results_list->SetSelection(sel + 1);
		}
		return;
	} else if (key == WXK_UP) {
		int sel = results_list->GetSelection();
		if (sel > 0) {
			results_list->SetSelection(sel - 1);
		}
		return;
	} else if (key == WXK_RETURN || key == WXK_NUMPAD_ENTER) {
		wxCommandEvent dummy;
		OnClickOK(dummy);
		return;
	} else if (key == WXK_ESCAPE) {
		EndModal(wxID_CANCEL);
		return;
	}
	evt.Skip();
}

void CommandPaletteDialog::OnListDClick(wxCommandEvent& evt) {
	OnClickOK(evt);
}

void CommandPaletteDialog::OnClickOK(wxCommandEvent& evt) {
	int sel = results_list->GetSelection();
	if (sel >= 0 && static_cast<size_t>(sel) < filtered_commands.size()) {
		selected_result = filtered_commands[sel];
		EndModal(wxID_OK);
	} else {
		EndModal(wxID_CANCEL);
	}
}

