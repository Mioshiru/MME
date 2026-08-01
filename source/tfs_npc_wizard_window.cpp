#include "tfs_npc_wizard_window.h"
#include "gui.h"
#include "editor.h"
#include "items.h"
#include "town.h"
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/wfstream.h>
#include <wx/textdlg.h>
#include <sstream>

enum {
	NPC_WIZARD_BTN_GENERATE = wxID_HIGHEST + 300,
	NPC_WIZARD_INTERACT_TYPE,
	NPC_WIZARD_ADD_SHOP_ITEM,
	NPC_WIZARD_REMOVE_SHOP_ITEM,
	NPC_WIZARD_ADD_DIALOGUE,
	NPC_WIZARD_REMOVE_DIALOGUE
};

BEGIN_EVENT_TABLE(NPCWizardDialog, wxDialog)
EVT_CHOICE(NPC_WIZARD_INTERACT_TYPE, NPCWizardDialog::OnInteractTypeChanged)
EVT_BUTTON(NPC_WIZARD_ADD_SHOP_ITEM, NPCWizardDialog::OnAddShopItem)
EVT_BUTTON(NPC_WIZARD_REMOVE_SHOP_ITEM, NPCWizardDialog::OnRemoveShopItem)
EVT_BUTTON(NPC_WIZARD_ADD_DIALOGUE, NPCWizardDialog::OnAddDialogueNode)
EVT_BUTTON(NPC_WIZARD_REMOVE_DIALOGUE, NPCWizardDialog::OnRemoveDialogueNode)
EVT_BUTTON(NPC_WIZARD_BTN_GENERATE, NPCWizardDialog::OnGenerate)
EVT_BUTTON(wxID_CANCEL, NPCWizardDialog::OnClose)
END_EVENT_TABLE()

