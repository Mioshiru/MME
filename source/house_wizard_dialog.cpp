//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "main.h"
#include "style_manager.h"
#include "house_wizard_dialog.h"
#include "house.h"
#include "map.h"
#include "town.h"
#include "gui.h"
#include "house_brush.h"
#include "tileset.h"
#include "map_tab.h"
#include "map_display.h"
#include <cstdlib>
#include <ctime>

BEGIN_EVENT_TABLE(HouseWizardDialog, wxDialog)
	EVT_BUTTON(WIZARD_ID_ROLL, HouseWizardDialog::OnClickRollName)
	EVT_BUTTON(wxID_OK, HouseWizardDialog::OnClickOK)
	EVT_BUTTON(wxID_CANCEL, HouseWizardDialog::OnClickCancel)
END_EVENT_TABLE()

std::string HouseWizardDialog::GenerateRandomHouseName() {
	static const std::vector<std::string> prefixes = {
		"Whispering", "Golden", "Emerald", "Moonlit", "Sunstrider", "Ravencrest", 
		"Silverleaf", "Ironforge", "Seabreeze", "Shadowglen", "Amberfall", "Stormhaven", 
		"Crystal", "Dragonfire", "Frostpeak", "Wildwood", "Oaken", "Starlight", 
		"Riverbend", "Highland", "Misty", "Bramblewood", "Ember", "Cobalt", 
		"Falcon", "Dawn", "Dusk", "Willow", "Ivory", "Sable", "Thunder",
		"Cedar", "Pine", "Suncrest", "Nightfall", "Valiant", "Rosewood", "Royal"
	};
	static const std::vector<std::string> nouns = {
		"Manor", "Cottage", "Villa", "Haven", "Sanctum", "Lodge", "Retreat", 
		"Residence", "Hall", "Keep", "Abode", "Chateau", "Estate", "Bastion", 
		"Refuge", "Cabin", "Dwelling", "Sanctuary", "Homestead", "Den", 
		"Quarters", "Garrison", "Hideaway", "Tower", "Alcove", "Meadow"
	};
	static bool seeded = false;
	if (!seeded) {
		srand(static_cast<unsigned int>(time(nullptr)));
		seeded = true;
	}
	int p_idx = rand() % prefixes.size();
	int n_idx = rand() % nouns.size();
	return prefixes[p_idx] + " " + nouns[n_idx];
}

