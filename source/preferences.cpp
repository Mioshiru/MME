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

#include <boost/asio.hpp>

#include <wx/clipbrd.h>
#include <wx/collpane.h>
#include <wx/hyperlink.h>
#include <wx/listctrl.h>
#include <wx/process.h>
#include <wx/sstream.h>
#include <wx/url.h>

#include <cpr/cpr.h>

#include "style_manager.h"

#include "preferences.h"
#include "firewall_helper.h"
#include "client_version.h"
#include "editor.h"

#include "gui.h"
#include "ui_theme.h"
#include "preferences.h"

#ifdef __WINDOWS__
#include <shellapi.h>
#endif

namespace {
	constexpr int kDefaultMultiplayerPort = 3074;
	const wxString kPortCheckerBaseUrl = "https://portchecker.io/api/";

	bool CopyTextToClipboard(const wxString& text) {
		if (!wxTheClipboard->Open()) {
			return false;
		}

		const bool copied = wxTheClipboard->SetData(new wxTextDataObject(text));
		wxTheClipboard->Close();
		return copied;
	}

	bool FetchUrlText(const wxString& urlString, wxString& response, wxString& errorMessage) {
		std::string url = urlString.ToStdString();
		auto r = cpr::Get(cpr::Url{url}, cpr::Timeout{5000});
		if (r.status_code == 200) {
			response = wxString::FromUTF8(r.text.c_str());
			response.Trim(true);
			response.Trim(false);
			if (response.empty()) {
				errorMessage = "The network service returned an empty response.";
				return false;
			}
			return true;
		}
		errorMessage = wxString::Format("Network error (code %d): %s", r.status_code, r.error.message);
		return false;
	}

	bool GetExternalIpAddress(wxString& ipAddress, wxString& errorMessage) {
		if (FetchUrlText(kPortCheckerBaseUrl + "me", ipAddress, errorMessage)) {
			return true;
		}

		return FetchUrlText("https://api.ipify.org", ipAddress, errorMessage);
	}

	bool CheckPortReachableFromInternet(const wxString& host, int port, bool& isReachable, wxString& errorMessage) {
		wxString response;
		if (!FetchUrlText(kPortCheckerBaseUrl + host + "/" + wxString::Format("%d", port), response, errorMessage)) {
			return false;
		}

		response.MakeLower();
		if (response == "true") {
			isReachable = true;
			return true;
		}
		if (response == "false") {
			isReachable = false;
			return true;
		}

		errorMessage = "The port check service returned an unexpected response.";
		return false;
	}

	bool TryStartTemporaryPortListener(int port, wxString& errorMessage, std::unique_ptr<boost::asio::io_context>& service, std::unique_ptr<boost::asio::ip::tcp::acceptor>& acceptor) {
		try {
			service = std::make_unique<boost::asio::io_context>();
			acceptor = std::make_unique<boost::asio::ip::tcp::acceptor>(*service);
			boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), static_cast<uint16_t>(port));
			acceptor->open(endpoint.protocol());
			acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
			acceptor->bind(endpoint);
			acceptor->listen();
			return true;
		} catch (const std::exception& ex) {
			errorMessage = wxString::Format("Could not listen on port %d: %s", port, ex.what());
			acceptor.reset();
			service.reset();
			return false;
		}
	}

	void ShowCopyableExternalIpDialog(wxWindow* parent, const wxString& ipAddress) {
		wxDialog dialog(parent, wxID_ANY, "Host IP Address", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
		auto* topSizer = new wxBoxSizer(wxVERTICAL);
		topSizer->Add(new wxStaticText(&dialog, wxID_ANY, "Click the IP address to copy it to the clipboard."), 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 10);

		auto* ipButton = new wxButton(&dialog, wxID_ANY, ipAddress, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
		ipButton->SetToolTip("Copy external IP to clipboard.");
		topSizer->Add(ipButton, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxALIGN_CENTER_HORIZONTAL, 10);

		auto* statusText = new wxStaticText(&dialog, wxID_ANY, "");
		topSizer->Add(statusText, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxALIGN_CENTER_HORIZONTAL, 10);

		ipButton->Bind(wxEVT_BUTTON, [ipAddress, statusText](wxCommandEvent&) {
			statusText->SetLabel(CopyTextToClipboard(ipAddress) ? "Copied to clipboard." : "Could not access the clipboard.");
		});

		if (wxSizer* buttonSizer = dialog.CreateSeparatedButtonSizer(wxOK)) {
			topSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 10);
		}

		dialog.SetSizerAndFit(topSizer);
		dialog.CentreOnParent();
		dialog.ShowModal();
	}
}

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
	position_choice(nullptr),
	autosave_enabled_chkbox(nullptr),
	autosave_interval_slider(nullptr),
	autosave_interval_label(nullptr),
	multiplayer_port_spin(nullptr),
	ui_scale_slider(nullptr) {
    
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

	book->AddPage(CreateGeneralPage(), "General", !clientVersionSelected);
	book->AddPage(CreateEditorPage(), "Editing");
	book->AddPage(CreatePerformancePage(), "Graphics");
	book->AddPage(CreateUIPage(), "Interface");
	book->AddPage(CreateHotkeysPage(), "Hotkeys");

	book->Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGED, [this](wxAuiNotebookEvent& event) {
		this->Layout();
		event.Skip();
	});

	sizer->Add(book, 1, wxEXPAND | wxALL, 6);

	wxSizer* subsizer = newd wxBoxSizer(wxHORIZONTAL);
	subsizer->Add(newd wxButton(this, wxID_OK, "OK"), wxSizerFlags(1).Center());
	subsizer->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(1).Border(wxALL, 4).Left().Center());
	subsizer->Add(newd wxButton(this, wxID_APPLY, "Apply"), wxSizerFlags(1).Center());
	sizer->Add(subsizer, 0, wxCENTER | wxLEFT | wxBOTTOM | wxRIGHT, 6);

	SetMinSize(wxSize(720, 460));
	SetSize(wxSize(740, 480));
	SetSizer(sizer);
	RME::UI::StyleManager::ApplyThemeRecursively(this, theme);
	
	// Layout updates
	if (default_version_choice) {
		UpdateScanStatus();
	}
	
	Centre(wxBOTH);
}

PreferencesWindow::~PreferencesWindow() {
	////
}

