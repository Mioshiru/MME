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
#include "house_exit_brush.h"

enum {
	WIZARD_ID_BACK = 10001,
	WIZARD_ID_NEXT,
	WIZARD_ID_PAINT_TILES,
	WIZARD_ID_SET_EXIT,
};

BEGIN_EVENT_TABLE(HouseWizardDialog, wxDialog)
	EVT_BUTTON(WIZARD_ID_NEXT, HouseWizardDialog::OnClickNext)
	EVT_BUTTON(WIZARD_ID_BACK, HouseWizardDialog::OnClickBack)
	EVT_BUTTON(wxID_OK, HouseWizardDialog::OnClickOK)
	EVT_BUTTON(wxID_CANCEL, HouseWizardDialog::OnClickCancel)
	EVT_TOGGLEBUTTON(WIZARD_ID_PAINT_TILES, HouseWizardDialog::OnClickPaintTiles)
	EVT_TOGGLEBUTTON(WIZARD_ID_SET_EXIT, HouseWizardDialog::OnClickSetExit)
	EVT_ACTIVATE(HouseWizardDialog::OnActivate)
	EVT_ICONIZE(HouseWizardDialog::OnIconize)
	EVT_ENTER_WINDOW(HouseWizardDialog::OnMouseEnter)
	EVT_LEAVE_WINDOW(HouseWizardDialog::OnMouseLeave)
END_EVENT_TABLE()

