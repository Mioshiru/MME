#include "find_creature_dialog.h"
#include "style_manager.h"
#include "tfs_npc_wizard_window.h"
#include "style_manager.h"
#include "gui.h"
#include "style_manager.h"
#include "creatures.h"
#include "style_manager.h"
#include <wx/sizer.h>
#include "style_manager.h"
#include <wx/stattext.h>
#include "style_manager.h"
#include <wx/button.h>
#include "style_manager.h"
#include <algorithm>
#include "style_manager.h"

enum {
	ID_FC_SEARCH = wxID_HIGHEST + 900,
	ID_FC_FILTER,
	ID_FC_LIST,
	ID_FC_OK
};

BEGIN_EVENT_TABLE(FindCreatureDialog, wxDialog)
	EVT_TEXT(ID_FC_SEARCH, FindCreatureDialog::OnSearchUpdated)
	EVT_CHOICE(ID_FC_FILTER, FindCreatureDialog::OnFilterChoiceChanged)
	EVT_LIST_ITEM_SELECTED(ID_FC_LIST, FindCreatureDialog::OnItemSelected)
	EVT_LIST_ITEM_ACTIVATED(ID_FC_LIST, FindCreatureDialog::OnItemActivated)
	EVT_BUTTON(ID_FC_OK, FindCreatureDialog::OnOK)
	EVT_BUTTON(wxID_CANCEL, FindCreatureDialog::OnCancel)
END_EVENT_TABLE()

std::string FindCreatureDialog::GetSelectedCreatureName() const {
	return selected_creature ? selected_creature->name : "";
}

