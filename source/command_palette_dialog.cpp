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
#include <wx/graphics.h>
#include <wx/region.h>
#include <algorithm>
#include <cctype>

enum {
	CMD_PALETTE_SEARCH = wxID_HIGHEST + 450,
	CMD_PALETTE_LIST,
	CMD_PALETTE_FILTER_BTN,
	CMD_MENU_FILTER_NO_RAW,
	CMD_MENU_FILTER_RAW
};

BEGIN_EVENT_TABLE(CommandPaletteDialog, wxDialog)
EVT_TEXT(CMD_PALETTE_SEARCH, CommandPaletteDialog::OnSearchText)
EVT_BUTTON(CMD_PALETTE_FILTER_BTN, CommandPaletteDialog::OnFilterButton)
EVT_MENU(CMD_MENU_FILTER_NO_RAW, CommandPaletteDialog::OnMenuFilterSelect)
EVT_MENU(CMD_MENU_FILTER_RAW, CommandPaletteDialog::OnMenuFilterSelect)
EVT_LISTBOX_DCLICK(CMD_PALETTE_LIST, CommandPaletteDialog::OnListDClick)
EVT_BUTTON(wxID_OK, CommandPaletteDialog::OnClickOK)
EVT_CHAR_HOOK(CommandPaletteDialog::OnKeyDown)
END_EVENT_TABLE()