HouseWizardDialog::HouseWizardDialog(wxWindow* parent, Map* map, uint32_t default_town_id, House* existing_house) :
	wxDialog(parent, wxID_ANY, existing_house ? wxString::Format("Edit House: %s", existing_house->name.c_str()) : wxString("Let's create a House"), wxDefaultPosition, wxSize(380, 240), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	map(map),
	draft_house(existing_house),
	is_editing(existing_house != nullptr),
	current_step(1),
	step1_panel(nullptr),
	step2_panel(nullptr),
	step3_panel(nullptr) {

	ASSERT(map);
	if (!is_editing) {
		draft_house = newd House(*map);
		draft_house->setID(map->houses.getEmptyID());
		map->houses.addHouse(draft_house);
	}

	SetBackgroundColour(wxColour(16, 28, 48));
	SetForegroundColour(wxColour(240, 245, 255));

	wxBoxSizer* main_sizer = newd wxBoxSizer(wxVERTICAL);

	// =========================================================================
	// STEP 1 PANEL (Properties)
	// =========================================================================
	step1_panel = newd wxPanel(this, wxID_ANY);
	wxBoxSizer* step1_sizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticText* step1_lbl = newd wxStaticText(step1_panel, wxID_ANY, "Step 1: Basic House Details", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
	step1_lbl->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	step1_lbl->SetForegroundColour(wxColour(240, 210, 120));
	step1_sizer->Add(step1_lbl, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 8);

	wxFlexGridSizer* grid1 = newd wxFlexGridSizer(2, 10, 10);
	grid1->AddGrowableCol(1);

	// House Name
	grid1->Add(newd wxStaticText(step1_panel, wxID_ANY, "House Name:"), 0, wxALIGN_CENTER_VERTICAL);
	std::string default_name = is_editing ? draft_house->name : ("House #" + std::to_string(draft_house->getID()));
	name_field = newd wxTextCtrl(step1_panel, wxID_ANY, wxstr(default_name));
	name_field->SetBackgroundColour(wxColour(10, 20, 35));
	name_field->SetForegroundColour(wxColour(240, 245, 255));
	grid1->Add(name_field, 1, wxEXPAND);

	// Town Selection
	grid1->Add(newd wxStaticText(step1_panel, wxID_ANY, "Town:"), 0, wxALIGN_CENTER_VERTICAL);
	town_choice = newd wxChoice(step1_panel, wxID_ANY);
	town_choice->SetBackgroundColour(wxColour(10, 20, 35));
	town_choice->SetForegroundColour(wxColour(240, 245, 255));

	uint32_t target_town_id = is_editing ? draft_house->townid : default_town_id;
	int select_idx = 0;
	if (map->towns.count() > 0) {
		int idx = 0;
		for (const auto& pair : map->towns) {
			const Town* town = pair.second;
			town_choice->Append(wxstr(town->getName()), reinterpret_cast<void*>(static_cast<uintptr_t>(town->getID())));
			if (town->getID() == target_town_id) {
				select_idx = idx;
			}
			++idx;
		}
	} else {
		town_choice->Append("No Town", reinterpret_cast<void*>(static_cast<uintptr_t>(0)));
	}
	town_choice->SetSelection(select_idx);
	grid1->Add(town_choice, 1, wxEXPAND);

	// Rent
	grid1->Add(newd wxStaticText(step1_panel, wxID_ANY, "Rent (Gold):"), 0, wxALIGN_CENTER_VERTICAL);
	rent_field = newd wxTextCtrl(step1_panel, wxID_ANY, wxString::Format("%d", is_editing ? draft_house->rent : 0));
	rent_field->SetBackgroundColour(wxColour(10, 20, 35));
	rent_field->SetForegroundColour(wxColour(240, 245, 255));
	grid1->Add(rent_field, 1, wxEXPAND);

	// Guildhall
	grid1->Add(newd wxStaticText(step1_panel, wxID_ANY, "Guildhall:"), 0, wxALIGN_CENTER_VERTICAL);
	guildhall_checkbox = newd wxCheckBox(step1_panel, wxID_ANY, "Is Guildhall");
	if (is_editing) {
		guildhall_checkbox->SetValue(draft_house->guildhall);
	}
	grid1->Add(guildhall_checkbox, 0, wxALIGN_CENTER_VERTICAL);

	step1_sizer->Add(grid1, 1, wxEXPAND | wxALL, 8);
	step1_panel->SetSizer(step1_sizer);
	main_sizer->Add(step1_panel, 1, wxEXPAND | wxALL, 6);

	// =========================================================================
	// STEP 2 PANEL (Paint House Tiles)
	// =========================================================================
	step2_panel = newd wxPanel(this, wxID_ANY);
	wxBoxSizer* step2_sizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticText* step2_lbl = newd wxStaticText(step2_panel, wxID_ANY, "Step 2: Paint House Tiles", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
	step2_lbl->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	step2_lbl->SetForegroundColour(wxColour(240, 210, 120));
	step2_sizer->Add(step2_lbl, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 6);

	wxStaticText* tile_hint = newd wxStaticText(step2_panel, wxID_ANY,
		"Click the button below to start drawing house tiles on the canvas.\n\n"
		"Hint: Holding the Ctrl key while drawing removes house tiles.", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
	tile_hint->SetForegroundColour(wxColour(200, 210, 225));
	step2_sizer->Add(tile_hint, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 8);

	paint_tiles_btn = newd wxToggleButton(step2_panel, WIZARD_ID_PAINT_TILES, "Paint Tiles", wxDefaultPosition, wxSize(130, 26));
	paint_tiles_btn->SetBackgroundColour(wxColour(35, 75, 150));
	paint_tiles_btn->SetForegroundColour(wxColour(240, 210, 120));
	step2_sizer->Add(paint_tiles_btn, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 8);

	tile_count_label = newd wxStaticText(step2_panel, wxID_ANY, "Tiles painted: 0", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
	tile_count_label->SetForegroundColour(wxColour(180, 190, 205));
	step2_sizer->Add(tile_count_label, 0, wxALIGN_CENTER_HORIZONTAL);

	step2_panel->SetSizer(step2_sizer);
	main_sizer->Add(step2_panel, 1, wxEXPAND | wxALL, 6);

	// =========================================================================
	// STEP 3 PANEL (Set Exit Position)
	// =========================================================================
	step3_panel = newd wxPanel(this, wxID_ANY);
	wxBoxSizer* step3_sizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticText* step3_lbl = newd wxStaticText(step3_panel, wxID_ANY, "Step 3: Set Exit Position", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
	step3_lbl->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	step3_lbl->SetForegroundColour(wxColour(240, 210, 120));
	step3_sizer->Add(step3_lbl, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 6);

	wxStaticText* exit_hint = newd wxStaticText(step3_panel, wxID_ANY,
		"Click the button below and then click on the map canvas\n"
		"to set the Exit position for this house (where players teleport when kicked).", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
	exit_hint->SetForegroundColour(wxColour(200, 210, 225));
	step3_sizer->Add(exit_hint, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 8);

	set_exit_btn = newd wxToggleButton(step3_panel, WIZARD_ID_SET_EXIT, "Set Exit", wxDefaultPosition, wxSize(130, 26));
	set_exit_btn->SetBackgroundColour(wxColour(35, 75, 150));
	set_exit_btn->SetForegroundColour(wxColour(240, 210, 120));
	step3_sizer->Add(set_exit_btn, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 8);

	exit_pos_label = newd wxStaticText(step3_panel, wxID_ANY, "Exit Position: Not Set", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
	exit_pos_label->SetForegroundColour(wxColour(180, 190, 205));
	step3_sizer->Add(exit_pos_label, 0, wxALIGN_CENTER_HORIZONTAL);

	step3_panel->SetSizer(step3_sizer);
	main_sizer->Add(step3_panel, 1, wxEXPAND | wxALL, 6);

	step2_panel->Hide();
	step3_panel->Hide();

	// Navigation Buttons
	wxBoxSizer* nav_sizer = newd wxBoxSizer(wxHORIZONTAL);
	back_btn = newd wxButton(this, WIZARD_ID_BACK, "< Back");
	next_btn = newd wxButton(this, WIZARD_ID_NEXT, "Next >");
	ok_btn = newd wxButton(this, wxID_OK, "OK");
	cancel_btn = newd wxButton(this, wxID_CANCEL, "Cancel");

	back_btn->SetBackgroundColour(wxColour(30, 45, 70));
	back_btn->SetForegroundColour(wxColour(200, 210, 225));
	next_btn->SetBackgroundColour(wxColour(35, 75, 150));
	next_btn->SetForegroundColour(wxColour(240, 210, 120));
	ok_btn->SetBackgroundColour(wxColour(35, 75, 150));
	ok_btn->SetForegroundColour(wxColour(240, 210, 120));
	cancel_btn->SetBackgroundColour(wxColour(30, 45, 70));
	cancel_btn->SetForegroundColour(wxColour(200, 210, 225));

	nav_sizer->Add(back_btn, 0, wxRIGHT, 6);
	nav_sizer->Add(next_btn, 0, wxRIGHT, 12);
	nav_sizer->Add(ok_btn, 0, wxRIGHT, 6);
	nav_sizer->Add(cancel_btn, 0);

	main_sizer->Add(nav_sizer, 0, wxALIGN_RIGHT | wxALL, 8);

	SetSizerAndFit(main_sizer);
	RME::UI::StyleManager::ApplyThemeRecursively(this, RME::UI::StyleManager::GetTheme());
	showStep(1);
	bindHoverEvents(this);
	Centre(wxBOTH);
}

HouseWizardDialog::~HouseWizardDialog() {
}

void HouseWizardDialog::makeSemiTransparent() {
	if (CanSetTransparent()) {
		SetTransparent(110); // Translucent so canvas underneath is clearly visible while drawing
	}
}

void HouseWizardDialog::makeOpaque() {
	if (CanSetTransparent()) {
		SetTransparent(255); // 100% Opaque when hovered or clicked
	}
}

void HouseWizardDialog::bindHoverEvents(wxWindow* win) {
	if (!win) return;
	win->Bind(wxEVT_ENTER_WINDOW, &HouseWizardDialog::OnMouseEnter, this);
	win->Bind(wxEVT_LEAVE_WINDOW, &HouseWizardDialog::OnMouseLeave, this);
	for (wxWindowList::compatibility_iterator node = win->GetChildren().GetFirst(); node; node = node->GetNext()) {
		bindHoverEvents(node->GetData());
	}
}

void HouseWizardDialog::showStep(int step) {
	current_step = step;

	step1_panel->Show(step == 1);
	step2_panel->Show(step == 2);
	step3_panel->Show(step == 3);

	if (step == 1) {
		makeOpaque();
		back_btn->Enable(false);
		next_btn->Show();
		ok_btn->Hide();
	} else if (step == 2) {
		makeSemiTransparent();
		if (g_gui.root) g_gui.root->Enable(true);
		back_btn->Enable(true);
		next_btn->Show();
		ok_btn->Hide();

		if (paint_tiles_btn) paint_tiles_btn->SetValue(true);

		if (draft_house && g_gui.house_brush) {
			g_gui.house_brush->setHouse(draft_house);
			g_gui.SelectBrush(g_gui.house_brush, TILESET_HOUSE);
		}

		if (tile_count_label && draft_house) {
			tile_count_label->SetLabel(wxString::Format("Tiles painted: %zu", draft_house->size()));
		}
	} else if (step == 3) {
		makeSemiTransparent();
		if (g_gui.root) g_gui.root->Enable(true);
		back_btn->Enable(true);
		next_btn->Hide();
		ok_btn->Show();

		if (set_exit_btn) set_exit_btn->SetValue(true);

		if (draft_house && g_gui.house_exit_brush) {
			g_gui.house_exit_brush->setHouse(draft_house);
			g_gui.SelectBrush(g_gui.house_exit_brush, TILESET_HOUSE);
		}

		updateExitText();
	}

	Layout();
	Fit();
}

void HouseWizardDialog::updateExitText() {
	if (!exit_pos_label || !draft_house) return;
	Position exit_pos = draft_house->getExit();
	if (exit_pos.isValid() && exit_pos != Position(0, 0, 0)) {
		exit_pos_label->SetLabel(wxString::Format("Exit Position: (%d, %d, %d)", exit_pos.x, exit_pos.y, exit_pos.z));
	} else {
		exit_pos_label->SetLabel("Exit Position: Not Set");
	}
}

void HouseWizardDialog::OnClickNext(wxCommandEvent& evt) {
	if (current_step == 1) {
		wxString name = name_field->GetValue().Trim().Trim(false);
		if (name.IsEmpty()) {
			g_gui.PopupDialog(this, "Error", "House name cannot be empty.", wxOK);
			return;
		}
		draft_house->name = nstr(name);

		long rent_val = 0;
		rent_field->GetValue().ToLong(&rent_val);
		draft_house->rent = static_cast<int>(rent_val);
		draft_house->guildhall = guildhall_checkbox->IsChecked();

		if (town_choice && town_choice->GetSelection() != wxNOT_FOUND) {
			uint32_t sel_town_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(town_choice->GetClientData(town_choice->GetSelection())));
			draft_house->townid = sel_town_id;
		} else {
			draft_house->townid = 0;
		}

		showStep(2);
	} else if (current_step == 2) {
		showStep(3);
	}
}

void HouseWizardDialog::OnClickBack(wxCommandEvent& evt) {
	if (current_step > 1) {
		showStep(current_step - 1);
	}
}

void HouseWizardDialog::OnClickPaintTiles(wxCommandEvent& evt) {
	if (paint_tiles_btn && draft_house) {
		bool active = paint_tiles_btn->GetValue();
		if (active) {
			if (g_gui.house_brush) {
				g_gui.house_brush->setHouse(draft_house);
				g_gui.SelectBrush(g_gui.house_brush, TILESET_HOUSE);
			}
			if (g_gui.root) g_gui.root->Enable(true);
			makeSemiTransparent();
		} else {
			makeOpaque();
		}
	}
}

void HouseWizardDialog::OnClickSetExit(wxCommandEvent& evt) {
	if (set_exit_btn && draft_house) {
		bool active = set_exit_btn->GetValue();
		if (active) {
			if (g_gui.house_exit_brush) {
				g_gui.house_exit_brush->setHouse(draft_house);
				g_gui.SelectBrush(g_gui.house_exit_brush, TILESET_HOUSE);
			}
			if (g_gui.root) g_gui.root->Enable(true);
			makeSemiTransparent();
		} else {
			makeOpaque();
		}
	}
}

void HouseWizardDialog::OnMouseEnter(wxMouseEvent& evt) {
	makeOpaque();
	if (current_step == 2 && tile_count_label && draft_house) {
		tile_count_label->SetLabel(wxString::Format("Tiles painted: %zu", draft_house->size()));
	} else if (current_step == 3) {
		updateExitText();
	}
	evt.Skip();
}

void HouseWizardDialog::OnMouseLeave(wxMouseEvent& evt) {
	if (current_step == 2 || current_step == 3) {
		makeSemiTransparent();
	}
	evt.Skip();
}

void HouseWizardDialog::OnActivate(wxActivateEvent& evt) {
	if (evt.GetActive()) {
		makeOpaque();
		if (current_step == 2 && tile_count_label && draft_house) {
			tile_count_label->SetLabel(wxString::Format("Tiles painted: %zu", draft_house->size()));
		} else if (current_step == 3) {
			updateExitText();
		}
	}
	evt.Skip();
}

void HouseWizardDialog::OnIconize(wxIconizeEvent& evt) {
	if (!evt.IsIconized()) {
		makeOpaque();
		if (current_step == 2 && tile_count_label && draft_house) {
			tile_count_label->SetLabel(wxString::Format("Tiles painted: %zu", draft_house->size()));
		} else if (current_step == 3) {
			updateExitText();
		}
	}
	evt.Skip();
}

void HouseWizardDialog::OnClickOK(wxCommandEvent& evt) {
	wxString name = name_field->GetValue().Trim().Trim(false);
	if (name.IsEmpty()) {
		g_gui.PopupDialog(this, "Error", "House name cannot be empty.", wxOK);
		return;
	}

	long rent_val = 0;
	if (!rent_field->GetValue().ToLong(&rent_val) || rent_val < 0) {
		g_gui.PopupDialog(this, "Error", "House rent must be a non-negative number.", wxOK);
		return;
	}

	draft_house->name = nstr(name);
	draft_house->rent = static_cast<int>(rent_val);
	draft_house->guildhall = guildhall_checkbox->IsChecked();

	if (town_choice && town_choice->GetSelection() != wxNOT_FOUND) {
		uint32_t sel_town_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(town_choice->GetClientData(town_choice->GetSelection())));
		draft_house->townid = sel_town_id;
	} else {
		draft_house->townid = 0;
	}

	EndModal(wxID_OK);
}

void HouseWizardDialog::cancelWizard() {
	if (!is_editing && draft_house && map) {
		map->houses.removeHouse(draft_house);
		draft_house = nullptr;
	}
}

void HouseWizardDialog::OnClickCancel(wxCommandEvent& evt) {
	cancelWizard();
	EndModal(wxID_CANCEL);
}
