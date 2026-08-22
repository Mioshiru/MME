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

#include "properties_window.h"
#include "tile.h"
#include "map.h"
#include "town.h"
#include "editor.h"
#include "gui.h"
#include "style_manager.h"

#include "gui_ids.h"
#include "complexitem.h"
#include <wx/wrapsizer.h>
#include "container_properties_window.h"

#include <wx/grid.h>
#include <wx/textctrl.h>
#include <wx/msgdlg.h>

BEGIN_EVENT_TABLE(PropertiesWindow, wxDialog)
EVT_BUTTON(wxID_OK, PropertiesWindow::OnClickOK)
EVT_BUTTON(wxID_CANCEL, PropertiesWindow::OnClickCancel)
EVT_CLOSE(PropertiesWindow::OnClose)

EVT_BUTTON(ITEM_PROPERTIES_ADD_ATTRIBUTE, PropertiesWindow::OnClickAddAttribute)
EVT_BUTTON(ITEM_PROPERTIES_REMOVE_ATTRIBUTE, PropertiesWindow::OnClickRemoveAttribute)
EVT_BUTTON(ITEM_PROPERTIES_TOWN_BTN, PropertiesWindow::OnClickTown)

EVT_NOTEBOOK_PAGE_CHANGED(wxID_ANY, PropertiesWindow::OnNotebookPageChanged)

EVT_GRID_CELL_CHANGED(PropertiesWindow::OnGridValueChanged)
END_EVENT_TABLE()

PropertiesWindow::PropertiesWindow(wxWindow* parent, const Map* map, const Tile* tile_parent, Item* item, wxPoint pos) :
	ObjectPropertiesWindowBase(parent, "Item Properties", map, tile_parent, item, pos),
	action_id_field(nullptr),
	unique_id_field(nullptr),
	count_field(nullptr),
	text_field(nullptr),
	depot_town_field(nullptr),
	waypoint_name_field(nullptr),
	currentPanel(nullptr) {
	ASSERT(edit_item);

	SetBackgroundColour(wxColour(10, 20, 35));
	wxSizer* topSizer = newd wxBoxSizer(wxVERTICAL);

	// Header Banner Panel
	wxPanel* headerPanel = newd wxPanel(this, wxID_ANY);
	headerPanel->SetBackgroundColour(wxColour(16, 28, 48));
	wxBoxSizer* headerSizer = newd wxBoxSizer(wxVERTICAL);

	wxString itemTitle = wxString::Format("Item #%d: %s", edit_item->getID(), wxstr(edit_item->getName()));
	wxStaticText* header = newd wxStaticText(headerPanel, wxID_ANY, itemTitle);
	header->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	header->SetForegroundColour(wxColour(180, 150, 50));

	wxStaticText* subheader = newd wxStaticText(headerPanel, wxID_ANY, "Configure item attributes, action/unique IDs, container items, and waypoint settings.");
	subheader->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	subheader->SetForegroundColour(wxColour(180, 190, 205));

	headerSizer->Add(header, 0, wxBOTTOM, 4);
	headerSizer->Add(subheader, 0);
	headerPanel->SetSizer(headerSizer);

	topSizer->Add(headerPanel, 0, wxEXPAND | wxALL, 12);

	notebook = newd wxNotebook(this, wxID_ANY, wxDefaultPosition, wxSize(600, 340));
	notebook->SetBackgroundColour(wxColour(16, 28, 48));
	notebook->SetForegroundColour(wxColour(240, 245, 255));

	notebook->AddPage(createGeneralPanel(notebook), "General", true);
	if (dynamic_cast<Container*>(item)) {
		notebook->AddPage(createContainerPanel(notebook), "Special");
	} else if (item->isDoor()) {
		notebook->AddPage(createDoorSpecialPanel(notebook), "Special");
	} else if (dynamic_cast<Teleport*>(item)) {
		notebook->AddPage(createTeleportSpecialPanel(notebook), "Special");
	}
	notebook->AddPage(createAttributesPanel(notebook), "Attributes");
	if (edit_tile) {
		notebook->AddPage(createWaypointPanel(notebook), "Waypoint");
	}

	topSizer->Add(notebook, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

	// Action Buttons
	wxBoxSizer* optSizer = newd wxBoxSizer(wxHORIZONTAL);
	wxButton* okBtn = newd wxButton(this, wxID_OK, "OK");
	wxButton* cancelBtn = newd wxButton(this, wxID_CANCEL, "Cancel");

	okBtn->SetBackgroundColour(wxColour(35, 75, 150));
	okBtn->SetForegroundColour(wxColour(240, 210, 120));
	okBtn->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));

	cancelBtn->SetBackgroundColour(wxColour(22, 36, 58));
	cancelBtn->SetForegroundColour(wxColour(180, 190, 205));

	optSizer->Add(okBtn, 0, wxRIGHT, 8);
	optSizer->Add(cancelBtn, 0);

	topSizer->Add(optSizer, 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT | wxBOTTOM, 12);

	SetSizerAndFit(topSizer);
	Centre(wxBOTH);

	RME::UI::StyleManager::ApplyThemeRecursively(this, RME::UI::StyleManager::GetTheme());
}

