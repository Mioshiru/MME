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
#include "common_windows.h"
#include "application.h"

#include <wx/grid.h>
#include <wx/textctrl.h>
#include <wx/msgdlg.h>
#include <wx/statline.h>

BEGIN_EVENT_TABLE(PropertiesWindow, wxDialog)
	EVT_BUTTON(wxID_OK, PropertiesWindow::OnClickOK)
	EVT_BUTTON(wxID_CANCEL, PropertiesWindow::OnClickCancel)
	EVT_CLOSE(PropertiesWindow::OnClose)

	EVT_BUTTON(ITEM_PROPERTIES_ADD_ATTRIBUTE, PropertiesWindow::OnClickAddAttribute)
	EVT_BUTTON(ITEM_PROPERTIES_REMOVE_ATTRIBUTE, PropertiesWindow::OnClickRemoveAttribute)
	EVT_BUTTON(ITEM_PROPERTIES_TOWN_BTN, PropertiesWindow::OnClickTown)

	EVT_GRID_CELL_CHANGED(PropertiesWindow::OnGridValueChanged)
END_EVENT_TABLE()

PropertiesWindow::PropertiesWindow(wxWindow* parent, const Map* map, const Tile* tile_parent, Item* item, wxPoint pos) :
	wxDialog(parent, wxID_ANY, "Item Attributes", pos, wxSize(660, 520), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	edit_map(map),
	edit_tile(tile_parent),
	edit_item(item),
	door_type_choice(nullptr),
	door_req_level(nullptr),
	door_action_id(nullptr),
	door_storage_key(nullptr),
	chest_mode_choice(nullptr),
	chest_quest_panel(nullptr),
	chest_req_level(nullptr),
	chest_action_id(nullptr),
	chest_reward_msg(nullptr),
	tele_dest_x(nullptr),
	tele_dest_y(nullptr),
	tele_dest_z(nullptr),
	attributesGrid(nullptr),
	waypoint_name_field(nullptr),
	notebook(nullptr),
	action_id_field(nullptr),
	unique_id_field(nullptr),
	ore_type_choice(nullptr),
	count_field(nullptr),
	text_field(nullptr),
	depot_town_field(nullptr),
	locked_door_checkbox(nullptr)
{
	ASSERT(edit_item);

	SetTitle(wxString::Format("Attributes - %s (#%d)", wxstr(edit_item->getName()), edit_item->getID()));
	SetBackgroundColour(wxColour(15, 23, 42)); // Slate 900
	SetForegroundColour(wxColour(248, 250, 252));

	wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);

	// Modern Header Banner
	wxPanel* headerPanel = new wxPanel(this, wxID_ANY);
	headerPanel->SetBackgroundColour(wxColour(30, 41, 59)); // Slate 800
	wxBoxSizer* headerSizer = new wxBoxSizer(wxVERTICAL);

	wxString itemTitle = wxString::Format("#%d - %s", edit_item->getID(), wxstr(edit_item->getName()));
	wxStaticText* header = new wxStaticText(headerPanel, wxID_ANY, itemTitle);
	header->SetFont(wxFont(13, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	header->SetForegroundColour(wxColour(251, 191, 36)); // Amber Gold

	wxStaticText* subheader = new wxStaticText(headerPanel, wxID_ANY, "Configure item parameters, Action/Unique IDs, mining resources, and custom attributes.");
	subheader->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	subheader->SetForegroundColour(wxColour(148, 163, 184)); // Slate 400

	headerSizer->Add(header, 0, wxLEFT | wxRIGHT | wxTOP, 10);
	headerSizer->Add(subheader, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
	headerPanel->SetSizer(headerSizer);

	topSizer->Add(headerPanel, 0, wxEXPAND | wxALL, 10);

	// Notebook with dark tabs
	notebook = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
	notebook->SetBackgroundColour(wxColour(30, 41, 59));
	notebook->SetForegroundColour(wxColour(248, 250, 252));

	notebook->AddPage(createGeneralPanel(notebook), "General", true);

	if (dynamic_cast<Container*>(item)) {
		notebook->AddPage(createContainerPanel(notebook), "Container / Chest");
	} else if (item->isDoor()) {
		notebook->AddPage(createDoorSpecialPanel(notebook), "Door Settings");
	} else if (dynamic_cast<Teleport*>(item)) {
		notebook->AddPage(createTeleportSpecialPanel(notebook), "Teleporter");
	}

	notebook->AddPage(createAttributesPanel(notebook), "Custom Attributes");

	if (edit_tile) {
		notebook->AddPage(createWaypointPanel(notebook), "Waypoint");
	}

	topSizer->Add(notebook, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	// Footer with modern styled action buttons
	wxBoxSizer* optSizer = new wxBoxSizer(wxHORIZONTAL);
	wxButton* okBtn = new wxButton(this, wxID_OK, "OK");
	wxButton* cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");

	okBtn->SetBackgroundColour(wxColour(37, 99, 235)); // Royal Blue
	okBtn->SetForegroundColour(wxColour(255, 255, 255));
	okBtn->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));

	cancelBtn->SetBackgroundColour(wxColour(51, 65, 85)); // Slate 700
	cancelBtn->SetForegroundColour(wxColour(203, 213, 225));

	optSizer->Add(okBtn, 0, wxRIGHT, 10);
	optSizer->Add(cancelBtn, 0);

	topSizer->Add(optSizer, 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT | wxBOTTOM, 12);

	SetSizerAndFit(topSizer);
	Centre(wxBOTH);

	RME::UI::StyleManager::ApplyThemeRecursively(this, RME::UI::StyleManager::GetTheme());
}

PropertiesWindow::~PropertiesWindow() {
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
	wxPanel* panel = new wxPanel(parent, ITEM_PROPERTIES_GENERAL_TAB);
	panel->SetBackgroundColour(wxColour(30, 41, 59));

	wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);

	auto createLabel = [panel](const wxString& labelText) -> wxStaticText* {
		wxStaticText* label = new wxStaticText(panel, wxID_ANY, labelText);
		label->SetForegroundColour(wxColour(148, 163, 184));
		label->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
		return label;
	};

	// 1. Header Info Row: Item Name & ID
	wxBoxSizer* itemInfoRow = new wxBoxSizer(wxHORIZONTAL);
	itemInfoRow->Add(createLabel("Item:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
	wxStaticText* idText = new wxStaticText(panel, wxID_ANY, wxString::Format("%s (#%d)", wxstr(edit_item->getName()), edit_item->getID()));
	idText->SetForegroundColour(wxColour(251, 191, 36));
	idText->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	itemInfoRow->Add(idText, 0, wxALIGN_CENTER_VERTICAL);
	topSizer->Add(itemInfoRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 4);

	// 2. Dropdowns Block (Ore / Resource, Depot Town, etc.)
	if (edit_item->isOreRockAsset()) {
		wxBoxSizer* oreRow = new wxBoxSizer(wxHORIZONTAL);
		oreRow->Add(createLabel("Mining Ore:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
		ore_type_choice = new wxChoice(panel, wxID_ANY);
		ore_type_choice->Append("None / Normal Rock (0)");
		ore_type_choice->Append("Copper Ore (Lvl 1 - AID 4501)");
		ore_type_choice->Append("Iron Ore (Lvl 20 - AID 4502)");
		ore_type_choice->Append("Gold Ore (Lvl 50 - AID 4503)");
		ore_type_choice->Append("Diamond Vein (Lvl 85 - AID 4504)");

		int curAid = edit_item->getActionID();
		if (curAid >= 4501 && curAid <= 4504) {
			ore_type_choice->SetSelection(curAid - 4500);
		} else {
			ore_type_choice->SetSelection(0);
		}

		ore_type_choice->Bind(wxEVT_CHOICE, [this](wxCommandEvent& evt) {
			int sel = evt.GetSelection();
			if (sel >= 1 && sel <= 4) {
				if (action_id_field) action_id_field->SetValue(4500 + sel);
			} else if (sel == 0 && action_id_field) {
				int val = action_id_field->GetValue();
				if (val >= 4501 && val <= 4504) {
					action_id_field->SetValue(0);
				}
			}
		});
		oreRow->Add(ore_type_choice, 0, wxALIGN_CENTER_VERTICAL);
		topSizer->Add(oreRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
	} else {
		ore_type_choice = nullptr;
	}

	// 3. Numeric Input Fields Block (Action ID, Unique ID)
	wxBoxSizer* idRow = new wxBoxSizer(wxHORIZONTAL);
	idRow->Add(createLabel("Action ID:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
	action_id_field = new wxSpinCtrl(panel, wxID_ANY, i2ws(edit_item->getActionID()), wxDefaultPosition, wxSize(90, -1), wxSP_ARROW_KEYS, 0, 0xFFFF, edit_item->getActionID());
	idRow->Add(action_id_field, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);

	idRow->Add(createLabel("Unique ID:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
	unique_id_field = new wxSpinCtrl(panel, wxID_ANY, i2ws(edit_item->getUniqueID()), wxDefaultPosition, wxSize(90, -1), wxSP_ARROW_KEYS, 0, 0xFFFF, edit_item->getUniqueID());
	idRow->Add(unique_id_field, 0, wxALIGN_CENTER_VERTICAL);
	topSizer->Add(idRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);

	// 4. Locked Door
	if (edit_item->isDoor()) {
		wxBoxSizer* doorRow = new wxBoxSizer(wxHORIZONTAL);
		doorRow->Add(createLabel("Locked Door:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
		locked_door_checkbox = new wxCheckBox(panel, wxID_ANY, "Locked Key Door (Action ID 100)");
		locked_door_checkbox->SetForegroundColour(wxColour(248, 250, 252));
		locked_door_checkbox->SetValue(edit_item->getActionID() == 100);

		locked_door_checkbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& evt) {
			if (evt.IsChecked()) {
				if (action_id_field) action_id_field->SetValue(100);
				if (door_action_id) door_action_id->SetValue(100);
				if (door_type_choice) door_type_choice->SetSelection(1);
			} else {
				if (action_id_field && action_id_field->GetValue() == 100) action_id_field->SetValue(0);
				if (door_action_id && door_action_id->GetValue() == 100) door_action_id->SetValue(0);
				if (door_type_choice && door_type_choice->GetSelection() == 1) door_type_choice->SetSelection(0);
			}
		});
		doorRow->Add(locked_door_checkbox, 1, wxALIGN_CENTER_VERTICAL);
		topSizer->Add(doorRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);
	} else {
		locked_door_checkbox = nullptr;
	}

	// Sync Action ID
	if (action_id_field) {
		auto syncFromActionId = [this](wxCommandEvent&) {
			if (!action_id_field) return;
			int val = action_id_field->GetValue();
			if (ore_type_choice) {
				if (val >= 4501 && val <= 4504) {
					ore_type_choice->SetSelection(val - 4500);
				} else {
					ore_type_choice->SetSelection(0);
				}
			}
			if (locked_door_checkbox) {
				locked_door_checkbox->SetValue(val == 100);
			}
			if (door_action_id) {
				door_action_id->SetValue(val);
			}
		};
		action_id_field->Bind(wxEVT_SPINCTRL, syncFromActionId);
		action_id_field->Bind(wxEVT_TEXT, syncFromActionId);
	}

	// 5. Count / Subtype
	if (edit_item->isStackable() || edit_item->isCharged() || edit_item->isFluidContainer() || edit_item->isSplash()) {
		int max_count = 100;
		if (edit_item->isFluidContainer() || edit_item->isSplash()) {
			max_count = 250;
		} else if (edit_item->isCharged()) {
			max_count = 65500;
		}
		wxBoxSizer* countRow = new wxBoxSizer(wxHORIZONTAL);
		countRow->Add(createLabel("Count / Subtype:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
		count_field = new wxSpinCtrl(panel, wxID_ANY, i2ws(edit_item->getCount()), wxDefaultPosition, wxSize(90, -1), wxSP_ARROW_KEYS, 1, max_count, edit_item->getCount());
		if (edit_item->isHangable()) {
			count_field->Enable(false);
		}
		countRow->Add(count_field, 0, wxALIGN_CENTER_VERTICAL);
		topSizer->Add(countRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);
	} else {
		count_field = nullptr;
	}

	// 6. Text / Inscription
	if (edit_item->canHoldText() || edit_item->canHoldDescription()) {
		wxBoxSizer* textRow = new wxBoxSizer(wxHORIZONTAL);
		textRow->Add(createLabel("Inscription:"), 0, wxTOP | wxRIGHT, 8);
		text_field = new wxTextCtrl(panel, wxID_ANY, wxstr(edit_item->getText()), wxDefaultPosition, wxSize(-1, 60), wxTE_MULTILINE);
		textRow->Add(text_field, 1, wxEXPAND);
		topSizer->Add(textRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);
	} else {
		text_field = nullptr;
	}

	// 7. Depot Town
	if (Depot* depot = dynamic_cast<Depot*>(edit_item)) {
		wxBoxSizer* depotRow = new wxBoxSizer(wxHORIZONTAL);
		depotRow->Add(createLabel("Depot Town:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
		depot_town_field = new wxChoice(panel, wxID_ANY);
		uint16_t cur_town_id = depot->getDepotID();
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
		}
		depot_town_field->Append("No Town", reinterpret_cast<void*>(static_cast<uintptr_t>(0)));
		if (cur_town_id == 0) {
			to_select_index = depot_town_field->GetCount() - 1;
		}
		depot_town_field->SetSelection(to_select_index);
		depotRow->Add(depot_town_field, 0, wxALIGN_CENTER_VERTICAL);
		topSizer->Add(depotRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);
	} else {
		depot_town_field = nullptr;
	}

	// 8. Town Temple
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

		wxBoxSizer* town_sizer = new wxBoxSizer(wxHORIZONTAL);
		town_sizer->Add(createLabel("Town Temple:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
		wxStaticText* town_lbl = new wxStaticText(panel, wxID_ANY, town_text_val);
		town_lbl->SetForegroundColour(wxColour(248, 250, 252));
		town_sizer->Add(town_lbl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
		wxButton* town_btn = new wxButton(panel, ITEM_PROPERTIES_TOWN_BTN, "Edit Towns...");
		town_btn->SetBackgroundColour(wxColour(51, 65, 85));
		town_btn->SetForegroundColour(wxColour(203, 213, 225));
		town_sizer->Add(town_btn, 0, wxALIGN_CENTER_VERTICAL);

		topSizer->Add(town_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);
	}

	panel->SetSizerAndFit(topSizer);
	return panel;
}

wxWindow* PropertiesWindow::createContainerPanel(wxWindow* parent) {
	Container* container = (Container*)edit_item;
	wxPanel* panel = new wxPanel(parent, ITEM_PROPERTIES_CONTAINER_TAB);
	panel->SetBackgroundColour(wxColour(30, 41, 59));
	wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);

	wxStaticText* headerNote = new wxStaticText(panel, wxID_ANY, "Chest & Container Special Configuration");
	headerNote->SetForegroundColour(wxColour(251, 191, 36));
	headerNote->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	topSizer->Add(headerNote, 0, wxLEFT | wxRIGHT | wxTOP, 8);

	wxStaticText* subNote = new wxStaticText(panel, wxID_ANY, "Right-click any slot to Add, Edit, or Delete items.");
	subNote->SetForegroundColour(wxColour(148, 163, 184));
	subNote->SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	topSizer->Add(subNote, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);

	wxBoxSizer* modeRow = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* modeLbl = new wxStaticText(panel, wxID_ANY, "Chest Mode:");
	modeLbl->SetForegroundColour(wxColour(148, 163, 184));
	modeLbl->SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	modeRow->Add(modeLbl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	wxArrayString modes;
	modes.Add("Casual (Standard Container / Session Loot)");
	modes.Add("Quest Chest (Per-Player Permanent Reward)");
	chest_mode_choice = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, modes);
	modeRow->Add(chest_mode_choice, 1, wxEXPAND);
	topSizer->Add(modeRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

	chest_quest_panel = new wxPanel(panel, wxID_ANY);
	chest_quest_panel->SetBackgroundColour(wxColour(30, 41, 59));
	wxFlexGridSizer* fgrid = new wxFlexGridSizer(2, 4, 8);
	fgrid->AddGrowableCol(1);

	auto addLbl = [this, fgrid](const wxString& text) {
		wxStaticText* lbl = new wxStaticText(chest_quest_panel, wxID_ANY, text);
		lbl->SetForegroundColour(wxColour(148, 163, 184));
		lbl->SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
		fgrid->Add(lbl, 0, wxALIGN_CENTER_VERTICAL);
	};

	addLbl("Required Min Level:");
	chest_req_level = new wxSpinCtrl(chest_quest_panel, wxID_ANY, "1", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 10000, 1);
	fgrid->Add(chest_req_level, wxSizerFlags(1).Expand());

	addLbl("Storage Key (ActionID):");
	chest_action_id = new wxSpinCtrl(chest_quest_panel, wxID_ANY, i2ws(edit_item->getActionID()), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 65535, edit_item->getActionID());
	fgrid->Add(chest_action_id, wxSizerFlags(1).Expand());

	addLbl("Reward Message:");
	chest_reward_msg = new wxTextCtrl(chest_quest_panel, wxID_ANY, "You have found a reward.");
	fgrid->Add(chest_reward_msg, wxSizerFlags(1).Expand());

	chest_quest_panel->SetSizer(fgrid);
	topSizer->Add(chest_quest_panel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

	bool isQuest = (edit_item->getActionID() >= 2000);
	chest_mode_choice->SetSelection(isQuest ? 1 : 0);
	chest_quest_panel->Show(isQuest);

	chest_mode_choice->Bind(wxEVT_CHOICE, [this, panel, topSizer](wxCommandEvent&) {
		bool qMode = (chest_mode_choice->GetSelection() == 1);
		chest_quest_panel->Show(qMode);
		if (qMode && chest_action_id && chest_action_id->GetValue() < 2000) {
			chest_action_id->SetValue(2001);
		}
		topSizer->Layout();
		panel->Layout();
		Layout();
	});

	wxWrapSizer* gridSizer = new wxWrapSizer(wxHORIZONTAL);
	bool use_large_sprites = g_settings.getBoolean(Config::USE_LARGE_CONTAINER_ICONS);
	for (uint32_t i = 0; i < container->getVolume(); ++i) {
		Item* cItem = container->getItem(i);
		ContainerItemButton* containerItemButton = new ContainerItemButton(panel, use_large_sprites, i, edit_map, cItem);
		container_items.push_back(containerItemButton);
		gridSizer->Add(containerItemButton, wxSizerFlags(0));
	}

	topSizer->Add(gridSizer, wxSizerFlags(1).Expand().Border(wxALL, 4));
	panel->SetSizer(topSizer);
	return panel;
}

void PropertiesWindow::saveContainerSpecialPanel() {
	if (!chest_mode_choice || !edit_item) return;
	if (chest_mode_choice->GetSelection() == 1) {
		if (chest_action_id && chest_action_id->GetValue() > 0) {
			edit_item->setActionID(chest_action_id->GetValue());
		}
		if (chest_reward_msg && !chest_reward_msg->GetValue().empty()) {
			edit_item->setText(chest_reward_msg->GetValue().ToStdString());
		}
	} else {
		if (edit_item->getActionID() >= 2000) {
			edit_item->setActionID(0);
		}
	}
}

wxWindow* PropertiesWindow::createDoorSpecialPanel(wxWindow* parent) {
	wxPanel* panel = new wxPanel(parent, wxID_ANY);
	panel->SetBackgroundColour(wxColour(30, 41, 59));

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	wxStaticText* hNote = new wxStaticText(panel, wxID_ANY, "Quest & Level Door Configuration");
	hNote->SetForegroundColour(wxColour(251, 191, 36));
	hNote->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	mainSizer->Add(hNote, 0, wxALL, 8);

	wxFlexGridSizer* fgrid = new wxFlexGridSizer(2, 10, 10);
	fgrid->AddGrowableCol(1);

	auto addLbl = [panel, fgrid](const wxString& text) {
		wxStaticText* lbl = new wxStaticText(panel, wxID_ANY, text);
		lbl->SetForegroundColour(wxColour(148, 163, 184));
		lbl->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
		fgrid->Add(lbl, 0, wxALIGN_CENTER_VERTICAL);
	};

	addLbl("Door Function Type:");
	wxArrayString doorTypes;
	doorTypes.Add("Standard Door");
	doorTypes.Add("Locked Door (Key / ActionID 100)");
	doorTypes.Add("Quest Door (Storage Key Check)");
	doorTypes.Add("Level Door (Required Min Level)");

	door_type_choice = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, doorTypes);
	fgrid->Add(door_type_choice, wxSizerFlags(1).Expand());

	addLbl("Required Level:");
	door_req_level = new wxSpinCtrl(panel, wxID_ANY, "1", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 10000, 1);
	fgrid->Add(door_req_level, wxSizerFlags(1).Expand());

	addLbl("Door Action ID:");
	door_action_id = new wxSpinCtrl(panel, wxID_ANY, i2ws(edit_item->getActionID()), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 65535, edit_item->getActionID());
	fgrid->Add(door_action_id, wxSizerFlags(1).Expand());

	addLbl("Required Storage Key:");
	door_storage_key = new wxSpinCtrl(panel, wxID_ANY, "50000", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 999999, 50000);
	fgrid->Add(door_storage_key, wxSizerFlags(1).Expand());

	int curAid = edit_item->getActionID();
	if (curAid == 100) {
		door_type_choice->SetSelection(1);
	} else if (curAid >= 1000 && curAid < 2000) {
		door_type_choice->SetSelection(3);
		door_req_level->SetValue(curAid - 1000);
	} else if (curAid >= 2000) {
		door_type_choice->SetSelection(2);
	} else {
		door_type_choice->SetSelection(0);
	}

	door_type_choice->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
		if (!door_type_choice) return;
		int sel = door_type_choice->GetSelection();
		if (sel == 0) {
			if (door_action_id) door_action_id->SetValue(0);
			if (action_id_field) action_id_field->SetValue(0);
			if (locked_door_checkbox) locked_door_checkbox->SetValue(false);
		} else if (sel == 1) {
			if (door_action_id) door_action_id->SetValue(100);
			if (action_id_field) action_id_field->SetValue(100);
			if (locked_door_checkbox) locked_door_checkbox->SetValue(true);
		} else if (sel == 2) {
			if (door_action_id && door_action_id->GetValue() < 2000) door_action_id->SetValue(2001);
			if (action_id_field && door_action_id) action_id_field->SetValue(door_action_id->GetValue());
			if (locked_door_checkbox) locked_door_checkbox->SetValue(false);
		} else if (sel == 3) {
			int lvl = door_req_level ? door_req_level->GetValue() : 1;
			if (door_action_id) door_action_id->SetValue(1000 + lvl);
			if (action_id_field) action_id_field->SetValue(1000 + lvl);
			if (locked_door_checkbox) locked_door_checkbox->SetValue(false);
		}
	});

	door_req_level->Bind(wxEVT_SPINCTRL, [this](wxCommandEvent&) {
		if (door_type_choice && door_type_choice->GetSelection() == 3 && door_req_level) {
			int lvl = door_req_level->GetValue();
			if (door_action_id) door_action_id->SetValue(1000 + lvl);
			if (action_id_field) action_id_field->SetValue(1000 + lvl);
		}
	});

	mainSizer->Add(fgrid, 1, wxEXPAND | wxALL, 8);
	panel->SetSizerAndFit(mainSizer);
	return panel;
}

void PropertiesWindow::saveDoorSpecialPanel() {
	if (!door_type_choice || !edit_item) return;
	int sel = door_type_choice->GetSelection();
	if (sel == 0) {
		edit_item->setActionID(door_action_id ? door_action_id->GetValue() : 0);
	} else if (sel == 1) {
		edit_item->setActionID(100);
	} else if (sel == 3) {
		edit_item->setActionID(1000 + (door_req_level ? door_req_level->GetValue() : 1));
	} else if (door_action_id) {
		edit_item->setActionID(door_action_id->GetValue());
	}
}

wxWindow* PropertiesWindow::createTeleportSpecialPanel(wxWindow* parent) {
	wxPanel* panel = new wxPanel(parent, wxID_ANY);
	panel->SetBackgroundColour(wxColour(30, 41, 59));

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	wxStaticText* hNote = new wxStaticText(panel, wxID_ANY, "Teleporter Destination Coordinates");
	hNote->SetForegroundColour(wxColour(251, 191, 36));
	hNote->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	mainSizer->Add(hNote, 0, wxALL, 8);

	wxFlexGridSizer* fgrid = new wxFlexGridSizer(2, 10, 10);
	fgrid->AddGrowableCol(1);

	auto addLbl = [panel, fgrid](const wxString& text) {
		wxStaticText* lbl = new wxStaticText(panel, wxID_ANY, text);
		lbl->SetForegroundColour(wxColour(148, 163, 184));
		lbl->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
		fgrid->Add(lbl, 0, wxALIGN_CENTER_VERTICAL);
	};

	Teleport* tele = dynamic_cast<Teleport*>(edit_item);
	Position dest = tele ? tele->getDestination() : Position(1000, 1000, 7);

	addLbl("Target Destination X:");
	tele_dest_x = new wxSpinCtrl(panel, wxID_ANY, i2ws(dest.x), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 65535, dest.x);
	fgrid->Add(tele_dest_x, wxSizerFlags(1).Expand());

	addLbl("Target Destination Y:");
	tele_dest_y = new wxSpinCtrl(panel, wxID_ANY, i2ws(dest.y), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 65535, dest.y);
	fgrid->Add(tele_dest_y, wxSizerFlags(1).Expand());

	addLbl("Target Destination Z (Floor):");
	tele_dest_z = new wxSpinCtrl(panel, wxID_ANY, i2ws(dest.z), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 15, dest.z);
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

static bool isReservedStandardAttribute(const std::string& key) {
	return key == "aid" || key == "uid" || key == "text" || key == "desc" ||
	       key == "count" || key == "charges" || key == "tier" || key == "depot_id";
}

wxWindow* PropertiesWindow::createAttributesPanel(wxWindow* parent) {
	wxPanel* panel = new wxPanel(parent, wxID_ANY);
	panel->SetBackgroundColour(wxColour(30, 41, 59));
	wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);

	attributesGrid = new wxGrid(panel, ITEM_PROPERTIES_ADVANCED_TAB, wxDefaultPosition, wxSize(-1, 160));
	topSizer->Add(attributesGrid, wxSizerFlags(1).Expand());

	wxFont time_font(*wxSWISS_FONT);
	attributesGrid->SetDefaultCellFont(time_font);
	attributesGrid->CreateGrid(0, 3);
	attributesGrid->DisableDragRowSize();
	attributesGrid->DisableDragColSize();
	attributesGrid->SetSelectionMode(wxGrid::wxGridSelectRows);
	attributesGrid->SetRowLabelSize(0);
	attributesGrid->EnableEditing(true);

	attributesGrid->SetColLabelValue(0, "Key");
	attributesGrid->SetColSize(0, 120);
	attributesGrid->SetColLabelValue(1, "Type");
	attributesGrid->SetColSize(1, 90);
	attributesGrid->SetColLabelValue(2, "Value");
	attributesGrid->SetColSize(2, 380);

	ItemAttributeMap attrs = edit_item->getAttributes();
	std::vector<std::pair<std::string, ItemAttribute>> customAttrs;
	for (const auto& kv : attrs) {
		if (!isReservedStandardAttribute(kv.first)) {
			customAttrs.push_back(kv);
		}
	}
	attributesGrid->AppendRows(customAttrs.size());
	int i = 0;
	for (const auto& kv : customAttrs) {
		SetGridValue(attributesGrid, i++, kv.first, kv.second);
	}

	wxBoxSizer* optSizer = new wxBoxSizer(wxHORIZONTAL);
	wxButton* addBtn = new wxButton(panel, ITEM_PROPERTIES_ADD_ATTRIBUTE, "+ Add Attribute");
	wxButton* remBtn = new wxButton(panel, ITEM_PROPERTIES_REMOVE_ATTRIBUTE, "- Remove Attribute");

	addBtn->SetBackgroundColour(wxColour(51, 65, 85));
	addBtn->SetForegroundColour(wxColour(203, 213, 225));
	remBtn->SetBackgroundColour(wxColour(51, 65, 85));
	remBtn->SetForegroundColour(wxColour(203, 213, 225));

	optSizer->Add(addBtn, 0, wxRIGHT, 8);
	optSizer->Add(remBtn, 0);
	topSizer->Add(optSizer, wxSizerFlags(0).Center().DoubleBorder());

	panel->SetSizer(topSizer);
	return panel;
}

void PropertiesWindow::SetGridValue(wxGrid* grid, int rowIndex, std::string label, const ItemAttribute& attr) {
	if (!grid || rowIndex < 0 || rowIndex >= grid->GetNumberRows()) return;
	wxArrayString types;
	types.Add("Number");
	types.Add("Float");
	types.Add("Boolean");
	types.Add("String");

	grid->SetCellValue(rowIndex, 0, label);
	switch (attr.type) {
		case ItemAttribute::STRING: {
			grid->SetCellValue(rowIndex, 1, "String");
			const std::string* s = attr.getString();
			grid->SetCellValue(rowIndex, 2, s ? wxstr(*s) : wxString(""));
			break;
		}
		case ItemAttribute::INTEGER: {
			grid->SetCellValue(rowIndex, 1, "Number");
			const int32_t* i = attr.getInteger();
			grid->SetCellValue(rowIndex, 2, i ? i2ws(*i) : wxString("0"));
			grid->SetCellEditor(rowIndex, 2, new wxGridCellNumberEditor);
			break;
		}
		case ItemAttribute::DOUBLE:
		case ItemAttribute::FLOAT: {
			grid->SetCellValue(rowIndex, 1, "Float");
			const double* f = attr.getFloat();
			grid->SetCellValue(rowIndex, 2, f ? wxString::Format("%g", *f) : wxString("0"));
			grid->SetCellEditor(rowIndex, 2, new wxGridCellFloatEditor);
			break;
		}
		case ItemAttribute::BOOLEAN: {
			grid->SetCellValue(rowIndex, 1, "Boolean");
			const bool* b = attr.getBoolean();
			grid->SetCellValue(rowIndex, 2, (b && *b) ? "1" : "");
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

void PropertiesWindow::saveGeneralPanel() {
	if (!edit_item) return;
	if (locked_door_checkbox && locked_door_checkbox->IsChecked()) {
		edit_item->setActionID(100);
	} else if (ore_type_choice && ore_type_choice->GetSelection() > 0) {
		edit_item->setActionID(4500 + ore_type_choice->GetSelection());
		if (action_id_field) action_id_field->SetValue(4500 + ore_type_choice->GetSelection());
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
	if (!edit_item || !attributesGrid) return;

	// Remove only custom attributes from edit_item to preserve standard fields
	std::vector<std::string> keysToErase;
	for (const auto& kv : edit_item->getAttributes()) {
		if (!isReservedStandardAttribute(kv.first)) {
			keysToErase.push_back(kv.first);
		}
	}
	for (const auto& k : keysToErase) {
		edit_item->eraseAttribute(k);
	}

	for (int32_t rowIndex = 0; rowIndex < attributesGrid->GetNumberRows(); ++rowIndex) {
		std::string key = nstr(attributesGrid->GetCellValue(rowIndex, 0));
		if (key.empty() || isReservedStandardAttribute(key)) continue;

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
		edit_item->setAttribute(key, attr);
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
	EndModal(wxID_OK);
}

void PropertiesWindow::OnClickAddAttribute(wxCommandEvent&) {
	if (!attributesGrid) return;
	attributesGrid->AppendRows(1);
	ItemAttribute attr(0);
	SetGridValue(attributesGrid, attributesGrid->GetNumberRows() - 1, "", attr);
}

void PropertiesWindow::OnClickRemoveAttribute(wxCommandEvent&) {
	if (!attributesGrid) return;
	wxArrayInt rowIndexes = attributesGrid->GetSelectedRows();
	if (rowIndexes.Count() != 1) {
		return;
	}
	int rowIndex = rowIndexes[0];
	attributesGrid->DeleteRows(rowIndex, 1);
}

void PropertiesWindow::OnClickTown(wxCommandEvent&) {
	if (!edit_tile || !edit_map) return;
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
		clicked_town = new Town(max_id + 1);
		clicked_town->setName("Unnamed Town");
		clicked_town->setTemplePosition(click_pos);
		map->towns.addTown(clicked_town);
		Tile* tile = map->getOrCreateTile(click_pos);
		if (tile && tile->getLocation()) {
			tile->getLocation()->increaseTownCount();
		}
		map->doChange();
	} else {
		clicked_town->setTemplePosition(click_pos);
	}

	if (g_gui.GetCurrentEditor()) {
		EditTownsDialog town_dialog(this, *g_gui.GetCurrentEditor(), clicked_town->getID());
		town_dialog.ShowModal();
	}
}

void PropertiesWindow::OnClickCancel(wxCommandEvent&) {
	EndModal(wxID_CANCEL);
}

void PropertiesWindow::OnClose(wxCloseEvent&) {
	EndModal(wxID_CANCEL);
}

wxWindow* PropertiesWindow::createWaypointPanel(wxWindow* parent) {
	wxPanel* panel = new wxPanel(parent, ITEM_PROPERTIES_WAYPOINT_TAB);
	panel->SetBackgroundColour(wxColour(30, 41, 59));
	wxFlexGridSizer* gridsizer = new wxFlexGridSizer(2, 10, 10);
	gridsizer->AddGrowableCol(1);

	wxStaticText* label = new wxStaticText(panel, wxID_ANY, "Waypoint Name:");
	label->SetForegroundColour(wxColour(148, 163, 184));
	label->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	gridsizer->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);

	std::string current_name = "";
	if (edit_tile && edit_map) {
		Waypoint* wp = const_cast<Map*>(edit_map)->waypoints.getWaypoint(const_cast<Tile*>(edit_tile)->getLocation());
		if (wp) {
			current_name = wp->name;
		}
	}

	waypoint_name_field = new wxTextCtrl(panel, wxID_ANY, wxstr(current_name));
	gridsizer->Add(waypoint_name_field, wxSizerFlags(1).Expand().Border(wxALL, 2));

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
			mutable_map->waypoints.removeWaypoint(old_name);
			g_gui.RefreshPalettes();
		} else if (old_name != new_name) {
			mutable_map->waypoints.removeWaypoint(old_name);
			Waypoint* nwp = new Waypoint(new_name, edit_tile->getPosition());
			mutable_map->waypoints.addWaypoint(nwp);
			g_gui.RefreshPalettes();
		}
	} else {
		if (!new_name.empty()) {
			Waypoint* nwp = new Waypoint(new_name, edit_tile->getPosition());
			mutable_map->waypoints.addWaypoint(nwp);
			g_gui.RefreshPalettes();
		}
	}
}
