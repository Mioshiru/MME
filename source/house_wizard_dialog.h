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
#include <wx/tglbtn.h>
#include "position.h"

class Map;
class House;

class HouseWizardDialog : public wxDialog {
public:
	HouseWizardDialog(wxWindow* parent, Map* map, uint32_t default_town_id = 0);
	virtual ~HouseWizardDialog();

	House* getCreatedHouse() const { return draft_house; }
	void cancelWizard();

protected:
	void showStep(int step);
	void updateExitText();

	void OnClickNext(wxCommandEvent& evt);
	void OnClickBack(wxCommandEvent& evt);
	void OnClickOK(wxCommandEvent& evt);
	void OnClickCancel(wxCommandEvent& evt);

	void OnClickPaintTiles(wxCommandEvent& evt);
	void OnClickSetExit(wxCommandEvent& evt);

	void OnActivate(wxActivateEvent& evt);
	void OnIconize(wxIconizeEvent& evt);

	void OnMouseEnter(wxMouseEvent& evt);
	void OnMouseLeave(wxMouseEvent& evt);

	void bindHoverEvents(wxWindow* win);
	void makeSemiTransparent();
	void makeOpaque();

private:
	Map* map;
	House* draft_house;
	int current_step;

	// UI panels for 3 steps
	wxPanel* step1_panel;
	wxPanel* step2_panel;
	wxPanel* step3_panel;

	// Step 1 controls
	wxTextCtrl* name_field;
	wxChoice* town_choice;
	wxTextCtrl* rent_field;
	wxCheckBox* guildhall_checkbox;

	// Step 2 controls
	wxToggleButton* paint_tiles_btn;
	wxStaticText* tile_count_label;

	// Step 3 controls
	wxToggleButton* set_exit_btn;
	wxStaticText* exit_pos_label;

	// Nav buttons
	wxButton* back_btn;
	wxButton* next_btn;
	wxButton* ok_btn;
	wxButton* cancel_btn;

	DECLARE_EVENT_TABLE()
};

#endif