CommandPaletteDialog::CommandPaletteDialog(wxWindow* parent) :
	wxDialog(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(620, 380), wxBORDER_NONE | wxSTAY_ON_TOP),
	current_filter(FILTER_NO_RAW) {

	// Corporate Navy Spotlight Background (#101C30) with Gold Accent
	SetBackgroundColour(wxColour(16, 28, 48));
	SetForegroundColour(wxColour(240, 245, 255));

#ifdef __WXMSW__
	// Native Windows Region für abgerundete Ecken
	HRGN hRgn = CreateRoundRectRgn(0, 0, 621, 381, 16, 16);
	if (hRgn) {
		SetShape(wxRegion(hRgn));
	}
#else
	SetShape(path);
#endif

	wxBoxSizer* main_sizer = newd wxBoxSizer(wxVERTICAL);

	// 1. Elegante, große Sucheingabezeile mit integriertem Dropdown-Button
	wxBoxSizer* input_sizer = newd wxBoxSizer(wxHORIZONTAL);

	// Schlankes, modernes Eingabefeld (#0B1424) mit zentriertem Text
	wxPanel* input_box = newd wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 46));
	input_box->SetBackgroundColour(wxColour(11, 20, 36));
	wxBoxSizer* box_inner_sizer = newd wxBoxSizer(wxVERTICAL);

	search_field = newd wxTextCtrl(input_box, CMD_PALETTE_SEARCH, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER | wxBORDER_NONE | wxTE_CENTRE);
	search_field->SetBackgroundColour(wxColour(11, 20, 36));
	search_field->SetForegroundColour(wxColour(245, 248, 255));
	wxFont search_font = search_field->GetFont();
	search_font.SetPointSize(search_font.GetPointSize() + 3);
	search_field->SetFont(search_font);

	box_inner_sizer->AddStretchSpacer(1);
	box_inner_sizer->Add(search_field, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
	box_inner_sizer->AddStretchSpacer(1);
	input_box->SetSizer(box_inner_sizer);

	input_sizer->Add(input_box, 1, wxEXPAND | wxRIGHT, 8);

	// Passig integrierter Dropdown-Button
	filter_btn = newd wxButton(this, CMD_PALETTE_FILTER_BTN, "No RAW v", wxDefaultPosition, wxSize(110, 46), wxBORDER_NONE);
	filter_btn->SetBackgroundColour(wxColour(24, 42, 70));
	filter_btn->SetForegroundColour(wxColour(255, 215, 0)); // Corporate Gold Accent
	wxFont btn_font = filter_btn->GetFont();
	btn_font.SetWeight(wxFONTWEIGHT_BOLD);
	btn_font.SetPointSize(btn_font.GetPointSize() + 1);
	filter_btn->SetFont(btn_font);
	filter_btn->SetToolTip("Select Search Scope: No RAW (Standard Palettes) or RAW (Item IDs)");
	input_sizer->Add(filter_btn, 0, wxALIGN_CENTER_VERTICAL);

	main_sizer->Add(input_sizer, 0, wxEXPAND | wxALL, 12);

	// 2. Schlichte, direkte Text-Trefferliste
	results_list = newd wxListBox(this, CMD_PALETTE_LIST, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE | wxBORDER_NONE);
	results_list->SetBackgroundColour(wxColour(11, 20, 36));
	results_list->SetForegroundColour(wxColour(240, 245, 255));
	wxFont list_font = results_list->GetFont();
	list_font.SetPointSize(list_font.GetPointSize() + 1);
	results_list->SetFont(list_font);
	main_sizer->Add(results_list, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

	SetSizer(main_sizer);
	CenterOnParent();
	UpdateFilterDisplay();
	PopulateCommands();
	FilterCommands();

	search_field->SetFocus();
}

CommandPaletteDialog::~CommandPaletteDialog() {
}

void CommandPaletteDialog::PopulateCommands() {
	all_commands.clear();

	// 1. Presets & Views
	all_commands.push_back({ "Preset: Mapper Focus (Grid, Spawns, Ghosting, full helpers)", "View Preset", PALETTE_ITEM_ACTION, MenuBar::PRESET_VIEW_MAPPER_FOCUS });
	all_commands.push_back({ "Preset: Ingame Pure (Original game visuals, clean view)", "View Preset", PALETTE_ITEM_ACTION, MenuBar::PRESET_VIEW_INGAME_PURE });
	all_commands.push_back({ "Preset: Performance Mode (Max FPS for massive maps)", "View Preset", PALETTE_ITEM_ACTION, MenuBar::PRESET_VIEW_PERFORMANCE_MODE });

	// 2. Tools & Generators
	all_commands.push_back({ "Map Diagnostic & Health Scanner", "Tools", PALETTE_ITEM_ACTION, TOOLS_MAP_DIAGNOSTIC });
	all_commands.push_back({ "Procedural Terrain Generator (Noise)", "Tools", PALETTE_ITEM_ACTION, TOOLS_PROCEDURAL_GENERATOR });
	all_commands.push_back({ "Map Diff Tool (Visual Comparison)", "Tools", PALETTE_ITEM_ACTION, TOOLS_MAP_DIFF });
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

	// 3. Actions & Tools
	all_commands.push_back({ "House Wizard (Create/Edit Houses)", "Houses", PALETTE_ITEM_ACTION, PALETTE_HOUSE_ADD_HOUSE });
	all_commands.push_back({ "Optional Border Tool", "Mode", PALETTE_ITEM_ACTION, PALETTE_TERRAIN_OPTIONAL_BORDER_TOOL });
	all_commands.push_back({ "Eraser Tool", "Tools", PALETTE_ITEM_ACTION, PALETTE_TERRAIN_ERASER });
	all_commands.push_back({ "Protection Zone Tool (PZ)", "Zones", PALETTE_ITEM_ACTION, PALETTE_TERRAIN_PZ_TOOL });
	all_commands.push_back({ "No PVP Zone Tool", "Zones", PALETTE_ITEM_ACTION, PALETTE_TERRAIN_NOPVP_TOOL });
	all_commands.push_back({ "No Logout Zone Tool", "Zones", PALETTE_ITEM_ACTION, PALETTE_TERRAIN_NOLOGOUT_TOOL });
	all_commands.push_back({ "Toggle Auto-Bordering (Automagic)", "Mode", PALETTE_ITEM_ACTION, MenuBar::AUTOMAGIC });
	all_commands.push_back({ "Borderize Selection", "Edit", PALETTE_ITEM_ACTION, MenuBar::BORDERIZE_SELECTION });
	all_commands.push_back({ "Randomize Selection", "Edit", PALETTE_ITEM_ACTION, MenuBar::RANDOMIZE_SELECTION });
	all_commands.push_back({ "Map Cleanup (Remove invalid tiles)", "Cleanups", PALETTE_ITEM_ACTION, MenuBar::MAP_CLEANUP });
	all_commands.push_back({ "Remove all Corpses", "Cleanups", PALETTE_ITEM_ACTION, MenuBar::MAP_REMOVE_CORPSES });
	all_commands.push_back({ "Remove all Unreachable Tiles", "Cleanups", PALETTE_ITEM_ACTION, MenuBar::MAP_REMOVE_UNREACHABLE_TILES });

	// 4. Brushes (Terrain, Walls, Doors, Carpets, etc. - without RAW)
	const auto& brushMap = g_brushes.getMap();
	for (const auto& kv : brushMap) {
		Brush* b = kv.second;
		if (b && !b->getName().empty() && !b->isRaw()) {
			PaletteCommand cmd;
			cmd.name = b->getName();
			cmd.category = "Brush";
			cmd.type = PALETTE_ITEM_BRUSH;
			cmd.brush = b;
			all_commands.push_back(cmd);
		}
	}

	// 5. Creatures (Monsters & NPCs)
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

	// 6. Towns & Houses from current map
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

void CommandPaletteDialog::UpdateFilterDisplay() {
	std::string label_text;
	switch (current_filter) {
		case FILTER_NO_RAW:
			label_text = "No RAW v";
			search_field->SetHint("Search standard palettes & actions (Terrain, Doodad, Item, Creature, NPC)...");
			break;
		case FILTER_RAW:
			label_text = "RAW v";
			search_field->SetHint("Search RAW palette by item ID or RAW name...");
			break;
	}
	filter_btn->SetLabel(wxstr(label_text));
	filter_btn->Refresh();
}

void CommandPaletteDialog::OnFilterButton(wxCommandEvent& evt) {
	wxMenu menu;
	wxMenuItem* item_no_raw = menu.AppendRadioItem(CMD_MENU_FILTER_NO_RAW, "No RAW (Standard Palettes)");
	wxMenuItem* item_raw = menu.AppendRadioItem(CMD_MENU_FILTER_RAW, "RAW (Item IDs & RAW Items)");

	if (current_filter == FILTER_NO_RAW) {
		item_no_raw->Check(true);
	} else {
		item_raw->Check(true);
	}

	PopupMenu(&menu, filter_btn->GetPosition().x, filter_btn->GetPosition().y + filter_btn->GetSize().y);
}

void CommandPaletteDialog::OnMenuFilterSelect(wxCommandEvent& evt) {
	int id = evt.GetId();
	if (id == CMD_MENU_FILTER_NO_RAW) {
		current_filter = FILTER_NO_RAW;
	} else if (id == CMD_MENU_FILTER_RAW) {
		current_filter = FILTER_RAW;
	}

	UpdateFilterDisplay();
	FilterCommands();
	search_field->SetFocus();
}

void CommandPaletteDialog::FilterCommands() {
	results_list->Clear();
	filtered_commands.clear();

	std::string raw_query = search_field->GetValue().ToStdString();
	std::string query = raw_query;
	std::transform(query.begin(), query.end(), query.begin(), ::tolower);

	// Wenn das Suchfeld leer ist, keine Trefferliste anzeigen
	if (query.empty()) {
		results_list->Show(false);
		Layout();
		return;
	}

	results_list->Show(true);

	if (current_filter == FILTER_NO_RAW) {
		// ==========================================
		// 1. NO RAW: Durchsucht alle Paletten-Brushes (Terrain, Doodad, Wall, Carpet, Monster, NPC) & Aktionen/Tools
		// ==========================================
		for (const auto& cmd : all_commands) {
			if (filtered_commands.size() >= 80) break;
			std::string name_lower = cmd.name;
			std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
			if (name_lower.find(query) != std::string::npos) {
				filtered_commands.push_back(cmd);
				results_list->Append(wxstr(cmd.name));
			}
		}
	} else if (current_filter == FILTER_RAW) {
		// ==========================================
		// 2. RAW: Ausschließlich RAW Palette (Item IDs & RAW Brushes)
		// ==========================================
		
		// Numerische ID Direkt-Suche
		bool is_numeric = !query.empty() && std::all_of(query.begin(), query.end(), ::isdigit);
		if (is_numeric) {
			try {
				int target_id = std::stoi(query);
				if (g_items.typeExists(target_id)) {
					ItemType& it = g_items[target_id];
					if (it.raw_brush) {
						PaletteCommand cmd;
						cmd.name = "ID " + std::to_string(target_id) + " - " + (it.name.empty() ? "Item" : it.name);
						cmd.category = "RAW";
						cmd.type = PALETTE_ITEM_BRUSH;
						cmd.brush = it.raw_brush;
						filtered_commands.push_back(cmd);
						results_list->Append(wxstr(cmd.name));
					}
				}
			} catch (...) {}
		}

		// Name Suche in RAW Items
		uint16_t max_id = g_items.getMaxID();
		for (uint16_t id = 100; id <= max_id && filtered_commands.size() < 80; ++id) {
			if (g_items.typeExists(id)) {
				const ItemType& it = g_items[id];
				if (!it.name.empty() && it.raw_brush) {
					std::string iname = it.name;
					std::string iname_lower = iname;
					std::transform(iname_lower.begin(), iname_lower.end(), iname_lower.begin(), ::tolower);
					if (iname_lower.find(query) != std::string::npos) {
						PaletteCommand cmd;
						cmd.name = iname + " (ID: " + std::to_string(id) + ")";
						cmd.category = "RAW";
						cmd.type = PALETTE_ITEM_BRUSH;
						cmd.brush = it.raw_brush;
						filtered_commands.push_back(cmd);
						results_list->Append(wxstr(cmd.name));
					}
				}
			}
		}
	}

	if (results_list->GetCount() > 0) {
		results_list->SetSelection(0);
	}
	Layout();
}

void CommandPaletteDialog::OnSearchText(wxCommandEvent& evt) {
	FilterCommands();
}

void CommandPaletteDialog::OnKeyDown(wxKeyEvent& evt) {
	int key = evt.GetKeyCode();
	if (key == WXK_TAB) {
		current_filter = (current_filter == FILTER_NO_RAW) ? FILTER_RAW : FILTER_NO_RAW;
		UpdateFilterDisplay();
		FilterCommands();
		return;
	} else if (key == WXK_DOWN) {
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

