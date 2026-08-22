#ifndef RME_TFS_SPECIAL_OBJECTS_WIZARD_WINDOW_H_
#define RME_TFS_SPECIAL_OBJECTS_WIZARD_WINDOW_H_

#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/spinctrl.h>
#include <wx/listctrl.h>
#include <vector>
#include <string>

struct ChestSlotData {
	int item_id = 0;
	int count = 1;
	std::string name;
};

class SpecialObjectsWizardDialog : public wxDialog {
public:
	SpecialObjectsWizardDialog(wxWindow* parent);
	virtual ~SpecialObjectsWizardDialog() {}

private:
	// Slot grid events
	void OnSlotClicked(int slot_index);
	void OnPickItemForSlot(wxCommandEvent& event);
	void OnClearSlot(wxCommandEvent& event);
	void OnClearAllSlots(wxCommandEvent& event);
	void OnAutoGenerateIds(wxCommandEvent& event);
	void OnPickKeyFromPalette(wxCommandEvent& event);
	void OnPickChestModel(wxCommandEvent& event);
	void OnGenerateScript(wxCommandEvent& event);
	void OnCopyScript(wxCommandEvent& event);
	void OnPlaceOnMap(wxCommandEvent& event);
	void OnClose(wxCommandEvent& event);

	void RefreshChestSlotsUI();
	std::string GenerateChestLuaScript() const;
	std::string GenerateDoorLuaScript() const;

	wxNotebook* notebook;

	// --- Tab 1: Quest Chest & Visual Multi-Slot Inventory ---
	wxChoice* chest_model_choice;
	wxPanel* chest_model_preview;
	int selected_chest_id;

	wxSpinCtrl* chest_action_id;
	wxSpinCtrl* chest_storage_key;
	wxTextCtrl* quest_name_ctrl;

	// 8 Visual Chest Slots
	static const int MAX_CHEST_SLOTS = 8;
	ChestSlotData chest_slots[MAX_CHEST_SLOTS];
	wxPanel* slot_panels[MAX_CHEST_SLOTS];
	int active_selected_slot;

	// Key opening requirement
	wxCheckBox* req_key_cb;
	wxPanel* key_preview_panel;
	wxStaticText* key_name_text;
	int req_key_id;
	wxSpinCtrl* req_key_action_id;

	// --- Tab 2: Doors (Quest & Level Doors) ---
	wxChoice* door_type_choice;
	wxChoice* door_model_choice;
	wxPanel* door_model_preview;
	wxSpinCtrl* door_action_id;
	wxSpinCtrl* door_req_level;
	wxSpinCtrl* door_storage_key;

	// --- Tab 3: Teleporters & Switches ---
	wxSpinCtrl* tele_dest_x;
	wxSpinCtrl* tele_dest_y;
	wxSpinCtrl* tele_dest_z;
	wxSpinCtrl* tele_action_id;

	DECLARE_EVENT_TABLE()
};

#endif // RME_TFS_SPECIAL_OBJECTS_WIZARD_WINDOW_H_
