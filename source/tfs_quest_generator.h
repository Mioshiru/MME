#ifndef RME_TFS_QUEST_GENERATOR_H_
#define RME_TFS_QUEST_GENERATOR_H_

#include "main.h"
#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>

#include "editor.h"

class TFSQuestDialog : public wxDialog {
public:
	TFSQuestDialog(wxWindow* parent, Editor& editor, Tile* target_tile = nullptr);
	virtual ~TFSQuestDialog();

	void OnClickGenerate(wxCommandEvent& event);
	void OnClickCancel(wxCommandEvent& event);

	std::string getGeneratedScript() const { return generated_script; }
	std::string getQuestName() const { return quest_name; }
	uint32_t getAssignedActionID() const { return assigned_aid; }

private:
	Editor& editor;
	Tile* tile;

	wxTextCtrl* questNameCtrl;
	wxSpinCtrl* storageIdSpin;
	wxSpinCtrl* actionIdSpin;
	wxSpinCtrl* rewardItemIdSpin;
	wxSpinCtrl* rewardCountSpin;
	wxSpinCtrl* rewardExpSpin;
	wxTextCtrl* successMsgCtrl;
	wxTextCtrl* emptyMsgCtrl;

	std::string generated_script;
	std::string quest_name;
	uint32_t assigned_aid;

	DECLARE_EVENT_TABLE()
};

#endif // RME_TFS_QUEST_GENERATOR_H_
