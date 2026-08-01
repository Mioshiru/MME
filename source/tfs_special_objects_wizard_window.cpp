#include "tfs_special_objects_wizard_window.h"
#include "gui.h"
#include "editor.h"
#include "items.h"
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/wfstream.h>
#include <sstream>

enum {
	OBJ_WIZARD_BTN_GENERATE = wxID_HIGHEST + 400,
	OBJ_WIZARD_CONTAINER_CHOICE,
	OBJ_WIZARD_ADD_ITEM,
	OBJ_WIZARD_REMOVE_ITEM
};

BEGIN_EVENT_TABLE(SpecialObjectsWizardDialog, wxDialog)
EVT_CHOICE(OBJ_WIZARD_CONTAINER_CHOICE, SpecialObjectsWizardDialog::OnContainerChoiceChanged)
EVT_BUTTON(OBJ_WIZARD_ADD_ITEM, SpecialObjectsWizardDialog::OnAddContainerItem)
EVT_BUTTON(OBJ_WIZARD_REMOVE_ITEM, SpecialObjectsWizardDialog::OnRemoveContainerItem)
EVT_BUTTON(OBJ_WIZARD_BTN_GENERATE, SpecialObjectsWizardDialog::OnGenerate)
EVT_BUTTON(wxID_CANCEL, SpecialObjectsWizardDialog::OnClose)
END_EVENT_TABLE()

