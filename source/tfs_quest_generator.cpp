#include "tfs_quest_generator.h"
#include "item.h"
#include "items.h"
#include "action.h"
#include "gui.h"

#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/statline.h>
#include <sstream>

BEGIN_EVENT_TABLE(TFSQuestDialog, wxDialog)
EVT_BUTTON(wxID_OK, TFSQuestDialog::OnClickGenerate)
EVT_BUTTON(wxID_CANCEL, TFSQuestDialog::OnClickCancel)
END_EVENT_TABLE()

TFSQuestDialog::TFSQuestDialog(wxWindow* parent, Editor& editor, Tile* target_tile) :
	wxDialog(parent, wxID_ANY, "TFS 1.6 Quest Chest Creator", wxDefaultPosition, wxSize(480, 560), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	editor(editor),
	tile(target_tile),
	assigned_aid(2000) {

	SetBackgroundColour(wxColour(15, 23, 42)); // Slate 900
	wxBoxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	// Header Banner Card
	wxPanel* headerPanel = newd wxPanel(this, wxID_ANY);
	headerPanel->SetBackgroundColour(wxColour(30, 41, 59)); // Slate 800
	wxBoxSizer* headerSizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticText* header = newd wxStaticText(headerPanel, wxID_ANY, "TFS 1.6 Quest Chest Creator");
	header->SetFont(wxFont(13, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	header->SetForegroundColour(wxColour(248, 250, 252));

	wxStaticText* subheader = newd wxStaticText(headerPanel, wxID_ANY, "Configure quest chest storage IDs, rewards, and RevScriptSys event messages.");
	subheader->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	subheader->SetForegroundColour(wxColour(148, 163, 184));

	headerSizer->Add(header, 0, wxBOTTOM, 4);
	headerSizer->Add(subheader, 0);
	headerPanel->SetSizer(headerSizer);

	topsizer->Add(headerPanel, 0, wxEXPAND | wxALL, 12);

	// Content Card Container
	wxPanel* cardPanel = newd wxPanel(this, wxID_ANY);
	cardPanel->SetBackgroundColour(wxColour(30, 41, 59));
	wxBoxSizer* cardSizer = newd wxBoxSizer(wxVERTICAL);

	wxFlexGridSizer* grid = newd wxFlexGridSizer(2, 8, 12);
	grid->AddGrowableCol(1);

	auto addLabel = [cardPanel, grid](const wxString& labelText) {
		wxStaticText* label = newd wxStaticText(cardPanel, wxID_ANY, labelText);
		label->SetForegroundColour(wxColour(203, 213, 225));
		label->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
		grid->Add(label, 0, wxALIGN_CENTER_VERTICAL);
	};

	auto styleTextCtrl = [](wxTextCtrl* ctrl) {
		ctrl->SetBackgroundColour(wxColour(51, 65, 85));
		ctrl->SetForegroundColour(wxColour(248, 250, 252));
	};

	auto styleSpinCtrl = [](wxSpinCtrl* ctrl) {
		ctrl->SetBackgroundColour(wxColour(51, 65, 85));
		ctrl->SetForegroundColour(wxColour(248, 250, 252));
	};

	addLabel("Quest Name:");
	questNameCtrl = newd wxTextCtrl(cardPanel, wxID_ANY, "MyFirstQuest");
	styleTextCtrl(questNameCtrl);
	grid->Add(questNameCtrl, 1, wxEXPAND);

	addLabel("Storage Value ID:");
	storageIdSpin = newd wxSpinCtrl(cardPanel, wxID_ANY, "50001", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 999999, 50001);
	styleSpinCtrl(storageIdSpin);
	grid->Add(storageIdSpin, 1, wxEXPAND);

	addLabel("Action ID (aid):");
	actionIdSpin = newd wxSpinCtrl(cardPanel, wxID_ANY, "2001", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 100, 65535, 2001);
	styleSpinCtrl(actionIdSpin);
	grid->Add(actionIdSpin, 1, wxEXPAND);

	addLabel("Reward Item ID:");
	rewardItemIdSpin = newd wxSpinCtrl(cardPanel, wxID_ANY, "2160", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 40000, 2160);
	styleSpinCtrl(rewardItemIdSpin);
	grid->Add(rewardItemIdSpin, 1, wxEXPAND);

	addLabel("Reward Count:");
	rewardCountSpin = newd wxSpinCtrl(cardPanel, wxID_ANY, "10", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 10);
	styleSpinCtrl(rewardCountSpin);
	grid->Add(rewardCountSpin, 1, wxEXPAND);

	addLabel("Reward Exp:");
	rewardExpSpin = newd wxSpinCtrl(cardPanel, wxID_ANY, "1000", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10000000, 1000);
	styleSpinCtrl(rewardExpSpin);
	grid->Add(rewardExpSpin, 1, wxEXPAND);

	addLabel("Success Message:");
	successMsgCtrl = newd wxTextCtrl(cardPanel, wxID_ANY, "You have found 10 crystal coins.");
	styleTextCtrl(successMsgCtrl);
	grid->Add(successMsgCtrl, 1, wxEXPAND);

	addLabel("Empty Message:");
	emptyMsgCtrl = newd wxTextCtrl(cardPanel, wxID_ANY, "It is empty.");
	styleTextCtrl(emptyMsgCtrl);
	grid->Add(emptyMsgCtrl, 1, wxEXPAND);

	cardSizer->Add(grid, 1, wxEXPAND | wxALL, 12);
	cardPanel->SetSizer(cardSizer);

	topsizer->Add(cardPanel, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

	// Action Buttons
	wxBoxSizer* btnsizer = newd wxBoxSizer(wxHORIZONTAL);
	wxButton* okBtn = newd wxButton(this, wxID_OK, "Generate Script");
	wxButton* cancelBtn = newd wxButton(this, wxID_CANCEL, "Cancel");

	okBtn->SetBackgroundColour(wxColour(79, 70, 229)); // Indigo 600
	okBtn->SetForegroundColour(wxColour(255, 255, 255));
	okBtn->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));

	cancelBtn->SetBackgroundColour(wxColour(51, 65, 85));
	cancelBtn->SetForegroundColour(wxColour(203, 213, 225));

	btnsizer->Add(okBtn, 0, wxRIGHT, 8);
	btnsizer->Add(cancelBtn, 0);

	topsizer->Add(btnsizer, 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT | wxBOTTOM, 12);
	SetSizerAndFit(topsizer);

	if (tile) {
		Item* top_item = tile->getTopItem();
		if (top_item && top_item->getActionID() != 0) {
			actionIdSpin->SetValue(top_item->getActionID());
		}
	}
}

TFSQuestDialog::~TFSQuestDialog() {
}

void TFSQuestDialog::OnClickGenerate(wxCommandEvent& WXUNUSED(event)) {
	quest_name = nstr(questNameCtrl->GetValue());
	if (quest_name.empty()) {
		wxMessageBox("Please enter a valid quest name.", "Error", wxOK | wxICON_ERROR, this);
		return;
	}

	int storage_id = storageIdSpin->GetValue();
	int action_id = actionIdSpin->GetValue();
	int reward_item_id = rewardItemIdSpin->GetValue();
	int reward_count = rewardCountSpin->GetValue();
	int reward_exp = rewardExpSpin->GetValue();
	std::string success_msg = nstr(successMsgCtrl->GetValue());
	std::string empty_msg = nstr(emptyMsgCtrl->GetValue());

	assigned_aid = static_cast<uint32_t>(action_id);

	std::ostringstream ss;
	ss << "-- Generated by Mios Map Editor for TFS 1.6 RevScriptSys\n";
	ss << "local questChest = Action()\n\n";
	ss << "function questChest.onUse(player, item, fromPosition, target, toPosition, isHotkey)\n";
	ss << "\tif player:getStorageValue(" << storage_id << ") > 0 then\n";
	ss << "\t\tplayer:sendTextMessage(MESSAGE_EVENT_ADVANCE, \"" << empty_msg << "\")\n";
	ss << "\t\treturn true\n";
	ss << "\tend\n\n";
	ss << "\tplayer:setStorageValue(" << storage_id << ", 1)\n";
	if (reward_item_id > 0 && reward_count > 0) {
		ss << "\tplayer:addItem(" << reward_item_id << ", " << reward_count << ")\n";
	}
	if (reward_exp > 0) {
		ss << "\tplayer:addExperience(" << reward_exp << ", true)\n";
	}
	ss << "\tplayer:sendTextMessage(MESSAGE_EVENT_ADVANCE, \"" << success_msg << "\")\n";
	ss << "\treturn true\n";
	ss << "end\n\n";
	ss << "questChest:aid(" << action_id << ")\n";
	ss << "questChest:register()\n";

	generated_script = ss.str();

	if (tile) {
		Item* top_item = tile->getTopItem();
		if (top_item) {
			Tile* new_tile = tile->deepCopy(editor.map);
			Item* new_top = new_tile->getTopItem();
			if (new_top) {
				new_top->setActionID(assigned_aid);
			}
			Action* action = editor.actionQueue->createAction(ACTION_CHANGE_PROPERTIES);
			action->addChange(newd Change(new_tile));
			editor.addAction(action);
		}
	}

	EndModal(wxID_OK);
}

void TFSQuestDialog::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	EndModal(wxID_CANCEL);
}
