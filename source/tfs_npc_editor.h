#ifndef RME_TFS_NPC_EDITOR_H_
#define RME_TFS_NPC_EDITOR_H_

#include "main.h"
#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/grid.h>

#include "editor.h"

class TFSNPCDialog : public wxDialog {
public:
	TFSNPCDialog(wxWindow* parent, Editor& editor, Creature* target_creature = nullptr);
	virtual ~TFSNPCDialog();

	void OnClickGenerate(wxCommandEvent& event);
	void OnClickCancel(wxCommandEvent& event);

	std::string getGeneratedXML() const { return generated_xml; }
	std::string getGeneratedLua() const { return generated_lua; }
	std::string getNPCName() const { return npc_name; }

private:
	Editor& editor;
	Creature* creature;

	wxTextCtrl* npcNameCtrl;
	wxSpinCtrl* lookTypeSpin;
	wxSpinCtrl* healthSpin;
	wxSpinCtrl* maxHealthSpin;
	wxSpinCtrl* walkIntervalSpin;

	wxTextCtrl* hiGreetingCtrl;
	wxTextCtrl* jobResponseCtrl;
	wxTextCtrl* questResponseCtrl;
	wxTextCtrl* byeResponseCtrl;

	wxGrid* shopGrid;

	std::string generated_xml;
	std::string generated_lua;
	std::string npc_name;

	DECLARE_EVENT_TABLE()
};

#endif // RME_TFS_NPC_EDITOR_H_