NPCWizardDialog::NPCWizardDialog(wxWindow* parent) :
	wxDialog(parent, wxID_ANY, "TFS 1.6 NPC Wizard", wxDefaultPosition, wxSize(640, 600), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {

	wxSizer* topsizer = new wxBoxSizer(wxVERTICAL);

	// Header Panel
	wxPanel* headerPanel = new wxPanel(this, wxID_ANY);
	headerPanel->SetBackgroundColour(wxColour(40, 42, 48));
	wxSizer* headerSizer = new wxBoxSizer(wxVERTICAL);

	wxStaticText* header = new wxStaticText(headerPanel, wxID_ANY, "NPC Creation Wizard (TFS 1.6)");
	wxFont font = header->GetFont();
	font.SetPointSize(12);
	font.SetWeight(wxFONTWEIGHT_BOLD);
	header->SetFont(font);
	header->SetForegroundColour(wxColour(220, 180, 80));
	headerSizer->Add(header, 0, wxALL, 8);

	wxStaticText* subheader = new wxStaticText(headerPanel, wxID_ANY, "Select items and towns directly from map data. Build dialogue trees and shop price lists.");
	subheader->SetForegroundColour(wxColour(180, 185, 195));
	headerSizer->Add(subheader, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

	headerPanel->SetSizer(headerSizer);
	topsizer->Add(headerPanel, 0, wxEXPAND);

	notebook = new wxNotebook(this, wxID_ANY);

	// ==========================================
	// --- Tab 1: Quest NPC ---
	// ==========================================
	wxPanel* questPanel = new wxPanel(notebook);
	wxBoxSizer* qMainSizer = new wxBoxSizer(wxVERTICAL);
	wxFlexGridSizer* qSizer = new wxFlexGridSizer(2, 5, 10);
	qSizer->AddGrowableCol(1, 1);

	qSizer->Add(new wxStaticText(questPanel, wxID_ANY, "NPC Name:"), 0, wxALIGN_CENTER_VERTICAL);
	quest_npc_name = new wxTextCtrl(questPanel, wxID_ANY, "Sir Ethan");
	qSizer->Add(quest_npc_name, 1, wxEXPAND);

	qSizer->Add(new wxStaticText(questPanel, wxID_ANY, "Quest Storage Key:"), 0, wxALIGN_CENTER_VERTICAL);
	quest_id_ctrl = new wxSpinCtrl(questPanel, wxID_ANY, "50001", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 999999, 50001);
	qSizer->Add(quest_id_ctrl, 1, wxEXPAND);

	qSizer->Add(new wxStaticText(questPanel, wxID_ANY, "Required Quest Value:"), 0, wxALIGN_CENTER_VERTICAL);
	quest_value_ctrl = new wxSpinCtrl(questPanel, wxID_ANY, "1", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 99999, 1);
	qSizer->Add(quest_value_ctrl, 1, wxEXPAND);

	qSizer->Add(new wxStaticText(questPanel, wxID_ANY, "EXP Reward:"), 0, wxALIGN_CENTER_VERTICAL);
	quest_reward_exp = new wxSpinCtrl(questPanel, wxID_ANY, "5000", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10000000, 5000);
	qSizer->Add(quest_reward_exp, 1, wxEXPAND);

	qSizer->Add(new wxStaticText(questPanel, wxID_ANY, "Gold Reward (0 = Free):"), 0, wxALIGN_CENTER_VERTICAL);
	quest_reward_gold = new wxSpinCtrl(questPanel, wxID_ANY, "1000", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 1000000, 1000);
	qSizer->Add(quest_reward_gold, 1, wxEXPAND);

	qSizer->Add(new wxStaticText(questPanel, wxID_ANY, "Item Reward:"), 0, wxALIGN_CENTER_VERTICAL);
	quest_reward_item_choice = new wxChoice(questPanel, wxID_ANY);
	PopulateItemChoices(quest_reward_item_choice);
	qSizer->Add(quest_reward_item_choice, 1, wxEXPAND);

	qSizer->Add(new wxStaticText(questPanel, wxID_ANY, "Item Count:"), 0, wxALIGN_CENTER_VERTICAL);
	quest_reward_count = new wxSpinCtrl(questPanel, wxID_ANY, "1", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 1);
	qSizer->Add(quest_reward_count, 1, wxEXPAND);

	qMainSizer->Add(qSizer, 0, wxALL | wxEXPAND, 10);

	wxStaticText* treeLabel = new wxStaticText(questPanel, wxID_ANY, "Dialogue Tree Steps:");
	treeLabel->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	qMainSizer->Add(treeLabel, 0, wxLEFT | wxRIGHT | wxTOP, 10);

	quest_dialogue_tree = new wxTreeCtrl(questPanel, wxID_ANY, wxDefaultPosition, wxSize(-1, 120), wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT);
	wxTreeItemId rootId = quest_dialogue_tree->AddRoot("Root");
	wxTreeItemId greetId = quest_dialogue_tree->AppendItem(rootId, "Greeting: Hello adventurer! Do you need a task?");
	wxTreeItemId questNode = quest_dialogue_tree->AppendItem(greetId, "Keyword 'task': Bring me 5 potions!");
	quest_dialogue_tree->AppendItem(questNode, "Keyword 'yes': Great! Here is your quest.");
	quest_dialogue_tree->ExpandAll();

	qMainSizer->Add(quest_dialogue_tree, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	wxBoxSizer* qBtnSizer = new wxBoxSizer(wxHORIZONTAL);
	qBtnSizer->Add(new wxButton(questPanel, NPC_WIZARD_ADD_DIALOGUE, "+ Add Step"), 0, wxRIGHT, 5);
	qBtnSizer->Add(new wxButton(questPanel, NPC_WIZARD_REMOVE_DIALOGUE, "- Remove Step"), 0);
	qMainSizer->Add(qBtnSizer, 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	questPanel->SetSizer(qMainSizer);

	// ==========================================
	// --- Tab 2: Interaction NPC ---
	// ==========================================
	wxPanel* interactPanel = new wxPanel(notebook);
	wxBoxSizer* iMainSizer = new wxBoxSizer(wxVERTICAL);

	wxFlexGridSizer* iTopSizer = new wxFlexGridSizer(2, 5, 10);
	iTopSizer->AddGrowableCol(1, 1);

	iTopSizer->Add(new wxStaticText(interactPanel, wxID_ANY, "NPC Name:"), 0, wxALIGN_CENTER_VERTICAL);
	interact_npc_name = new wxTextCtrl(interactPanel, wxID_ANY, "Captain Jack");
	iTopSizer->Add(interact_npc_name, 1, wxEXPAND);

	iTopSizer->Add(new wxStaticText(interactPanel, wxID_ANY, "NPC Function:"), 0, wxALIGN_CENTER_VERTICAL);
	wxArrayString types;
	types.Add("Ship Captain (Travel Destination)");
	types.Add("Shop Merchant (Buy / Sell Items)");
	types.Add("Healer (Restore HP & Mana)");
	types.Add("Town Temple (Set Home Town)");
	interact_type_choice = new wxChoice(interactPanel, NPC_WIZARD_INTERACT_TYPE, wxDefaultPosition, wxDefaultSize, types);
	interact_type_choice->SetSelection(0);
	iTopSizer->Add(interact_type_choice, 1, wxEXPAND);

	iMainSizer->Add(iTopSizer, 0, wxALL | wxEXPAND, 10);

	// Dynamic sub-panels for Interaction NPC
	// 1. Ship Panel
	ship_panel = new wxPanel(interactPanel);
	wxFlexGridSizer* shipSizer = new wxFlexGridSizer(2, 5, 10);
	shipSizer->AddGrowableCol(1, 1);
	shipSizer->Add(new wxStaticText(ship_panel, wxID_ANY, "Destination Town:"), 0, wxALIGN_CENTER_VERTICAL);
	ship_town_choice = new wxChoice(ship_panel, wxID_ANY);
	PopulateTownChoices(ship_town_choice);
	shipSizer->Add(ship_town_choice, 1, wxEXPAND);

	shipSizer->Add(new wxStaticText(ship_panel, wxID_ANY, "Ticket Price (0 = Free):"), 0, wxALIGN_CENTER_VERTICAL);
	ship_cost_ctrl = new wxSpinCtrl(ship_panel, wxID_ANY, "110", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100000, 110);
	shipSizer->Add(ship_cost_ctrl, 1, wxEXPAND);
	ship_panel->SetSizer(shipSizer);

	// 2. Shop Panel
	shop_panel = new wxPanel(interactPanel);
	wxBoxSizer* shopSizer = new wxBoxSizer(wxVERTICAL);
	wxFlexGridSizer* shopAddSizer = new wxFlexGridSizer(2, 5, 10);
	shopAddSizer->AddGrowableCol(1, 1);

	shopAddSizer->Add(new wxStaticText(shop_panel, wxID_ANY, "Select Item:"), 0, wxALIGN_CENTER_VERTICAL);
	shop_item_choice = new wxChoice(shop_panel, wxID_ANY);
	PopulateItemChoices(shop_item_choice);
	shopAddSizer->Add(shop_item_choice, 1, wxEXPAND);

	shopAddSizer->Add(new wxStaticText(shop_panel, wxID_ANY, "Buy Price (0 = Free):"), 0, wxALIGN_CENTER_VERTICAL);
	shop_buy_price = new wxSpinCtrl(shop_panel, wxID_ANY, "100", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 1000000, 100);
	shopAddSizer->Add(shop_buy_price, 1, wxEXPAND);

	shopAddSizer->Add(new wxStaticText(shop_panel, wxID_ANY, "Sell Price (0 = N/A):"), 0, wxALIGN_CENTER_VERTICAL);
	shop_sell_price = new wxSpinCtrl(shop_panel, wxID_ANY, "50", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 1000000, 50);
	shopAddSizer->Add(shop_sell_price, 1, wxEXPAND);

	shopSizer->Add(shopAddSizer, 0, wxEXPAND | wxBOTTOM, 8);

	wxBoxSizer* shopBtnSizer = new wxBoxSizer(wxHORIZONTAL);
	shopBtnSizer->Add(new wxButton(shop_panel, NPC_WIZARD_ADD_SHOP_ITEM, "+ Add to Shop"), 0, wxRIGHT, 5);
	shopBtnSizer->Add(new wxButton(shop_panel, NPC_WIZARD_REMOVE_SHOP_ITEM, "- Remove Selected"), 0);
	shopSizer->Add(shopBtnSizer, 0, wxALIGN_RIGHT | wxBOTTOM, 8);

	shop_items_list = new wxListView(shop_panel, wxID_ANY, wxDefaultPosition, wxSize(-1, 120));
	shop_items_list->AppendColumn("Item Name", wxLIST_FORMAT_LEFT, 180);
	shop_items_list->AppendColumn("Buy Price", wxLIST_FORMAT_RIGHT, 80);
	shop_items_list->AppendColumn("Sell Price", wxLIST_FORMAT_RIGHT, 80);
	shopSizer->Add(shop_items_list, 1, wxEXPAND);

	shop_panel->SetSizer(shopSizer);

	// 3. Heal Panel
	heal_panel = new wxPanel(interactPanel);
	wxFlexGridSizer* healSizer = new wxFlexGridSizer(2, 5, 10);
	healSizer->AddGrowableCol(1, 1);
	healSizer->Add(new wxStaticText(heal_panel, wxID_ANY, "Healing Price (0 = Free):"), 0, wxALIGN_CENTER_VERTICAL);
	heal_cost_ctrl = new wxSpinCtrl(heal_panel, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10000, 0);
	healSizer->Add(heal_cost_ctrl, 1, wxEXPAND);
	heal_panel->SetSizer(healSizer);

	// 4. Temple Panel
	temple_panel = new wxPanel(interactPanel);
	wxFlexGridSizer* templeSizer = new wxFlexGridSizer(2, 5, 10);
	templeSizer->AddGrowableCol(1, 1);
	templeSizer->Add(new wxStaticText(temple_panel, wxID_ANY, "Home Town:"), 0, wxALIGN_CENTER_VERTICAL);
	temple_town_choice = new wxChoice(temple_panel, wxID_ANY);
	PopulateTownChoices(temple_town_choice);
	templeSizer->Add(temple_town_choice, 1, wxEXPAND);
	temple_panel->SetSizer(templeSizer);

	iMainSizer->Add(ship_panel, 1, wxEXPAND | wxALL, 10);
	iMainSizer->Add(shop_panel, 1, wxEXPAND | wxALL, 10);
	iMainSizer->Add(heal_panel, 1, wxEXPAND | wxALL, 10);
	iMainSizer->Add(temple_panel, 1, wxEXPAND | wxALL, 10);

	shop_panel->Hide();
	heal_panel->Hide();
	temple_panel->Hide();

	interactPanel->SetSizer(iMainSizer);

	// ==========================================
	// --- Tab 3: Information NPC ---
	// ==========================================
	wxPanel* infoPanel = new wxPanel(notebook);
	wxBoxSizer* infoMainSizer = new wxBoxSizer(wxVERTICAL);

	wxFlexGridSizer* infoTopSizer = new wxFlexGridSizer(2, 5, 10);
	infoTopSizer->AddGrowableCol(1, 1);

	infoTopSizer->Add(new wxStaticText(infoPanel, wxID_ANY, "Guard Name:"), 0, wxALIGN_CENTER_VERTICAL);
	info_npc_name = new wxTextCtrl(infoPanel, wxID_ANY, "Guard Thomas");
	infoTopSizer->Add(info_npc_name, 1, wxEXPAND);

	infoTopSizer->Add(new wxStaticText(infoPanel, wxID_ANY, "Guarded Town:"), 0, wxALIGN_CENTER_VERTICAL);
	info_town_choice = new wxChoice(infoPanel, wxID_ANY);
	PopulateTownChoices(info_town_choice);
	infoTopSizer->Add(info_town_choice, 1, wxEXPAND);

	infoMainSizer->Add(infoTopSizer, 0, wxALL | wxEXPAND, 10);

	wxStaticText* infoTreeLabel = new wxStaticText(infoPanel, wxID_ANY, "Information Dialogue Topics:");
	infoTreeLabel->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	infoMainSizer->Add(infoTreeLabel, 0, wxLEFT | wxRIGHT | wxTOP, 10);

	info_dialogue_tree = new wxTreeCtrl(infoPanel, wxID_ANY, wxDefaultPosition, wxSize(-1, 160), wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT);
	wxTreeItemId iRootId = info_dialogue_tree->AddRoot("Root");
	wxTreeItemId iGreetId = info_dialogue_tree->AppendItem(iRootId, "Greeting: Welcome to our city!");
	info_dialogue_tree->AppendItem(iGreetId, "Topic 'job': I guard the city gates.");
	info_dialogue_tree->AppendItem(iGreetId, "Topic 'shop': The weapons shop is to the east.");
	info_dialogue_tree->AppendItem(iGreetId, "Topic 'king': King Tibianus rules our realm.");
	info_dialogue_tree->ExpandAll();

	infoMainSizer->Add(info_dialogue_tree, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	infoPanel->SetSizer(infoMainSizer);

	notebook->AddPage(questPanel, "Quest NPC");
	notebook->AddPage(interactPanel, "Interaction NPC");
	notebook->AddPage(infoPanel, "Information NPC");

	topsizer->Add(notebook, 1, wxEXPAND | wxALL, 10);

	wxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	buttonSizer->Add(new wxButton(this, NPC_WIZARD_BTN_GENERATE, "Generate & Export Scripts..."), 0, wxRIGHT, 10);
	buttonSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0);

	topsizer->Add(buttonSizer, 0, wxALIGN_RIGHT | wxALL, 12);
	SetSizerAndFit(topsizer);
}

void NPCWizardDialog::PopulateItemChoices(wxChoice* choice) {
	choice->Clear();
	choice->Append("Crystal Coin (2160)");
	choice->Append("Platinum Coin (2152)");
	choice->Append("Gold Coin (2148)");
	choice->Append("Health Potion (7618)");
	choice->Append("Mana Potion (7620)");
	choice->Append("Magic Light Wand (2162)");
	choice->Append("Backpack (1988)");
	choice->SetSelection(0);
}

void NPCWizardDialog::PopulateTownChoices(wxChoice* choice) {
	choice->Clear();
	Editor* editor = g_gui.GetCurrentEditor();
	if (editor && editor->map.towns.count() > 0) {

		for (auto& pair : editor->map.towns) {
			if (pair.second && !pair.second->getName().empty()) {
				choice->Append(pair.second->getName());
			}
		}
	}
	if (choice->GetCount() == 0) {
		choice->Append("Thais");
		choice->Append("Venore");
		choice->Append("Carlin");
		choice->Append("Edron");
	}
	choice->SetSelection(0);
}

void NPCWizardDialog::OnInteractTypeChanged(wxCommandEvent& WXUNUSED(event)) {
	int sel = interact_type_choice->GetSelection();
	ship_panel->Hide();
	shop_panel->Hide();
	heal_panel->Hide();
	temple_panel->Hide();

	if (sel == 0) ship_panel->Show();
	else if (sel == 1) shop_panel->Show();
	else if (sel == 2) heal_panel->Show();
	else if (sel == 3) temple_panel->Show();

	Layout();
}

void NPCWizardDialog::OnAddShopItem(wxCommandEvent& WXUNUSED(event)) {
	int sel = shop_item_choice->GetSelection();
	if (sel != wxNOT_FOUND) {
		wxString itemName = shop_item_choice->GetString(sel);
		int buy = shop_buy_price->GetValue();
		int sell = shop_sell_price->GetValue();

		ShopItemEntry entry{2160, itemName.ToStdString(), buy, sell};
		shop_items.push_back(entry);

		long index = shop_items_list->InsertItem(shop_items_list->GetItemCount(), itemName);
		wxString buyStr = (buy == 0 ? wxString("Free (0)") : wxString::Format("%d", buy));
		wxString sellStr = (sell == 0 ? wxString("N/A (0)") : wxString::Format("%d", sell));
		shop_items_list->SetItem(index, 1, buyStr);
		shop_items_list->SetItem(index, 2, sellStr);

	}
}

void NPCWizardDialog::OnRemoveShopItem(wxCommandEvent& WXUNUSED(event)) {
	long selected = shop_items_list->GetFirstSelected();
	if (selected != -1 && selected < (long)shop_items.size()) {
		shop_items_list->DeleteItem(selected);
		shop_items.erase(shop_items.begin() + selected);
	}
}

void NPCWizardDialog::OnAddDialogueNode(wxCommandEvent& WXUNUSED(event)) {
	wxTextEntryDialog dlg(this, "Enter Dialogue Step Text:", "Add Dialogue Step");
	if (dlg.ShowModal() == wxID_OK && !dlg.GetValue().IsEmpty()) {
		wxTreeItemId sel = quest_dialogue_tree->GetSelection();
		if (!sel.IsOk()) {
			sel = quest_dialogue_tree->GetRootItem();
		}
		quest_dialogue_tree->AppendItem(sel, dlg.GetValue());
		quest_dialogue_tree->ExpandAll();
	}
}

void NPCWizardDialog::OnRemoveDialogueNode(wxCommandEvent& WXUNUSED(event)) {
	wxTreeItemId sel = quest_dialogue_tree->GetSelection();
	if (sel.IsOk() && sel != quest_dialogue_tree->GetRootItem()) {
		quest_dialogue_tree->Delete(sel);
	}
}

void NPCWizardDialog::OnGenerate(wxCommandEvent& WXUNUSED(event)) {
	int sel = notebook->GetSelection();
	std::ostringstream lua;

	if (sel == 0) {
		std::string name = quest_npc_name->GetValue().ToStdString();
		int storage = quest_id_ctrl->GetValue();
		int exp = quest_reward_exp->GetValue();
		int gold = quest_reward_gold->GetValue();

		lua << "-- TFS 1.6 RevScript Quest NPC: " << name << "\n";
		lua << "local keywordHandler = KeywordHandler:new()\n";
		lua << "local npcHandler = NpcHandler:new(keywordHandler)\n";
		lua << "NpcSystem.parseScopes(npcHandler)\n\n";
		lua << "function onCreatureSay(cid, type, msg) npcHandler:onCreatureSay(cid, type, msg) end\n";
		lua << "local function creatureSayCallback(cid, type, msg)\n";
		lua << "    if not npcHandler:isFocused(cid) then return false end\n";
		lua << "    local player = Player(cid)\n";
		lua << "    if msgcontains(msg, 'quest') then\n";
		lua << "        if player:getStorageValue(" << storage << ") < 1 then\n";
		lua << "            player:setStorageValue(" << storage << ", 1)\n";
		if (exp > 0) lua << "            player:addExperience(" << exp << ")\n";
		if (gold > 0) lua << "            player:addMoney(" << gold << ")\n";
		lua << "            npcHandler:say(\"Thank you! Quest completed.\", cid)\n";
		lua << "        end\n";
		lua << "    end\n";
		lua << "    return true\n";
		lua << "end\n";
		lua << "npcHandler:setCallback(CALLBACK_MESSAGE_DEFAULT, creatureSayCallback)\n";
	}

	wxFileDialog saveDialog(this, "Save TFS 1.6 NPC Script", "", "npc_script.lua", "LUA Scripts (*.lua)|*.lua", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (saveDialog.ShowModal() == wxID_OK) {
		wxFileOutputStream output(saveDialog.GetPath());
		if (output.IsOk()) {
			output.Write(lua.str().c_str(), lua.str().length());
			wxMessageBox("TFS 1.6 NPC script generated successfully!", "Success", wxOK | wxICON_INFORMATION, this);
			EndModal(wxID_OK);
		}
	}
}

void NPCWizardDialog::OnClose(wxCommandEvent& WXUNUSED(event)) {
	EndModal(wxID_CANCEL);
}

