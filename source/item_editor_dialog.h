#ifndef RME_ITEM_EDITOR_DIALOG_H_
#define RME_ITEM_EDITOR_DIALOG_H_

#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/spinctrl.h>
#include <wx/listctrl.h>
#include <vector>
#include <string>

class ItemEditorDialog : public wxDialog {
public:
	ItemEditorDialog(wxWindow* parent);
	virtual ~ItemEditorDialog() {}

private:
	// Tab 1: Item Inspector & Attributes
	void OnSearchChanged(wxCommandEvent& event);
	void OnItemSelected(wxListEvent& event);
	void OnApplyChanges(wxCommandEvent& event);
	void OnExportItemXml(wxCommandEvent& event);
	void OnPickItemFromPalette(wxCommandEvent& event);

	// Tab 2: Custom Sprite & Free Item Slot Creator
	void OnFindNextFreeSlot(wxCommandEvent& event);
	void OnImportCustomPng(wxCommandEvent& event);
	void OnRegisterNewItem(wxCommandEvent& event);

	void OnSaveAll(wxCommandEvent& event);
	void OnClose(wxCommandEvent& event);

	void RefreshItemList(const std::string& filter = "");
	void DisplayItemData(int server_id);

	wxNotebook* notebook;

	// Tab 1 Controls
	wxTextCtrl* search_ctrl;
	wxListView* item_list_view;
	wxPanel* item_preview_panel;

	wxStaticText* badge_server_id;
	wxStaticText* badge_client_id;

	wxTextCtrl* item_name_ctrl;
	wxTextCtrl* item_desc_ctrl;
	wxSpinCtrl* item_weight_ctrl;
	wxChoice* item_type_choice;

	wxCheckBox* flag_stackable;
	wxCheckBox* flag_unpassable;
	wxCheckBox* flag_block_missiles;
	wxCheckBox* flag_moveable;
	wxCheckBox* flag_rotatable;
	wxCheckBox* flag_hangable;
	wxCheckBox* flag_has_light;
	wxSpinCtrl* light_level_ctrl;
	wxSpinCtrl* light_color_ctrl;

	wxCheckBox* flag_container;
	wxSpinCtrl* container_slots_ctrl;

	// Tab 2 Controls (Custom Item / PNG Creator)
	wxSpinCtrl* new_item_id_ctrl;
	wxSpinCtrl* new_client_id_ctrl;
	wxTextCtrl* new_item_name_ctrl;
	wxPanel* new_item_preview_panel;
	wxImage custom_png_image;
	bool has_custom_png;

	int current_selected_id;

	DECLARE_EVENT_TABLE()
};

#endif // RME_ITEM_EDITOR_DIALOG_H_