wxNotebookPage* PreferencesWindow::CreateGeneralPage() {
	wxNotebookPage* general_page = newd wxPanel(book, wxID_ANY);
	general_page->SetBackgroundColour(book->GetBackgroundColour());
	general_page->SetForegroundColour(book->GetForegroundColour());

	wxBoxSizer* main_sizer = newd wxBoxSizer(wxVERTICAL);
	wxNotebook* sub_book = newd wxNotebook(general_page, wxID_ANY);

	// --- Sub-Tab 1: Startup & Auto-Save ---
	wxPanel* startup_panel = newd wxPanel(sub_book, wxID_ANY);
	wxBoxSizer* startup_sizer = newd wxBoxSizer(wxVERTICAL);

	show_welcome_dialog_chkbox = newd wxCheckBox(startup_panel, wxID_ANY, "Show welcome dialog on startup");
	show_welcome_dialog_chkbox->SetValue(g_settings.getInteger(Config::WELCOME_DIALOG) == 1);
	show_welcome_dialog_chkbox->SetToolTip("Show welcome dialog when starting the editor.");
	startup_sizer->Add(show_welcome_dialog_chkbox, 0, wxLEFT | wxTOP, 10);

	always_make_backup_chkbox = newd wxCheckBox(startup_panel, wxID_ANY, "Always make map backup");
	always_make_backup_chkbox->SetValue(g_settings.getInteger(Config::ALWAYS_MAKE_BACKUP) == 1);
	startup_sizer->Add(always_make_backup_chkbox, 0, wxLEFT | wxTOP, 10);

	update_check_on_startup_chkbox = newd wxCheckBox(startup_panel, wxID_ANY, "Check for updates on startup");
	update_check_on_startup_chkbox->SetValue(g_settings.getInteger(Config::USE_UPDATER) == 1);
	startup_sizer->Add(update_check_on_startup_chkbox, 0, wxLEFT | wxTOP, 10);

	only_one_instance_chkbox = newd wxCheckBox(startup_panel, wxID_ANY, "Open all maps in the same instance");
	only_one_instance_chkbox->SetValue(g_settings.getInteger(Config::ONLY_ONE_INSTANCE) == 1);
	only_one_instance_chkbox->SetToolTip("When checked, maps opened using the shell will all be opened in the same instance.");
	startup_sizer->Add(only_one_instance_chkbox, 0, wxLEFT | wxTOP, 10);

	enable_tileset_editing_chkbox = newd wxCheckBox(startup_panel, wxID_ANY, "Enable tileset editing");
	enable_tileset_editing_chkbox->SetValue(g_settings.getInteger(Config::SHOW_TILESET_EDITOR) == 1);
	enable_tileset_editing_chkbox->SetToolTip("Show tileset editing options.");
	startup_sizer->Add(enable_tileset_editing_chkbox, 0, wxLEFT | wxTOP, 10);

	wxStaticBoxSizer* autosave_box = newd wxStaticBoxSizer(wxVERTICAL, startup_panel, "Auto-Save Settings");
	autosave_enabled_chkbox = newd wxCheckBox(startup_panel, wxID_ANY, "Enable Auto-Save");
	autosave_enabled_chkbox->SetValue(g_settings.getBoolean(Config::AUTO_SAVE_ENABLED));
	autosave_enabled_chkbox->SetToolTip("Automatically saves the current map at a fixed interval.");
	autosave_box->Add(autosave_enabled_chkbox, 0, wxALL, 5);

	wxBoxSizer* slider_row = newd wxBoxSizer(wxHORIZONTAL);
	slider_row->Add(newd wxStaticText(startup_panel, wxID_ANY, "Interval:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
	int cur_interval = g_settings.getInteger(Config::AUTO_SAVE_INTERVAL);
	if (cur_interval < 5)  cur_interval = 5;
	if (cur_interval > 40) cur_interval = 40;
	autosave_interval_slider = newd wxSlider(startup_panel, wxID_ANY, cur_interval, 5, 40,
		wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	autosave_interval_slider->SetToolTip("Auto-save interval in minutes (5 to 40).");
	slider_row->Add(autosave_interval_slider, 1, wxEXPAND);
	autosave_interval_label = newd wxStaticText(startup_panel, wxID_ANY, wxString::Format(" %d min", cur_interval));
	slider_row->Add(autosave_interval_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
	autosave_box->Add(slider_row, 0, wxEXPAND | wxALL, 5);

	autosave_interval_slider->Enable(autosave_enabled_chkbox->GetValue());
	autosave_enabled_chkbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
		autosave_interval_slider->Enable(autosave_enabled_chkbox->GetValue());
	});
	autosave_interval_slider->Bind(wxEVT_SLIDER, [this](wxCommandEvent&) {
		autosave_interval_label->SetLabel(wxString::Format(" %d min", autosave_interval_slider->GetValue()));
	});
	startup_sizer->Add(autosave_box, 0, wxEXPAND | wxALL, 10);
	wxBoxSizer* pos_row = newd wxBoxSizer(wxHORIZONTAL);
	pos_row->Add(new wxStaticText(startup_panel, wxID_ANY, "Copy Position Format:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
	wxString position_choices[] = { "{x = 0, y = 0, z = 0}",
									R"({"x":0,"y":0,"z":0})",
									"x, y, z",
									"(x, y, z)",
									"Position(x, y, z)" };
	position_choice = new wxChoice(startup_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 5, position_choices);
	position_choice->SetSelection(g_settings.getInteger(Config::COPY_POSITION_FORMAT));
	pos_row->Add(position_choice, 1, wxEXPAND);
	startup_sizer->Add(pos_row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	startup_panel->SetSizer(startup_sizer);
	sub_book->AddPage(startup_panel, "Startup");

	// --- Sub-Tab 2: Assets ---
	wxPanel* asset_panel = newd wxPanel(sub_book, wxID_ANY);
	wxBoxSizer* asset_sizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer* asset_box = newd wxStaticBoxSizer(wxVERTICAL, asset_panel, "Asset & Client Configuration");
	ClientVersion::saveVersions();
	ClientVersionList versions = ClientVersion::getAllVisible();
	std::sort(versions.begin(), versions.end(), [](const ClientVersion* a, const ClientVersion* b) {
		return a->getID() < b->getID();
	});

	auto* asset_row_sizer = newd wxFlexGridSizer(2, 8, 12);
	asset_row_sizer->AddGrowableCol(1);

	default_version_choice = newd wxChoice(asset_panel, wxID_ANY);
	asset_row_sizer->Add(newd wxStaticText(asset_panel, wxID_ANY, "Client version:"), 0, wxALIGN_CENTER_VERTICAL);
	asset_row_sizer->Add(default_version_choice, 1, wxEXPAND);

	check_sigs_chkbox = newd wxCheckBox(asset_panel, wxID_ANY, "Check file signatures");
	check_sigs_chkbox->SetValue(g_settings.getBoolean(Config::CHECK_SIGNATURES));
	check_sigs_chkbox->SetToolTip("When this option is not checked, the editor will load any OTB/DAT/SPR combination without complaints.");
	asset_row_sizer->Add(check_sigs_chkbox, 0, wxTOP, 5);
	asset_row_sizer->AddSpacer(0);

	asset_row_sizer->Add(newd wxStaticText(asset_panel, wxID_ANY, "Scan status:"), 0, wxALIGN_CENTER_VERTICAL | wxTOP, 5);
	scan_status_txt = newd wxStaticText(asset_panel, wxID_ANY, "Scan: Pending...");
	asset_row_sizer->Add(scan_status_txt, 1, wxEXPAND | wxTOP | wxALIGN_CENTER_VERTICAL, 5);

	open_folder_btn = newd wxButton(asset_panel, wxID_ANY, "Open Directory");
	open_folder_btn->SetToolTip("Open the local client data folder and the asset download webpage in your browser.");
	asset_row_sizer->AddSpacer(0);
	asset_row_sizer->Add(open_folder_btn, 0, wxTOP | wxALIGN_LEFT, 5);

	asset_box->Add(asset_row_sizer, 1, wxEXPAND | wxALL, 10);

	help_link = newd wxHyperlinkCtrl(asset_panel, wxID_ANY,
		"Download Tibia DAT & SPR files here",
		"https://downloads.ots.me/?sort_by=mod&sort_as=desc&dir=data/tibia-clients/dat_and_spr/");
	asset_box->Add(help_link, 0, wxTOP | wxBOTTOM | wxALIGN_CENTER_HORIZONTAL, 5);

	int version_counter = 0;
	for (auto version : versions) {
		if (!version->isVisible()) continue;
		default_version_choice->Append(wxstr(version->getName()));
		if (version->getID() == g_settings.getInteger(Config::DEFAULT_CLIENT_VERSION)) {
			default_version_choice->SetSelection(version_counter);
		}
		version_counter++;
	}

	default_version_choice->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
		UpdateScanStatus();
	});

	open_folder_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
		ClientVersionList env_versions = ClientVersion::getAllVisible();
		int selection = default_version_choice->GetSelection();
		if (selection != wxNOT_FOUND) {
			int counter = 0;
			ClientVersion* selected_version = nullptr;
			for (auto version : env_versions) {
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

	asset_sizer->Add(asset_box, 1, wxEXPAND | wxALL, 10);
	asset_panel->SetSizer(asset_sizer);
	sub_book->AddPage(asset_panel, "Assets");

	main_sizer->Add(sub_book, 1, wxEXPAND | wxALL, 5);

	wxBoxSizer* def_sizer = newd wxBoxSizer(wxHORIZONTAL);
	wxButton* def_btn = newd wxButton(general_page, wxID_ANY, "Default");
	def_btn->SetToolTip("Reset General settings to default values.");
	def_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
		show_welcome_dialog_chkbox->SetValue(true);
		always_make_backup_chkbox->SetValue(false);
		update_check_on_startup_chkbox->SetValue(true);
		only_one_instance_chkbox->SetValue(false);
		enable_tileset_editing_chkbox->SetValue(false);
		autosave_enabled_chkbox->SetValue(false);
		autosave_interval_slider->SetValue(10);
		autosave_interval_slider->Enable(false);
		autosave_interval_label->SetLabel(" 10 min");
		check_sigs_chkbox->SetValue(false);
		position_choice->SetSelection(0);
	});
	def_sizer->AddStretchSpacer();
	def_sizer->Add(def_btn, 0, wxRIGHT | wxBOTTOM, 5);
	main_sizer->Add(def_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);

	general_page->SetSizer(main_sizer);
	return general_page;
}

wxNotebookPage* PreferencesWindow::CreateEditorPage() {
	wxNotebookPage* editor_page = newd wxPanel(book, wxID_ANY);
	editor_page->SetBackgroundColour(book->GetBackgroundColour());
	editor_page->SetForegroundColour(book->GetForegroundColour());

	wxBoxSizer* main_sizer = newd wxBoxSizer(wxVERTICAL);
	wxNotebook* sub_book = newd wxNotebook(editor_page, wxID_ANY);

	// --- Sub-Tab 1: Actions ---
	wxPanel* action_panel = newd wxPanel(sub_book, wxID_ANY);
	wxBoxSizer* action_sizer = newd wxBoxSizer(wxVERTICAL);

	group_actions_chkbox = newd wxCheckBox(action_panel, wxID_ANY, "Group actions");
	group_actions_chkbox->SetValue(g_settings.getBoolean(Config::GROUP_ACTIONS));
	group_actions_chkbox->SetToolTip("When grouping is enabled, undo will affect whole strokes instead of single nodes.");
	action_sizer->Add(group_actions_chkbox, 0, wxLEFT | wxTOP, 12);

	duplicate_id_warn_chkbox = newd wxCheckBox(action_panel, wxID_ANY, "Warn on duplicate item id");
	duplicate_id_warn_chkbox->SetValue(g_settings.getBoolean(Config::WARN_FOR_DUPLICATE_ID));
	duplicate_id_warn_chkbox->SetToolTip("Display a warning dialog when you add an item with an unique id that is already in use.");
	action_sizer->Add(duplicate_id_warn_chkbox, 0, wxLEFT | wxTOP, 12);

	house_remove_chkbox = newd wxCheckBox(action_panel, wxID_ANY, "Confirm house removal");
	house_remove_chkbox->SetValue(g_settings.getBoolean(Config::HOUSE_BRUSH_REMOVE_ITEMS));
	house_remove_chkbox->SetToolTip("Display a confirmation dialog when you try to remove a house from the map.");
	action_sizer->Add(house_remove_chkbox, 0, wxLEFT | wxTOP, 12);

	auto_assign_doors_chkbox = newd wxCheckBox(action_panel, wxID_ANY, "Auto assign doorid");
	auto_assign_doors_chkbox->SetValue(g_settings.getBoolean(Config::AUTO_ASSIGN_DOORID));
	auto_assign_doors_chkbox->SetToolTip("Auto-assigns unique door ids to all doors placed with the door brush or house brush.");
	action_sizer->Add(auto_assign_doors_chkbox, 0, wxLEFT | wxTOP, 12);

	allow_multiple_orderitems_chkbox = newd wxCheckBox(action_panel, wxID_ANY, "Prevent toporder conflict");
	allow_multiple_orderitems_chkbox->SetValue(g_settings.getBoolean(Config::RAW_LIKE_SIMONE));
	allow_multiple_orderitems_chkbox->SetToolTip("When checked, you cannot place several items with the same toporder on one tile using a RAW Brush.");
	action_sizer->Add(allow_multiple_orderitems_chkbox, 0, wxLEFT | wxTOP, 12);

	action_panel->SetSizer(action_sizer);
	sub_book->AddPage(action_panel, "Actions");

	// --- Sub-Tab 2: Brushes ---
	wxPanel* brush_panel = newd wxPanel(sub_book, wxID_ANY);
	wxBoxSizer* brush_sizer = newd wxBoxSizer(wxVERTICAL);

	doodad_erase_same_chkbox = newd wxCheckBox(brush_panel, wxID_ANY, "Doodad brush only erases same type");
	doodad_erase_same_chkbox->SetValue(g_settings.getBoolean(Config::DOODAD_BRUSH_ERASE_LIKE));
	doodad_erase_same_chkbox->SetToolTip("The doodad brush will only erase items belonging to the current brush.");
	brush_sizer->Add(doodad_erase_same_chkbox, 0, wxLEFT | wxTOP, 12);

	eraser_leave_unique_chkbox = newd wxCheckBox(brush_panel, wxID_ANY, "Eraser leaves unique / action items");
	eraser_leave_unique_chkbox->SetValue(g_settings.getBoolean(Config::ERASER_LEAVE_UNIQUE));
	eraser_leave_unique_chkbox->SetToolTip("The eraser will leave containers with items in them, and items with unique or action IDs.");
	brush_sizer->Add(eraser_leave_unique_chkbox, 0, wxLEFT | wxTOP, 12);

	auto_create_spawn_chkbox = newd wxCheckBox(brush_panel, wxID_ANY, "Auto create spawn when placing creature");
	auto_create_spawn_chkbox->SetValue(g_settings.getBoolean(Config::AUTO_CREATE_SPAWN));
	auto_create_spawn_chkbox->SetToolTip("Automatically places a spawn zone when placing a creature on the map.");
	brush_sizer->Add(auto_create_spawn_chkbox, 0, wxLEFT | wxTOP, 12);

	merge_move_chkbox = newd wxCheckBox(brush_panel, wxID_ANY, "Use merge move");
	merge_move_chkbox->SetValue(g_settings.getBoolean(Config::MERGE_MOVE));
	merge_move_chkbox->SetToolTip("Moved tiles won't replace already placed tiles.");
	brush_sizer->Add(merge_move_chkbox, 0, wxLEFT | wxTOP, 12);

	merge_paste_chkbox = newd wxCheckBox(brush_panel, wxID_ANY, "Use merge paste");
	merge_paste_chkbox->SetValue(g_settings.getBoolean(Config::MERGE_PASTE));
	merge_paste_chkbox->SetToolTip("Pasted tiles won't replace already placed tiles.");
	brush_sizer->Add(merge_paste_chkbox, 0, wxLEFT | wxTOP, 12);

	brush_panel->SetSizer(brush_sizer);
	sub_book->AddPage(brush_panel, "Brushes");

	main_sizer->Add(sub_book, 1, wxEXPAND | wxALL, 5);

	wxBoxSizer* def_sizer = newd wxBoxSizer(wxHORIZONTAL);
	wxButton* def_btn = newd wxButton(editor_page, wxID_ANY, "Default");
	def_btn->SetToolTip("Reset Editing settings to default values.");
	def_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
		group_actions_chkbox->SetValue(true);
		duplicate_id_warn_chkbox->SetValue(true);
		house_remove_chkbox->SetValue(true);
		auto_assign_doors_chkbox->SetValue(true);
		allow_multiple_orderitems_chkbox->SetValue(true);
		doodad_erase_same_chkbox->SetValue(true);
		eraser_leave_unique_chkbox->SetValue(true);
		auto_create_spawn_chkbox->SetValue(true);
		merge_move_chkbox->SetValue(false);
		merge_paste_chkbox->SetValue(false);
	});
	def_sizer->AddStretchSpacer();
	def_sizer->Add(def_btn, 0, wxRIGHT | wxBOTTOM, 5);
	main_sizer->Add(def_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);

	editor_page->SetSizer(main_sizer);
	return editor_page;
}

wxNotebookPage* PreferencesWindow::CreatePerformancePage() {
	wxNotebookPage* performance_page = newd wxPanel(book, wxID_ANY);
	performance_page->SetBackgroundColour(book->GetBackgroundColour());
	performance_page->SetForegroundColour(book->GetForegroundColour());

	wxBoxSizer* main_sizer = newd wxBoxSizer(wxVERTICAL);
	wxNotebook* sub_book = newd wxNotebook(performance_page, wxID_ANY);

	// --- Sub-Tab 1: Visuals & Rendering ---
	wxPanel* visual_panel = newd wxPanel(sub_book, wxID_ANY);
	wxBoxSizer* visual_sizer = newd wxBoxSizer(wxVERTICAL);
	wxStaticBoxSizer* visual_group = newd wxStaticBoxSizer(wxVERTICAL, visual_panel, "Editor Visuals & Rendering");
	
	// Row 1: BG Color & WIP Effects side by side
	wxBoxSizer* top_row = newd wxBoxSizer(wxHORIZONTAL);
	
	wxBoxSizer* bg_color_sizer = newd wxBoxSizer(wxHORIZONTAL);
	bg_color_sizer->Add(newd wxStaticText(visual_panel, wxID_ANY, "BG Color:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
	
	wxArrayString bg_choices;
	bg_choices.Add("Black");
	bg_choices.Add("Parchment");
	bg_choices.Add("Dark Slate");
	bg_choices.Add("Ocean Blue");
	bg_choices.Add("Forest Dark");
	bg_choices.Add("Classic Grey");
	bg_choices.Add("Pure White");

	bg_color_choice = newd wxChoice(visual_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, bg_choices);
	int cur_bg = g_settings.getInteger(Config::BG_COLOR);
	if (cur_bg < 0 || cur_bg >= static_cast<int>(bg_choices.size())) cur_bg = 0;
	bg_color_choice->SetSelection(cur_bg);
	bg_color_choice->SetToolTip("Select the canvas background clear color (no restart required).");
	bg_color_sizer->Add(bg_color_choice, 0, wxALIGN_CENTER_VERTICAL);
	top_row->Add(bg_color_sizer, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 15);

	wxStaticBoxSizer* wip_group = newd wxStaticBoxSizer(wxHORIZONTAL, visual_panel, "Enhancements");
	fake_hd_chkbox = newd wxCheckBox(visual_panel, wxID_ANY, "Graphic Upgrader (Sattere Fantasy-Farben)");
	fake_hd_chkbox->SetValue(g_settings.getBoolean(Config::FAKE_HD_ASSETS));
	fake_hd_chkbox->SetToolTip("Aktiviert saubere, satte Fantasy-Farbsaettigung und Kontrast ohne Ueberbelichtung.");
	wip_group->Add(fake_hd_chkbox, 0, wxALL | wxALIGN_CENTER_VERTICAL, 4);
	top_row->Add(wip_group, 1, wxEXPAND);
	visual_group->Add(top_row, 0, wxEXPAND | wxALL, 4);

	// 1. Cinematic Color Grading Moods
	wxBoxSizer* mood_sizer = newd wxBoxSizer(wxHORIZONTAL);
	mood_sizer->Add(newd wxStaticText(visual_panel, wxID_ANY, "Biome Farbstimmung:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
	wxArrayString mood_choices;
	mood_choices.Add("Vibrant Fantasy RPG (Oberwelt - Standard & Natuerlich)");
	mood_choices.Add("Dark & Dangerous (Drachen, Untote, Blight & Dungeons)");
	mood_choices.Add("Gloomy Crypt & Cave (Kuehler Hoehlen-Look)");
	mood_choices.Add("Golden Sunset & Twilight (Warme Abenddaemmerung)");
	mood_choices.Add("Frozen Wastes & Frost (Kuehles Eis-Blau)");
	mood_choices.Add("Neutral / Classic Vanilla (Ungefiltert)");
	exp_color_grading_choice = newd wxChoice(visual_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, mood_choices);
	int cur_mood = g_settings.getInteger(Config::EXP_COLOR_GRADING);
	if (cur_mood < 0 || cur_mood >= static_cast<int>(mood_choices.size())) cur_mood = 0;
	exp_color_grading_choice->SetSelection(cur_mood);
	exp_color_grading_choice->SetToolTip("Waehle die atmosphaerische Farbstimmung fuer verschiedene Biome, Dungeons und Gebiete.");
	mood_sizer->Add(exp_color_grading_choice, 0, wxALIGN_CENTER_VERTICAL);
	visual_group->Add(mood_sizer, 0, wxALL, 4);

	// 2. Cinematic Vignette
	wxBoxSizer* vig_sizer = newd wxBoxSizer(wxHORIZONTAL);
	exp_vignette_chkbox = newd wxCheckBox(visual_panel, wxID_ANY, "Cinematic Vignette:");
	exp_vignette_chkbox->SetValue(g_settings.getBoolean(Config::EXP_VIGNETTE));
	exp_vignette_chkbox->SetToolTip("Sanfte, kreisfoermige Ecken-Abdunklung fuer mehr Tiefe.");
	vig_sizer->Add(exp_vignette_chkbox, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 15);

	int cur_vig_lvl = std::clamp(int(std::round(g_settings.getFloat(Config::EXP_VIGNETTE_STRENGTH) * 10.0f)), 1, 10);
	if (cur_vig_lvl == 0) cur_vig_lvl = 4;
	exp_vignette_slider = newd wxSlider(visual_panel, wxID_ANY, cur_vig_lvl, 1, 10, wxDefaultPosition, wxSize(160, -1), wxSL_HORIZONTAL | wxSL_AUTOTICKS);
	vig_sizer->Add(exp_vignette_slider, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
	wxStaticText* vig_pct_label = newd wxStaticText(visual_panel, wxID_ANY, wxString::Format("Level %d (%d%%)", cur_vig_lvl, cur_vig_lvl * 10));
	vig_pct_label->SetForegroundColour(wxColor(255, 205, 50));
	vig_sizer->Add(vig_pct_label, 0, wxALIGN_CENTER_VERTICAL);
	exp_vignette_slider->Bind(wxEVT_SLIDER, [vig_pct_label, this](wxCommandEvent&) {
		int lvl = exp_vignette_slider->GetValue();
		vig_pct_label->SetLabel(wxString::Format("Level %d (%d%%)", lvl, lvl * 10));
	});
	visual_group->Add(vig_sizer, 0, wxALL, 4);

	wxBoxSizer* opacity_sizer = newd wxBoxSizer(wxHORIZONTAL);
	opacity_sizer->Add(newd wxStaticText(visual_panel, wxID_ANY, "Grid Opacity:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
	grid_opacity_slider = newd wxSlider(visual_panel, wxID_ANY, g_settings.getInteger(Config::GRID_OPACITY), 0, 255, wxDefaultPosition, wxSize(240, -1));
	opacity_sizer->Add(grid_opacity_slider, 0, wxALIGN_CENTER_VERTICAL);
	visual_group->Add(opacity_sizer, 0, wxALL, 4);

	wxStaticBoxSizer* scale_group = newd wxStaticBoxSizer(wxVERTICAL, visual_panel, "UI & Icon Scaling (Scale 1 to 10)");

	int cur_scale = g_settings.getInteger(Config::UI_SCALE);
	if (cur_scale < 100) cur_scale = 100;
	if (cur_scale > 170) cur_scale = 170;
	int cur_level = std::clamp((cur_scale - 100) * 9 / 70 + 1, 1, 10);

	wxBoxSizer* slider_row = newd wxBoxSizer(wxHORIZONTAL);
	slider_row->Add(newd wxStaticText(visual_panel, wxID_ANY, "Scale Level:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	ui_scale_slider = newd wxSlider(visual_panel, wxID_ANY, cur_level, 1, 10, wxDefaultPosition, wxSize(240, -1), wxSL_HORIZONTAL | wxSL_AUTOTICKS);
	ui_scale_slider->SetToolTip("Adjust UI scale from 1 (Smallest / 100%) to 10 (Largest / 170%).");
	slider_row->Add(ui_scale_slider, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);

	ui_scale_level_txt = newd wxStaticText(visual_panel, wxID_ANY, wxString::Format("Level %d (%d%%)", cur_level, 100 + (cur_level - 1) * 70 / 9));
	ui_scale_level_txt->SetForegroundColour(wxColor(255, 205, 50));
	ui_scale_level_txt->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	slider_row->Add(ui_scale_level_txt, 0, wxALIGN_CENTER_VERTICAL);
	scale_group->Add(slider_row, 0, wxALL, 3);

	// Interactive Live Preview Panel showing sample palette tiles
	wxBoxSizer* preview_row = newd wxBoxSizer(wxHORIZONTAL);
	preview_row->Add(newd wxStaticText(visual_panel, wxID_ANY, "Live Preview:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);

	ui_scale_preview_panel = newd wxPanel(visual_panel, wxID_ANY, wxDefaultPosition, wxSize(220, 38));
	ui_scale_preview_panel->SetBackgroundColour(wxColor(10, 15, 25));
	ui_scale_preview_panel->Bind(wxEVT_PAINT, [this](wxPaintEvent&) {
		if (!ui_scale_preview_panel) return;
		wxPaintDC dc(ui_scale_preview_panel);
		dc.SetBackground(wxBrush(wxColor(10, 15, 25)));
		dc.Clear();

		int level = ui_scale_slider ? ui_scale_slider->GetValue() : 1;
		int scale_pct = 100 + (level - 1) * 70 / 9;
		if (scale_pct > 170) scale_pct = 170;

		int btn_w = 30 * scale_pct / 100;

		int start_x = 10;
		int y = (38 - btn_w) / 2;
		if (y < 2) y = 2;

		// Draw 3 preview tiles (Sample Grass, Dirt, Cobblestone)
		wxColor sample_colors[3] = { wxColor(65, 125, 45), wxColor(120, 85, 45), wxColor(85, 95, 105) };

		for (int i = 0; i < 3; ++i) {
			int x = start_x + i * (btn_w + 6);
			if (x + btn_w > 215) break;

			// Base tile box
			dc.SetBrush(wxBrush(sample_colors[i]));
			dc.SetPen(wxPen(wxColor(30, 45, 65), 1));
			dc.DrawRectangle(x, y, btn_w, btn_w);

			// Tile inner texture simulation
			dc.SetPen(wxPen(wxColor(255, 255, 255, 40), 1));
			dc.DrawLine(x + 2, y + 2, x + btn_w - 2, y + 2);
			dc.DrawLine(x + 2, y + 2, x + 2, y + btn_w - 2);

			if (i == 0) {
				dc.SetPen(wxPen(wxColor(0, 0, 0), 2, wxSOLID));
				dc.SetBrush(*wxTRANSPARENT_BRUSH);
				dc.DrawRectangle(x, y, btn_w, btn_w);
				dc.SetPen(wxPen(wxColor(255, 205, 50), 2, wxSOLID));
				dc.DrawRectangle(x + 1, y + 1, btn_w - 2, btn_w - 2);
			}
		}
	});

	ui_scale_slider->Bind(wxEVT_SLIDER, [this](wxCommandEvent&) {
		UpdateScalePreview();
	});

	preview_row->Add(ui_scale_preview_panel, 0, wxALIGN_CENTER_VERTICAL);
	scale_group->Add(preview_row, 0, wxALL, 2);

	visual_group->Add(scale_group, 0, wxEXPAND | wxALL, 3);

	visual_sizer->Add(visual_group, 0, wxEXPAND | wxALL, 4);
	visual_panel->SetSizer(visual_sizer);
	sub_book->AddPage(visual_panel, "Visuals");

	// --- Sub-Tab 2: Screenshots ---
	wxPanel* screenshot_panel = newd wxPanel(sub_book, wxID_ANY);
	wxBoxSizer* screenshot_sizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer* shot_group = newd wxStaticBoxSizer(wxVERTICAL, screenshot_panel, "Screenshot Configuration");

	wxBoxSizer* dir_row = newd wxBoxSizer(wxHORIZONTAL);
	dir_row->Add(newd wxStaticText(screenshot_panel, wxID_ANY, "Save Directory:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
	screenshot_directory_picker = newd wxDirPickerCtrl(screenshot_panel, wxID_ANY, g_settings.getString(Config::SCREENSHOT_DIRECTORY));
	dir_row->Add(screenshot_directory_picker, 1, wxEXPAND);
	shot_group->Add(dir_row, 0, wxEXPAND | wxALL, 8);

	wxBoxSizer* fmt_row = newd wxBoxSizer(wxHORIZONTAL);
	fmt_row->Add(newd wxStaticText(screenshot_panel, wxID_ANY, "Image Format:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
	screenshot_format_choice = newd wxChoice(screenshot_panel, wxID_ANY);
	screenshot_format_choice->Append("PNG");
	screenshot_format_choice->Append("TGA");
	screenshot_format_choice->Append("JPG");
	screenshot_format_choice->Append("BMP");
	std::string cur_format = g_settings.getString(Config::SCREENSHOT_FORMAT);
	if (cur_format == "png") screenshot_format_choice->SetSelection(0);
	else if (cur_format == "tga") screenshot_format_choice->SetSelection(1);
	else if (cur_format == "jpg") screenshot_format_choice->SetSelection(2);
	else if (cur_format == "bmp") screenshot_format_choice->SetSelection(3);
	else screenshot_format_choice->SetSelection(0);
	fmt_row->Add(screenshot_format_choice, 0);
	shot_group->Add(fmt_row, 0, wxALL, 8);

	hide_items_when_zoomed_chkbox = newd wxCheckBox(screenshot_panel, wxID_ANY, "Hide items when zoomed out");
	hide_items_when_zoomed_chkbox->SetValue(g_settings.getBoolean(Config::HIDE_ITEMS_WHEN_ZOOMED));
	shot_group->Add(hide_items_when_zoomed_chkbox, 0, wxALL, 8);

	screenshot_sizer->Add(shot_group, 1, wxEXPAND | wxALL, 10);
	screenshot_panel->SetSizer(screenshot_sizer);
	sub_book->AddPage(screenshot_panel, "Screenshots");

	main_sizer->Add(sub_book, 1, wxEXPAND | wxALL, 5);

	wxBoxSizer* def_sizer = newd wxBoxSizer(wxHORIZONTAL);
	wxButton* def_btn = newd wxButton(performance_page, wxID_ANY, "Default");
	def_btn->SetToolTip("Reset Graphic settings to default values.");
	def_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
		if (bg_color_choice) bg_color_choice->SetSelection(0);
		if (fake_hd_chkbox) fake_hd_chkbox->SetValue(false);
		if (exp_color_grading_choice) exp_color_grading_choice->SetSelection(0);
		if (exp_vignette_chkbox) exp_vignette_chkbox->SetValue(false);
		if (exp_vignette_slider) exp_vignette_slider->SetValue(4);
		grid_opacity_slider->SetValue(128);
		ui_scale_slider->SetValue(3);
		UpdateScalePreview();
		if (screenshot_format_choice) screenshot_format_choice->SetSelection(0);
		if (hide_items_when_zoomed_chkbox) hide_items_when_zoomed_chkbox->SetValue(true);
	});
	def_sizer->AddStretchSpacer();
	def_sizer->Add(def_btn, 0, wxRIGHT | wxBOTTOM, 5);
	main_sizer->Add(def_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);

	performance_page->SetSizer(main_sizer);
	return performance_page;
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

	wxBoxSizer* main_sizer = newd wxBoxSizer(wxVERTICAL);
	wxNotebook* sub_book = newd wxNotebook(ui_page, wxID_ANY);

	// --- Sub-Tab 1: Theme & Icons ---
	wxPanel* theme_panel = newd wxPanel(sub_book, wxID_ANY);
	wxBoxSizer* theme_sizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer* theme_group = new wxStaticBoxSizer(wxVERTICAL, theme_panel, "Visual Theme & Cursors");
	wxString theme_choices[] = { "Dark Mode (Restart required)", "Light Mode" };
	theme_radio = new wxRadioBox(theme_panel, wxID_ANY, "Visual Theme", wxDefaultPosition, wxDefaultSize, 2, theme_choices, 1, wxRA_SPECIFY_COLS);
	theme_group->Add(theme_radio, 0, wxALL | wxEXPAND, 3);

	wxFlexGridSizer* color_grid = new wxFlexGridSizer(2, 4, 10);
	color_grid->Add(new wxStaticText(theme_panel, wxID_ANY, "Cursor Color:"), 0, wxALIGN_CENTER_VERTICAL);
	cursor_color_pick = new wxColourPickerCtrl(theme_panel, wxID_ANY, wxColor(g_settings.getInteger(Config::CURSOR_RED), g_settings.getInteger(Config::CURSOR_GREEN), g_settings.getInteger(Config::CURSOR_BLUE)));
	color_grid->Add(cursor_color_pick);
	color_grid->Add(new wxStaticText(theme_panel, wxID_ANY, "Secondary Cursor:"), 0, wxALIGN_CENTER_VERTICAL);
	cursor_alt_color_pick = new wxColourPickerCtrl(theme_panel, wxID_ANY, wxColor(g_settings.getInteger(Config::CURSOR_ALT_RED), g_settings.getInteger(Config::CURSOR_ALT_GREEN), g_settings.getInteger(Config::CURSOR_ALT_BLUE)));
	color_grid->Add(cursor_alt_color_pick);
	theme_group->Add(color_grid, 0, wxALL, 3);
	theme_sizer->Add(theme_group, 0, wxEXPAND | wxALL, 4);

	wxStaticBoxSizer* icon_group = new wxStaticBoxSizer(wxVERTICAL, theme_panel, "Icon Sizing");
	wxGridSizer* icon_grid = new wxGridSizer(4, 2, 2, 8);

	large_terrain_tools_chkbox = newd wxCheckBox(theme_panel, wxID_ANY, "Large terrain tool icons");
	large_terrain_tools_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_TERRAIN_TOOLBAR));
	icon_grid->Add(large_terrain_tools_chkbox);

	large_collection_tools_chkbox = newd wxCheckBox(theme_panel, wxID_ANY, "Large collection tool icons");
	large_collection_tools_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_COLLECTION_TOOLBAR));
	icon_grid->Add(large_collection_tools_chkbox);

	large_doodad_sizebar_chkbox = newd wxCheckBox(theme_panel, wxID_ANY, "Large doodad size icons");
	large_doodad_sizebar_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_DOODAD_SIZEBAR));
	icon_grid->Add(large_doodad_sizebar_chkbox);

	large_item_sizebar_chkbox = newd wxCheckBox(theme_panel, wxID_ANY, "Large item size icons");
	large_item_sizebar_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_ITEM_SIZEBAR));
	icon_grid->Add(large_item_sizebar_chkbox);

	large_house_sizebar_chkbox = newd wxCheckBox(theme_panel, wxID_ANY, "Large house size icons");
	large_house_sizebar_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_HOUSE_SIZEBAR));
	icon_grid->Add(large_house_sizebar_chkbox);

	large_raw_sizebar_chkbox = newd wxCheckBox(theme_panel, wxID_ANY, "Large raw size icons");
	large_raw_sizebar_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_RAW_SIZEBAR));
	icon_grid->Add(large_raw_sizebar_chkbox);

	large_container_icons_chkbox = newd wxCheckBox(theme_panel, wxID_ANY, "Large container view icons");
	large_container_icons_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_CONTAINER_ICONS));
	icon_grid->Add(large_container_icons_chkbox);

	large_pick_item_icons_chkbox = newd wxCheckBox(theme_panel, wxID_ANY, "Large item picker icons");
	large_pick_item_icons_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_CHOOSE_ITEM_ICONS));
	icon_grid->Add(large_pick_item_icons_chkbox);

	icon_group->Add(icon_grid, 0, wxEXPAND | wxALL, 3);
	theme_sizer->Add(icon_group, 0, wxEXPAND | wxALL, 4);
	theme_panel->SetSizer(theme_sizer);
	sub_book->AddPage(theme_panel, "Theme");

	// --- Sub-Tab 2: Palettes ---
	wxPanel* palette_panel = newd wxPanel(sub_book, wxID_ANY);
	wxBoxSizer* palette_box_sizer = newd wxBoxSizer(wxVERTICAL);

	auto* palette_sizer = newd wxFlexGridSizer(2, 8, 12);
	palette_sizer->AddGrowableCol(1);

	terrain_palette_style_choice = AddPaletteStyleChoice(palette_panel, palette_sizer, "Terrain Palette Style:", 
		"Configures the look of the terrain palette.", g_settings.getString(Config::PALETTE_TERRAIN_STYLE));
	
	collection_palette_style_choice = AddPaletteStyleChoice(palette_panel, palette_sizer, "Collections Palette Style:", 
		"Configures the look of the collections palette.", g_settings.getString(Config::PALETTE_COLLECTION_STYLE));
	
	doodad_palette_style_choice = AddPaletteStyleChoice(palette_panel, palette_sizer, "Doodad Palette Style:", 
		"Configures the look of the doodad palette.", g_settings.getString(Config::PALETTE_DOODAD_STYLE));
	
	item_palette_style_choice = AddPaletteStyleChoice(palette_panel, palette_sizer, "Item Palette Style:", 
		"Configures the look of the item palette.", g_settings.getString(Config::PALETTE_ITEM_STYLE));
	
	raw_palette_style_choice = AddPaletteStyleChoice(palette_panel, palette_sizer, "RAW Palette Style:", 
		"Configures the look of the raw palette.", g_settings.getString(Config::PALETTE_RAW_STYLE));

	palette_box_sizer->Add(palette_sizer, 1, wxEXPAND | wxALL, 15);
	palette_panel->SetSizer(palette_box_sizer);
	sub_book->AddPage(palette_panel, "Palettes");

	// --- Sub-Tab 3: Controls ---
	wxPanel* ctrl_panel = newd wxPanel(sub_book, wxID_ANY);
	wxBoxSizer* ctrl_sizer = newd wxBoxSizer(wxVERTICAL);

	switch_mousebtn_chkbox = newd wxCheckBox(ctrl_panel, wxID_ANY, "Switch mousebuttons");
	switch_mousebtn_chkbox->SetValue(g_settings.getBoolean(Config::SWITCH_MOUSEBUTTONS));
	switch_mousebtn_chkbox->SetToolTip("Switches the right and center mouse button.");
	ctrl_sizer->Add(switch_mousebtn_chkbox, 0, wxLEFT | wxTOP, 8);

	doubleclick_properties_chkbox = newd wxCheckBox(ctrl_panel, wxID_ANY, "Double click for properties");
	doubleclick_properties_chkbox->SetValue(g_settings.getBoolean(Config::DOUBLECLICK_PROPERTIES));
	doubleclick_properties_chkbox->SetToolTip("Double clicking on a tile will bring up the properties menu for the top item.");
	ctrl_sizer->Add(doubleclick_properties_chkbox, 0, wxLEFT | wxTOP, 8);

	inversed_scroll_chkbox = newd wxCheckBox(ctrl_panel, wxID_ANY, "Use inversed scroll");
	inversed_scroll_chkbox->SetValue(g_settings.getFloat(Config::SCROLL_SPEED) < 0);
	inversed_scroll_chkbox->SetToolTip("When checked, dragging the map using the center mouse button will be inversed (RTS style).");
	ctrl_sizer->Add(inversed_scroll_chkbox, 0, wxLEFT | wxTOP, 8);

	wxStaticText* scroll_label = newd wxStaticText(ctrl_panel, wxID_ANY, "Scroll speed: ");
	ctrl_sizer->Add(scroll_label, 0, wxLEFT | wxTOP, 8);

	auto true_scrollspeed = std::clamp(int(std::round(std::abs(g_settings.getFloat(Config::SCROLL_SPEED)) * 2.0f)), 1, 10);
	scroll_speed_slider = newd wxSlider(ctrl_panel, wxID_ANY, true_scrollspeed, 1, 10);
	scroll_speed_slider->SetToolTip("Controls map scrolling speed when holding down the center mouse button.");
	ctrl_sizer->Add(scroll_speed_slider, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);

	auto update_scroll_label = [scroll_label, this]() {
		scroll_label->SetLabel(wxString::Format("Scroll speed: %d", scroll_speed_slider->GetValue()));
	};
	scroll_speed_slider->Bind(wxEVT_SLIDER, [update_scroll_label](wxCommandEvent&) {
		update_scroll_label();
	});
	update_scroll_label();

	wxStaticText* zoom_label = newd wxStaticText(ctrl_panel, wxID_ANY, "Zoom speed: ");
	ctrl_sizer->Add(zoom_label, 0, wxLEFT | wxTOP, 8);

	auto true_zoomspeed = std::clamp(int(std::round(g_settings.getFloat(Config::ZOOM_SPEED) * 5.0f)), 1, 10);
	zoom_speed_slider = newd wxSlider(ctrl_panel, wxID_ANY, true_zoomspeed, 1, 10);
	zoom_speed_slider->SetToolTip("Controls map zoom speed when scrolling the mouse wheel.");
	ctrl_sizer->Add(zoom_speed_slider, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);

	auto update_zoom_label = [zoom_label, this]() {
		zoom_label->SetLabel(wxString::Format("Zoom speed: %d", zoom_speed_slider->GetValue()));
	};
	zoom_speed_slider->Bind(wxEVT_SLIDER, [update_zoom_label](wxCommandEvent&) {
		update_zoom_label();
	});
	update_zoom_label();

	wxStaticText* minimap_label = newd wxStaticText(ctrl_panel, wxID_ANY, "Minimap scroll speed: ");
	ctrl_sizer->Add(minimap_label, 0, wxLEFT | wxTOP, 8);

	auto true_minispeed = std::clamp(int(std::round(g_settings.getFloat(Config::MINIMAP_SCROLL_SPEED))), 1, 10);
	minimap_scroll_speed_slider = newd wxSlider(ctrl_panel, wxID_ANY, true_minispeed, 1, 10);
	minimap_scroll_speed_slider->SetToolTip("Controls jump/drag speed inside the minimap.");
	ctrl_sizer->Add(minimap_scroll_speed_slider, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);

	auto update_mini_label = [minimap_label, this]() {
		minimap_label->SetLabel(wxString::Format("Minimap scroll speed: %d", minimap_scroll_speed_slider->GetValue()));
	};
	minimap_scroll_speed_slider->Bind(wxEVT_SLIDER, [update_mini_label](wxCommandEvent&) {
		update_mini_label();
	});
	update_mini_label();

	ctrl_panel->SetSizer(ctrl_sizer);
	sub_book->AddPage(ctrl_panel, "Controls");

	main_sizer->Add(sub_book, 1, wxEXPAND | wxALL, 5);

	wxBoxSizer* def_sizer = newd wxBoxSizer(wxHORIZONTAL);
	wxButton* def_btn = newd wxButton(ui_page, wxID_ANY, "Default");
	def_btn->SetToolTip("Reset Interface settings to default values.");
	def_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
		theme_radio->SetSelection(0);
		cursor_color_pick->SetColour(wxColor(255, 255, 255));
		cursor_alt_color_pick->SetColour(wxColor(255, 255, 0));
		large_terrain_tools_chkbox->SetValue(false);
		large_collection_tools_chkbox->SetValue(false);
		large_doodad_sizebar_chkbox->SetValue(false);
		large_item_sizebar_chkbox->SetValue(false);
		large_house_sizebar_chkbox->SetValue(false);
		large_raw_sizebar_chkbox->SetValue(false);
		large_container_icons_chkbox->SetValue(false);
		large_pick_item_icons_chkbox->SetValue(false);
		terrain_palette_style_choice->SetSelection(0);
		collection_palette_style_choice->SetSelection(0);
		doodad_palette_style_choice->SetSelection(0);
		item_palette_style_choice->SetSelection(0);
		raw_palette_style_choice->SetSelection(0);
		switch_mousebtn_chkbox->SetValue(false);
		doubleclick_properties_chkbox->SetValue(false);
		inversed_scroll_chkbox->SetValue(false);
		scroll_speed_slider->SetValue(5);
		zoom_speed_slider->SetValue(5);
		minimap_scroll_speed_slider->SetValue(5);
	});
	def_sizer->AddStretchSpacer();
	def_sizer->Add(def_btn, 0, wxRIGHT | wxBOTTOM, 5);
	main_sizer->Add(def_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);

	ui_page->SetSizer(main_sizer);
	return ui_page;
}

