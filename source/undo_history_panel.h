#ifndef RME_UNDO_HISTORY_PANEL_H
#define RME_UNDO_HISTORY_PANEL_H

#include <wx/panel.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/timer.h>

class MapEditor;
typedef MapEditor Editor;

class UndoHistoryPanel : public wxPanel {
public:
	UndoHistoryPanel(wxWindow* parent, Editor* editor = nullptr);
	virtual ~UndoHistoryPanel();

	void SetEditor(Editor* new_editor);
	void RefreshHistory();

private:
	void OnItemSelect(wxListEvent& evt);
	void OnTimer(wxTimerEvent& evt);

	Editor* editor;
	wxListCtrl* list_ctrl;
	wxTimer refresh_timer;

	DECLARE_EVENT_TABLE()
};

#endif // RME_UNDO_HISTORY_PANEL_H
