#ifndef RME_MAP_DIFF_WINDOW_H_
#define RME_MAP_DIFF_WINDOW_H_

#include "main.h"
#include <wx/dialog.h>
#include <wx/filepicker.h>
#include <set>
#include "position.h"

class Editor;

class MapDiffDialog : public wxDialog {
public:
	MapDiffDialog(wxWindow* parent, Editor& editor);
	virtual ~MapDiffDialog();

	void OnClickCompare(wxCommandEvent& event);
	void OnClickClear(wxCommandEvent& event);
	void OnClickClose(wxCommandEvent& event);

	static std::set<Position> addedPositions;
	static std::set<Position> removedPositions;
	static bool isDiffModeActive;

private:
	Editor& editor;
	wxFilePickerCtrl* filePicker;

	DECLARE_EVENT_TABLE()
};

#endif