FindCreatureDialog::FindCreatureDialog(wxWindow* parent, const wxString& title) :
	wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(640, 520), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	selected_creature(nullptr)
{
	wxBoxSizer* rootSizer = new wxBoxSizer(wxVERTICAL);

	// Header Banner
	wxPanel* headerPanel = new wxPanel(this, wxID_ANY);
	headerPanel->SetBackgroundColour(wxColour(16, 20, 30));
	wxBoxSizer* headerSizer = new wxBoxSizer(wxVERTICAL);

	wxStaticText* t = new wxStaticText(headerPanel, wxID_ANY, "Select Monster / Creature from Palette");
	wxFont tFont = t->GetFont();
	tFont.SetPointSize(11);
	tFont.SetWeight(wxFONTWEIGHT_BOLD);
	t->SetFont(tFont);
	t->SetForegroundColour(wxColour(255, 215, 0));
	headerSizer->Add(t, 0, wxALL, 6);

	headerPanel->SetSizer(headerSizer);
	rootSizer->Add(headerPanel, 0, wxEXPAND);

	// Search & Filter Row
	wxBoxSizer* topFilterRow = new wxBoxSizer(wxHORIZONTAL);
	search_ctrl = new wxTextCtrl(this, ID_FC_SEARCH, "", wxDefaultPosition, wxDefaultSize, 0);
	search_ctrl->SetHint("Search creature name or LookType...");
	topFilterRow->Add(search_ctrl, 1, wxRIGHT | wxALIGN_CENTER_VERTICAL, 6);

	wxArrayString filterOpts;
	filterOpts.Add("All Creatures");
	filterOpts.Add("Monsters Only");
	filterOpts.Add("NPCs Only");
	filter_choice = new wxChoice(this, ID_FC_FILTER, wxDefaultPosition, wxDefaultSize, filterOpts);
	filter_choice->SetSelection(1); // Monsters Only by default
	topFilterRow->Add(filter_choice, 0, wxALIGN_CENTER_VERTICAL);

	rootSizer->Add(topFilterRow, 0, wxEXPAND | wxALL, 8);

	// Center Row: List on left, preview on right
	wxBoxSizer* centerRow = new wxBoxSizer(wxHORIZONTAL);

	creature_list = new wxListView(this, ID_FC_LIST, wxDefaultPosition, wxSize(380, 300), wxLC_REPORT | wxLC_SINGLE_SEL);
	creature_list->InsertColumn(0, "LookType", wxLIST_FORMAT_RIGHT, 75);
	creature_list->InsertColumn(1, "Creature Name", wxLIST_FORMAT_LEFT, 260);
	centerRow->Add(creature_list, 1, wxEXPAND | wxRIGHT, 8);

	// Preview Panel
	wxBoxSizer* rightCol = new wxBoxSizer(wxVERTICAL);
	preview_panel = new CreaturePreviewPanel(this, wxID_ANY, 150);
	rightCol->Add(preview_panel, 0, wxALIGN_CENTER | wxBOTTOM, 6);

	info_label = new wxStaticText(this, wxID_ANY, "Select a creature to preview", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
	info_label->SetForegroundColour(wxColour(255, 215, 0));
	rightCol->Add(info_label, 0, wxEXPAND);

	centerRow->Add(rightCol, 0, wxALIGN_CENTER_VERTICAL);
	rootSizer->Add(centerRow, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

	// Bottom Action Row
	wxBoxSizer* bottomSizer = new wxBoxSizer(wxHORIZONTAL);
	wxButton* okBtn = new wxButton(this, ID_FC_OK, "Select Creature");
	okBtn->SetBackgroundColour(wxColour(40, 120, 60));
	okBtn->SetForegroundColour(*wxWHITE);
	bottomSizer->Add(okBtn, 0, wxRIGHT, 6);

	wxButton* cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");
	bottomSizer->Add(cancelBtn, 0);

	rootSizer->Add(bottomSizer, 0, wxALIGN_RIGHT | wxALL, 8);

	SetSizer(rootSizer);
	RME::UI::StyleManager::ApplyThemeRecursively(this, RME::UI::StyleManager::GetTheme());
	Layout();
	CenterOnParent();

	PopulateCreatures();
}

void FindCreatureDialog::PopulateCreatures(const std::string& filter) {
	creature_list->DeleteAllItems();
	std::string lowerFilter = filter;
	std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);

	int filterMode = filter_choice->GetSelection();

	for (auto iter = g_creatures.begin(); iter != g_creatures.end(); ++iter) {
		CreatureType* ct = iter->second;
		if (!ct) continue;

		if (filterMode == 1 && ct->isNpc) continue; // Monsters only
		if (filterMode == 2 && !ct->isNpc) continue; // NPCs only

		std::string lowerName = ct->name;
		std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

		if (!lowerFilter.empty() &&
			lowerName.find(lowerFilter) == std::string::npos &&
			std::to_string(ct->outfit.lookType).find(lowerFilter) == std::string::npos) {
			continue;
		}

		long idx = creature_list->InsertItem(creature_list->GetItemCount(), std::to_string(ct->outfit.lookType));
		creature_list->SetItem(idx, 1, ct->name);
		creature_list->SetItemData(idx, (wxUIntPtr)ct);
	}

	if (creature_list->GetItemCount() > 0) {
		creature_list->Select(0);
		creature_list->Focus(0);
		CreatureType* firstCt = (CreatureType*)creature_list->GetItemData(0);
		if (firstCt) {
			selected_creature = firstCt;
			preview_panel->SetOutfit(firstCt->outfit.lookType, firstCt->outfit.lookHead, firstCt->outfit.lookBody, firstCt->outfit.lookLegs, firstCt->outfit.lookFeet, firstCt->outfit.lookAddon);
			info_label->SetLabel(wxString::Format("%s (LookType %d)", firstCt->name, firstCt->outfit.lookType));
		}
	}
}

void FindCreatureDialog::OnSearchUpdated(wxCommandEvent& WXUNUSED(event)) {
	PopulateCreatures(search_ctrl->GetValue().ToStdString());
}

void FindCreatureDialog::OnFilterChoiceChanged(wxCommandEvent& WXUNUSED(event)) {
	PopulateCreatures(search_ctrl->GetValue().ToStdString());
}

void FindCreatureDialog::OnItemSelected(wxListEvent& event) {
	long idx = event.GetIndex();
	CreatureType* ct = (CreatureType*)creature_list->GetItemData(idx);
	if (ct) {
		selected_creature = ct;
		preview_panel->SetOutfit(ct->outfit.lookType, ct->outfit.lookHead, ct->outfit.lookBody, ct->outfit.lookLegs, ct->outfit.lookFeet, ct->outfit.lookAddon);
		info_label->SetLabel(wxString::Format("%s (LookType %d)", ct->name, ct->outfit.lookType));
	}
}

void FindCreatureDialog::OnItemActivated(wxListEvent& event) {
	long idx = event.GetIndex();
	CreatureType* ct = (CreatureType*)creature_list->GetItemData(idx);
	if (ct) {
		selected_creature = ct;
		EndModal(wxID_OK);
	}
}

void FindCreatureDialog::OnOK(wxCommandEvent& WXUNUSED(event)) {
	long item = creature_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (item != -1) {
		selected_creature = (CreatureType*)creature_list->GetItemData(item);
		EndModal(wxID_OK);
	} else if (selected_creature) {
		EndModal(wxID_OK);
	} else {
		wxMessageBox("Please select a creature first.", "Notice", wxOK | wxICON_INFORMATION, this);
	}
}

void FindCreatureDialog::OnCancel(wxCommandEvent& WXUNUSED(event)) {
	EndModal(wxID_CANCEL);
}