PropertiesWindow::~PropertiesWindow() {
	;
}

void PropertiesWindow::Update() {
	Container* container = dynamic_cast<Container*>(edit_item);
	if (container) {
		for (size_t i = 0; i < container_items.size() && i < container->getVolume(); ++i) {
			if (container_items[i]) {
				container_items[i]->setItem(container->getItem(i));
			}
		}
	}
	wxDialog::Update();
}

wxWindow* PropertiesWindow::createGeneralPanel(wxWindow* parent) {
	wxPanel* panel = newd wxPanel(parent, ITEM_PROPERTIES_GENERAL_TAB);
	panel->SetBackgroundColour(wxColour(16, 28, 48));

	wxFlexGridSizer* gridsizer = newd wxFlexGridSizer(2, 10, 10);
	gridsizer->AddGrowableCol(1);

	auto addLabel = [panel, gridsizer](const wxString& labelText) {
		wxStaticText* label = newd wxStaticText(panel, wxID_ANY, labelText);
		label->SetForegroundColour(wxColour(180, 190, 205));
		label->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
		gridsizer->Add(label, 0, wxALIGN_CENTER_VERTICAL);
	};

	auto styleSpin = [](wxSpinCtrl* ctrl) {
		ctrl->SetBackgroundColour(wxColour(16, 28, 48));
		ctrl->SetForegroundColour(wxColour(240, 245, 255));
	};

	auto styleText = [](wxTextCtrl* ctrl) {
		ctrl->SetBackgroundColour(wxColour(16, 28, 48));
		ctrl->SetForegroundColour(wxColour(240, 245, 255));
	};

	addLabel("Item ID:");
	wxStaticText* idText = newd wxStaticText(panel, wxID_ANY, i2ws(edit_item->getID()) + " (\"" + wxstr(edit_item->getName()) + "\")");
	idText->SetForegroundColour(wxColour(180, 150, 50));
	idText->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	gridsizer->Add(idText, 0, wxALIGN_CENTER_VERTICAL);

	addLabel("Action ID:");
	action_id_field = newd wxSpinCtrl(panel, wxID_ANY, i2ws(edit_item->getActionID()), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 0xFFFF, edit_item->getActionID());
	styleSpin(action_id_field);
	gridsizer->Add(action_id_field, wxSizerFlags(1).Expand());

	addLabel("Unique ID:");
	unique_id_field = newd wxSpinCtrl(panel, wxID_ANY, i2ws(edit_item->getUniqueID()), wxDefaultPosition, wxSize(-1, 24), wxSP_ARROW_KEYS, 0, 0xFFFF, edit_item->getUniqueID());
	styleSpin(unique_id_field);
	gridsizer->Add(unique_id_field, wxSizerFlags(1).Expand());

	if (edit_item->isDoor()) {
		addLabel("Locked Door:");
		locked_door_checkbox = newd wxCheckBox(panel, wxID_ANY, "Locked (Action ID 100)");
		locked_door_checkbox->SetForegroundColour(wxColour(240, 245, 255));
		locked_door_checkbox->SetValue(edit_item->getActionID() == 100);
		
		locked_door_checkbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& evt) {
			if (evt.IsChecked()) {
				action_id_field->SetValue(100);
			} else if (action_id_field->GetValue() == 100) {
				action_id_field->SetValue(0);
			}
		});

		action_id_field->Bind(wxEVT_SPINCTRL, [this](wxCommandEvent& evt) {
			if (locked_door_checkbox) {
				locked_door_checkbox->SetValue(action_id_field->GetValue() == 100);
			}
		});

		gridsizer->Add(locked_door_checkbox, wxSizerFlags(1).Expand());
	} else {
		locked_door_checkbox = nullptr;
	}

	if (edit_item->isStackable() || edit_item->isCharged() || edit_item->isFluidContainer() || edit_item->isSplash()) {
		int max_count = 100;
		if (edit_item->isFluidContainer() || edit_item->isSplash()) {
			max_count = 250;
		} else if (edit_item->isCharged()) {
			max_count = 65500;
		}
		addLabel("Count/Subtype:");
		count_field = newd wxSpinCtrl(panel, wxID_ANY, i2ws(edit_item->getCount()), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, max_count, edit_item->getCount());
		styleSpin(count_field);
		if (edit_item->isHangable()) {
			count_field->Enable(false);
		}
		gridsizer->Add(count_field, wxSizerFlags(1).Expand());
	} else {
		count_field = nullptr;
	}

	if (edit_item->canHoldText() || edit_item->canHoldDescription()) {
		addLabel("Text Inscription:");
		text_field = newd wxTextCtrl(panel, wxID_ANY, wxstr(edit_item->getText()), wxDefaultPosition, wxSize(-1, 80), wxTE_MULTILINE);
		styleText(text_field);
		gridsizer->Add(text_field, wxSizerFlags(1).Expand());
	} else {
		text_field = nullptr;
	}

	if (Depot* depot = dynamic_cast<Depot*>(edit_item)) {
		addLabel("Depot Town:");
		depot_town_field = newd wxChoice(panel, wxID_ANY);
		depot_town_field->SetBackgroundColour(wxColour(16, 28, 48));
		depot_town_field->SetForegroundColour(wxColour(240, 245, 255));

		uint16_t cur_town_id = depot->getDepotID();
		if (cur_town_id == 0 && edit_map && edit_map->towns.count() > 0) {
			Position tile_pos = edit_tile ? edit_tile->getPosition() : Position();
			uint32_t nearest_town_id = 0;
			int min_dist = std::numeric_limits<int>::max();
			for (const auto& pair : edit_map->towns) {
				const Town* town = pair.second;
				if (town) {
					const Position& temple = town->getTemplePosition();
					if (temple.isValid() && tile_pos.isValid()) {
						int dist = std::abs((int)tile_pos.x - (int)temple.x) + std::abs((int)tile_pos.y - (int)temple.y) + std::abs((int)tile_pos.z - (int)temple.z) * 5;
						if (dist < min_dist) {
							min_dist = dist;
							nearest_town_id = town->getID();
						}
					} else if (nearest_town_id == 0) {
						nearest_town_id = town->getID();
					}
				}
			}
			if (nearest_town_id == 0 && edit_map->towns.count() > 0) {
				nearest_town_id = edit_map->towns.begin()->second->getID();
			}
			if (nearest_town_id > 0) {
				cur_town_id = static_cast<uint16_t>(nearest_town_id);
			}
		}

		int to_select_index = 0;
		if (edit_map) {
			for (const auto& pair : edit_map->towns) {
				const Town* town = pair.second;
				if (town) {
					if (town->getID() == cur_town_id) {
						to_select_index = depot_town_field->GetCount();
					}
					depot_town_field->Append(wxstr(town->getName()), reinterpret_cast<void*>(static_cast<uintptr_t>(town->getID())));
				}
			}
			if (to_select_index == 0 && cur_town_id != 0) {
				depot_town_field->Append("Undefined Town (id:" + i2ws(cur_town_id) + ")", reinterpret_cast<void*>(static_cast<uintptr_t>(cur_town_id)));
			}
		}
		depot_town_field->Append("No Town", reinterpret_cast<void*>(static_cast<uintptr_t>(0)));
		if (cur_town_id == 0) {
			to_select_index = depot_town_field->GetCount() - 1;
		}
		depot_town_field->SetSelection(to_select_index);
		gridsizer->Add(depot_town_field, wxSizerFlags(1).Expand());
	} else {
		depot_town_field = nullptr;
	}

	if (edit_item->isGroundTile() && !edit_item->hasProperty(BLOCKSOLID)) {
		Town* clicked_town = nullptr;
		if (edit_tile && edit_map) {
			Position click_pos = edit_tile->getPosition();
			for (const auto& pair : edit_map->towns) {
				if (pair.second->getTemplePosition() == click_pos) {
					clicked_town = pair.second;
					break;
				}
			}
		}

		wxString town_text_val = "None";
		if (clicked_town) {
			town_text_val = wxString::Format("%d - %s", clicked_town->getID(), wxstr(clicked_town->getName()));
		}

		addLabel("Town Temple:");
		wxSizer* town_sizer = newd wxBoxSizer(wxHORIZONTAL);
		wxStaticText* town_lbl = newd wxStaticText(panel, wxID_ANY, town_text_val);
		town_lbl->SetForegroundColour(wxColour(248, 250, 252));
		town_sizer->Add(town_lbl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
		wxButton* town_btn = newd wxButton(panel, ITEM_PROPERTIES_TOWN_BTN, "Edit Towns...");
		town_btn->SetBackgroundColour(wxColour(51, 65, 85));
		town_btn->SetForegroundColour(wxColour(203, 213, 225));
		town_sizer->Add(town_btn, 0, wxALIGN_CENTER_VERTICAL);

		gridsizer->Add(town_sizer, wxSizerFlags(0));
	}

	panel->SetSizerAndFit(gridsizer);
	return panel;
}

