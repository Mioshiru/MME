#ifndef RME_MAP_DIAGNOSTIC_WINDOW_H_
#define RME_MAP_DIAGNOSTIC_WINDOW_H_

#include "main.h"
#include "position.h"
#include <wx/dialog.h>
#include <wx/listctrl.h>
#include <vector>

class Editor;

struct DiagnosticIssue {
	Position pos;
	wxString category;
	wxString description;
};

class MapDiagnosticDialog : public wxDialog {
public:
	MapDiagnosticDialog(wxWindow* parent, Editor& editor);
	virtual ~MapDiagnosticDialog();

	void OnClickScan(wxCommandEvent& event);
	void OnItemSelect(wxListEvent& event);
	void OnClickClose(wxCommandEvent& event);

private:
	void runDiagnostics();

	Editor& editor;
	wxListCtrl* issueListCtrl;
	std::vector<DiagnosticIssue> issues;

	DECLARE_EVENT_TABLE()
};

#endif
