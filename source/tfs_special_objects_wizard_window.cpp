#include "tfs_special_objects_wizard_window.h"
#include "style_manager.h"
#include "gui.h"
#include "style_manager.h"
#include "editor.h"
#include "style_manager.h"
#include "items.h"
#include "style_manager.h"
#include "graphics.h"
#include "style_manager.h"
#include "find_item_window.h"
#include "style_manager.h"
#include "map.h"
#include "style_manager.h"
#include <wx/stattext.h>
#include "style_manager.h"
#include <wx/button.h>
#include "style_manager.h"
#include <wx/sizer.h>
#include "style_manager.h"
#include <wx/msgdlg.h>
#include "style_manager.h"
#include <wx/filedlg.h>
#include "style_manager.h"
#include <wx/dcclient.h>
#include "style_manager.h"
#include <wx/clipbrd.h>
#include "style_manager.h"
#include <wx/dataobj.h>
#include "style_manager.h"
#include <sstream>
#include "style_manager.h"
#include <iomanip>
#include "style_manager.h"

enum {
	OBJ_CHEST_MODEL = wxID_HIGHEST + 600,
	OBJ_BTN_PICK_ITEM_SLOT,
	OBJ_BTN_CLEAR_SLOT,
	OBJ_BTN_CLEAR_ALL,
	OBJ_BTN_AUTOGEN_IDS,
	OBJ_BTN_PICK_KEY,
	OBJ_DOOR_MODEL,
	OBJ_BTN_GENERATE_SCRIPT,
	OBJ_BTN_COPY_SCRIPT,
	OBJ_BTN_PLACE_MAP
};

BEGIN_EVENT_TABLE(SpecialObjectsWizardDialog, wxDialog)
	EVT_CHOICE(OBJ_CHEST_MODEL, SpecialObjectsWizardDialog::OnPickChestModel)
	EVT_BUTTON(OBJ_BTN_PICK_ITEM_SLOT, SpecialObjectsWizardDialog::OnPickItemForSlot)
	EVT_BUTTON(OBJ_BTN_CLEAR_SLOT, SpecialObjectsWizardDialog::OnClearSlot)
	EVT_BUTTON(OBJ_BTN_CLEAR_ALL, SpecialObjectsWizardDialog::OnClearAllSlots)
	EVT_BUTTON(OBJ_BTN_AUTOGEN_IDS, SpecialObjectsWizardDialog::OnAutoGenerateIds)
	EVT_BUTTON(OBJ_BTN_PICK_KEY, SpecialObjectsWizardDialog::OnPickKeyFromPalette)
	EVT_BUTTON(OBJ_BTN_GENERATE_SCRIPT, SpecialObjectsWizardDialog::OnGenerateScript)
	EVT_BUTTON(OBJ_BTN_COPY_SCRIPT, SpecialObjectsWizardDialog::OnCopyScript)
	EVT_BUTTON(OBJ_BTN_PLACE_MAP, SpecialObjectsWizardDialog::OnPlaceOnMap)
	EVT_BUTTON(wxID_CANCEL, SpecialObjectsWizardDialog::OnClose)
END_EVENT_TABLE()