SpecialObjectsWizardDialog::SpecialObjectsWizardDialog(wxWindow* parent) :
	wxDialog(parent, wxID_ANY, "Special Objects Wizard (Doors & Containers)", wxDefaultPosition, wxSize(600, 560), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {

	wxSizer* topsizer = new wxBoxSizer(wxVERTICAL);

	// Header Panel
	wxPanel* headerPanel = new wxPanel(this, wxID_ANY);
	headerPanel->SetBackgroundColour(wxColour(40, 42, 48));
	wxSizer* headerSizer = new wxBoxSizer(wxVERTICAL);

	wxStaticText* header = new wxStaticText(headerPanel, wxID_ANY, "Special Objects Creation Wizard");
	wxFont font = header->GetFont();
	font.SetPointSize(12);
	font.SetWeight(wxFONTWEIGHT_BOLD);
	header->SetFont(font);
	header->SetForegroundColour(wxColour(220, 180, 80));
	headerSizer->Add(header, 0, wxALL, 8);

	wxStaticText* subheader = new wxStaticText(headerPanel, wxID_ANY, "Configure Doors and Container Chests. Container slot limits are automatically enforced based on container type.");
	subheader->SetForegroundColour(wxColour(180, 185, 195));
	headerSizer->Add(subheader, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

	headerPanel->SetSizer(headerSizer);
	topsizer->Add(headerPanel, 0, wxEXPAND);

	notebook = new wxNotebook(this, wxID_ANY);

	// ==========================================
	// --- Tab 1: Doors ---
	// ==========================================
	wxPanel* doorsPanel = new wxPanel(notebook);
	wxFlexGridSizer* dSizer = new wxFlexGridSizer(2, 5, 10);
	dSizer->AddGrowableCol(1, 1);

	dSizer->Add(new wxStaticText(doorsPanel, wxID_ANY, "Door Type:"), 0, wxALIGN_CENTER_VERTICAL);
	wxArrayString doorTypes;
	doorTypes.Add("Quest Door (Storage Key Check)");
	doorTypes.Add("Level Door (Minimum Level Requirement)");
	doorTypes.Add("Key Door (Key ActionID 2088-2092)");
	door_type_choice = new wxChoice(doorsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, doorTypes);
	door_type_choice->SetSelection(0);
	dSizer->Add(door_type_choice, 1, wxEXPAND);

	dSizer->Add(new wxStaticText(doorsPanel, wxID_ANY, "Door ActionID:"), 0, wxALIGN_CENTER_VERTICAL);
	door_action_id = new wxSpinCtrl(doorsPanel, wxID_ANY, "2000", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1000, 65535, 2000);
	dSizer->Add(door_action_id, 1, wxEXPAND);

	dSizer->Add(new wxStaticText(doorsPanel, wxID_ANY, "Required Level (for Level Doors):"), 0, wxALIGN_CENTER_VERTICAL);
	door_req_level = new wxSpinCtrl(doorsPanel, wxID_ANY, "100", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 2000, 100);
	dSizer->Add(door_req_level, 1, wxEXPAND);

	dSizer->Add(new wxStaticText(doorsPanel, wxID_ANY, "Quest Storage Key (for Quest Doors):"), 0, wxALIGN_CENTER_VERTICAL);
	door_quest_storage = new wxSpinCtrl(doorsPanel, wxID_ANY, "50001", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 999999, 50001);
	dSizer->Add(door_quest_storage, 1, wxEXPAND);

	wxBoxSizer* dBox = new wxBoxSizer(wxVERTICAL);
	dBox->Add(dSizer, 1, wxALL | wxEXPAND, 12);
	doorsPanel->SetSizer(dBox);

	// ==========================================
	// --- Tab 2: Container (Quest Chests) ---
	// ==========================================
	wxPanel* containerPanel = new wxPanel(notebook);
	wxBoxSizer* cMainSizer = new wxBoxSizer(wxVERTICAL);

	wxFlexGridSizer* cSizer = new wxFlexGridSizer(2, 5, 10);
	cSizer->AddGrowableCol(1, 1);

	cSizer->Add(new wxStaticText(containerPanel, wxID_ANY, "Container Type:"), 0, wxALIGN_CENTER_VERTICAL);
	container_type_choice = new wxChoice(containerPanel, OBJ_WIZARD_CONTAINER_CHOICE);
	PopulateContainerChoices(container_type_choice);
	cSizer->Add(container_type_choice, 1, wxEXPAND);

	cSizer->Add(new wxStaticText(containerPanel, wxID_ANY, "Chest ActionID:"), 0, wxALIGN_CENTER_VERTICAL);
	chest_action_id = new wxSpinCtrl(containerPanel, wxID_ANY, "2000", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1000, 65535, 2000);
	cSizer->Add(chest_action_id, 1, wxEXPAND);

	cSizer->Add(new wxStaticText(containerPanel, wxID_ANY, "Quest Storage Key:"), 0, wxALIGN_CENTER_VERTICAL);
	chest_storage_key = new wxSpinCtrl(containerPanel, wxID_ANY, "50001", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 999999, 50001);
	cSizer->Add(chest_storage_key, 1, wxEXPAND);

	cMainSizer->Add(cSizer, 0, wxALL | wxEXPAND, 10);

	container_slot_info = new wxStaticText(containerPanel, wxID_ANY, "Capacity: 0 / 8 Slots Used");
	container_slot_info->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	container_slot_info->SetForegroundColour(wxColour(220, 180, 80));
	cMainSizer->Add(container_slot_info, 0, wxLEFT | wxRIGHT | wxTOP, 10);

	wxFlexGridSizer* itemAddSizer = new wxFlexGridSizer(2, 5, 10);
	itemAddSizer->AddGrowableCol(1, 1);

	itemAddSizer->Add(new wxStaticText(containerPanel, wxID_ANY, "Select Reward Item:"), 0, wxALIGN_CENTER_VERTICAL);
	item_picker_choice = new wxChoice(containerPanel, wxID_ANY);
	PopulateItemChoices(item_picker_choice);
	itemAddSizer->Add(item_picker_choice, 1, wxEXPAND);

	itemAddSizer->Add(new wxStaticText(containerPanel, wxID_ANY, "Item Count:"), 0, wxALIGN_CENTER_VERTICAL);
	item_picker_count = new wxSpinCtrl(containerPanel, wxID_ANY, "1", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 1);
	itemAddSizer->Add(item_picker_count, 1, wxEXPAND);

	cMainSizer->Add(itemAddSizer, 0, wxALL | wxEXPAND, 10);

	wxBoxSizer* cBtnSizer = new wxBoxSizer(wxHORIZONTAL);
	cBtnSizer->Add(new wxButton(containerPanel, OBJ_WIZARD_ADD_ITEM, "+ Add Item to Chest"), 0, wxRIGHT, 5);
	cBtnSizer->Add(new wxButton(containerPanel, OBJ_WIZARD_REMOVE_ITEM, "- Remove Selected"), 0);
	cMainSizer->Add(cBtnSizer, 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT | wxBOTTOM, 8);

	container_items_list = new wxListView(containerPanel, wxID_ANY, wxDefaultPosition, wxSize(-1, 120));
	container_items_list->AppendColumn("Slot", wxLIST_FORMAT_LEFT, 50);
	container_items_list->AppendColumn("Item Name", wxLIST_FORMAT_LEFT, 240);
	container_items_list->AppendColumn("Count", wxLIST_FORMAT_RIGHT, 80);
	cMainSizer->Add(container_items_list, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	containerPanel->SetSizer(cMainSizer);

	notebook->AddPage(doorsPanel, "Doors");
	notebook->AddPage(containerPanel, "Containers (Chests)");

	topsizer->Add(notebook, 1, wxEXPAND | wxALL, 10);

	wxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	buttonSizer->Add(new wxButton(this, OBJ_WIZARD_BTN_GENERATE, "Generate & Export Scripts..."), 0, wxRIGHT, 10);
	buttonSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0);

	topsizer->Add(buttonSizer, 0, wxALIGN_RIGHT | wxALL, 12);
	SetSizerAndFit(topsizer);
}

void SpecialObjectsWizardDialog::PopulateContainerChoices(wxChoice* choice) {
	choice->Clear();
	choice->Append("Chest (8 Slots)");
	choice->Append("Pirate Chest (10 Slots)");
	choice->Append("Gold Chest (12 Slots)");
	choice->Append("Backpack (20 Slots)");
	choice->Append("Bag (8 Slots)");
	choice->SetSelection(0);
	max_container_capacity = 8;
}

void SpecialObjectsWizardDialog::PopulateItemChoices(wxChoice* choice) {
	choice->Clear();
	choice->Append("Crystal Coin (2160)");
	choice->Append("Platinum Coin (2152)");
	choice->Append("Gold Coin (2148)");
	choice->Append("Magic Sword (2400)");
	choice->Append("Dragon Shield (2516)");
	choice->Append("Demon Helmet (2493)");
	choice->Append("Boots of Haste (2195)");
	choice->SetSelection(0);
}

void SpecialObjectsWizardDialog::OnContainerChoiceChanged(wxCommandEvent& WXUNUSED(event)) {
	int sel = container_type_choice->GetSelection();
	if (sel == 0) max_container_capacity = 8;
	else if (sel == 1) max_container_capacity = 10;
	else if (sel == 2) max_container_capacity = 12;
	else if (sel == 3) max_container_capacity = 20;
	else if (sel == 4) max_container_capacity = 8;

	container_slot_info->SetLabel(wxString::Format("Capacity: %zu / %d Slots Used", container_items.size(), max_container_capacity));
}

void SpecialObjectsWizardDialog::OnAddContainerItem(wxCommandEvent& WXUNUSED(event)) {
	if (container_items.size() >= (size_t)max_container_capacity) {
		wxMessageBox(wxString::Format("Container is full! Maximum capacity of %d slots reached.", max_container_capacity), "Slot Limit Reached", wxOK | wxICON_WARNING, this);
		return;
	}

	int sel = item_picker_choice->GetSelection();
	if (sel != wxNOT_FOUND) {
		wxString itemName = item_picker_choice->GetString(sel);
		int count = item_picker_count->GetValue();

		ContainerItemEntry entry{2160, itemName.ToStdString(), count};
		container_items.push_back(entry);

		long index = container_items_list->InsertItem(container_items_list->GetItemCount(), wxString::Format("#%zu", container_items.size()));
		container_items_list->SetItem(index, 1, itemName);
		container_items_list->SetItem(index, 2, wxString::Format("%d", count));

		container_slot_info->SetLabel(wxString::Format("Capacity: %zu / %d Slots Used", container_items.size(), max_container_capacity));
	}
}

void SpecialObjectsWizardDialog::OnRemoveContainerItem(wxCommandEvent& WXUNUSED(event)) {
	long selected = container_items_list->GetFirstSelected();
	if (selected != -1 && selected < (long)container_items.size()) {
		container_items_list->DeleteItem(selected);
		container_items.erase(container_items.begin() + selected);
		container_slot_info->SetLabel(wxString::Format("Capacity: %zu / %d Slots Used", container_items.size(), max_container_capacity));
	}
}

void SpecialObjectsWizardDialog::OnGenerate(wxCommandEvent& WXUNUSED(event)) {
	int sel = notebook->GetSelection();
	std::ostringstream lua;

	if (sel == 0) {
		int aid = door_action_id->GetValue();
		int reqLvl = door_req_level->GetValue();
		lua << "-- TFS 1.6 Level Door Action Script (ActionID " << aid << ")\n";
		lua << "local action = Action()\n";
		lua << "function action.onUse(player, item, fromPosition, target, toPosition, isHotkey)\n";
		lua << "    if player:getLevel() < " << reqLvl << " then\n";
		lua << "        player:sendTextMessage(MESSAGE_EVENT_ADVANCE, 'Only the worthy of level " << reqLvl << " may pass.')\n";
		lua << "        return true\n";
		lua << "    end\n";
		lua << "    item:transform(item:getId() + 1)\n";
		lua << "    return true\n";
		lua << "end\n";
		lua << "action:aid(" << aid << ")\n";
		lua << "action:register()\n";
	} else {
		int aid = chest_action_id->GetValue();
		int storage = chest_storage_key->GetValue();

		lua << "-- TFS 1.6 Quest Chest Action Script (ActionID " << aid << ")\n";
		lua << "local action = Action()\n";
		lua << "function action.onUse(player, item, fromPosition, target, toPosition, isHotkey)\n";
		lua << "    if player:getStorageValue(" << storage << ") > 0 then\n";
		lua << "        player:sendTextMessage(MESSAGE_EVENT_ADVANCE, 'It is empty.')\n";
		lua << "        return true\n";
		lua << "    end\n";
		lua << "    player:setStorageValue(" << storage << ", 1)\n";
		for (const auto& entry : container_items) {
			lua << "    player:addItem(" << entry.id << ", " << entry.count << ")\n";
		}
		lua << "    player:sendTextMessage(MESSAGE_EVENT_ADVANCE, 'You have found a reward!')\n";
		lua << "    return true\n";
		lua << "end\n";
		lua << "action:aid(" << aid << ")\n";
		lua << "action:register()\n";
	}

	wxFileDialog saveDialog(this, "Save TFS 1.6 Action Script", "", "special_object_script.lua", "LUA Scripts (*.lua)|*.lua", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (saveDialog.ShowModal() == wxID_OK) {
		wxFileOutputStream output(saveDialog.GetPath());
		if (output.IsOk()) {
			output.Write(lua.str().c_str(), lua.str().length());
			wxMessageBox("TFS 1.6 Action script generated successfully!", "Success", wxOK | wxICON_INFORMATION, this);
			EndModal(wxID_OK);
		}
	}
}

void SpecialObjectsWizardDialog::OnClose(wxCommandEvent& WXUNUSED(event)) {
	EndModal(wxID_CANCEL);
}
