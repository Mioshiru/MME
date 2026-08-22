#ifndef RME_FIND_CREATURE_DIALOG_H_
#define RME_FIND_CREATURE_DIALOG_H_

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <string>

class CreatureType;

class FindCreatureDialog : public wxDialog {
public:
	FindCreatureDialog(wxWindow* parent, const wxString& title = "Select Creature from Palette");
	virtual ~FindCreatureDialog() {}

	CreatureType* GetSelectedCreature() const { return selected_creature; }
	std::string GetSelectedCreatureName() const;

private:
	void PopulateCreatures(const std::string& filter = "");
	void OnSearchUpdated(wxCommandEvent& event);
	void OnFilterChoiceChanged(wxCommandEvent& event);
	void OnItemSelected(wxListEvent& event);
	void OnItemActivated(wxListEvent& event);
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);

	wxTextCtrl* search_ctrl;
	wxChoice* filter_choice;
	wxListView* creature_list;
	class CreaturePreviewPanel* preview_panel;
	wxStaticText* info_label;

	CreatureType* selected_creature;

	DECLARE_EVENT_TABLE()
};

#endif // RME_FIND_CREATURE_DIALOG_H_
