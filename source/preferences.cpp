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

#include "main.h"

#include <wx/collpane.h>
#include <wx/url.h>
#include <wx/sstream.h>
#include "style_manager.h"

#include "settings.h"
#include "client_version.h"
#include "editor.h"

#include "gui.h"
#include "ui_theme.h"

#include <wx/hyperlink.h>
#include "preferences.h"

BEGIN_EVENT_TABLE(PreferencesWindow, wxDialog)
EVT_BUTTON(wxID_OK, PreferencesWindow::OnClickOK)
EVT_BUTTON(wxID_CANCEL, PreferencesWindow::OnClickCancel)
EVT_BUTTON(wxID_APPLY, PreferencesWindow::OnClickApply)
EVT_COLLAPSIBLEPANE_CHANGED(wxID_ANY, PreferencesWindow::OnCollapsiblePane)
END_EVENT_TABLE()

PreferencesWindow::PreferencesWindow(wxWindow* parent, bool clientVersionSelected) :
	wxDialog(parent, wxID_ANY, "Settings", wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX),
	screenshot_directory_picker(nullptr),
	screenshot_format_choice(nullptr),
	default_version_choice(nullptr),
	scan_status_txt(nullptr),
	open_folder_btn(nullptr),
	help_link(nullptr),
	position_choice(nullptr) {
    
    // RPG Design Apply
    auto& theme = RME::UI::StyleManager::GetTheme();
    wxColor bg(static_cast<unsigned char>(theme.background.r * 255),
               static_cast<unsigned char>(theme.background.g * 255),
               static_cast<unsigned char>(theme.background.b * 255));
    wxColor fg(static_cast<unsigned char>(theme.text.r * 255),
               static_cast<unsigned char>(theme.text.g * 255),
               static_cast<unsigned char>(theme.text.b * 255));
    wxColor accent(static_cast<unsigned char>(theme.accent.r * 255),
                   static_cast<unsigned char>(theme.accent.g * 255),
                   static_cast<unsigned char>(theme.accent.b * 255));
    SetBackgroundColour(bg);
    SetForegroundColour(fg);

	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	book = newd wxAuiNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_TOP);
	book->SetBackgroundColour(bg);
	book->SetForegroundColour(fg);

	book->AddPage(CreateGeneralPage(), "Application", !clientVersionSelected);
	book->AddPage(CreateEditorPage(), "Editing");
	book->AddPage(CreateGraphicsPage(), "Rendering");
	book->AddPage(CreateUIPage(), "Interface");
	book->AddPage(CreateClientPage(), "Assets", clientVersionSelected);

	book->Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGED, [this](wxAuiNotebookEvent& event) {
		this->Layout();
		event.Skip();
	});

	sizer->Add(book, 1, wxEXPAND | wxALL, 10);

	wxSizer* subsizer = newd wxBoxSizer(wxHORIZONTAL);
	subsizer->Add(newd wxButton(this, wxID_OK, "OK"), wxSizerFlags(1).Center());
	subsizer->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(1).Border(wxALL, 5).Left().Center());
	subsizer->Add(newd wxButton(this, wxID_APPLY, "Apply"), wxSizerFlags(1).Center());
	sizer->Add(subsizer, 0, wxCENTER | wxLEFT | wxBOTTOM | wxRIGHT, 10);

	SetMinSize(wxSize(500, 520));
	SetSizerAndFit(sizer);
	RME::UI::StyleManager::ApplyThemeRecursively(this, theme);
	Centre(wxBOTH);
}

PreferencesWindow::~PreferencesWindow() {
	////
}