HouseWizardDialog::HouseWizardDialog(wxWindow* parent, Map* map, uint32_t default_town_id) :
	wxDialog(parent, wxID_ANY, "Create House Wizard", wxDefaultPosition, wxSize(520, 390), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	map(map),
	created_house(nullptr),
	name_field(nullptr),
	roll_btn(nullptr),
	town_choice(nullptr),
	id_field(nullptr),
	rent_field(nullptr),
	guildhall_checkbox(nullptr),
	ok_btn(nullptr),
	cancel_btn(nullptr) {

	ASSERT(map);

	SetBackgroundColour(wxColour(16, 24, 38));
	SetForegroundColour(wxColour(240, 245, 255));

	wxBoxSizer* main_sizer = newd wxBoxSizer(wxVERTICAL);

	// Header Banner Box
	wxPanel* header_panel = newd wxPanel(this, wxID_ANY);
	header_panel->SetBackgroundColour(wxColour(12, 18, 30));
	wxBoxSizer* header_sizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticText* title_lbl = newd wxStaticText(header_panel, wxID_ANY, "House Creation Wizard");
	title_lbl->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	title_lbl->SetForegroundColour(wxColour(255, 215, 80));
	header_sizer->Add(title_lbl, 0, wxBOTTOM, 4);

	wxStaticText* subtitle_lbl = newd wxStaticText(header_panel, wxID_ANY,
		"Configure the house properties below.\nOnce created, you can paint floor tiles and set the exit doorway on the map.");
	subtitle_lbl->SetForegroundColour(wxColour(180, 195, 215));
	header_sizer->Add(subtitle_lbl, 0, wxEXPAND);

	header_panel->SetSizer(header_sizer);
	main_sizer->Add(header_panel, 0, wxEXPAND | wxALL, 12);

	// Form Grid (5 rows, 2 columns with ample spacing)
	wxFlexGridSizer* grid = newd wxFlexGridSizer(5, 2, 10, 14);
	grid->AddGrowableCol(1);

	// 1. House Name with Random Button
	wxStaticText* name_lbl = newd wxStaticText(this, wxID_ANY, "House Name:");
	name_lbl->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	name_lbl->SetForegroundColour(wxColour(230, 235, 245));
	grid->Add(name_lbl, 0, wxALIGN_CENTER_VERTICAL);

	wxBoxSizer* name_row = newd wxBoxSizer(wxHORIZONTAL);
	std::string initial_name = GenerateRandomHouseName();
	name_field = newd wxTextCtrl(this, wxID_ANY, wxString::FromUTF8(initial_name));
	name_field->SetBackgroundColour(wxColour(10, 15, 26));
	name_field->SetForegroundColour(wxColour(245, 245, 255));
	name_row->Add(name_field, 1, wxEXPAND | wxRIGHT, 8);

	roll_btn = newd wxButton(this, WIZARD_ID_ROLL, "Random", wxDefaultPosition, wxSize(75, 24));
	roll_btn->SetBackgroundColour(wxColour(30, 60, 110));
	roll_btn->SetForegroundColour(wxColour(255, 225, 120));
	roll_btn->SetToolTip("Generate a random English fantasy house name");
	name_row->Add(roll_btn, 0, wxALIGN_CENTER_VERTICAL);
	grid->Add(name_row, 1, wxEXPAND);

	// 2. Town Selection
	wxStaticText* town_lbl = newd wxStaticText(this, wxID_ANY, "Town / City:");
	town_lbl->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	town_lbl->SetForegroundColour(wxColour(230, 235, 245));
	grid->Add(town_lbl, 0, wxALIGN_CENTER_VERTICAL);

	town_choice = newd wxChoice(this, wxID_ANY);
	town_choice->SetBackgroundColour(wxColour(10, 15, 26));
	town_choice->SetForegroundColour(wxColour(245, 245, 255));

	uint32_t target_town_id = default_town_id;
	int select_idx = 0;
	if (map->towns.count() > 0) {
		int idx = 0;
		for (const auto& pair : map->towns) {
			const Town* town = pair.second;
			town_choice->Append(wxString::FromUTF8(town->getName()), reinterpret_cast<void*>(static_cast<uintptr_t>(town->getID())));
			if (town->getID() == target_town_id || (target_town_id == 0 && idx == 0)) {
				select_idx = idx;
			}
			++idx;
		}
	} else {
		town_choice->Append("No Town", reinterpret_cast<void*>(static_cast<uintptr_t>(0)));
	}
	town_choice->SetSelection(select_idx);
	grid->Add(town_choice, 1, wxEXPAND);

	// 3. House ID
	wxStaticText* id_lbl = newd wxStaticText(this, wxID_ANY, "House ID:");
	id_lbl->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	id_lbl->SetForegroundColour(wxColour(230, 235, 245));
	grid->Add(id_lbl, 0, wxALIGN_CENTER_VERTICAL);

	uint32_t next_id = map->houses.getEmptyID();
	id_field = newd wxSpinCtrl(this, wxID_ANY, wxString::Format("%u", next_id), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 999999, next_id);
	id_field->SetBackgroundColour(wxColour(10, 15, 26));
	id_field->SetForegroundColour(wxColour(245, 245, 255));
	grid->Add(id_field, 1, wxEXPAND);

	// 4. Rent
	wxStaticText* rent_lbl = newd wxStaticText(this, wxID_ANY, "Rent (Gold):");
	rent_lbl->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	rent_lbl->SetForegroundColour(wxColour(230, 235, 245));
	grid->Add(rent_lbl, 0, wxALIGN_CENTER_VERTICAL);

	rent_field = newd wxTextCtrl(this, wxID_ANY, "0");
	rent_field->SetBackgroundColour(wxColour(10, 15, 26));
	rent_field->SetForegroundColour(wxColour(245, 245, 255));
	grid->Add(rent_field, 1, wxEXPAND);

	// 5. Guildhall
	wxStaticText* gh_lbl = newd wxStaticText(this, wxID_ANY, "Guildhall:");
	gh_lbl->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	gh_lbl->SetForegroundColour(wxColour(230, 235, 245));
	grid->Add(gh_lbl, 0, wxALIGN_CENTER_VERTICAL);

	guildhall_checkbox = newd wxCheckBox(this, wxID_ANY, "Designate this house as a Guildhall");
	guildhall_checkbox->SetForegroundColour(wxColour(200, 215, 235));
	grid->Add(guildhall_checkbox, 0, wxALIGN_CENTER_VERTICAL);

	main_sizer->Add(grid, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 14);

	// Info tip banner
	wxStaticText* tip_lbl = newd wxStaticText(this, wxID_ANY,
		"Tip: Left-click on map paints house tiles. Click on an existing tile to set the exit.");
	tip_lbl->SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL));
	tip_lbl->SetForegroundColour(wxColour(140, 180, 220));
	main_sizer->Add(tip_lbl, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

	// Buttons Row
	wxBoxSizer* btn_sizer = newd wxBoxSizer(wxHORIZONTAL);
	btn_sizer->AddStretchSpacer();

	ok_btn = newd wxButton(this, wxID_OK, "Create House", wxDefaultPosition, wxSize(125, 28));
	ok_btn->SetBackgroundColour(wxColour(25, 80, 150));
	ok_btn->SetForegroundColour(wxColour(255, 225, 120));
	ok_btn->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	btn_sizer->Add(ok_btn, 0, wxRIGHT, 8);

	cancel_btn = newd wxButton(this, wxID_CANCEL, "Cancel", wxDefaultPosition, wxSize(85, 28));
	cancel_btn->SetBackgroundColour(wxColour(30, 40, 60));
	cancel_btn->SetForegroundColour(wxColour(210, 220, 235));
	btn_sizer->Add(cancel_btn, 0);

	main_sizer->Add(btn_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 14);

	SetSizerAndFit(main_sizer);
	SetMinSize(GetSize());
	RME::UI::StyleManager::ApplyThemeRecursively(this, RME::UI::StyleManager::GetTheme());
	Centre(wxBOTH);
}

