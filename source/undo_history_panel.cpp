#include "main.h"
#include "undo_history_panel.h"
#include "editor.h"
#include "action.h"
#include "style_manager.h"
#include <wx/stattext.h>

enum {
	UNDO_HIST_LIST = wxID_HIGHEST + 400,
	UNDO_HIST_TIMER
};

BEGIN_EVENT_TABLE(UndoHistoryPanel, wxPanel)
EVT_LIST_ITEM_SELECTED(UNDO_HIST_LIST, UndoHistoryPanel::OnItemSelect)
EVT_TIMER(UNDO_HIST_TIMER, UndoHistoryPanel::OnTimer)
END_EVENT_TABLE()

UndoHistoryPanel::UndoHistoryPanel(wxWindow* parent, Editor* editor) :
	wxPanel(parent, wxID_ANY),
	editor(editor),
	refresh_timer(this, UNDO_HIST_TIMER) {

	SetBackgroundColour(wxColour(16, 28, 48));
	SetForegroundColour(wxColour(240, 245, 255));

	wxBoxSizer* main_sizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticText* header = newd wxStaticText(this, wxID_ANY, "Undo / Redo History");
	header->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	header->SetForegroundColour(wxColour(240, 210, 120));
	main_sizer->Add(header, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 6);

	list_ctrl = newd wxListCtrl(this, UNDO_HIST_LIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
	list_ctrl->InsertColumn(0, "#", wxLIST_FORMAT_LEFT, 40);
	list_ctrl->InsertColumn(1, "Aktion", wxLIST_FORMAT_LEFT, 160);
	list_ctrl->InsertColumn(2, "Status", wxLIST_FORMAT_LEFT, 80);
	list_ctrl->SetBackgroundColour(wxColour(10, 20, 35));
	list_ctrl->SetForegroundColour(wxColour(240, 245, 255));

	main_sizer->Add(list_ctrl, 1, wxEXPAND | wxALL, 4);
	SetSizer(main_sizer);

	RME::UI::StyleManager::ApplyThemeRecursively(this, RME::UI::StyleManager::GetTheme());

	refresh_timer.Start(500); // Check for stack changes periodically
}

UndoHistoryPanel::~UndoHistoryPanel() {
	refresh_timer.Stop();
}

void UndoHistoryPanel::SetEditor(Editor* new_editor) {
	editor = new_editor;
	RefreshHistory();
}

void UndoHistoryPanel::RefreshHistory() {
	list_ctrl->DeleteAllItems();
	if (!editor) return;

	ActionQueue* queue = editor->actionQueue;
	if (!queue) return;

	size_t curr_idx = queue->getCurrentIndex();
	size_t total = queue->getSize();

	for (size_t i = 0; i < total; ++i) {
		std::string name = queue->getActionName(i);
		if (name.empty()) name = "Bearbeitung";

		long item_idx = list_ctrl->InsertItem(i, wxString::Format("%zu", i + 1));
		list_ctrl->SetItem(item_idx, 1, wxstr(name));

		if (i < curr_idx) {
			list_ctrl->SetItem(item_idx, 2, "Angewendet");
			list_ctrl->SetItemTextColour(item_idx, wxColour(180, 240, 180));
		} else {
			list_ctrl->SetItem(item_idx, 2, "Wiederholbar");
			list_ctrl->SetItemTextColour(item_idx, wxColour(160, 170, 190));
		}
	}
}

void UndoHistoryPanel::OnTimer(wxTimerEvent& evt) {
	RefreshHistory();
}

void UndoHistoryPanel::OnItemSelect(wxListEvent& evt) {
	if (!editor) return;

	ActionQueue* queue = editor->actionQueue;
	if (!queue) return;

	long selected = evt.GetIndex();
	if (selected < 0) return;

	size_t target_idx = static_cast<size_t>(selected) + 1;
	size_t curr_idx = queue->getCurrentIndex();

	if (target_idx < curr_idx) {
		while (queue->getCurrentIndex() > target_idx && queue->canUndo()) {
			queue->undo();
		}
	} else if (target_idx > curr_idx) {
		while (queue->getCurrentIndex() < target_idx && queue->canRedo()) {
			queue->redo();
		}
	}
	editor->map.doChange();
	RefreshHistory();
}