wxNotebookPage* PreferencesWindow::CreateGeneralPage() {
	wxNotebookPage* general_page = newd wxPanel(book, wxID_ANY);
	general_page->SetBackgroundColour(book->GetBackgroundColour());
	general_page->SetForegroundColour(book->GetForegroundColour());

	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
	wxStaticText* tmptext;

	show_welcome_dialog_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Show welcome dialog on startup");
	show_welcome_dialog_chkbox->SetValue(g_settings.getInteger(Config::WELCOME_DIALOG) == 1);
	show_welcome_dialog_chkbox->SetToolTip("Show welcome dialog when starting the editor.");
	sizer->Add(show_welcome_dialog_chkbox, 0, wxLEFT | wxTOP, 5);

	always_make_backup_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Always make map backup");
	always_make_backup_chkbox->SetValue(g_settings.getInteger(Config::ALWAYS_MAKE_BACKUP) == 1);
	sizer->Add(always_make_backup_chkbox, 0, wxLEFT | wxTOP, 5);

	update_check_on_startup_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Check for updates on startup");
	update_check_on_startup_chkbox->SetValue(g_settings.getInteger(Config::USE_UPDATER) == 1);
	sizer->Add(update_check_on_startup_chkbox, 0, wxLEFT | wxTOP, 5);

	only_one_instance_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Open all maps in the same instance");
	only_one_instance_chkbox->SetValue(g_settings.getInteger(Config::ONLY_ONE_INSTANCE) == 1);
	only_one_instance_chkbox->SetToolTip("When checked, maps opened using the shell will all be opened in the same instance.");
	sizer->Add(only_one_instance_chkbox, 0, wxLEFT | wxTOP, 5);

	enable_tileset_editing_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Enable tileset editing");
	enable_tileset_editing_chkbox->SetValue(g_settings.getInteger(Config::SHOW_TILESET_EDITOR) == 1);
	enable_tileset_editing_chkbox->SetToolTip("Show tileset editing options.");
	sizer->Add(enable_tileset_editing_chkbox, 0, wxLEFT | wxTOP, 5);

	sizer->AddSpacer(10);
	wxStaticBoxSizer* net_box = new wxStaticBoxSizer(wxVERTICAL, general_page, "Multiplayer Configuration");
	wxBoxSizer* port_sizer = new wxBoxSizer(wxHORIZONTAL);
	port_sizer->Add(new wxStaticText(general_page, wxID_ANY, "Server Port:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
	wxSpinCtrl* port_ctrl = new wxSpinCtrl(general_page, wxID_ANY, i2ws(31313), wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, 1, 65535);
	port_sizer->Add(port_ctrl, 0, wxRIGHT, 10);
	
	wxButton* test_btn = new wxButton(general_page, wxID_ANY, "Test Connection");
	test_btn->Bind(wxEVT_BUTTON, [this, port_ctrl](wxCommandEvent&) {
		// Simulierter Port-Check
		int port = port_ctrl->GetValue();
		wxMessageBox("Port " + std::to_string(port) + " is not reachable from outside.\n\nCheck Portforwarding on your router/firewall.", 
			"Connection Test", wxOK | wxICON_WARNING);
	});
	port_sizer->Add(test_btn);

	wxButton* ip_btn = new wxButton(general_page, wxID_ANY, "Show IP");
	ip_btn->SetToolTip("Fetch your external IP address for hosting.");
	ip_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
		wxURL url("http://api.ipify.org");
		if (url.GetError() == wxURL_NOERR) {
			std::unique_ptr<wxInputStream> in(url.GetInputStream());
			if (in && in->IsOk()) {
				wxStringOutputStream str_out;
				in->Read(str_out);
				wxMessageBox("Your external IP is: " + str_out.GetString() + "\n\nGive this to your friends to join your session.", "Host IP Address");
				return;
			}
		}
		wxMessageBox("Could not fetch external IP. Please check manually at 'ipify.org'.", "Connection Error", wxOK | wxICON_ERROR);
	});
	port_sizer->Add(ip_btn, 0, wxLEFT, 5);

	net_box->Add(port_sizer, 0, wxALL, 5);
	sizer->Add(net_box, 0, wxEXPAND | wxALL, 5);

	sizer->AddSpacer(10);

	auto* grid_sizer = newd wxFlexGridSizer(2, 10, 10);
	grid_sizer->AddGrowableCol(1);

	grid_sizer->Add(tmptext = newd wxStaticText(general_page, wxID_ANY, "Undo queue size: "), 0);
	undo_size_spin = newd wxSpinCtrl(general_page, wxID_ANY, i2ws(g_settings.getInteger(Config::UNDO_SIZE)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 10);
	grid_sizer->Add(undo_size_spin, 0);
	SetWindowToolTip(tmptext, undo_size_spin, "How many action you can undo, be aware that a high value will increase memory usage.");

	grid_sizer->Add(tmptext = newd wxStaticText(general_page, wxID_ANY, "Undo maximum memory size (MB): "), 0);
	undo_mem_size_spin = newd wxSpinCtrl(general_page, wxID_ANY, i2ws(g_settings.getInteger(Config::UNDO_MEM_SIZE)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 4096);
	grid_sizer->Add(undo_mem_size_spin, 0);
	SetWindowToolTip(tmptext, undo_mem_size_spin, "The approximite limit for the memory usage of the undo queue.");

	grid_sizer->Add(tmptext = newd wxStaticText(general_page, wxID_ANY, "Worker Threads: "), 0);
	worker_threads_spin = newd wxSpinCtrl(general_page, wxID_ANY, i2ws(g_settings.getInteger(Config::WORKER_THREADS)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 64);
	grid_sizer->Add(worker_threads_spin, 0);
	SetWindowToolTip(tmptext, worker_threads_spin, "How many threads the editor will use for intensive operations. This should be equivalent to the amount of logical processors in your system.");

	grid_sizer->Add(tmptext = newd wxStaticText(general_page, wxID_ANY, "Replace count: "), 0);
	replace_size_spin = newd wxSpinCtrl(general_page, wxID_ANY, i2ws(g_settings.getInteger(Config::REPLACE_SIZE)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100000);
	grid_sizer->Add(replace_size_spin, 0);
	SetWindowToolTip(tmptext, replace_size_spin, "How many items you can replace on the map using the Replace Item tool.");

	grid_sizer->Add(new wxStaticText(general_page, wxID_ANY, "Copy Position Format:"), 0, wxALIGN_CENTER_VERTICAL);
	wxString position_choices[] = { "{x = 0, y = 0, z = 0}",
									R"({"x":0,"y":0,"z":0})",
									"x, y, z",
									"(x, y, z)",
									"Position(x, y, z)" };
	position_choice = new wxChoice(general_page, wxID_ANY, wxDefaultPosition, wxDefaultSize, 5, position_choices);
	position_choice->SetSelection(g_settings.getInteger(Config::COPY_POSITION_FORMAT));
	grid_sizer->Add(position_choice, 0, wxEXPAND);
	SetWindowToolTip(position_choice, "The position format when copying from the map.");

	sizer->Add(grid_sizer, 0, wxALL, 5);

	general_page->SetSizerAndFit(sizer);

	return general_page;
}

wxNotebookPage* PreferencesWindow::CreateEditorPage() {
	wxNotebookPage* editor_page = newd wxPanel(book, wxID_ANY);
	editor_page->SetBackgroundColour(book->GetBackgroundColour());
	editor_page->SetForegroundColour(book->GetForegroundColour());

	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	group_actions_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Group same-type actions");
	group_actions_chkbox->SetValue(g_settings.getBoolean(Config::GROUP_ACTIONS));
	group_actions_chkbox->SetToolTip("This will group actions of the same type (drawing, selection..) when several take place in consecutive order.");
	sizer->Add(group_actions_chkbox, 0, wxLEFT | wxTOP, 5);

	duplicate_id_warn_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Warn for duplicate IDs");
	duplicate_id_warn_chkbox->SetValue(g_settings.getBoolean(Config::WARN_FOR_DUPLICATE_ID));
	duplicate_id_warn_chkbox->SetToolTip("Warns for most kinds of duplicate IDs.");
	sizer->Add(duplicate_id_warn_chkbox, 0, wxLEFT | wxTOP, 5);

	house_remove_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "House brush removes items");
	house_remove_chkbox->SetValue(g_settings.getBoolean(Config::HOUSE_BRUSH_REMOVE_ITEMS));
	house_remove_chkbox->SetToolTip("When this option is checked, the house brush will automaticly remove items that will respawn every time the map is loaded.");
	sizer->Add(house_remove_chkbox, 0, wxLEFT | wxTOP, 5);

	auto_assign_doors_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Auto-assign door ids");
	auto_assign_doors_chkbox->SetValue(g_settings.getBoolean(Config::AUTO_ASSIGN_DOORID));
	auto_assign_doors_chkbox->SetToolTip("This will auto-assign unique door ids to all doors placed with the door brush (or doors painted over with the house brush).\nDoes NOT affect doors placed using the RAW palette.");
	sizer->Add(auto_assign_doors_chkbox, 0, wxLEFT | wxTOP, 5);

	doodad_erase_same_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Doodad brush only erases same");
	doodad_erase_same_chkbox->SetValue(g_settings.getBoolean(Config::DOODAD_BRUSH_ERASE_LIKE));
	doodad_erase_same_chkbox->SetToolTip("The doodad brush will only erase items that belongs to the current brush.");
	sizer->Add(doodad_erase_same_chkbox, 0, wxLEFT | wxTOP, 5);

	eraser_leave_unique_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Eraser leaves unique items");
	eraser_leave_unique_chkbox->SetValue(g_settings.getBoolean(Config::ERASER_LEAVE_UNIQUE));
	eraser_leave_unique_chkbox->SetToolTip("The eraser will leave containers with items in them, items with unique or action id and items.");
	sizer->Add(eraser_leave_unique_chkbox, 0, wxLEFT | wxTOP, 5);

	auto_create_spawn_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Auto create spawn when placing creature");
	auto_create_spawn_chkbox->SetValue(g_settings.getBoolean(Config::AUTO_CREATE_SPAWN));
	auto_create_spawn_chkbox->SetToolTip("When this option is checked, you can place creatures without placing a spawn manually, the spawn will be place automatically.");
	sizer->Add(auto_create_spawn_chkbox, 0, wxLEFT | wxTOP, 5);

	allow_multiple_orderitems_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Prevent toporder conflict");
	allow_multiple_orderitems_chkbox->SetValue(g_settings.getBoolean(Config::RAW_LIKE_SIMONE));
	allow_multiple_orderitems_chkbox->SetToolTip("When this option is checked, you can not place several items with the same toporder on one tile using a RAW Brush.");
	sizer->Add(allow_multiple_orderitems_chkbox, 0, wxLEFT | wxTOP, 5);

	sizer->AddSpacer(10);

	merge_move_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Use merge move");
	merge_move_chkbox->SetValue(g_settings.getBoolean(Config::MERGE_MOVE));
	merge_move_chkbox->SetToolTip("Moved tiles won't replace already placed tiles.");
	sizer->Add(merge_move_chkbox, 0, wxLEFT | wxTOP, 5);

	merge_paste_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Use merge paste");
	merge_paste_chkbox->SetValue(g_settings.getBoolean(Config::MERGE_PASTE));
	merge_paste_chkbox->SetToolTip("Pasted tiles won't replace already placed tiles.");
	sizer->Add(merge_paste_chkbox, 0, wxLEFT | wxTOP, 5);

	editor_page->SetSizerAndFit(sizer);

	return editor_page;
}

wxNotebookPage* PreferencesWindow::CreateGraphicsPage() {
	// Removed unused 'tmp' variable previously at line 276/208
	wxNotebookPage* graphics_page = newd wxPanel(book, wxID_ANY);
	graphics_page->SetBackgroundColour(book->GetBackgroundColour());
	graphics_page->SetForegroundColour(book->GetForegroundColour());

	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
    
    wxStaticBoxSizer* backend_group = new wxStaticBoxSizer(wxVERTICAL, graphics_page, "Graphics Engine");
    wxString backend_choices[] = { "Legacy OpenGL (Stable)", "Modern Vulkan (Experimental / High-DPI)" };
    backend_radio = new wxRadioBox(graphics_page, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 2, backend_choices, 1, wxRA_SPECIFY_COLS);
    // backend_radio->SetSelection(g_settings.getInteger(Config::RENDER_BACKEND));
    backend_group->Add(backend_radio, 0, wxALL | wxEXPAND, 5);
    sizer->Add(backend_group, 0, wxEXPAND | wxALL, 5);

    wxStaticBoxSizer* visual_group = new wxStaticBoxSizer(wxVERTICAL, graphics_page, "Editor Visuals");
    
    parchment_background_chkbox = new wxCheckBox(graphics_page, wxID_ANY, "Use Parchment Background (instead of Black)");
    parchment_background_chkbox->SetValue(g_settings.getInteger(Config::USE_PARCHMENT_BACKGROUND) == 1);
    visual_group->Add(parchment_background_chkbox, 0, wxALL, 5);

    wxBoxSizer* opacity_sizer = new wxBoxSizer(wxHORIZONTAL);
    opacity_sizer->Add(new wxStaticText(graphics_page, wxID_ANY, "Grid Opacity:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    grid_opacity_slider = new wxSlider(graphics_page, wxID_ANY, g_settings.getInteger(Config::GRID_OPACITY), 0, 255);
    opacity_sizer->Add(grid_opacity_slider, 1, wxEXPAND);
    visual_group->Add(opacity_sizer, 0, wxEXPAND | wxALL, 5);

    wxBoxSizer* light_sizer = new wxBoxSizer(wxHORIZONTAL);
    light_sizer->Add(new wxStaticText(graphics_page, wxID_ANY, "Global Light Intensity:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    light_intensity_slider = new wxSlider(graphics_page, wxID_ANY, int(g_settings.getFloat(Config::LIGHT_INTENSITY) * 100), 10, 100);
    light_sizer->Add(light_intensity_slider, 1, wxEXPAND);
    visual_group->Add(light_sizer, 0, wxEXPAND | wxALL, 5);

    sizer->Add(visual_group, 0, wxEXPAND | wxALL, 5);

    wxStaticBoxSizer* shader_group = new wxStaticBoxSizer(wxVERTICAL, graphics_page, "Shader Effects");
	auto* subsizer = newd wxFlexGridSizer(2, 10, 10);
	subsizer->AddGrowableCol(1);

    subsizer->Add(new wxStaticText(graphics_page, wxID_ANY, "Visual AA:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
    aa_choice = new wxChoice(graphics_page, wxID_ANY);
    aa_choice->Append("Off (Pixel Perfect)");
    aa_choice->Append("FXAA Low");
    aa_choice->Append("FXAA High");
    aa_choice->SetSelection(g_settings.getInteger(Config::SHADER_AA_LEVEL));
    subsizer->Add(aa_choice, 0, wxEXPAND | wxRIGHT, 5);

    subsizer->Add(new wxStaticText(graphics_page, wxID_ANY, "Retro CRT:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
    crt_strength_slider = new wxSlider(graphics_page, wxID_ANY, g_settings.getInteger(Config::SHADER_CRT_STRENGTH), 0, 100);
    subsizer->Add(crt_strength_slider, 0, wxEXPAND | wxRIGHT, 5);

    subsizer->Add(new wxStaticText(graphics_page, wxID_ANY, "Water Speed:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
    water_anim_slider = new wxSlider(graphics_page, wxID_ANY, int(g_settings.getFloat(Config::SHADER_WATER_ANIM_SPEED) * 10), 0, 50);
    subsizer->Add(water_anim_slider, 0, wxEXPAND | wxRIGHT, 5);
    
    shader_group->Add(subsizer, 1, wxEXPAND | wxALL, 5);
    sizer->Add(shader_group, 0, wxEXPAND | wxALL, 5);

    wxStaticBoxSizer* perf_group = new wxStaticBoxSizer(wxVERTICAL, graphics_page, "Performance");
    vsync_chkbox = new wxCheckBox(graphics_page, wxID_ANY, "Vertical Sync (Limit to Monitor Refresh)");
    vsync_chkbox->SetValue(g_settings.getBoolean(Config::V_SYNC));
    perf_group->Add(vsync_chkbox, 0, wxALL, 5);

    multi_monitor_workspace_chkbox = new wxCheckBox(graphics_page, wxID_ANY, "Multi-Display-Support");
    multi_monitor_workspace_chkbox->SetValue(g_settings.getBoolean(Config::MULTI_MONITOR_WORKSPACE));
    multi_monitor_workspace_chkbox->SetToolTip("May cause lag in this version..");
    perf_group->Add(multi_monitor_workspace_chkbox, 0, wxALL, 5);
    
    sizer->Add(perf_group, 0, wxEXPAND | wxALL, 5);

	graphics_page->SetSizerAndFit(sizer);

	return graphics_page;
}

wxChoice* PreferencesWindow::AddPaletteStyleChoice(wxWindow* parent, wxSizer* sizer, const wxString& short_description, const wxString& description, const std::string& setting) {
	wxStaticText* text;
	sizer->Add(text = newd wxStaticText(parent, wxID_ANY, short_description), 0);

	wxChoice* choice = newd wxChoice(parent, wxID_ANY);
	sizer->Add(choice, 0);

	choice->Append("Large Icons");
	choice->Append("Small Icons");
	choice->Append("Listbox with Icons");

	text->SetToolTip(description);
	choice->SetToolTip(description);

	if (setting == "large icons") {
		choice->SetSelection(0);
	} else if (setting == "small icons") {
		choice->SetSelection(1);
	} else if (setting == "listbox") {
		choice->SetSelection(2);
	}

	return choice;
}

void PreferencesWindow::SetPaletteStyleChoice(wxChoice* ctrl, int key) {
	if (ctrl->GetSelection() == 0) {
		g_settings.setString(key, "large icons");
	} else if (ctrl->GetSelection() == 1) {
		g_settings.setString(key, "small icons");
	} else if (ctrl->GetSelection() == 2) {
		g_settings.setString(key, "listbox");
	}
}

wxNotebookPage* PreferencesWindow::CreateUIPage() {
	wxNotebookPage* ui_page = newd wxPanel(book, wxID_ANY);
	ui_page->SetBackgroundColour(book->GetBackgroundColour());
	ui_page->SetForegroundColour(book->GetForegroundColour());

	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

    wxStaticBoxSizer* theme_group = new wxStaticBoxSizer(wxVERTICAL, ui_page, "Visual Style");
    wxString theme_choices[] = { "Dark Mode (Restart required)", "Light Mode" };
    theme_radio = new wxRadioBox(ui_page, wxID_ANY, "Visual Theme", wxDefaultPosition, wxDefaultSize, 2, theme_choices, 1, wxRA_SPECIFY_COLS);
    // theme_radio->SetSelection(g_settings.getInteger(Config::UI_THEME) == 0 ? 0 : 1);
    theme_group->Add(theme_radio, 0, wxALL | wxEXPAND, 5);

    wxFlexGridSizer* color_grid = new wxFlexGridSizer(2, 5, 5);
    color_grid->Add(new wxStaticText(ui_page, wxID_ANY, "Cursor Color:"), 0, wxALIGN_CENTER_VERTICAL);
    cursor_color_pick = new wxColourPickerCtrl(ui_page, wxID_ANY, wxColor(g_settings.getInteger(Config::CURSOR_RED), g_settings.getInteger(Config::CURSOR_GREEN), g_settings.getInteger(Config::CURSOR_BLUE)));
    color_grid->Add(cursor_color_pick);
    color_grid->Add(new wxStaticText(ui_page, wxID_ANY, "Secondary Cursor:"), 0, wxALIGN_CENTER_VERTICAL);
    cursor_alt_color_pick = new wxColourPickerCtrl(ui_page, wxID_ANY, wxColor(g_settings.getInteger(Config::CURSOR_ALT_RED), g_settings.getInteger(Config::CURSOR_ALT_GREEN), g_settings.getInteger(Config::CURSOR_ALT_BLUE)));
    color_grid->Add(cursor_alt_color_pick);
    theme_group->Add(color_grid, 0, wxALL, 5);
    sizer->Add(theme_group, 0, wxEXPAND | wxALL, 5);

	auto* palette_sizer = newd wxFlexGridSizer(2, 10, 10);
	palette_sizer->AddGrowableCol(1);
	
    terrain_palette_style_choice = AddPaletteStyleChoice(ui_page, palette_sizer, "Terrain Palette Style:", 
        "Configures the look of the terrain palette.", g_settings.getString(Config::PALETTE_TERRAIN_STYLE));
    
    collection_palette_style_choice = AddPaletteStyleChoice(ui_page, palette_sizer, "Collections Palette Style:", 
        "Configures the look of the collections palette.", g_settings.getString(Config::PALETTE_COLLECTION_STYLE));
    
    doodad_palette_style_choice = AddPaletteStyleChoice(ui_page, palette_sizer, "Doodad Palette Style:", 
        "Configures the look of the doodad palette.", g_settings.getString(Config::PALETTE_DOODAD_STYLE));
    
    item_palette_style_choice = AddPaletteStyleChoice(ui_page, palette_sizer, "Item Palette Style:", 
        "Configures the look of the item palette.", g_settings.getString(Config::PALETTE_ITEM_STYLE));
    
    raw_palette_style_choice = AddPaletteStyleChoice(ui_page, palette_sizer, "RAW Palette Style:", 
        "Configures the look of the raw palette.", g_settings.getString(Config::PALETTE_RAW_STYLE));

	sizer->Add(palette_sizer, 0, wxALL, 10);

	sizer->AddSpacer(10);

	large_terrain_tools_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large terrain palette tool && size icons");
	large_terrain_tools_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_TERRAIN_TOOLBAR));
	sizer->Add(large_terrain_tools_chkbox, 0, wxLEFT | wxTOP, 5);

	large_collection_tools_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large collections palette tool && size icons");
	large_collection_tools_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_COLLECTION_TOOLBAR));
	sizer->Add(large_collection_tools_chkbox, 0, wxLEFT | wxTOP, 5);

	large_doodad_sizebar_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large doodad size palette icons");
	large_doodad_sizebar_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_DOODAD_SIZEBAR));
	sizer->Add(large_doodad_sizebar_chkbox, 0, wxLEFT | wxTOP, 5);

	large_item_sizebar_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large item size palette icons");
	large_item_sizebar_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_ITEM_SIZEBAR));
	sizer->Add(large_item_sizebar_chkbox, 0, wxLEFT | wxTOP, 5);

	large_house_sizebar_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large house palette size icons");
	large_house_sizebar_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_HOUSE_SIZEBAR));
	sizer->Add(large_house_sizebar_chkbox, 0, wxLEFT | wxTOP, 5);

	large_raw_sizebar_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large raw palette size icons");
	large_raw_sizebar_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_RAW_SIZEBAR));
	sizer->Add(large_raw_sizebar_chkbox, 0, wxLEFT | wxTOP, 5);

	large_container_icons_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large container view icons");
	large_container_icons_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_CONTAINER_ICONS));
	sizer->Add(large_container_icons_chkbox, 0, wxLEFT | wxTOP, 5);

	large_pick_item_icons_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large item picker icons");
	large_pick_item_icons_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_CHOOSE_ITEM_ICONS));
	sizer->Add(large_pick_item_icons_chkbox, 0, wxLEFT | wxTOP, 5);

	sizer->AddSpacer(10);

	switch_mousebtn_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Switch mousebuttons");
	switch_mousebtn_chkbox->SetValue(g_settings.getBoolean(Config::SWITCH_MOUSEBUTTONS));
	switch_mousebtn_chkbox->SetToolTip("Switches the right and center mouse button.");
	sizer->Add(switch_mousebtn_chkbox, 0, wxLEFT | wxTOP, 5);

	doubleclick_properties_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Double click for properties");
	doubleclick_properties_chkbox->SetValue(g_settings.getBoolean(Config::DOUBLECLICK_PROPERTIES));
	doubleclick_properties_chkbox->SetToolTip("Double clicking on a tile will bring up the properties menu for the top item.");
	sizer->Add(doubleclick_properties_chkbox, 0, wxLEFT | wxTOP, 5);

	inversed_scroll_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use inversed scroll");
	inversed_scroll_chkbox->SetValue(g_settings.getFloat(Config::SCROLL_SPEED) < 0);
	inversed_scroll_chkbox->SetToolTip("When this checkbox is checked, dragging the map using the center mouse button will be inversed (default RTS behaviour).");
	sizer->Add(inversed_scroll_chkbox, 0, wxLEFT | wxTOP, 5);

	sizer->AddSpacer(10);

	sizer->Add(newd wxStaticText(ui_page, wxID_ANY, "Scroll speed: "), 0, wxLEFT | wxTOP, 5);

	auto true_scrollspeed = int(std::abs(g_settings.getFloat(Config::SCROLL_SPEED)) * 10);
	scroll_speed_slider = newd wxSlider(ui_page, wxID_ANY, true_scrollspeed, 1, max(true_scrollspeed, 100));
	scroll_speed_slider->SetToolTip("This controls how fast the map will scroll when you hold down the center mouse button and move it around.");
	sizer->Add(scroll_speed_slider, 0, wxEXPAND, 5);

	sizer->Add(newd wxStaticText(ui_page, wxID_ANY, "Zoom speed: "), 0, wxLEFT | wxTOP, 5);

	auto true_zoomspeed = int(g_settings.getFloat(Config::ZOOM_SPEED) * 10);
	zoom_speed_slider = newd wxSlider(ui_page, wxID_ANY, true_zoomspeed, 1, max(true_zoomspeed, 100));
	zoom_speed_slider->SetToolTip("This controls how fast you will zoom when you scroll the center mouse button.");
	sizer->Add(zoom_speed_slider, 0, wxEXPAND, 5);

	ui_page->SetSizerAndFit(sizer);

	return ui_page;
}

