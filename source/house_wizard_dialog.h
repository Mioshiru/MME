#ifndef RME_HOUSE_WIZARD_DIALOG_H_
#define RME_HOUSE_WIZARD_DIALOG_H_

#include "main.h"
#include <wx/dialog.h>
#include <wx/spinctrl.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <string>
#include <vector>

class Map;
class House;

class HouseWizardDialog : public wxDialog {
public:
	HouseWizardDialog(wxWindow* parent, Map* map, uint32_t default_town_id = 0);
	virtual ~HouseWizardDialog();

	House* getCreatedHouse() const { return created_house; }
	static std::string GenerateRandomHouseName();
	static void NotifyTileOrExitChanged() {}

protected:
	void OnClickRollName(wxCommandEvent& evt);
	void OnClickOK(wxCommandEvent& evt);
	void OnClickCancel(wxCommandEvent& evt);

private:
	Map* map;
	House* created_house;

	wxTextCtrl* name_field;
	wxButton* roll_btn;
	wxChoice* town_choice;
	wxSpinCtrl* id_field;
	wxTextCtrl* rent_field;
	wxCheckBox* guildhall_checkbox;

	wxButton* ok_btn;
	wxButton* cancel_btn;

	enum {
		WIZARD_ID_ROLL = 10050
	};

	DECLARE_EVENT_TABLE()
};

#endif