wxNotebookPage* PreferencesWindow::CreateHotkeysPage() {
	wxNotebookPage* page = newd wxPanel(book, wxID_ANY);
	page->SetBackgroundColour(book->GetBackgroundColour());
	page->SetForegroundColour(book->GetForegroundColour());

	wxBoxSizer* main_sizer = newd wxBoxSizer(wxVERTICAL);
	wxNotebook* sub_book = newd wxNotebook(page, wxID_ANY);

	struct HotkeyInfo {
		wxString action;
		wxString key;
	};

	auto create_list = [&](wxPanel* parent, const std::vector<HotkeyInfo>& items) {
		wxBoxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
		wxListCtrl* list = newd wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
		list->SetBackgroundColour(book->GetBackgroundColour());
		list->SetForegroundColour(book->GetForegroundColour());
		list->InsertColumn(0, "Action", wxLIST_FORMAT_LEFT, 340);
		list->InsertColumn(1, "Hotkey / Control", wxLIST_FORMAT_LEFT, 300);

		for (size_t i = 0; i < items.size(); ++i) {
			long tmp = list->InsertItem(static_cast<long>(i), items[i].action);
			list->SetItem(tmp, 1, items[i].key);
		}
		sizer->Add(list, 1, wxEXPAND | wxALL, 5);
		parent->SetSizer(sizer);
	};

	// --- Sub-Tab 1: Navigation & View ---
	wxPanel* nav_panel = newd wxPanel(sub_book, wxID_ANY);
	std::vector<HotkeyInfo> nav_keys = {
		{"Move View (Canvas)", "W / A / S / D  or  Arrow Keys"},
		{"Move View (Drag)", "Middle Mouse Button Drag"},
		{"Pan Canvas", "Space + Left Mouse Drag"},
		{"Zoom In / Out / 100%", "Mouse Wheel  or  Ctrl + + / - / 0"},
		{"Change Floor (Up / Down)", "PageUp / PageDown"},
		{"Toggle Minimap (HUD Window)", "M"},
		{"Show All Floors", "Ctrl + W"},
		{"Ghost Higher Floors", "Ctrl + L"},
		{"Shade Lower Floors", "Q"},
		{"Toggle Fullscreen", "F11"},
		{"Take Screenshot", "F10"},
		{"Go to Position", "Ctrl + G"},
		{"Previous Position", "P"},
		{"Show Pathing / Blocking", "O"},
		{"Show Creature Spawns", "S"},
		{"Show Creatures & NPCs", "F"},
		{"Show Special Zones / Items", "E"},
		{"Show Wall Hooks", "K"},
		{"Show Tooltips", "Y"},
		{"Highlight Items", "V"},
		{"Highlight Locked Doors", "U"},
		{"Ghost Loose Items", "G"},
		{"Render as 2D Minimap", "Shift + E"}
	};
	create_list(nav_panel, nav_keys);
	sub_book->AddPage(nav_panel, "Navigation");

	// --- Sub-Tab 2: Editing ---
	wxPanel* edit_panel = newd wxPanel(sub_book, wxID_ANY);
	std::vector<HotkeyInfo> edit_keys = {
		{"Undo Action", "Ctrl + Z"},
		{"Redo Action", "Ctrl + Y  or  Ctrl + Shift + Z"},
		{"Cut Selection", "Ctrl + X"},
		{"Copy Selection", "Ctrl + C"},
		{"Paste Selection", "Ctrl + V"},
		{"Delete Selection / Item", "Delete"},
		{"Select All", "Ctrl + A"},
		{"Deselect / Cancel", "Ctrl + D  or  Escape"},
		{"Rotate Item / Active Brush / Variation", "Z  or  R"},
		{"Toggle Border Automagic", "A"},
		{"Borderize Selection", "Ctrl + B"},
		{"Brush Size (1x1 to 10x10)", "1, 2, 3, 4, 5, 6, 7, 8, 9, 0"},
		{"Cycle Variation / Size (Wheel)", "Shift + Mouse Wheel"},
		{"Quick Tool Wheel", "Shift + Q"},
		{"Find Item", "Ctrl + F"},
		{"Replace Items", "Ctrl + Shift + F"},
		{"Remove Items by ID", "Ctrl + Shift + R"},
		{"New Map", "Ctrl + N"},
		{"Open Map", "Ctrl + O"},
		{"Save Map", "Ctrl + S"},
		{"Map Properties", "Ctrl + P"},
		{"Map Statistics", "F8"},
		{"Edit Towns", "Ctrl + T"}
	};
	create_list(edit_panel, edit_keys);
	sub_book->AddPage(edit_panel, "Editing");

	// --- Sub-Tab 3: Palettes ---
	wxPanel* pal_panel = newd wxPanel(sub_book, wxID_ANY);
	std::vector<HotkeyInfo> pal_keys = {
		{"Terrain Palette", "T"},
		{"Doodad Palette", "D"},
		{"Item Palette", "I"},
		{"Collection Palette", "N"},
		{"House Palette", "H"},
		{"Creature & NPC Palette", "C"},
		{"Waypoint Palette", "W"},
		{"RAW Palette", "R"},
		{"Jump to Brush", "J"},
		{"Jump to Item (RAW)", "Ctrl + J"},
		{"Interactive Playtest Mode", "F6"},
		{"Reload Data Files", "F5"},
		{"Reload Lua Scripts", "Ctrl + Shift + F5"},
		{"Extensions / Plugins", "F2"},
		{"Toggle Single-Letter Hotkeys Mode", "Ctrl + Alt + H"}
	};
	create_list(pal_panel, pal_keys);
	sub_book->AddPage(pal_panel, "Palettes");

	main_sizer->Add(sub_book, 1, wxEXPAND | wxALL, 5);

	wxBoxSizer* def_sizer = newd wxBoxSizer(wxHORIZONTAL);
	wxButton* def_btn = newd wxButton(page, wxID_ANY, "Default");
	def_btn->SetToolTip("Reset Hotkeys to default configuration.");
	def_sizer->AddStretchSpacer();
	def_sizer->Add(def_btn, 0, wxRIGHT | wxBOTTOM, 5);
	main_sizer->Add(def_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);

	page->SetSizer(main_sizer);
	return page;
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
	bool palette_style_changed = false;

	// General
	g_settings.setInteger(Config::WELCOME_DIALOG, show_welcome_dialog_chkbox->GetValue());
	g_settings.setInteger(Config::ALWAYS_MAKE_BACKUP, always_make_backup_chkbox->GetValue());
	g_settings.setInteger(Config::AUTO_SAVE_ENABLED, autosave_enabled_chkbox ? autosave_enabled_chkbox->GetValue() : 0);
	g_settings.setInteger(Config::AUTO_SAVE_INTERVAL, autosave_interval_slider ? autosave_interval_slider->GetValue() : 10);
	// Restart the auto-save timer immediately with new settings
	if (g_gui.root) {
		g_gui.root->RestartAutoSaveTimer();
	}
	g_settings.setInteger(Config::USE_UPDATER, update_check_on_startup_chkbox->GetValue());
	g_settings.setInteger(Config::ONLY_ONE_INSTANCE, only_one_instance_chkbox->GetValue());
	if (multiplayer_port_spin) {
		g_settings.setInteger(Config::MULTIPLAYER_PORT, multiplayer_port_spin->GetValue());
	}
	if (undo_size_spin) {
		g_settings.setInteger(Config::UNDO_SIZE, undo_size_spin->GetValue());
	}
	if (undo_mem_size_spin) {
		g_settings.setInteger(Config::UNDO_MEM_SIZE, undo_mem_size_spin->GetValue());
	}
	if (replace_size_spin) {
		g_settings.setInteger(Config::REPLACE_SIZE, replace_size_spin->GetValue());
	}
	if (position_choice) {
		g_settings.setInteger(Config::COPY_POSITION_FORMAT, position_choice->GetSelection());
	}

	if (g_settings.getBoolean(Config::SHOW_TILESET_EDITOR) != enable_tileset_editing_chkbox->GetValue()) {
		palette_update_needed = true;
	}
	g_settings.setInteger(Config::SHOW_TILESET_EDITOR, enable_tileset_editing_chkbox->GetValue());

	// Save Client Settings from within General Page
	ClientVersionList versions = ClientVersion::getAllVisible();
	if (default_version_choice) {
		for (auto version : versions) {
			if (version->getName() == default_version_choice->GetStringSelection()) {
				g_settings.setInteger(Config::DEFAULT_CLIENT_VERSION, version->getID());
			}
		}
	}
	if (check_sigs_chkbox) {
		g_settings.setInteger(Config::CHECK_SIGNATURES, check_sigs_chkbox->GetValue());
	}

	// Make sure to reload client paths
	ClientVersion::saveVersions();
	ClientVersion::loadVersions();

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
	if (bg_color_choice) {
		int new_bg = bg_color_choice->GetSelection();
		if (new_bg != g_settings.getInteger(Config::BG_COLOR)) {
			g_settings.setInteger(Config::BG_COLOR, new_bg);
			g_gui.RefreshView();
		}
	}
	if (grid_opacity_slider)
		g_settings.setInteger(Config::GRID_OPACITY, grid_opacity_slider->GetValue());
	if (fake_hd_chkbox) {
		bool old_fake_hd = g_settings.getBoolean(Config::FAKE_HD_ASSETS);
		bool new_fake_hd = fake_hd_chkbox->GetValue();
		if (old_fake_hd != new_fake_hd) {
			g_settings.setInteger(Config::FAKE_HD_ASSETS, new_fake_hd ? 1 : 0);
			g_gui.RefreshView();
		}
	}

	// Experimental Graphics
	if (exp_color_grading_choice) {
		g_settings.setInteger(Config::EXP_COLOR_GRADING, exp_color_grading_choice->GetSelection());
	}
	if (exp_vignette_chkbox) {
		g_settings.setInteger(Config::EXP_VIGNETTE, exp_vignette_chkbox->GetValue() ? 1 : 0);
	}
	if (exp_vignette_slider) {
		g_settings.setFloat(Config::EXP_VIGNETTE_STRENGTH, float(exp_vignette_slider->GetValue()) / 10.0f);
	}
	g_gui.RefreshView();

	// if (g_settings.getInteger(Config::RENDER_BACKEND) != backend_radio->GetSelection()) {
	// 	g_settings.setInteger(Config::RENDER_BACKEND, backend_radio->GetSelection());
	// 	must_restart = true;
	// }

	if (ui_scale_slider) {
		int old_scale = g_settings.getInteger(Config::UI_SCALE);
		if (old_scale < 100) old_scale = 100;
		if (old_scale > 170) old_scale = 170;
		int level = ui_scale_slider->GetValue();
		int new_scale = 100 + (level - 1) * 70 / 9;
		if (new_scale > 170) new_scale = 170;
		if (old_scale != new_scale) {
			g_settings.setInteger(Config::UI_SCALE, new_scale);
			palette_style_changed = true;
		}
	}

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

	if (cursor_color_pick) {
		wxColor clr = cursor_color_pick->GetColour();
		g_settings.setInteger(Config::CURSOR_RED, clr.Red());
		g_settings.setInteger(Config::CURSOR_GREEN, clr.Green());
		g_settings.setInteger(Config::CURSOR_BLUE, clr.Blue());
	}

	if (cursor_alt_color_pick) {
		wxColor clr = cursor_alt_color_pick->GetColour();
		g_settings.setInteger(Config::CURSOR_ALT_RED, clr.Red());
		g_settings.setInteger(Config::CURSOR_ALT_GREEN, clr.Green());
		g_settings.setInteger(Config::CURSOR_ALT_BLUE, clr.Blue());
	}


	// Interface
	if (theme_radio) {
		int theme_idx = theme_radio->GetSelection();
		g_settings.setInteger(Config::UI_THEME, theme_idx);
		RME::UI::Theme::SetTheme(theme_idx == 0 ? RME::UI::Theme::Type::Dark : RME::UI::Theme::Type::Light);
		palette_style_changed = true;
	}

	if (terrain_palette_style_choice && collection_palette_style_choice &&
		doodad_palette_style_choice && item_palette_style_choice && raw_palette_style_choice) {
		palette_style_changed =
			g_settings.getString(Config::PALETTE_TERRAIN_STYLE) != (terrain_palette_style_choice->GetSelection() == 0 ? "large icons" : terrain_palette_style_choice->GetSelection() == 1 ? "small icons" : "listbox") ||
			g_settings.getString(Config::PALETTE_COLLECTION_STYLE) != (collection_palette_style_choice->GetSelection() == 0 ? "large icons" : collection_palette_style_choice->GetSelection() == 1 ? "small icons" : "listbox") ||
			g_settings.getString(Config::PALETTE_DOODAD_STYLE) != (doodad_palette_style_choice->GetSelection() == 0 ? "large icons" : doodad_palette_style_choice->GetSelection() == 1 ? "small icons" : "listbox") ||
			g_settings.getString(Config::PALETTE_ITEM_STYLE) != (item_palette_style_choice->GetSelection() == 0 ? "large icons" : item_palette_style_choice->GetSelection() == 1 ? "small icons" : "listbox") ||
			g_settings.getString(Config::PALETTE_RAW_STYLE) != (raw_palette_style_choice->GetSelection() == 0 ? "large icons" : raw_palette_style_choice->GetSelection() == 1 ? "small icons" : "listbox");

		SetPaletteStyleChoice(terrain_palette_style_choice, Config::PALETTE_TERRAIN_STYLE);
		SetPaletteStyleChoice(collection_palette_style_choice, Config::PALETTE_COLLECTION_STYLE);
		SetPaletteStyleChoice(doodad_palette_style_choice, Config::PALETTE_DOODAD_STYLE);
		SetPaletteStyleChoice(item_palette_style_choice, Config::PALETTE_ITEM_STYLE);
		SetPaletteStyleChoice(raw_palette_style_choice, Config::PALETTE_RAW_STYLE);
	}
	if (large_terrain_tools_chkbox) g_settings.setInteger(Config::USE_LARGE_TERRAIN_TOOLBAR, large_terrain_tools_chkbox->GetValue());
	if (large_collection_tools_chkbox) g_settings.setInteger(Config::USE_LARGE_COLLECTION_TOOLBAR, large_collection_tools_chkbox->GetValue());
	if (large_doodad_sizebar_chkbox) g_settings.setInteger(Config::USE_LARGE_DOODAD_SIZEBAR, large_doodad_sizebar_chkbox->GetValue());
	if (large_item_sizebar_chkbox) g_settings.setInteger(Config::USE_LARGE_ITEM_SIZEBAR, large_item_sizebar_chkbox->GetValue());
	if (large_house_sizebar_chkbox) g_settings.setInteger(Config::USE_LARGE_HOUSE_SIZEBAR, large_house_sizebar_chkbox->GetValue());
	if (large_raw_sizebar_chkbox) g_settings.setInteger(Config::USE_LARGE_RAW_SIZEBAR, large_raw_sizebar_chkbox->GetValue());
	if (large_container_icons_chkbox) g_settings.setInteger(Config::USE_LARGE_CONTAINER_ICONS, large_container_icons_chkbox->GetValue());
	if (large_pick_item_icons_chkbox) g_settings.setInteger(Config::USE_LARGE_CHOOSE_ITEM_ICONS, large_pick_item_icons_chkbox->GetValue());

	if (switch_mousebtn_chkbox) g_settings.setInteger(Config::SWITCH_MOUSEBUTTONS, switch_mousebtn_chkbox->GetValue());
	if (doubleclick_properties_chkbox) g_settings.setInteger(Config::DOUBLECLICK_PROPERTIES, doubleclick_properties_chkbox->GetValue());

	if (inversed_scroll_chkbox && scroll_speed_slider) {
		float scroll_mul = inversed_scroll_chkbox->GetValue() ? -1.0f : 1.0f;
		g_settings.setFloat(Config::SCROLL_SPEED, scroll_mul * scroll_speed_slider->GetValue() / 2.f);
	}
	if (zoom_speed_slider) g_settings.setFloat(Config::ZOOM_SPEED, zoom_speed_slider->GetValue() / 5.f);
	if (minimap_scroll_speed_slider) g_settings.setFloat(Config::MINIMAP_SCROLL_SPEED, (float)minimap_scroll_speed_slider->GetValue());

	g_settings.save();

	if (palette_update_needed) {
		// change palette structure
		wxString error;
		wxArrayString warnings;
		g_gui.LoadVersion(g_gui.GetCurrentVersionID(), error, warnings, true);
		g_gui.PopupDialog("Error", error, wxOK);
		g_gui.ListDialog("Warnings", warnings);
	} else {
		if (palette_style_changed) {
			g_gui.RebuildPalettes();
		}
	}

	g_gui.RefreshPalettes();
	g_gui.RefreshView();
	if (g_gui.root) {
		g_gui.root->UpdateMenubar();
		g_gui.root->Refresh();
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

	// Dynamic visibility of check_sigs_chkbox: Google Research shows signatures verification is only critical 
	// for old client versions (lower or equal to 10.98 / Tibia 10). For modern client architectures (Tibia 11+ / client versions higher than 10.98),
	// we hide the checkbox control automatically to prevent signature mismatch bug states and error dialog loops.
	bool is_modern_client = false;
	if (selected_version->getID() > 1098) {
		is_modern_client = true;
	}

	if (is_modern_client) {
		check_sigs_chkbox->Hide();
		// Automatically disable signature tracking for advanced simulated formats (since they lack valid headers)
		check_sigs_chkbox->SetValue(false);
	} else {
		check_sigs_chkbox->Show();
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
}

void PreferencesWindow::UpdateScalePreview() {
	if (!ui_scale_slider || !ui_scale_level_txt || !ui_scale_preview_panel) return;
	int level = ui_scale_slider->GetValue();
	int scale_pct = 100 + (level - 1) * 70 / 9;
	if (scale_pct > 170) scale_pct = 170;

	ui_scale_level_txt->SetLabel(wxString::Format("Level %d (%d%%)", level, scale_pct));
	ui_scale_preview_panel->Refresh();
}