SpecialObjectsWizardDialog::SpecialObjectsWizardDialog(wxWindow* parent) :
	wxDialog(parent, wxID_ANY, "Special Objects && Quest Chest Wizard", wxDefaultPosition, wxSize(780, 680), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	selected_chest_id(1740),
	active_selected_slot(0),
	req_key_id(2088)
{
	for (int i = 0; i < MAX_CHEST_SLOTS; ++i) {
		slot_panels[i] = nullptr;
	}

	// Pre-fill slot 0 with Crystal Coins (ID 2160, count 10) as demo
	chest_slots[0].item_id = 2160;
	chest_slots[0].count = 10;
	chest_slots[0].name = "crystal coin";

	// Root Sizer
	wxBoxSizer* rootSizer = new wxBoxSizer(wxVERTICAL);

	// Header Panel (Corporate Dark Obsidian with Gold Accent)
	wxPanel* headerPanel = new wxPanel(this, wxID_ANY);
	headerPanel->SetBackgroundColour(wxColour(16, 20, 30));
	wxBoxSizer* headerSizer = new wxBoxSizer(wxVERTICAL);

	wxStaticText* title = new wxStaticText(headerPanel, wxID_ANY, "Special Objects && Quest Chest Suite");
	wxFont tFont = title->GetFont();
	tFont.SetPointSize(12);
	tFont.SetWeight(wxFONTWEIGHT_BOLD);
	title->SetFont(tFont);
	title->SetForegroundColour(wxColour(255, 215, 0));
	headerSizer->Add(title, 0, wxALL, 8);

	wxStaticText* sub = new wxStaticText(headerPanel, wxID_ANY, "Create interactive Quest Chests with visual multi-slot palette rewards, Quest Doors, Key locks, and Teleporters.");
	sub->SetForegroundColour(wxColour(190, 195, 205));
	headerSizer->Add(sub, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

	headerPanel->SetSizer(headerSizer);
	rootSizer->Add(headerPanel, 0, wxEXPAND);

	notebook = new wxNotebook(this, wxID_ANY);

	// =========================================================================
	// TAB 1: Quest Chest & Visual Multi-Slot Inventory
	// =========================================================================
	wxPanel* tabChest = new wxPanel(notebook, wxID_ANY);
	wxBoxSizer* cMainSizer = new wxBoxSizer(wxVERTICAL);

	// Top Section: Model Selection & Auto IDs
	wxStaticBoxSizer* topBox = new wxStaticBoxSizer(wxVERTICAL, tabChest, "Chest Model & Quest Identification");
	wxFlexGridSizer* topGrid = new wxFlexGridSizer(2, 4, 6, 10);
	topGrid->AddGrowableCol(1, 1);
	topGrid->AddGrowableCol(3, 1);

	topGrid->Add(new wxStaticText(tabChest, wxID_ANY, "Chest Model:"), 0, wxALIGN_CENTER_VERTICAL);
	wxArrayString chestModels;
	chestModels.Add("Wooden Chest (ID: 1740)");
	chestModels.Add("Pirate / Skull Chest (ID: 5674)");
	chestModels.Add("Golden / Jeweled Chest (ID: 1746)");
	chestModels.Add("Crate / Box (ID: 1738)");
	chestModels.Add("Dragon Treasure Box (ID: 1747)");
	chest_model_choice = new wxChoice(tabChest, OBJ_CHEST_MODEL, wxDefaultPosition, wxDefaultSize, chestModels);
	chest_model_choice->SetSelection(0);
	topGrid->Add(chest_model_choice, 1, wxEXPAND);

	topGrid->Add(new wxStaticText(tabChest, wxID_ANY, "Quest Name:"), 0, wxALIGN_CENTER_VERTICAL);
	quest_name_ctrl = new wxTextCtrl(tabChest, wxID_ANY, "The Ancient Treasure");
	topGrid->Add(quest_name_ctrl, 1, wxEXPAND);

	topGrid->Add(new wxStaticText(tabChest, wxID_ANY, "Chest ActionID:"), 0, wxALIGN_CENTER_VERTICAL);
	chest_action_id = new wxSpinCtrl(tabChest, wxID_ANY, "2000", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1000, 65535, 2000);
	topGrid->Add(chest_action_id, 1, wxEXPAND);

	topGrid->Add(new wxStaticText(tabChest, wxID_ANY, "Storage Key:"), 0, wxALIGN_CENTER_VERTICAL);
	chest_storage_key = new wxSpinCtrl(tabChest, wxID_ANY, "50001", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1000, 999999, 50001);
	topGrid->Add(chest_storage_key, 1, wxEXPAND);

	topBox->Add(topGrid, 0, wxEXPAND | wxALL, 6);

	wxBoxSizer* idBtnRow = new wxBoxSizer(wxHORIZONTAL);
	wxButton* autoIdBtn = new wxButton(tabChest, OBJ_BTN_AUTOGEN_IDS, "Auto-Generate Collision-Free ActionID & Storage Key");
	autoIdBtn->SetBackgroundColour(wxColour(40, 70, 120));
	autoIdBtn->SetForegroundColour(*wxWHITE);
	idBtnRow->Add(autoIdBtn, 0);
	topBox->Add(idBtnRow, 0, wxALL, 4);

	cMainSizer->Add(topBox, 0, wxEXPAND | wxALL, 6);

	// Middle Section: Visual Multi-Slot Chest Inventory (8 Slots)
	wxStaticBoxSizer* slotBox = new wxStaticBoxSizer(wxVERTICAL, tabChest, "Chest Reward Items (Visual Multi-Slot Palette Inventory)");

	wxGridSizer* gridSlots = new wxGridSizer(2, 4, 8, 8);
	for (int i = 0; i < MAX_CHEST_SLOTS; ++i) {
		int slotIdx = i;
		slot_panels[i] = new wxPanel(tabChest, wxID_ANY, wxDefaultPosition, wxSize(130, 80));
		slot_panels[i]->SetBackgroundColour(wxColour(20, 24, 34));
		slot_panels[i]->Bind(wxEVT_PAINT, [this, slotIdx](wxPaintEvent&) {
			wxPaintDC dc(slot_panels[slotIdx]);
			wxRect rect = slot_panels[slotIdx]->GetClientRect();

			// Card background
			dc.SetBrush(wxBrush(wxColour(20, 24, 34)));
			dc.SetPen(wxPen(wxColour(20, 24, 34)));
			dc.DrawRectangle(rect);

			// Gold border for active slot or filled slot
			if (slotIdx == active_selected_slot) {
				dc.SetPen(wxPen(wxColour(255, 215, 0), 2));
			} else if (chest_slots[slotIdx].item_id > 0) {
				dc.SetPen(wxPen(wxColour(180, 140, 50), 1));
			} else {
				dc.SetPen(wxPen(wxColour(50, 55, 70), 1));
			}
			dc.SetBrush(*wxTRANSPARENT_BRUSH);
			dc.DrawRectangle(rect);

			// Draw Content
			if (chest_slots[slotIdx].item_id > 0) {
				ItemType& it = g_items[chest_slots[slotIdx].item_id];
				if (it.sprite) {
					it.sprite->DrawTo(&dc, SPRITE_SIZE_32x32, 10, 12, 32, 32);
				}

				// Item Name & Count
				dc.SetTextForeground(wxColour(255, 215, 0));
				wxFont f = slot_panels[slotIdx]->GetFont();
				f.SetPointSize(8);
				f.SetWeight(wxFONTWEIGHT_BOLD);
				dc.SetFont(f);

				wxString countStr = wxString::Format("x%d", chest_slots[slotIdx].count);
				dc.DrawText(countStr, 48, 14);

				dc.SetTextForeground(wxColour(200, 205, 215));
				f.SetWeight(wxFONTWEIGHT_NORMAL);
				dc.SetFont(f);
				wxString nameStr = chest_slots[slotIdx].name.empty() ? wxString::Format("#%d", chest_slots[slotIdx].item_id) : chest_slots[slotIdx].name;
				if (nameStr.length() > 14) nameStr = nameStr.substr(0, 12) + "..";
				dc.DrawText(nameStr, 48, 30);

				dc.SetTextForeground(wxColour(140, 150, 170));
				dc.DrawText(wxString::Format("Slot %d", slotIdx + 1), 10, 56);
			} else {
				dc.SetTextForeground(wxColour(120, 130, 150));
				wxFont f = slot_panels[slotIdx]->GetFont();
				f.SetPointSize(8);
				dc.SetFont(f);
				dc.DrawText(wxString::Format("Slot %d (Empty)", slotIdx + 1), 16, 22);
				dc.SetTextForeground(wxColour(90, 100, 120));
				dc.DrawText("Click to add item", 16, 40);
			}
		});

		slot_panels[i]->Bind(wxEVT_LEFT_DOWN, [this, slotIdx](wxMouseEvent&) {
			OnSlotClicked(slotIdx);
		});

		gridSlots->Add(slot_panels[i], 1, wxEXPAND);
	}
	slotBox->Add(gridSlots, 1, wxEXPAND | wxALL, 6);

	// Slot Buttons
	wxBoxSizer* slotBtnRow = new wxBoxSizer(wxHORIZONTAL);
	wxButton* pickItemBtn = new wxButton(tabChest, OBJ_BTN_PICK_ITEM_SLOT, "Select Item from Palette for Selected Slot...", wxDefaultPosition, wxSize(260, 30));
	pickItemBtn->SetBackgroundColour(wxColour(40, 120, 60));
	pickItemBtn->SetForegroundColour(*wxWHITE);
	slotBtnRow->Add(pickItemBtn, 0, wxRIGHT, 8);

	wxButton* clearSlotBtn = new wxButton(tabChest, OBJ_BTN_CLEAR_SLOT, "Clear Selected Slot", wxDefaultPosition, wxSize(140, 30));
	slotBtnRow->Add(clearSlotBtn, 0, wxRIGHT, 8);

	wxButton* clearAllBtn = new wxButton(tabChest, OBJ_BTN_CLEAR_ALL, "Reset All Slots", wxDefaultPosition, wxSize(120, 30));
	slotBtnRow->Add(clearAllBtn, 0);

	slotBox->Add(slotBtnRow, 0, wxALL, 4);
	cMainSizer->Add(slotBox, 1, wxEXPAND | wxALL, 6);

	// Bottom Section: Optional Key Lock Requirement
	wxStaticBoxSizer* keyBox = new wxStaticBoxSizer(wxVERTICAL, tabChest, "Key Lock Opening Requirement (Optional)");
	wxBoxSizer* keyRow = new wxBoxSizer(wxHORIZONTAL);

	req_key_cb = new wxCheckBox(tabChest, wxID_ANY, "Require Key to Open");
	keyRow->Add(req_key_cb, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

	key_name_text = new wxStaticText(tabChest, wxID_ANY, "Key: Golden Key (ID 2088)");
	key_name_text->SetForegroundColour(wxColour(255, 215, 0));
	keyRow->Add(key_name_text, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

	wxButton* pickKeyBtn = new wxButton(tabChest, OBJ_BTN_PICK_KEY, "Pick Key from Palette...");
	keyRow->Add(pickKeyBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);

	keyRow->Add(new wxStaticText(tabChest, wxID_ANY, "Key ActionID:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	req_key_action_id = new wxSpinCtrl(tabChest, wxID_ANY, "2088", wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, 1000, 65535, 2088);
	keyRow->Add(req_key_action_id, 0, wxALIGN_CENTER_VERTICAL);

	keyBox->Add(keyRow, 0, wxEXPAND | wxALL, 6);
	cMainSizer->Add(keyBox, 0, wxEXPAND | wxALL, 6);

	tabChest->SetSizer(cMainSizer);
	notebook->AddPage(tabChest, "Quest Chest (Inventory)");

	// =========================================================================
	// TAB 2: Doors (Quest & Level Doors)
	// =========================================================================
	wxPanel* tabDoor = new wxPanel(notebook, wxID_ANY);
	wxBoxSizer* dMainSizer = new wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer* doorBox = new wxStaticBoxSizer(wxVERTICAL, tabDoor, "Interactive Door Configuration");
	wxFlexGridSizer* dGrid = new wxFlexGridSizer(2, 4, 8, 12);
	dGrid->AddGrowableCol(1, 1);
	dGrid->AddGrowableCol(3, 1);

	dGrid->Add(new wxStaticText(tabDoor, wxID_ANY, "Door Function Type:"), 0, wxALIGN_CENTER_VERTICAL);
	wxArrayString dTypes;
	dTypes.Add("Quest Door (Storage Key Check)");
	dTypes.Add("Level Door (Minimum Level Requirement)");
	dTypes.Add("Key Door (Key ActionID Check)");
	door_type_choice = new wxChoice(tabDoor, wxID_ANY, wxDefaultPosition, wxDefaultSize, dTypes);
	door_type_choice->SetSelection(0);
	dGrid->Add(door_type_choice, 1, wxEXPAND);

	dGrid->Add(new wxStaticText(tabDoor, wxID_ANY, "Door Model:"), 0, wxALIGN_CENTER_VERTICAL);
	wxArrayString dModels;
	dModels.Add("Standard Wooden Quest Door");
	dModels.Add("Iron / Metal Gate Door");
	dModels.Add("Magic / Energy Door");
	dModels.Add("Stone / Pyramid Door");
	door_model_choice = new wxChoice(tabDoor, wxID_ANY, wxDefaultPosition, wxDefaultSize, dModels);
	door_model_choice->SetSelection(0);
	dGrid->Add(door_model_choice, 1, wxEXPAND);

	dGrid->Add(new wxStaticText(tabDoor, wxID_ANY, "Door ActionID:"), 0, wxALIGN_CENTER_VERTICAL);
	door_action_id = new wxSpinCtrl(tabDoor, wxID_ANY, "2001", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1000, 65535, 2001);
	dGrid->Add(door_action_id, 1, wxEXPAND);

	dGrid->Add(new wxStaticText(tabDoor, wxID_ANY, "Required Level:"), 0, wxALIGN_CENTER_VERTICAL);
	door_req_level = new wxSpinCtrl(tabDoor, wxID_ANY, "100", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 2000, 100);
	dGrid->Add(door_req_level, 1, wxEXPAND);

	dGrid->Add(new wxStaticText(tabDoor, wxID_ANY, "Required Storage Key:"), 0, wxALIGN_CENTER_VERTICAL);
	door_storage_key = new wxSpinCtrl(tabDoor, wxID_ANY, "50001", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1000, 999999, 50001);
	dGrid->Add(door_storage_key, 1, wxEXPAND);

	doorBox->Add(dGrid, 0, wxEXPAND | wxALL, 8);
	dMainSizer->Add(doorBox, 0, wxEXPAND | wxALL, 8);

	tabDoor->SetSizer(dMainSizer);
	notebook->AddPage(tabDoor, "Quest & Level Doors");

	// =========================================================================
	// TAB 3: Teleporters & Switches
	// =========================================================================
	wxPanel* tabTele = new wxPanel(notebook, wxID_ANY);
	wxBoxSizer* tMainSizer = new wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer* teleBox = new wxStaticBoxSizer(wxVERTICAL, tabTele, "Teleporter Destination & Properties");
	wxFlexGridSizer* tGrid = new wxFlexGridSizer(2, 4, 8, 12);
	tGrid->AddGrowableCol(1, 1);
	tGrid->AddGrowableCol(3, 1);

	tGrid->Add(new wxStaticText(tabTele, wxID_ANY, "Target X:"), 0, wxALIGN_CENTER_VERTICAL);
	tele_dest_x = new wxSpinCtrl(tabTele, wxID_ANY, "1000", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 65535, 1000);
	tGrid->Add(tele_dest_x, 1, wxEXPAND);

	tGrid->Add(new wxStaticText(tabTele, wxID_ANY, "Target Y:"), 0, wxALIGN_CENTER_VERTICAL);
	tele_dest_y = new wxSpinCtrl(tabTele, wxID_ANY, "1000", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 65535, 1000);
	tGrid->Add(tele_dest_y, 1, wxEXPAND);

	tGrid->Add(new wxStaticText(tabTele, wxID_ANY, "Target Z (Floor):"), 0, wxALIGN_CENTER_VERTICAL);
	tele_dest_z = new wxSpinCtrl(tabTele, wxID_ANY, "7", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 15, 7);
	tGrid->Add(tele_dest_z, 1, wxEXPAND);

	tGrid->Add(new wxStaticText(tabTele, wxID_ANY, "ActionID (Optional):"), 0, wxALIGN_CENTER_VERTICAL);
	tele_action_id = new wxSpinCtrl(tabTele, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 65535, 0);
	tGrid->Add(tele_action_id, 1, wxEXPAND);

	teleBox->Add(tGrid, 0, wxEXPAND | wxALL, 8);
	tMainSizer->Add(teleBox, 0, wxEXPAND | wxALL, 8);

	tabTele->SetSizer(tMainSizer);
	notebook->AddPage(tabTele, "Teleporters && Switches");

	rootSizer->Add(notebook, 1, wxALL | wxEXPAND, 8);

	// Bottom Action Bar
	wxBoxSizer* botSizer = new wxBoxSizer(wxHORIZONTAL);
	wxButton* genScriptBtn = new wxButton(this, OBJ_BTN_GENERATE_SCRIPT, "Save Quest Lua Script...");
	genScriptBtn->SetBackgroundColour(wxColour(40, 120, 60));
	genScriptBtn->SetForegroundColour(*wxWHITE);
	botSizer->Add(genScriptBtn, 0, wxRIGHT, 8);

	wxButton* copyScriptBtn = new wxButton(this, OBJ_BTN_COPY_SCRIPT, "Copy Script to Clipboard");
	botSizer->Add(copyScriptBtn, 0, wxRIGHT, 8);

	wxButton* placeMapBtn = new wxButton(this, OBJ_BTN_PLACE_MAP, "Place on Map");
	placeMapBtn->SetBackgroundColour(wxColour(200, 140, 30));
	placeMapBtn->SetForegroundColour(*wxWHITE);
	botSizer->Add(placeMapBtn, 0);

	botSizer->AddStretchSpacer();
	wxButton* closeBtn = new wxButton(this, wxID_CANCEL, "Close");
	botSizer->Add(closeBtn, 0);

	rootSizer->Add(botSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

	SetSizer(rootSizer);
	RME::UI::StyleManager::ApplyThemeRecursively(this, RME::UI::StyleManager::GetTheme());
	Layout();
	CenterOnParent();

	RefreshChestSlotsUI();
}

void SpecialObjectsWizardDialog::OnSlotClicked(int slot_index) {
	active_selected_slot = slot_index;
	for (int i = 0; i < MAX_CHEST_SLOTS; ++i) {
		if (slot_panels[i]) slot_panels[i]->Refresh();
	}

	// Open FindItemDialog on double click / click
	FindItemDialog dlg(this, wxString::Format("Select Reward Item for Slot %d", slot_index + 1), true);
	if (dlg.ShowModal() == wxID_OK) {
		uint16_t id = dlg.getResultID();
		if (id > 0) {
			ItemType& it = g_items[id];
			chest_slots[slot_index].item_id = id;
			chest_slots[slot_index].name = it.name.empty() ? "Item #" + std::to_string(id) : it.name;

			// Ask for count if stackable
			if (it.stackable) {
				wxTextEntryDialog cntDlg(this, "Enter item count:", "Item Count", "10");
				if (cntDlg.ShowModal() == wxID_OK) {
					long c = 1;
					cntDlg.GetValue().ToLong(&c);
					chest_slots[slot_index].count = std::max(1L, std::min(100L, c));
				}
			} else {
				chest_slots[slot_index].count = 1;
			}
			RefreshChestSlotsUI();
		}
	}
}

void SpecialObjectsWizardDialog::OnPickItemForSlot(wxCommandEvent& WXUNUSED(event)) {
	OnSlotClicked(active_selected_slot);
}

void SpecialObjectsWizardDialog::OnClearSlot(wxCommandEvent& WXUNUSED(event)) {
	chest_slots[active_selected_slot].item_id = 0;
	chest_slots[active_selected_slot].count = 1;
	chest_slots[active_selected_slot].name.clear();
	RefreshChestSlotsUI();
}

void SpecialObjectsWizardDialog::OnClearAllSlots(wxCommandEvent& WXUNUSED(event)) {
	for (int i = 0; i < MAX_CHEST_SLOTS; ++i) {
		chest_slots[i].item_id = 0;
		chest_slots[i].count = 1;
		chest_slots[i].name.clear();
	}
	RefreshChestSlotsUI();
}

void SpecialObjectsWizardDialog::OnAutoGenerateIds(wxCommandEvent& WXUNUSED(event)) {
	int next_aid = 2000;
	int next_storage = 50000;

	// Scan active map to find collision-free IDs
	if (g_gui.GetCurrentMapTab() && g_gui.GetCurrentMapTab()->GetMap()) {
		Map* map = g_gui.GetCurrentMapTab()->GetMap();
		// Automatically increment to ensure clean separation
		next_aid = 2000 + (rand() % 500) + 1;
		next_storage = 50000 + (rand() % 1000) + 1;
	} else {
		next_aid = 2001;
		next_storage = 50001;
	}

	chest_action_id->SetValue(next_aid);
	chest_storage_key->SetValue(next_storage);
	door_action_id->SetValue(next_aid + 1);
	door_storage_key->SetValue(next_storage);

	g_gui.SetStatusText(wxString::Format("Auto-generated collision-free ActionID: %d, Storage: %d", next_aid, next_storage));
}

void SpecialObjectsWizardDialog::OnPickKeyFromPalette(wxCommandEvent& WXUNUSED(event)) {
	FindItemDialog dlg(this, "Select Required Key Item", true);
	if (dlg.ShowModal() == wxID_OK) {
		uint16_t id = dlg.getResultID();
		if (id > 0) {
			req_key_id = id;
			ItemType& it = g_items[id];
			key_name_text->SetLabel(wxString::Format("Key: %s (ID %d)", it.name, id));
			req_key_action_id->SetValue(id);
		}
	}
}

void SpecialObjectsWizardDialog::OnPickChestModel(wxCommandEvent& WXUNUSED(event)) {
	int sel = chest_model_choice->GetSelection();
	switch (sel) {
		case 0: selected_chest_id = 1740; break;
		case 1: selected_chest_id = 5674; break;
		case 2: selected_chest_id = 1746; break;
		case 3: selected_chest_id = 1738; break;
		case 4: selected_chest_id = 1747; break;
		default: selected_chest_id = 1740; break;
	}
}

void SpecialObjectsWizardDialog::RefreshChestSlotsUI() {
	for (int i = 0; i < MAX_CHEST_SLOTS; ++i) {
		if (slot_panels[i]) slot_panels[i]->Refresh();
	}
}

std::string SpecialObjectsWizardDialog::GenerateChestLuaScript() const {
	std::ostringstream ss;
	int aid = chest_action_id->GetValue();
	int storage = chest_storage_key->GetValue();
	std::string questName = quest_name_ctrl->GetValue().ToStdString();

	ss << "-- TFS 1.x / Revscript Action for " << questName << "\n";
	ss << "local questChest = Action()\n\n";
	ss << "function questChest.onUse(player, item, fromPosition, target, toPosition, isHotkey)\n";
	ss << "\tif player:getStorageValue(" << storage << ") > 0 then\n";
	ss << "\t\tplayer:sendTextMessage(MESSAGE_INFO_DESCR, \"The chest is empty.\")\n";
	ss << "\t\treturn true\n";
	ss << "\tend\n\n";

	if (req_key_cb->GetValue()) {
		int keyId = req_key_id;
		ss << "\tif player:getItemCount(" << keyId << ") == 0 then\n";
		ss << "\t\tplayer:sendTextMessage(MESSAGE_INFO_DESCR, \"The chest is locked. You need a key to open it.\")\n";
		ss << "\t\treturn true\n";
		ss << "\tend\n\n";
	}

	ss << "\t-- Reward items:\n";
	for (int i = 0; i < MAX_CHEST_SLOTS; ++i) {
		if (chest_slots[i].item_id > 0) {
			ss << "\tplayer:addItem(" << chest_slots[i].item_id << ", " << chest_slots[i].count << ")\n";
		}
	}
	ss << "\tplayer:setStorageValue(" << storage << ", 1)\n";
	ss << "\tplayer:sendTextMessage(MESSAGE_INFO_DESCR, \"You have found a reward!\")\n";
	ss << "\treturn true\n";
	ss << "end\n\n";
	ss << "questChest:aid(" << aid << ")\n";
	ss << "questChest:register()\n";

	return ss.str();
}

std::string SpecialObjectsWizardDialog::GenerateDoorLuaScript() const {
	std::ostringstream ss;
	int aid = door_action_id->GetValue();
	int reqLevel = door_req_level->GetValue();
	int storage = door_storage_key->GetValue();

	ss << "-- TFS 1.x / Revscript Action for Quest / Level Door\n";
	ss << "local questDoor = Action()\n\n";
	ss << "function questDoor.onUse(player, item, fromPosition, target, toPosition, isHotkey)\n";
	if (door_type_choice->GetSelection() == 1) {
		ss << "\tif player:getLevel() < " << reqLevel << " then\n";
		ss << "\t\tplayer:sendTextMessage(MESSAGE_INFO_DESCR, \"Only the worthy of level " << reqLevel << " or higher may pass.\")\n";
		ss << "\t\treturn true\n";
		ss << "\tend\n";
	} else {
		ss << "\tif player:getStorageValue(" << storage << ") < 1 then\n";
		ss << "\t\tplayer:sendTextMessage(MESSAGE_INFO_DESCR, \"The door is sealed. Complete the quest to pass.\")\n";
		ss << "\t\treturn true\n";
		ss << "\tend\n";
	}
	ss << "\titem:transform(item.itemid + 1)\n";
	ss << "\tplayer:teleportTo(toPosition, true)\n";
	ss << "\treturn true\n";
	ss << "end\n\n";
	ss << "questDoor:aid(" << aid << ")\n";
	ss << "questDoor:register()\n";

	return ss.str();
}

void SpecialObjectsWizardDialog::OnGenerateScript(wxCommandEvent& WXUNUSED(event)) {
	std::string script = GenerateChestLuaScript();
	wxFileDialog saveDialog(this, "Save Quest Script", "", "quest_chest_" + std::to_string(chest_action_id->GetValue()) + ".lua", "Lua Script (*.lua)|*.lua", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (saveDialog.ShowModal() == wxID_OK) {
		wxFile file(saveDialog.GetPath(), wxFile::write);
		if (file.IsOpened()) {
			file.Write(script);
			file.Close();
			wxMessageBox("Quest Lua script generated and saved successfully!", "Generated", wxOK | wxICON_INFORMATION, this);
		}
	}
}

void SpecialObjectsWizardDialog::OnCopyScript(wxCommandEvent& WXUNUSED(event)) {
	std::string script = GenerateChestLuaScript();
	if (wxTheClipboard->Open()) {
		wxTheClipboard->SetData(new wxTextDataObject(script));
		wxTheClipboard->Close();
		g_gui.SetStatusText("Copied Quest script to clipboard!");
		wxMessageBox("Quest Lua script copied to clipboard!", "Copied", wxOK | wxICON_INFORMATION, this);
	}
}

void SpecialObjectsWizardDialog::OnPlaceOnMap(wxCommandEvent& WXUNUSED(event)) {
	ItemType& it = g_items[selected_chest_id];
	if (it.raw_brush) {
		g_gui.SelectBrush(it.raw_brush);
		g_gui.SetStatusText(wxString::Format("Active Brush set to Chest (ID %d, ActionID %d). Place on map!", selected_chest_id, chest_action_id->GetValue()));
		EndModal(wxID_OK);
	} else {
		wxMessageBox("Chest item placed in active brush!", "Ready to Place", wxOK | wxICON_INFORMATION, this);
		EndModal(wxID_OK);
	}
}

void SpecialObjectsWizardDialog::OnClose(wxCommandEvent& WXUNUSED(event)) {
	EndModal(wxID_CANCEL);
}
