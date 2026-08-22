//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#ifndef RME_PREFERENCES_WINDOW_H_
#define RME_PREFERENCES_WINDOW_H_

#include "main.h"
#include <wx/listbook.h>
#include <wx/collpane.h>
#include <wx/clrpicker.h>
#include <wx/aui/auibook.h>

class wxHyperlinkCtrl;

class PreferencesWindow : public wxDialog {
public:
	explicit PreferencesWindow(wxWindow* parent) :
		PreferencesWindow(parent, false) {};
	PreferencesWindow(wxWindow* parent, bool clientVersionSelected);
	virtual ~PreferencesWindow();

	void SelectHotkeysTab() { if (book) book->SetSelection(4); }

	void OnClickDefaults(wxCommandEvent&);
	void OnClickApply(wxCommandEvent&);
	void OnClickOK(wxCommandEvent&);
	void OnClickCancel(wxCommandEvent&);

	void OnCollapsiblePane(wxCollapsiblePaneEvent&);

 protected:
	void SetDefaults();
	void Apply();

	wxAuiNotebook* book;

	// General
	wxCheckBox* always_make_backup_chkbox = nullptr;
	wxCheckBox* autosave_enabled_chkbox = nullptr;
	wxSlider*   autosave_interval_slider = nullptr;
	wxStaticText* autosave_interval_label = nullptr;
	wxCheckBox* create_on_startup_chkbox = nullptr;
	wxCheckBox* update_check_on_startup_chkbox = nullptr;
	wxCheckBox* only_one_instance_chkbox = nullptr;
	wxCheckBox* show_welcome_dialog_chkbox = nullptr;
	wxCheckBox* enable_tileset_editing_chkbox = nullptr;
	wxSpinCtrl* multiplayer_port_spin = nullptr;
	wxSpinCtrl* undo_size_spin = nullptr;
	wxSpinCtrl* undo_mem_size_spin = nullptr;
	wxSpinCtrl* worker_threads_spin = nullptr;
	wxSpinCtrl* replace_size_spin = nullptr;
	wxChoice* position_choice = nullptr;

	// Editor
	wxCheckBox* group_actions_chkbox = nullptr;
	wxCheckBox* duplicate_id_warn_chkbox = nullptr;
	wxCheckBox* house_remove_chkbox = nullptr;
	wxCheckBox* auto_assign_doors_chkbox = nullptr;
	wxCheckBox* eraser_leave_unique_chkbox = nullptr;
	wxCheckBox* doodad_erase_same_chkbox = nullptr;
	wxCheckBox* auto_create_spawn_chkbox = nullptr;
	wxCheckBox* allow_multiple_orderitems_chkbox = nullptr;
	wxCheckBox* merge_move_chkbox = nullptr;
	wxCheckBox* merge_paste_chkbox = nullptr;

	// Graphics
	wxCheckBox* icon_selection_shadow_chkbox = nullptr;
	wxChoice* icon_background_choice = nullptr;
	wxCheckBox* use_memcached_chkbox = nullptr;
	wxDirPickerCtrl* screenshot_directory_picker = nullptr;
	wxChoice* screenshot_format_choice = nullptr;
	wxCheckBox* hide_items_when_zoomed_chkbox = nullptr;
	wxCheckBox* fake_hd_chkbox = nullptr;
	wxCheckBox* ambient_effects_chkbox = nullptr;

	// Interface
	wxChoice* terrain_palette_style_choice = nullptr;
	wxChoice* collection_palette_style_choice = nullptr;
	wxChoice* doodad_palette_style_choice = nullptr;
	wxChoice* item_palette_style_choice = nullptr;
	wxChoice* raw_palette_style_choice = nullptr;

	// Visuals & Themes
	wxSlider* ui_scale_slider = nullptr;
	wxStaticText* ui_scale_level_txt = nullptr;
	wxPanel* ui_scale_preview_panel = nullptr;
	void UpdateScalePreview();
	wxRadioBox* backend_radio = nullptr;
	wxRadioBox* theme_radio = nullptr;
	wxColourPickerCtrl* cursor_color_pick = nullptr;
	wxColourPickerCtrl* cursor_alt_color_pick = nullptr;
	wxSlider* crt_strength_slider = nullptr;
	wxChoice* aa_choice = nullptr;
	wxSlider* water_anim_slider = nullptr;
	wxSlider* light_intensity_slider = nullptr;
	wxChoice* bg_color_choice = nullptr;
	wxSlider* grid_opacity_slider = nullptr;
	wxCheckBox* multi_monitor_workspace_chkbox = nullptr;

	wxCheckBox* large_terrain_tools_chkbox = nullptr;
	wxCheckBox* large_collection_tools_chkbox = nullptr;
	wxCheckBox* large_doodad_sizebar_chkbox = nullptr;
	wxCheckBox* large_item_sizebar_chkbox = nullptr;
	wxCheckBox* large_house_sizebar_chkbox = nullptr;
	wxCheckBox* large_raw_sizebar_chkbox = nullptr;
	wxCheckBox* large_container_icons_chkbox = nullptr;
	wxCheckBox* large_pick_item_icons_chkbox = nullptr;

	wxCheckBox* switch_mousebtn_chkbox = nullptr;
	wxCheckBox* doubleclick_properties_chkbox = nullptr;
	wxCheckBox* inversed_scroll_chkbox = nullptr;
	wxSlider* scroll_speed_slider = nullptr;
	wxSlider* zoom_speed_slider = nullptr;
	wxSlider* minimap_scroll_speed_slider = nullptr;

	wxCheckBox* vsync_chkbox = nullptr;
	// Client info
	wxChoice* default_version_choice = nullptr;
	wxStaticText* scan_status_txt = nullptr;
	wxButton* open_folder_btn = nullptr;
	wxHyperlinkCtrl* help_link = nullptr;
	wxCheckBox* check_sigs_chkbox = nullptr;

	void UpdateScanStatus();

	// Create controls
	wxChoice* AddPaletteStyleChoice(wxWindow* parent, wxSizer* sizer, const wxString& short_description, const wxString& description, const std::string& setting);
	void SetPaletteStyleChoice(wxChoice* ctrl, int key);

	// Create windows
	wxNotebookPage* CreateGeneralPage();
	wxNotebookPage* CreatePerformancePage();
	wxNotebookPage* CreateUIPage();
	wxNotebookPage* CreateEditorPage();
	wxNotebookPage* CreateHotkeysPage();

	DECLARE_EVENT_TABLE()
};

#endif