HouseWizardDialog::~HouseWizardDialog() {
}

void HouseWizardDialog::OnClickRollName(wxCommandEvent& WXUNUSED(evt)) {
	if (name_field) {
		name_field->SetValue(wxString::FromUTF8(GenerateRandomHouseName()));
	}
}

void HouseWizardDialog::OnClickOK(wxCommandEvent& WXUNUSED(evt)) {
	if (!map) {
		EndModal(wxID_CANCEL);
		return;
	}

	wxString house_name = name_field ? name_field->GetValue().Trim().Trim(false) : wxString();
	if (house_name.IsEmpty()) {
		g_gui.PopupDialog(this, "Input Error", "House name cannot be empty.", wxOK);
		return;
	}

	uint32_t house_id = id_field ? static_cast<uint32_t>(id_field->GetValue()) : 0;
	if (house_id < 1) {
		g_gui.PopupDialog(this, "Input Error", "House ID must be 1 or higher.", wxOK);
		return;
	}

	// Check for duplicate ID
	if (map->houses.getHouse(house_id) != nullptr) {
		g_gui.PopupDialog(this, "Input Error", wxString::Format("House ID %u is already in use by another house. Please choose a different ID.", house_id), wxOK);
		return;
	}

	long house_rent = 0;
	if (rent_field) {
		rent_field->GetValue().ToLong(&house_rent);
		if (house_rent < 0) house_rent = 0;
	}

	uint32_t selected_town_id = 0;
	if (town_choice) {
		int town_sel = town_choice->GetSelection();
		if (town_sel != wxNOT_FOUND) {
			selected_town_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(town_choice->GetClientData(town_sel)));
		}
	}

	bool is_guildhall = guildhall_checkbox ? guildhall_checkbox->GetValue() : false;

	created_house = newd House(*map);
	created_house->setID(house_id);
	created_house->name = nstr(house_name);
	created_house->townid = selected_town_id;
	created_house->rent = house_rent;
	created_house->guildhall = is_guildhall;

	map->houses.addHouse(created_house);
	map->doChange();

	// Automatically switch Palette to Houses tab
	g_gui.SelectPalettePage(TILESET_HOUSE);

	// Select house brush immediately so user can paint tiles
	if (g_gui.house_brush) {
		g_gui.house_brush->setHouse(created_house);
		g_gui.SelectBrush(g_gui.house_brush, TILESET_HOUSE);
	}

	MapTab* mt = g_gui.GetCurrentMapTab();
	if (mt && mt->GetCanvas()) {
		mt->GetCanvas()->ShowHUDNotification("House \"" + created_house->name + "\" created! Paint floor tiles on the map.", 0xFF10B981);
	}
	g_gui.SetStatusText(wxString::Format("Created house \"%s\" (ID: %u). Paint floor tiles on map. Click existing tile for Exit.", wxstr(created_house->name), created_house->getID()));
	g_gui.RefreshView();

	EndModal(wxID_OK);
}

void HouseWizardDialog::OnClickCancel(wxCommandEvent& WXUNUSED(evt)) {
	EndModal(wxID_CANCEL);
}