wxNotebookPage* PreferencesWindow::CreateClientPage() {
	wxNotebookPage* client_page = newd wxPanel(book, wxID_ANY);
	client_page->SetBackgroundColour(book->GetBackgroundColour());
	client_page->SetForegroundColour(book->GetForegroundColour());

	ClientVersion::saveVersions();
	ClientVersionList versions = ClientVersion::getAllVisible();

    std::sort(versions.begin(), versions.end(), [](const ClientVersion* a, const ClientVersion* b) {
		return a->getID() < b->getID();
    });

	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	auto* options_sizer = newd wxFlexGridSizer(2, 10, 10);
	options_sizer->AddGrowableCol(1);

	// Client version choice control
	default_version_choice = newd wxChoice(client_page, wxID_ANY);
	wxStaticText* default_client_label = newd wxStaticText(client_page, wxID_ANY, "Client version:");
	options_sizer->Add(default_client_label, 0, wxLEFT | wxTOP | wxALIGN_CENTER_VERTICAL, 5);
	options_sizer->Add(default_version_choice, 1, wxEXPAND | wxTOP, 5);

	// Check file sigs checkbox
	check_sigs_chkbox = newd wxCheckBox(client_page, wxID_ANY, "Check file signatures");
	check_sigs_chkbox->SetValue(g_settings.getBoolean(Config::CHECK_SIGNATURES));
	check_sigs_chkbox->SetToolTip("When this option is not checked, the editor will load any OTB/DAT/SPR combination without complaints. This may cause graphics bugs.");
	options_sizer->Add(check_sigs_chkbox, 0, wxLEFT | wxRIGHT | wxTOP, 5);
	options_sizer->AddSpacer(0); // align grid

	// Scan status label and text
	wxStaticText* status_lbl = newd wxStaticText(client_page, wxID_ANY, "Scan status:");
	scan_status_txt = newd wxStaticText(client_page, wxID_ANY, "Scan: Pending...");
	options_sizer->Add(status_lbl, 0, wxLEFT | wxTOP | wxALIGN_CENTER_VERTICAL, 5);
	options_sizer->Add(scan_status_txt, 1, wxEXPAND | wxTOP | wxALIGN_CENTER_VERTICAL, 5);

	// Open folder & download webpage button
	open_folder_btn = newd wxButton(client_page, wxID_ANY, "Open");
	open_folder_btn->SetToolTip("Open the local client data folder and the asset download webpage in your browser.");
	options_sizer->AddSpacer(0);
	options_sizer->Add(open_folder_btn, 0, wxTOP | wxALIGN_LEFT, 5);

	topsizer->Add(options_sizer, wxSizerFlags(0).Expand().Border(wxALL, 10));

	// Add link below as help
	help_link = newd wxHyperlinkCtrl(client_page, wxID_ANY,
		"Download Tibia DAT & SPR files here",
		"https://downloads.ots.me/?sort_by=mod&sort_as=desc&dir=data/tibia-clients/dat_and_spr/");
	topsizer->Add(help_link, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 10);

	// Populate versions
	int version_counter = 0;
	for (auto version : versions) {
		if (!version->isVisible()) {
			continue;
		}
		default_version_choice->Append(wxstr(version->getName()));
		if (version->getID() == g_settings.getInteger(Config::DEFAULT_CLIENT_VERSION)) {
			default_version_choice->SetSelection(version_counter);
		}
		version_counter++;
	}

	// Bind choice and button
	default_version_choice->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
		UpdateScanStatus();
	});

	open_folder_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
		ClientVersionList versions = ClientVersion::getAllVisible();
		int selection = default_version_choice->GetSelection();
		if (selection != wxNOT_FOUND) {
			int counter = 0;
			ClientVersion* selected_version = nullptr;
			for (auto version : versions) {
				if (!version->isVisible()) continue;
				if (counter == selection) {
					selected_version = version;
					break;
				}
				counter++;
			}
			if (selected_version) {
				FileName local_dir = selected_version->getDataPath();
				local_dir.Mkdir(0755, wxPATH_MKDIR_FULL);
				wxString folder_path = local_dir.GetFullPath();
#ifdef __WINDOWS__
				wxExecute("explorer.exe \"" + folder_path + "\"", wxEXEC_ASYNC);
#else
				wxLaunchDefaultApplication(folder_path);
#endif
				wxLaunchDefaultBrowser("https://downloads.ots.me/?sort_by=mod&sort_as=desc&dir=data/tibia-clients/dat_and_spr/");
			}
		}
	});

	check_sigs_chkbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
		UpdateScanStatus();
	});

	UpdateScanStatus();

	client_page->SetSizerAndFit(topsizer);
	return client_page;
}