wxWindow* PropertiesWindow::createContainerPanel(wxWindow* parent) {
	Container* container = (Container*)edit_item;
	wxPanel* panel = newd wxPanel(parent, ITEM_PROPERTIES_CONTAINER_TAB);
	panel->SetBackgroundColour(wxColour(16, 28, 48));
	wxSizer* topSizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticText* headerNote = newd wxStaticText(panel, wxID_ANY, "Chest & Container Special Configuration");
	headerNote->SetForegroundColour(wxColour(255, 215, 0));
	headerNote->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	topSizer->Add(headerNote, 0, wxLEFT | wxRIGHT | wxTOP, 8);

	wxStaticText* subNote = newd wxStaticText(panel, wxID_ANY, "Right-click any slot to Add, Edit, or Delete items.");
	subNote->SetForegroundColour(wxColour(180, 190, 205));
	subNote->SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	topSizer->Add(subNote, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);

	wxBoxSizer* modeRow = newd wxBoxSizer(wxHORIZONTAL);
	wxStaticText* modeLbl = newd wxStaticText(panel, wxID_ANY, "Chest Mode:");
	modeLbl->SetForegroundColour(wxColour(180, 190, 205));
	modeLbl->SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	modeRow->Add(modeLbl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	wxArrayString modes;
	modes.Add("Casual (Standard Container / Session Loot)");
	modes.Add("Quest Chest (Per-Player Permanent Reward)");
	chest_mode_choice = newd wxChoice(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, modes);
	modeRow->Add(chest_mode_choice, 1, wxEXPAND);
	topSizer->Add(modeRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

	// Quest configuration panel (shown only when Quest Chest is selected)
	chest_quest_panel = newd wxPanel(panel, wxID_ANY);
	chest_quest_panel->SetBackgroundColour(wxColour(16, 28, 48));
	wxFlexGridSizer* fgrid = newd wxFlexGridSizer(2, 4, 8);
	fgrid->AddGrowableCol(1);

	auto addLbl = [this, fgrid](const wxString& text) {
		wxStaticText* lbl = newd wxStaticText(chest_quest_panel, wxID_ANY, text);
		lbl->SetForegroundColour(wxColour(180, 190, 205));
		lbl->SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
		fgrid->Add(lbl, 0, wxALIGN_CENTER_VERTICAL);
	};

	addLbl("Required Min Level:");
	chest_req_level = newd wxSpinCtrl(chest_quest_panel, wxID_ANY, "1", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 10000, 1);
	fgrid->Add(chest_req_level, wxSizerFlags(1).Expand());

	addLbl("Storage Key (ActionID):");
	chest_action_id = newd wxSpinCtrl(chest_quest_panel, wxID_ANY, i2ws(edit_item->getActionID()), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 65535, edit_item->getActionID());
	fgrid->Add(chest_action_id, wxSizerFlags(1).Expand());

	addLbl("Reward Message:");
	chest_reward_msg = newd wxTextCtrl(chest_quest_panel, wxID_ANY, "You have found a reward.");
	fgrid->Add(chest_reward_msg, wxSizerFlags(1).Expand());

	chest_quest_panel->SetSizer(fgrid);
	topSizer->Add(chest_quest_panel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

	bool isQuest = (edit_item->getActionID() >= 2000);
	chest_mode_choice->SetSelection(isQuest ? 1 : 0);
	chest_quest_panel->Show(isQuest);

	chest_mode_choice->Bind(wxEVT_CHOICE, [this, panel, topSizer](wxCommandEvent&) {
		bool qMode = (chest_mode_choice->GetSelection() == 1);
		chest_quest_panel->Show(qMode);
		if (qMode && chest_action_id->GetValue() < 2000) {
			chest_action_id->SetValue(2001);
		}
		topSizer->Layout();
		panel->Layout();
		Layout();
	});

	wxSizer* gridSizer = newd wxWrapSizer(wxHORIZONTAL);
	bool use_large_sprites = g_settings.getBoolean(Config::USE_LARGE_CONTAINER_ICONS);
	for (uint32_t i = 0; i < container->getVolume(); ++i) {
		Item* item = container->getItem(i);
		ContainerItemButton* containerItemButton = newd ContainerItemButton(panel, use_large_sprites, i, edit_map, item);

		container_items.push_back(containerItemButton);
		gridSizer->Add(containerItemButton, wxSizerFlags(0));
	}

	topSizer->Add(gridSizer, wxSizerFlags(1).Expand().Border(wxALL, 4));
	panel->SetSizer(topSizer);
	return panel;
}

void PropertiesWindow::saveContainerSpecialPanel() {
	if (!chest_mode_choice) return;
	if (chest_mode_choice->GetSelection() == 1) {
		if (chest_action_id && chest_action_id->GetValue() > 0) {
			edit_item->setActionID(chest_action_id->GetValue());
		}
		if (chest_reward_msg && !chest_reward_msg->GetValue().empty()) {
			std::string msg = chest_reward_msg->GetValue().ToStdString();
			edit_item->setText(msg);
		}
	} else {
		// Casual mode: clean chest without quest action ID if previously >= 2000
		if (edit_item->getActionID() >= 2000) {
			edit_item->setActionID(0);
		}
	}
}

wxWindow* PropertiesWindow::createDoorSpecialPanel(wxWindow* parent) {
	wxPanel* panel = newd wxPanel(parent, wxID_ANY);
	panel->SetBackgroundColour(wxColour(16, 28, 48));

	wxBoxSizer* mainSizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticText* hNote = newd wxStaticText(panel, wxID_ANY, "Quest & Level Door Configuration");
	hNote->SetForegroundColour(wxColour(255, 215, 0));
	hNote->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	mainSizer->Add(hNote, 0, wxALL, 8);

	wxFlexGridSizer* fgrid = newd wxFlexGridSizer(2, 10, 10);
	fgrid->AddGrowableCol(1);

	auto addLbl = [panel, fgrid](const wxString& text) {
		wxStaticText* lbl = newd wxStaticText(panel, wxID_ANY, text);
		lbl->SetForegroundColour(wxColour(180, 190, 205));
		lbl->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
		fgrid->Add(lbl, 0, wxALIGN_CENTER_VERTICAL);
	};

	addLbl("Door Function Type:");
	wxArrayString doorTypes;
	doorTypes.Add("Standard Door");
	doorTypes.Add("Locked Door (Key / ActionID 100)");
	doorTypes.Add("Quest Door (Storage Key Check)");
	doorTypes.Add("Level Door (Required Min Level)");

	door_type_choice = newd wxChoice(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, doorTypes);
	fgrid->Add(door_type_choice, wxSizerFlags(1).Expand());

	addLbl("Required Level:");
	door_req_level = newd wxSpinCtrl(panel, wxID_ANY, "1", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 10000, 1);
	fgrid->Add(door_req_level, wxSizerFlags(1).Expand());

	addLbl("Door Action ID:");
	door_action_id = newd wxSpinCtrl(panel, wxID_ANY, i2ws(edit_item->getActionID()), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 65535, edit_item->getActionID());
	fgrid->Add(door_action_id, wxSizerFlags(1).Expand());

	addLbl("Required Storage Key:");
	door_storage_key = newd wxSpinCtrl(panel, wxID_ANY, "50000", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 999999, 50000);
	fgrid->Add(door_storage_key, wxSizerFlags(1).Expand());

	// Initial selection based on current ActionID
	int curAid = edit_item->getActionID();
	if (curAid == 100) {
		door_type_choice->SetSelection(1); // Locked
	} else if (curAid >= 1000 && curAid < 2000) {
		door_type_choice->SetSelection(3); // Level door
		door_req_level->SetValue(curAid - 1000);
	} else if (curAid >= 2000) {
		door_type_choice->SetSelection(2); // Quest door
	} else {
		door_type_choice->SetSelection(0);
	}

	door_type_choice->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
		int sel = door_type_choice->GetSelection();
		if (sel == 1) { // Locked
			door_action_id->SetValue(100);
		} else if (sel == 2) { // Quest door
			if (door_action_id->GetValue() < 2000) door_action_id->SetValue(2001);
		} else if (sel == 3) { // Level door
			door_action_id->SetValue(1000 + door_req_level->GetValue());
		}
	});

	door_req_level->Bind(wxEVT_SPINCTRL, [this](wxCommandEvent&) {
		if (door_type_choice->GetSelection() == 3) {
			door_action_id->SetValue(1000 + door_req_level->GetValue());
		}
	});

	mainSizer->Add(fgrid, 1, wxEXPAND | wxALL, 8);
	panel->SetSizerAndFit(mainSizer);
	return panel;
}

void PropertiesWindow::saveDoorSpecialPanel() {
	if (!door_type_choice) return;
	int sel = door_type_choice->GetSelection();
	if (sel == 1) {
		edit_item->setActionID(100);
	} else if (sel == 3) {
		edit_item->setActionID(1000 + door_req_level->GetValue());
	} else if (door_action_id) {
		edit_item->setActionID(door_action_id->GetValue());
	}
}

wxWindow* PropertiesWindow::createTeleportSpecialPanel(wxWindow* parent) {
	wxPanel* panel = newd wxPanel(parent, wxID_ANY);
	panel->SetBackgroundColour(wxColour(16, 28, 48));

	wxBoxSizer* mainSizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticText* hNote = newd wxStaticText(panel, wxID_ANY, "Teleporters & Switches Configuration");
	hNote->SetForegroundColour(wxColour(255, 215, 0));
	hNote->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	mainSizer->Add(hNote, 0, wxALL, 8);

	wxFlexGridSizer* fgrid = newd wxFlexGridSizer(2, 10, 10);
	fgrid->AddGrowableCol(1);

	auto addLbl = [panel, fgrid](const wxString& text) {
		wxStaticText* lbl = newd wxStaticText(panel, wxID_ANY, text);
		lbl->SetForegroundColour(wxColour(180, 190, 205));
		lbl->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
		fgrid->Add(lbl, 0, wxALIGN_CENTER_VERTICAL);
	};

	Teleport* tele = dynamic_cast<Teleport*>(edit_item);
	Position dest = tele ? tele->getDestination() : Position(1000, 1000, 7);

	addLbl("Target Destination X:");
	tele_dest_x = newd wxSpinCtrl(panel, wxID_ANY, i2ws(dest.x), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 65535, dest.x);
	fgrid->Add(tele_dest_x, wxSizerFlags(1).Expand());

	addLbl("Target Destination Y:");
	tele_dest_y = newd wxSpinCtrl(panel, wxID_ANY, i2ws(dest.y), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 65535, dest.y);
	fgrid->Add(tele_dest_y, wxSizerFlags(1).Expand());

	addLbl("Target Destination Z (Floor):");
	tele_dest_z = newd wxSpinCtrl(panel, wxID_ANY, i2ws(dest.z), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 15, dest.z);
	fgrid->Add(tele_dest_z, wxSizerFlags(1).Expand());

	mainSizer->Add(fgrid, 1, wxEXPAND | wxALL, 8);
	panel->SetSizerAndFit(mainSizer);
	return panel;
}

void PropertiesWindow::saveTeleportSpecialPanel() {
	Teleport* tele = dynamic_cast<Teleport*>(edit_item);
	if (tele && tele_dest_x && tele_dest_y && tele_dest_z) {
		tele->setDestination(Position(tele_dest_x->GetValue(), tele_dest_y->GetValue(), tele_dest_z->GetValue()));
	}
}

wxWindow* PropertiesWindow::createAttributesPanel(wxWindow* parent) {
	wxPanel* panel = newd wxPanel(parent, wxID_ANY);
	wxSizer* topSizer = newd wxBoxSizer(wxVERTICAL);

	attributesGrid = newd wxGrid(panel, ITEM_PROPERTIES_ADVANCED_TAB, wxDefaultPosition, wxSize(-1, 160));
	topSizer->Add(attributesGrid, wxSizerFlags(1).Expand());

	wxFont time_font(*wxSWISS_FONT);
	attributesGrid->SetDefaultCellFont(time_font);
	attributesGrid->CreateGrid(0, 3);
	attributesGrid->DisableDragRowSize();
	attributesGrid->DisableDragColSize();
	attributesGrid->SetSelectionMode(wxGrid::wxGridSelectRows);
	attributesGrid->SetRowLabelSize(0);
	// log->SetColLabelSize(0);
	// log->EnableGridLines(false);
	attributesGrid->EnableEditing(true);

	attributesGrid->SetColLabelValue(0, "Key");
	attributesGrid->SetColSize(0, 100);
	attributesGrid->SetColLabelValue(1, "Type");
	attributesGrid->SetColSize(1, 80);
	attributesGrid->SetColLabelValue(2, "Value");
	attributesGrid->SetColSize(2, 410);

	// contents
	ItemAttributeMap attrs = edit_item->getAttributes();
	attributesGrid->AppendRows(attrs.size());
	int i = 0;
	for (ItemAttributeMap::iterator aiter = attrs.begin(); aiter != attrs.end(); ++aiter, ++i) {
		SetGridValue(attributesGrid, i, aiter->first, aiter->second);
	}

	wxSizer* optSizer = newd wxBoxSizer(wxHORIZONTAL);
	optSizer->Add(newd wxButton(panel, ITEM_PROPERTIES_ADD_ATTRIBUTE, "Add Attribute"), wxSizerFlags(0).Center());
	optSizer->Add(newd wxButton(panel, ITEM_PROPERTIES_REMOVE_ATTRIBUTE, "Remove Attribute"), wxSizerFlags(0).Center());
	topSizer->Add(optSizer, wxSizerFlags(0).Center().DoubleBorder());

	panel->SetSizer(topSizer);

	return panel;
}

void PropertiesWindow::SetGridValue(wxGrid* grid, int rowIndex, std::string label, const ItemAttribute& attr) {
	wxArrayString types;
	types.Add("Number");
	types.Add("Float");
	types.Add("Boolean");
	types.Add("String");

	grid->SetCellValue(rowIndex, 0, label);
	switch (attr.type) {
		case ItemAttribute::STRING: {
			grid->SetCellValue(rowIndex, 1, "String");
			grid->SetCellValue(rowIndex, 2, wxstr(*attr.getString()));
			break;
		}
		case ItemAttribute::INTEGER: {
			grid->SetCellValue(rowIndex, 1, "Number");
			grid->SetCellValue(rowIndex, 2, i2ws(*attr.getInteger()));
			grid->SetCellEditor(rowIndex, 2, new wxGridCellNumberEditor);
			break;
		}
		case ItemAttribute::DOUBLE:
		case ItemAttribute::FLOAT: {
			grid->SetCellValue(rowIndex, 1, "Float");
			wxString f;
			f << *attr.getFloat();
			grid->SetCellValue(rowIndex, 2, f);
			grid->SetCellEditor(rowIndex, 2, new wxGridCellFloatEditor);
			break;
		}
		case ItemAttribute::BOOLEAN: {
			grid->SetCellValue(rowIndex, 1, "Boolean");
			grid->SetCellValue(rowIndex, 2, *attr.getBoolean() ? "1" : "");
			grid->SetCellRenderer(rowIndex, 2, new wxGridCellBoolRenderer);
			grid->SetCellEditor(rowIndex, 2, new wxGridCellBoolEditor);
			break;
		}
		default: {
			grid->SetCellValue(rowIndex, 1, "Unknown");
			grid->SetCellBackgroundColour(rowIndex, 1, *wxLIGHT_GREY);
			grid->SetCellBackgroundColour(rowIndex, 2, *wxLIGHT_GREY);
			grid->SetReadOnly(rowIndex, 1, true);
			grid->SetReadOnly(rowIndex, 2, true);
			break;
		}
	}
	grid->SetCellEditor(rowIndex, 1, new wxGridCellChoiceEditor(types));
}

void PropertiesWindow::OnResize(wxSizeEvent& evt) {
	/*
	if(wxGrid* grid = (wxGrid*)currentPanel->FindWindowByName("AdvancedGrid")) {
		int tWidth = 0;
		for(int i = 0; i < 3; ++i)
			tWidth += grid->GetColumnWidth(i);

		int wWidth = grid->GetParent()->GetSize().GetWidth();

		grid->SetColumnWidth(2, wWidth - 100 - 80);
	}
	*/
}

void PropertiesWindow::OnNotebookPageChanged(wxNotebookEvent& evt) {
	wxWindow* page = notebook->GetCurrentPage();

	// TODO: Save

	switch (page->GetId()) {
		case ITEM_PROPERTIES_GENERAL_TAB: {
			// currentPanel = createGeneralPanel(page);
			break;
		}
		case ITEM_PROPERTIES_ADVANCED_TAB: {
			// currentPanel = createAttributesPanel(page);
			break;
		}
		default:
			break;
	}
}

void PropertiesWindow::saveGeneralPanel() {
	if (locked_door_checkbox && locked_door_checkbox->IsChecked()) {
		edit_item->setActionID(100);
	} else if (action_id_field) {
		edit_item->setActionID(action_id_field->GetValue());
	}
	if (unique_id_field) {
		edit_item->setUniqueID(unique_id_field->GetValue());
	}
	if (count_field && (edit_item->isStackable() || edit_item->isCharged() || edit_item->isFluidContainer() || edit_item->isSplash())) {
		edit_item->setSubtype(count_field->GetValue());
	}
	if (text_field && (edit_item->canHoldText() || edit_item->canHoldDescription())) {
		edit_item->setText(nstr(text_field->GetValue()));
	}
	if (depot_town_field) {
		Depot* depot = dynamic_cast<Depot*>(edit_item);
		if (depot && depot_town_field->GetSelection() != wxNOT_FOUND) {
			uint16_t selected_town_id = static_cast<uint16_t>(reinterpret_cast<uintptr_t>(depot_town_field->GetClientData(depot_town_field->GetSelection())));
			depot->setDepotID(selected_town_id);
		}
	}
}

void PropertiesWindow::saveContainerPanel() {
	////
}

void PropertiesWindow::saveAttributesPanel() {
	if (!attributesGrid) return;
	// Preserve general panel attributes before clearing
	std::string text_val = edit_item->getText();
	uint16_t aid_val = edit_item->getActionID();
	uint16_t uid_val = edit_item->getUniqueID();

	edit_item->clearAllAttributes();

	if (!text_val.empty()) edit_item->setText(text_val);
	if (aid_val > 0) edit_item->setActionID(aid_val);
	if (uid_val > 0) edit_item->setUniqueID(uid_val);

	for (int32_t rowIndex = 0; rowIndex < attributesGrid->GetNumberRows(); ++rowIndex) {
		ItemAttribute attr;
		wxString type = attributesGrid->GetCellValue(rowIndex, 1);
		if (type == "String") {
			attr.set(nstr(attributesGrid->GetCellValue(rowIndex, 2)));
		} else if (type == "Float") {
			double value;
			if (attributesGrid->GetCellValue(rowIndex, 2).ToDouble(&value)) {
				attr.set(value);
			}
		} else if (type == "Number") {
			long value;
			if (attributesGrid->GetCellValue(rowIndex, 2).ToLong(&value)) {
				attr.set(static_cast<int32_t>(value));
			}
		} else if (type == "Boolean") {
			attr.set(attributesGrid->GetCellValue(rowIndex, 2) == "1");
		} else {
			continue;
		}
		std::string key = nstr(attributesGrid->GetCellValue(rowIndex, 0));
		if (!key.empty()) {
			edit_item->setAttribute(key, attr);
		}
	}
}

void PropertiesWindow::OnGridValueChanged(wxGridEvent& event) {
	if (event.GetCol() == 1) {
		wxString newType = attributesGrid->GetCellValue(event.GetRow(), 1);
		if (newType == event.GetString()) {
			return;
		}

		ItemAttribute attr;
		if (newType == "String") {
			attr.set("");
		} else if (newType == "Float") {
			attr.set(0.0f);
		} else if (newType == "Number") {
			attr.set(0);
		} else if (newType == "Boolean") {
			attr.set(false);
		}
		SetGridValue(attributesGrid, event.GetRow(), nstr(attributesGrid->GetCellValue(event.GetRow(), 0)), attr);
	}
}

void PropertiesWindow::OnClickOK(wxCommandEvent&) {
	if (!validateWaypointPanel()) {
		return;
	}
	saveGeneralPanel();
	saveContainerSpecialPanel();
	saveDoorSpecialPanel();
	saveTeleportSpecialPanel();
	saveAttributesPanel();
	saveWaypointPanel();
	EndModal(1);
}

void PropertiesWindow::OnClickAddAttribute(wxCommandEvent&) {
	attributesGrid->AppendRows(1);
	ItemAttribute attr(0);
	SetGridValue(attributesGrid, attributesGrid->GetNumberRows() - 1, "", attr);
}

void PropertiesWindow::OnClickRemoveAttribute(wxCommandEvent&) {
	wxArrayInt rowIndexes = attributesGrid->GetSelectedRows();
	if (rowIndexes.Count() != 1) {
		return;
	}

	int rowIndex = rowIndexes[0];
	attributesGrid->DeleteRows(rowIndex, 1);
}

void PropertiesWindow::OnClickTown(wxCommandEvent&) {
	if (!edit_tile) return;
	Position click_pos = edit_tile->getPosition();

	Town* clicked_town = nullptr;
	for (const auto& pair : edit_map->towns) {
		if (pair.second->getTemplePosition() == click_pos) {
			clicked_town = pair.second;
			break;
		}
	}

	if (!clicked_town) {
		uint32_t max_id = 0;
		for (const auto& pair : edit_map->towns) {
			if (pair.second->getID() > max_id) {
				max_id = pair.second->getID();
			}
		}
		Map* map = const_cast<Map*>(edit_map);
		clicked_town = newd Town(max_id + 1);
		clicked_town->setName("Unnamed Town");
		clicked_town->setTemplePosition(click_pos);
		map->towns.addTown(clicked_town);
		Tile* tile = map->getOrCreateTile(click_pos);
		if (tile) {
			tile->getLocation()->increaseTownCount();
		}
		map->doChange();
	} else {
		clicked_town->setTemplePosition(click_pos);
	}

	if (g_gui.GetCurrentEditor()) {
		wxDialog* town_dialog = newd EditTownsDialog(this, *g_gui.GetCurrentEditor(), clicked_town->getID());
		town_dialog->ShowModal();
		town_dialog->Destroy();
	}
}

void PropertiesWindow::OnClickCancel(wxCommandEvent&) {
	EndModal(0);
}

void PropertiesWindow::OnClose(wxCloseEvent&) {
	EndModal(0);
}

wxWindow* PropertiesWindow::createWaypointPanel(wxWindow* parent) {
	wxPanel* panel = newd wxPanel(parent, ITEM_PROPERTIES_WAYPOINT_TAB);
	wxFlexGridSizer* gridsizer = newd wxFlexGridSizer(2, 10, 10);
	gridsizer->AddGrowableCol(1);

	gridsizer->Add(newd wxStaticText(panel, wxID_ANY, "Create Waypoint"));
	gridsizer->AddSpacer(0);

	gridsizer->Add(newd wxStaticText(panel, wxID_ANY, "Waypoint Name:"));
	
	std::string current_wp_name = "";
	if (edit_tile && edit_map) {
		Map* mutable_map = const_cast<Map*>(edit_map);
		Tile* mutable_tile = const_cast<Tile*>(edit_tile);
		Waypoint* wp = mutable_map->waypoints.getWaypoint(mutable_tile->getLocation());
		if (wp) {
			current_wp_name = wp->name;
		}
	}

	waypoint_name_field = newd wxTextCtrl(panel, wxID_ANY, wxstr(current_wp_name));
	gridsizer->Add(waypoint_name_field, wxSizerFlags(1).Expand());

	panel->SetSizerAndFit(gridsizer);
	return panel;
}

bool PropertiesWindow::validateWaypointPanel() {
	if (!edit_tile || !edit_map || !waypoint_name_field) return true;

	std::string new_name = nstr(waypoint_name_field->GetValue());
	new_name.erase(0, new_name.find_first_not_of(" \t\r\n"));
	new_name.erase(new_name.find_last_not_of(" \t\r\n") + 1);

	if (new_name.empty()) return true;

	Map* mutable_map = const_cast<Map*>(edit_map);
	Tile* mutable_tile = const_cast<Tile*>(edit_tile);
	Waypoint* current_wp = mutable_map->waypoints.getWaypoint(mutable_tile->getLocation());
	Waypoint* existing_wp = mutable_map->waypoints.getWaypoint(new_name);

	if (existing_wp && (!current_wp || current_wp->name != new_name)) {
		wxMessageBox("There already is a waypoint with this name.", "Error", wxOK | wxICON_ERROR, this);
		return false;
	}
	return true;
}

void PropertiesWindow::saveWaypointPanel() {
	if (!edit_tile || !edit_map || !waypoint_name_field) return;

	std::string new_name = nstr(waypoint_name_field->GetValue());
	new_name.erase(0, new_name.find_first_not_of(" \t\r\n"));
	new_name.erase(new_name.find_last_not_of(" \t\r\n") + 1);

	Map* mutable_map = const_cast<Map*>(edit_map);
	Tile* mutable_tile = const_cast<Tile*>(edit_tile);
	Waypoint* current_wp = mutable_map->waypoints.getWaypoint(mutable_tile->getLocation());

	if (current_wp) {
		std::string old_name = current_wp->name;
		if (new_name.empty()) {
			// User cleared the name, remove the waypoint
			mutable_map->waypoints.removeWaypoint(old_name);
			g_gui.RefreshPalettes();
		} else if (old_name != new_name) {
			// Rename the waypoint
			mutable_map->waypoints.removeWaypoint(old_name);

			Waypoint* nwp = newd Waypoint(new_name, edit_tile->getPosition());
			mutable_map->waypoints.addWaypoint(nwp);
			g_gui.RefreshPalettes();
		}
	} else {
		if (!new_name.empty()) {
			Waypoint* nwp = newd Waypoint(new_name, edit_tile->getPosition());
			mutable_map->waypoints.addWaypoint(nwp);
			g_gui.RefreshPalettes();
		}
	}
}
