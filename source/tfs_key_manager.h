#ifndef RME_TFS_KEY_MANAGER_H_
#define RME_TFS_KEY_MANAGER_H_

#include "main.h"
#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>

#include "editor.h"

class TFSKeyDoorDialog : public wxDialog {
public:
	TFSKeyDoorDialog(wxWindow* parent, Editor& editor, Tile* target_tile = nullptr);
	virtual ~TFSKeyDoorDialog();

	void OnClickApply(wxCommandEvent& event);
	void OnClickCancel(wxCommandEvent& event);

	uint32_t getKeyActionID() const { return assigned_key_aid; }
	std::string getGeneratedKeyScript() const { return generated_script; }

private:
	Editor& editor;
	Tile* tile;

	wxSpinCtrl* keyItemIdSpin;
	wxSpinCtrl* keyActionIdSpin;
	wxTextCtrl* keyNameCtrl;

	uint32_t assigned_key_aid;
	std::string generated_script;

	DECLARE_EVENT_TABLE()
};

#endif // RME_TFS_KEY_MANAGER_H_