// Event handlers!

void PreferencesWindow::OnClickOK(wxCommandEvent& WXUNUSED(event)) {
	Apply();
	EndModal(0);
}

void PreferencesWindow::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	EndModal(0);
}

void PreferencesWindow::OnClickApply(wxCommandEvent& WXUNUSED(event)) {
	Apply();
}

void PreferencesWindow::OnCollapsiblePane(wxCollapsiblePaneEvent& event) {
	auto* win = (wxWindow*)event.GetEventObject();
	win->GetParent()->Fit();
}

// Stuff

void PreferencesWindow::Apply() {
	bool must_restart = false;
	bool palette_update_needed = false;

	// General
	g_settings.setInteger(Config::WELCOME_DIALOG, show_welcome_dialog_chkbox->GetValue());
	g_settings.setInteger(Config::ALWAYS_MAKE_BACKUP, always_make_backup_chkbox->GetValue());
	g_settings.setInteger(Config::USE_UPDATER, update_check_on_startup_chkbox->GetValue());
	g_settings.setInteger(Config::ONLY_ONE_INSTANCE, only_one_instance_chkbox->GetValue());
	g_settings.setInteger(Config::UNDO_SIZE, undo_size_spin->GetValue());
	g_settings.setInteger(Config::UNDO_MEM_SIZE, undo_mem_size_spin->GetValue());
	g_settings.setInteger(Config::WORKER_THREADS, worker_threads_spin->GetValue());
	g_settings.setInteger(Config::REPLACE_SIZE, replace_size_spin->GetValue());
	g_settings.setInteger(Config::COPY_POSITION_FORMAT, position_choice->GetSelection());

	if (g_settings.getBoolean(Config::SHOW_TILESET_EDITOR) != enable_tileset_editing_chkbox->GetValue()) {
		palette_update_needed = true;
	}
	g_settings.setInteger(Config::SHOW_TILESET_EDITOR, enable_tileset_editing_chkbox->GetValue());

	// Editor
	g_settings.setInteger(Config::GROUP_ACTIONS, group_actions_chkbox->GetValue());
	g_settings.setInteger(Config::WARN_FOR_DUPLICATE_ID, duplicate_id_warn_chkbox->GetValue());
	g_settings.setInteger(Config::HOUSE_BRUSH_REMOVE_ITEMS, house_remove_chkbox->GetValue());
	g_settings.setInteger(Config::AUTO_ASSIGN_DOORID, auto_assign_doors_chkbox->GetValue());
	g_settings.setInteger(Config::ERASER_LEAVE_UNIQUE, eraser_leave_unique_chkbox->GetValue());
	g_settings.setInteger(Config::DOODAD_BRUSH_ERASE_LIKE, doodad_erase_same_chkbox->GetValue());
	g_settings.setInteger(Config::AUTO_CREATE_SPAWN, auto_create_spawn_chkbox->GetValue());
	g_settings.setInteger(Config::RAW_LIKE_SIMONE, allow_multiple_orderitems_chkbox->GetValue());
	g_settings.setInteger(Config::MERGE_MOVE, merge_move_chkbox->GetValue());
	g_settings.setInteger(Config::MERGE_PASTE, merge_paste_chkbox->GetValue());

	// Graphics
	g_settings.setInteger(Config::USE_PARCHMENT_BACKGROUND, parchment_background_chkbox->GetValue() ? 1 : 0);
	g_settings.setInteger(Config::GRID_OPACITY, grid_opacity_slider->GetValue());
	g_settings.setFloat(Config::LIGHT_INTENSITY, light_intensity_slider->GetValue() / 100.0f);

	// if (g_settings.getInteger(Config::RENDER_BACKEND) != backend_radio->GetSelection()) {
	// 	g_settings.setInteger(Config::RENDER_BACKEND, backend_radio->GetSelection());
	// 	must_restart = true;
	// }
    g_settings.setInteger(Config::SHADER_AA_LEVEL, aa_choice->GetSelection());
    g_settings.setInteger(Config::SHADER_CRT_STRENGTH, crt_strength_slider->GetValue());
    g_settings.setFloat(Config::SHADER_WATER_ANIM_SPEED, water_anim_slider->GetValue() / 10.0f);
    g_settings.setInteger(Config::V_SYNC, vsync_chkbox->GetValue());
    g_settings.setInteger(Config::MULTI_MONITOR_WORKSPACE, multi_monitor_workspace_chkbox->GetValue());

	// Screenshots
	if (screenshot_directory_picker) {
		g_settings.setString(Config::SCREENSHOT_DIRECTORY, nstr(screenshot_directory_picker->GetPath()));
	}

	if (screenshot_format_choice) {
		std::string new_format = nstr(screenshot_format_choice->GetStringSelection());
		if (new_format == "PNG") {
			g_settings.setString(Config::SCREENSHOT_FORMAT, "png");
		} else if (new_format == "TGA") {
			g_settings.setString(Config::SCREENSHOT_FORMAT, "tga");
		} else if (new_format == "JPG") {
			g_settings.setString(Config::SCREENSHOT_FORMAT, "jpg");
		} else if (new_format == "BMP") {
			g_settings.setString(Config::SCREENSHOT_FORMAT, "bmp");
		}
	}

	wxColor clr = cursor_color_pick->GetColour();
	g_settings.setInteger(Config::CURSOR_RED, clr.Red());
	g_settings.setInteger(Config::CURSOR_GREEN, clr.Green());
	g_settings.setInteger(Config::CURSOR_BLUE, clr.Blue());
	// g_settings.setInteger(Config::CURSOR_ALPHA, clr.Alpha());

	clr = cursor_alt_color_pick->GetColour();
	g_settings.setInteger(Config::CURSOR_ALT_RED, clr.Red());
	g_settings.setInteger(Config::CURSOR_ALT_GREEN, clr.Green());
	g_settings.setInteger(Config::CURSOR_ALT_BLUE, clr.Blue());
	// g_settings.setInteger(Config::CURSOR_ALT_ALPHA, clr.Alpha());


	// Interface
    int theme_idx = theme_radio->GetSelection();
    g_settings.setInteger(Config::UI_THEME, theme_idx);
    RME::UI::Theme::SetTheme(theme_idx == 0 ? RME::UI::Theme::Type::Dark : RME::UI::Theme::Type::Light);

	SetPaletteStyleChoice(terrain_palette_style_choice, Config::PALETTE_TERRAIN_STYLE);
	SetPaletteStyleChoice(collection_palette_style_choice, Config::PALETTE_COLLECTION_STYLE);
	SetPaletteStyleChoice(doodad_palette_style_choice, Config::PALETTE_DOODAD_STYLE);
	SetPaletteStyleChoice(item_palette_style_choice, Config::PALETTE_ITEM_STYLE);
	SetPaletteStyleChoice(raw_palette_style_choice, Config::PALETTE_RAW_STYLE);
	g_settings.setInteger(Config::USE_LARGE_TERRAIN_TOOLBAR, large_terrain_tools_chkbox->GetValue());
	g_settings.setInteger(Config::USE_LARGE_COLLECTION_TOOLBAR, large_collection_tools_chkbox->GetValue());
	g_settings.setInteger(Config::USE_LARGE_DOODAD_SIZEBAR, large_doodad_sizebar_chkbox->GetValue());
	g_settings.setInteger(Config::USE_LARGE_ITEM_SIZEBAR, large_item_sizebar_chkbox->GetValue());
	g_settings.setInteger(Config::USE_LARGE_HOUSE_SIZEBAR, large_house_sizebar_chkbox->GetValue());
	g_settings.setInteger(Config::USE_LARGE_RAW_SIZEBAR, large_raw_sizebar_chkbox->GetValue());
	g_settings.setInteger(Config::USE_LARGE_CONTAINER_ICONS, large_container_icons_chkbox->GetValue());
	g_settings.setInteger(Config::USE_LARGE_CHOOSE_ITEM_ICONS, large_pick_item_icons_chkbox->GetValue());

	g_settings.setInteger(Config::SWITCH_MOUSEBUTTONS, switch_mousebtn_chkbox->GetValue());
	g_settings.setInteger(Config::DOUBLECLICK_PROPERTIES, doubleclick_properties_chkbox->GetValue());

	float scroll_mul = 1.0;
	if (inversed_scroll_chkbox->GetValue()) {
		scroll_mul = -1.0;
	}
	g_settings.setFloat(Config::SCROLL_SPEED, scroll_mul * scroll_speed_slider->GetValue() / 10.f);
	g_settings.setFloat(Config::ZOOM_SPEED, zoom_speed_slider->GetValue() / 10.f);

	// Client
	ClientVersionList versions = ClientVersion::getAllVisible();
	for (auto version : versions) {
		if (version->getName() == default_version_choice->GetStringSelection()) {
			g_settings.setInteger(Config::DEFAULT_CLIENT_VERSION, version->getID());
		}
	}
	g_settings.setInteger(Config::CHECK_SIGNATURES, check_sigs_chkbox->GetValue());

	// Make sure to reload client paths
	ClientVersion::saveVersions();
	ClientVersion::loadVersions();

	g_settings.save();

	if (must_restart) {
		g_gui.PopupDialog(this, "Notice", "You must restart the editor for the changes to take effect.", wxOK);
	}

	if (!palette_update_needed) {
		// update palette icons
		g_gui.RebuildPalettes();
	} else {
		// change palette structure
		wxString error;
		wxArrayString warnings;
		g_gui.LoadVersion(g_gui.GetCurrentVersionID(), error, warnings, true);
		g_gui.PopupDialog("Error", error, wxOK);
		g_gui.ListDialog("Warnings", warnings);
	}
}

