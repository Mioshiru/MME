#ifndef RME_TFS_SPECIAL_OBJECTS_WIZARD_WINDOW_H_
#define RME_TFS_SPECIAL_OBJECTS_WIZARD_WINDOW_H_

#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/spinctrl.h>
#include <wx/listctrl.h>
#include <vector>


struct ContainerItemEntry {
	int id;
	std::string name;
	int count;
};

class SpecialObjectsWizardDialog : public wxDialog {
public:
	SpecialObjectsWizardDialog(wxWindow* parent);
	virtual ~SpecialObjectsWizardDialog() {}

private:
	void OnContainerChoiceChanged(wxCommandEvent& event);
	void OnAddContainerItem(wxCommandEvent& event);
	void OnRemoveContainerItem(wxCommandEvent& event);
	void OnGenerate(wxCommandEvent& event);
	void OnClose(wxCommandEvent& event);

	void PopulateContainerChoices(wxChoice* choice);
	void PopulateItemChoices(wxChoice* choice);

	wxNotebook* notebook;

	// 1. Doors Controls
	wxChoice* door_type_choice; // Quest Door, Level Door, Key Door
	wxSpinCtrl* door_action_id;
	wxSpinCtrl* door_req_level;
	wxSpinCtrl* door_quest_storage;

	// 2. Container (Quest Chests) Controls
	wxChoice* container_type_choice;
	wxStaticText* container_slot_info;
	int max_container_capacity = 8;
	wxSpinCtrl* chest_action_id;
	wxSpinCtrl* chest_storage_key;
	wxChoice* item_picker_choice;
	wxSpinCtrl* item_picker_count;
	wxListView* container_items_list;
	std::vector<ContainerItemEntry> container_items;

	DECLARE_EVENT_TABLE()
};

#endif // RME_TFS_SPECIAL_OBJECTS_WIZARD_WINDOW_H_

