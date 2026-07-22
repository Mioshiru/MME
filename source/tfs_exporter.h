#ifndef RME_TFS_EXPORTER_H_
#define RME_TFS_EXPORTER_H_

#include "main.h"
#include <wx/dialog.h>
#include <wx/filepicker.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>

#include "editor.h"

class TFSExportDialog : public wxDialog {
public:
	TFSExportDialog(wxWindow* parent, Editor& editor);
	virtual ~TFSExportDialog();

	void OnClickExport(wxCommandEvent& event);
	void OnClickCancel(wxCommandEvent& event);

private:
	Editor& editor;
	wxDirPickerCtrl* dirPicker;

	DECLARE_EVENT_TABLE()
};

#endif // RME_TFS_EXPORTER_H_