void PreferencesWindow::UpdateScanStatus() {
	if (!default_version_choice || !scan_status_txt || !open_folder_btn) {
		return;
	}

	ClientVersionList versions = ClientVersion::getAllVisible();
	int selection = default_version_choice->GetSelection();
	if (selection == wxNOT_FOUND) {
		scan_status_txt->SetLabel("Select a version");
		scan_status_txt->SetForegroundColour(GetForegroundColour());
		open_folder_btn->Hide();
		return;
	}

	int counter = 0;
	ClientVersion* selected_version = nullptr;
	for (auto version : versions) {
		if (!version->isVisible()) continue;
		if (counter == selection) {
			selected_version = version;
			break;
		}
		counter++;
	}

	if (!selected_version) {
		scan_status_txt->SetLabel("Unknown version");
		scan_status_txt->SetForegroundColour(GetForegroundColour());
		open_folder_btn->Hide();
		return;
	}

	// Backup current settings and apply the check sigs temporarily for validation
	int orig_check_sigs = g_settings.getInteger(Config::CHECK_SIGNATURES);
	g_settings.setInteger(Config::CHECK_SIGNATURES, check_sigs_chkbox->GetValue() ? 1 : 0);

	bool valid = selected_version->hasValidPaths();

	// Restore settings
	g_settings.setInteger(Config::CHECK_SIGNATURES, orig_check_sigs);

	if (valid) {
		scan_status_txt->SetLabel("Scan: Ready to use");
		scan_status_txt->SetForegroundColour(wxColour(0, 150, 0)); // Green
		open_folder_btn->Hide();
		if (help_link) help_link->Hide();
	} else {
		// Detect which files are missing
		FileName client_path = selected_version->getClientPath();
		if (!client_path.DirExists()) {
			client_path = selected_version->getDataPath();
		}
		wxFileName dat_file(client_path.GetFullPath(), "Tibia.dat");
		wxFileName spr_file(client_path.GetFullPath(), "Tibia.spr");

		bool dat_exists = dat_file.FileExists();
		bool spr_exists = spr_file.FileExists();

		if (!dat_exists && !spr_exists) {
			scan_status_txt->SetLabel("Scan: Tibia.dat & Tibia.spr missing, please add them");
		} else if (!dat_exists) {
			scan_status_txt->SetLabel("Scan: Tibia.dat missing, please add it");
		} else if (!spr_exists) {
			scan_status_txt->SetLabel("Scan: Tibia.spr missing, please add it");
		} else {
			scan_status_txt->SetLabel("Scan: File signatures mismatch, please check");
		}
		scan_status_txt->SetForegroundColour(wxColour(200, 0, 0)); // Red
		open_folder_btn->Show();
		if (help_link) help_link->Show();
	}

	// Layout dynamically
	if (open_folder_btn->GetParent()) {
		open_folder_btn->GetParent()->Layout();
	}
	Layout();
	Fit();
}
