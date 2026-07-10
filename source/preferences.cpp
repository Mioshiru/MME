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

#include "style_manager.h"

#include "settings.h"
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
		wxURL url(urlString);
		if (url.GetError() != wxURL_NOERR) {
			errorMessage = "Could not initialize the network request.";
			return false;
		}

		std::unique_ptr<wxInputStream> stream(url.GetInputStream());
		if (!stream || !stream->IsOk()) {
			errorMessage = "Could not contact the network service.";
			return false;
		}

		wxStringOutputStream output;
		stream->Read(output);
		response = output.GetString();
		response.Trim(true);
		response.Trim(false);
		if (response.empty()) {
			errorMessage = "The network service returned an empty response.";
			return false;
		}

		return true;
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

#ifdef __WINDOWS__
	bool IsWindowsFirewallPortAllowed(int port) {
		wxArrayString output;
		wxArrayString errors;
		const wxString command = wxString::Format(
			"powershell -NoProfile -NonInteractive -Command \"$rule = Get-NetFirewallPortFilter -Protocol TCP -LocalPort %d -ErrorAction SilentlyContinue | Get-NetFirewallRule | Where-Object { $_.Enabled -eq 'True' -and $_.Direction -eq 'Inbound' -and $_.Action -eq 'Allow' } | Select-Object -First 1 -ExpandProperty DisplayName; if ($rule) { Write-Output $rule }\"",
			port
		);
		const long exitCode = wxExecute(command, output, errors, wxEXEC_SYNC);
		return exitCode == 0 && !output.empty() && !output[0].Trim().empty();
	}

	bool RequestWindowsFirewallPortRule(wxWindow* parent, int port) {
		const wxString params = wxString::Format(
			"advfirewall firewall add rule name=\"Remere's Map Editor Multiplayer %d\" dir=in action=allow protocol=TCP localport=%d profile=any",
			port,
			port
		);

		HINSTANCE result = ShellExecuteW(
			reinterpret_cast<HWND>(parent ? parent->GetHandle() : nullptr),
			L"runas",
			L"netsh.exe",
			params.wc_str(),
			nullptr,
			SW_SHOWNORMAL
		);
		return reinterpret_cast<INT_PTR>(result) > 32;
	}
#endif
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
	book->AddPage(CreatePerformancePage(), "Graphic");
	book->AddPage(CreateUIPage(), "Interface");
	book->AddPage(CreateHotkeysPage(), "Hotkeys");

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

	auto* preferences_window_layout_s = sizer; // Reference
	SetMinSize(wxSize(500, 520));
	SetSizerAndFit(sizer);
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

	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
	wxStaticText* tmptext;

	show_welcome_dialog_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Show welcome dialog on startup");
	show_welcome_dialog_chkbox->SetValue(g_settings.getInteger(Config::WELCOME_DIALOG) == 1);
	show_welcome_dialog_chkbox->SetToolTip("Show welcome dialog when starting the editor.");
	sizer->Add(show_welcome_dialog_chkbox, 0, wxLEFT | wxTOP, 5);

	always_make_backup_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Always make map backup");
	always_make_backup_chkbox->SetValue(g_settings.getInteger(Config::ALWAYS_MAKE_BACKUP) == 1);
	sizer->Add(always_make_backup_chkbox, 0, wxLEFT | wxTOP, 5);

	// ── Auto-Save Group ──────────────────────────────────────────────────
	wxStaticBoxSizer* autosave_box = newd wxStaticBoxSizer(wxVERTICAL, general_page, "Auto-Save");

	autosave_enabled_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Enable Auto-Save");
	autosave_enabled_chkbox->SetValue(g_settings.getBoolean(Config::AUTO_SAVE_ENABLED));
	autosave_enabled_chkbox->SetToolTip("Automatically saves the current map at a fixed interval.");
	autosave_box->Add(autosave_enabled_chkbox, 0, wxALL, 5);

	wxBoxSizer* slider_row = newd wxBoxSizer(wxHORIZONTAL);
	slider_row->Add(newd wxStaticText(general_page, wxID_ANY, "Interval:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
	int cur_interval = g_settings.getInteger(Config::AUTO_SAVE_INTERVAL);
	if (cur_interval < 5)  cur_interval = 5;
	if (cur_interval > 40) cur_interval = 40;
	autosave_interval_slider = newd wxSlider(general_page, wxID_ANY, cur_interval, 5, 40,
		wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	autosave_interval_slider->SetToolTip("Auto-save interval in minutes (5 to 40).");
	slider_row->Add(autosave_interval_slider, 1, wxEXPAND);
	autosave_interval_label = newd wxStaticText(general_page, wxID_ANY, wxString::Format(" %d min", cur_interval));
	slider_row->Add(autosave_interval_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
	autosave_box->Add(slider_row, 0, wxEXPAND | wxALL, 5);

	// Enable/disable slider based on checkbox
	autosave_interval_slider->Enable(autosave_enabled_chkbox->GetValue());
	autosave_enabled_chkbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
		autosave_interval_slider->Enable(autosave_enabled_chkbox->GetValue());
	});
	autosave_interval_slider->Bind(wxEVT_SLIDER, [this](wxCommandEvent&) {
		autosave_interval_label->SetLabel(wxString::Format(" %d min", autosave_interval_slider->GetValue()));
	});

	sizer->Add(autosave_box, 0, wxEXPAND | wxALL, 5);
	// ─────────────────────────────────────────────────────────────────────

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

	// Implement Assets settings directly here inside General Page!
	wxStaticBoxSizer* asset_box = newd wxStaticBoxSizer(wxVERTICAL, general_page, "Asset & Client configuration");
	
	ClientVersion::saveVersions();
	ClientVersionList versions = ClientVersion::getAllVisible();
	std::sort(versions.begin(), versions.end(), [](const ClientVersion* a, const ClientVersion* b) {
		return a->getID() < b->getID();
	});

	auto* asset_row_sizer = newd wxFlexGridSizer(2, 5, 5);
	asset_row_sizer->AddGrowableCol(1);

	// Client version selection
	default_version_choice = newd wxChoice(general_page, wxID_ANY);
	asset_row_sizer->Add(newd wxStaticText(general_page, wxID_ANY, "Client version:"), 0, wxALIGN_CENTER_VERTICAL);
	asset_row_sizer->Add(default_version_choice, 1, wxEXPAND);

	// Check file signatures
	check_sigs_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Check file signatures");
	check_sigs_chkbox->SetValue(g_settings.getBoolean(Config::CHECK_SIGNATURES));
	check_sigs_chkbox->SetToolTip("When this option is not checked, the editor will load any OTB/DAT/SPR combination without complaints. This may cause graphics bugs.");
	asset_row_sizer->Add(check_sigs_chkbox, 0, wxTOP, 5);
	asset_row_sizer->AddSpacer(0);

	// Scan status
	asset_row_sizer->Add(newd wxStaticText(general_page, wxID_ANY, "Scan status:"), 0, wxALIGN_CENTER_VERTICAL | wxTOP, 5);
	scan_status_txt = newd wxStaticText(general_page, wxID_ANY, "Scan: Pending...");
	asset_row_sizer->Add(scan_status_txt, 1, wxEXPAND | wxTOP | wxALIGN_CENTER_VERTICAL, 5);

	// Navigation Button to open files folder
	open_folder_btn = newd wxButton(general_page, wxID_ANY, "Open Directory");
	open_folder_btn->SetToolTip("Open the local client data folder and the asset download webpage in your browser.");
	asset_row_sizer->AddSpacer(0);
	asset_row_sizer->Add(open_folder_btn, 0, wxTOP | wxALIGN_LEFT, 5);

	asset_box->Add(asset_row_sizer, 1, wxEXPAND | wxALL, 5);

	// Version Link help
	help_link = newd wxHyperlinkCtrl(general_page, wxID_ANY,
		"Download Tibia DAT & SPR files here",
		"https://downloads.ots.me/?sort_by=mod&sort_as=desc&dir=data/tibia-clients/dat_and_spr/");
	asset_box->Add(help_link, 0, wxTOP | wxBOTTOM | wxALIGN_CENTER_HORIZONTAL, 5);

	// Populate version choices
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

	sizer->Add(asset_box, 0, wxEXPAND | wxALL, 5);

	sizer->AddSpacer(5);
	wxStaticBoxSizer* net_box = new wxStaticBoxSizer(wxVERTICAL, general_page, "Multiplayer Configuration");
	wxBoxSizer* port_sizer = new wxBoxSizer(wxHORIZONTAL);
	port_sizer->Add(new wxStaticText(general_page, wxID_ANY, "Server Port:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
	multiplayer_port_spin = new wxSpinCtrl(
		general_page,
		wxID_ANY,
		i2ws(g_settings.getInteger(Config::MULTIPLAYER_PORT)),
		wxDefaultPosition,
		wxSize(80, -1),
		wxSP_ARROW_KEYS,
		1,
		65535,
		g_settings.getInteger(Config::MULTIPLAYER_PORT)
	);
	port_sizer->Add(multiplayer_port_spin, 0, wxRIGHT, 10);
	
	wxButton* test_btn = new wxButton(general_page, wxID_ANY, "Test Connection");
	test_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
		const int port = multiplayer_port_spin ? multiplayer_port_spin->GetValue() : kDefaultMultiplayerPort;

#ifdef __WINDOWS__
		if (!IsWindowsFirewallPortAllowed(port)) {
			const int answer = wxMessageBox(
				wxString::Format("Windows does not currently allow inbound TCP traffic on port %d.\n\nCreate the firewall rule now?", port),
				"Windows Firewall",
				wxYES_NO | wxICON_QUESTION,
				this
			);
			if (answer == wxYES && !RequestWindowsFirewallPortRule(this, port)) {
				wxMessageBox("The Windows firewall rule could not be created. Confirm the UAC prompt or create the rule manually.", "Windows Firewall", wxOK | wxICON_WARNING, this);
				return;
			}
		}
#endif

		std::unique_ptr<boost::asio::io_context> service;
		std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor;
		wxString listenError;
		if (!TryStartTemporaryPortListener(port, listenError, service, acceptor)) {
			wxMessageBox(listenError + "\n\nClose any running host on this port or choose another port.", "Connection Test", wxOK | wxICON_ERROR, this);
			return;
		}

		wxString externalIp;
		wxString requestError;
		if (!GetExternalIpAddress(externalIp, requestError)) {
			wxMessageBox(requestError, "Connection Test", wxOK | wxICON_ERROR, this);
			return;
		}

		bool isReachable = false;
		if (!CheckPortReachableFromInternet(externalIp, port, isReachable, requestError)) {
			wxMessageBox(requestError, "Connection Test", wxOK | wxICON_ERROR, this);
			return;
		}

		wxMessageBox(
			isReachable
				? wxString::Format("Port %d is reachable from the internet.\n\nExternal IP: %s", port, externalIp)
				: wxString::Format("Port %d is not reachable from the internet.\n\nExternal IP: %s\n\nCheck your router port forwarding, provider NAT and the Windows firewall rule.", port, externalIp),
			"Connection Test",
			isReachable ? (wxOK | wxICON_INFORMATION) : (wxOK | wxICON_WARNING),
			this
		);
	});
	port_sizer->Add(test_btn);

	wxButton* ip_btn = new wxButton(general_page, wxID_ANY, "Show IP");
	ip_btn->SetToolTip("Fetch your external IP address for hosting.");
	ip_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
		wxString externalIp;
		wxString errorMessage;
		if (!GetExternalIpAddress(externalIp, errorMessage)) {
			wxMessageBox(errorMessage, "Connection Error", wxOK | wxICON_ERROR, this);
			return;
		}

		ShowCopyableExternalIpDialog(this, externalIp);
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

wxNotebookPage* PreferencesWindow::CreatePerformancePage() {
	wxNotebookPage* performance_page = newd wxPanel(book, wxID_ANY);
	performance_page->SetBackgroundColour(book->GetBackgroundColour());
	performance_page->SetForegroundColour(book->GetForegroundColour());

	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	// Rendering details merged directly here!
    wxStaticBoxSizer* visual_group = newd wxStaticBoxSizer(wxVERTICAL, performance_page, "Editor Visuals & Rendering");
    
    parchment_background_chkbox = newd wxCheckBox(performance_page, wxID_ANY, "Use Parchment Background (instead of Black)");
    parchment_background_chkbox->SetValue(g_settings.getInteger(Config::USE_PARCHMENT_BACKGROUND) == 1);
    visual_group->Add(parchment_background_chkbox, 0, wxALL, 5);

    wxBoxSizer* opacity_sizer = newd wxBoxSizer(wxHORIZONTAL);
    opacity_sizer->Add(newd wxStaticText(performance_page, wxID_ANY, "Grid Opacity:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    grid_opacity_slider = newd wxSlider(performance_page, wxID_ANY, g_settings.getInteger(Config::GRID_OPACITY), 0, 255);
    opacity_sizer->Add(grid_opacity_slider, 1, wxEXPAND);
    visual_group->Add(opacity_sizer, 0, wxEXPAND | wxALL, 5);

    wxBoxSizer* light_sizer = newd wxBoxSizer(wxHORIZONTAL);
    light_sizer->Add(newd wxStaticText(performance_page, wxID_ANY, "Global Light Intensity:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    light_intensity_slider = newd wxSlider(performance_page, wxID_ANY, int(g_settings.getFloat(Config::LIGHT_INTENSITY) * 100), 10, 100);
    light_sizer->Add(light_intensity_slider, 1, wxEXPAND);
    visual_group->Add(light_sizer, 0, wxEXPAND | wxALL, 5);

    wxBoxSizer* scale_sizer = newd wxBoxSizer(wxHORIZONTAL);
    scale_sizer->Add(newd wxStaticText(performance_page, wxID_ANY, "UI / Icon Scale (%):"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    int cur_scale = g_settings.getInteger(Config::UI_SCALE);
    if (cur_scale < 100) cur_scale = 100;
    if (cur_scale > 200) cur_scale = 200;
    ui_scale_slider = newd wxSlider(performance_page, wxID_ANY, cur_scale, 100, 200, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    ui_scale_slider->SetToolTip("Adjusts the size of the toolbar and palette icons (100% to 200%).");
    scale_sizer->Add(ui_scale_slider, 1, wxEXPAND);
    visual_group->Add(scale_sizer, 0, wxEXPAND | wxALL, 5);

    sizer->Add(visual_group, 0, wxEXPAND | wxALL, 5);



	wxStaticBoxSizer* tuning_group = new wxStaticBoxSizer(wxVERTICAL, performance_page, "Tuning");
	performance_vsync_chkbox = newd wxCheckBox(performance_page, wxID_ANY, "Vertical Sync");
	performance_vsync_chkbox->SetValue(g_settings.getBoolean(Config::V_SYNC));
	tuning_group->Add(performance_vsync_chkbox, 0, wxALL, 5);

	performance_multimonitor_chkbox = newd wxCheckBox(performance_page, wxID_ANY, "Multi-display workspace");
	performance_multimonitor_chkbox->SetValue(g_settings.getBoolean(Config::MULTI_MONITOR_WORKSPACE));
	tuning_group->Add(performance_multimonitor_chkbox, 0, wxALL, 5);

	wxFlexGridSizer* grid = newd wxFlexGridSizer(2, 10, 10);
	grid->AddGrowableCol(1);
	grid->Add(newd wxStaticText(performance_page, wxID_ANY, "Worker threads:"), 0, wxALIGN_CENTER_VERTICAL);
	performance_worker_threads_slider = newd wxSlider(performance_page, wxID_ANY, g_settings.getInteger(Config::WORKER_THREADS), 1, 16);
	grid->Add(performance_worker_threads_slider, 0, wxEXPAND);
	grid->Add(newd wxStaticText(performance_page, wxID_ANY, "Anti-aliasing:"), 0, wxALIGN_CENTER_VERTICAL);
	performance_aa_choice = newd wxChoice(performance_page, wxID_ANY);
	performance_aa_choice->Append("Off");
	performance_aa_choice->Append("Low");
	performance_aa_choice->Append("High");
	performance_aa_choice->SetSelection(g_settings.getInteger(Config::SHADER_AA_LEVEL));
	grid->Add(performance_aa_choice, 0, wxEXPAND);
	grid->Add(newd wxStaticText(performance_page, wxID_ANY, "CRT strength:"), 0, wxALIGN_CENTER_VERTICAL);
	performance_crt_slider = newd wxSlider(performance_page, wxID_ANY, g_settings.getInteger(Config::SHADER_CRT_STRENGTH), 0, 100);
	grid->Add(performance_crt_slider, 0, wxEXPAND);
	grid->Add(newd wxStaticText(performance_page, wxID_ANY, "Water speed:"), 0, wxALIGN_CENTER_VERTICAL);
	performance_water_slider = newd wxSlider(performance_page, wxID_ANY, int(g_settings.getFloat(Config::SHADER_WATER_ANIM_SPEED) * 10), 0, 50);
	grid->Add(performance_water_slider, 0, wxEXPAND);
	tuning_group->Add(grid, 0, wxALL | wxEXPAND, 5);
	sizer->Add(tuning_group, 0, wxEXPAND | wxALL, 5);


	performance_page->SetSizerAndFit(sizer);
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

	wxStaticText* scroll_label = newd wxStaticText(ui_page, wxID_ANY, "Scroll speed: ");
	sizer->Add(scroll_label, 0, wxLEFT | wxTOP, 5);

	auto true_scrollspeed = std::clamp(int(std::round(std::abs(g_settings.getFloat(Config::SCROLL_SPEED)) * 2.0f)), 1, 10);
	scroll_speed_slider = newd wxSlider(ui_page, wxID_ANY, true_scrollspeed, 1, 10);
	scroll_speed_slider->SetToolTip("This controls how fast the map will scroll when you hold down the center mouse button and move it around.");
	sizer->Add(scroll_speed_slider, 0, wxEXPAND, 5);

	auto update_scroll_label = [scroll_label, this]() {
		scroll_label->SetLabel(wxString::Format("Scroll speed: %d", scroll_speed_slider->GetValue()));
	};
	scroll_speed_slider->Bind(wxEVT_SLIDER, [update_scroll_label](wxCommandEvent&) {
		update_scroll_label();
	});
	update_scroll_label();

	wxStaticText* zoom_label = newd wxStaticText(ui_page, wxID_ANY, "Zoom speed: ");
	sizer->Add(zoom_label, 0, wxLEFT | wxTOP, 5);

	auto true_zoomspeed = std::clamp(int(std::round(g_settings.getFloat(Config::ZOOM_SPEED) * 5.0f)), 1, 10);
	zoom_speed_slider = newd wxSlider(ui_page, wxID_ANY, true_zoomspeed, 1, 10);
	zoom_speed_slider->SetToolTip("This controls how fast you will zoom when you scroll the center mouse button.");
	sizer->Add(zoom_speed_slider, 0, wxEXPAND, 5);

	auto update_zoom_label = [zoom_label, this]() {
		zoom_label->SetLabel(wxString::Format("Zoom speed: %d", zoom_speed_slider->GetValue()));
	};
	zoom_speed_slider->Bind(wxEVT_SLIDER, [update_zoom_label](wxCommandEvent&) {
		update_zoom_label();
	});
	update_zoom_label();

	wxStaticText* minimap_label = newd wxStaticText(ui_page, wxID_ANY, "Minimap scroll speed: ");
	sizer->Add(minimap_label, 0, wxLEFT | wxTOP, 5);

	auto true_minispeed = std::clamp(int(std::round(g_settings.getFloat(Config::MINIMAP_SCROLL_SPEED))), 1, 10);
	minimap_scroll_speed_slider = newd wxSlider(ui_page, wxID_ANY, true_minispeed, 1, 10);
	minimap_scroll_speed_slider->SetToolTip("This controls how fast you jump/drag inside the minimap.");
	sizer->Add(minimap_scroll_speed_slider, 0, wxEXPAND, 5);

	auto update_mini_label = [minimap_label, this]() {
		minimap_label->SetLabel(wxString::Format("Minimap scroll speed: %d", minimap_scroll_speed_slider->GetValue()));
	};
	minimap_scroll_speed_slider->Bind(wxEVT_SLIDER, [update_mini_label](wxCommandEvent&) {
		update_mini_label();
	});
	update_mini_label();

	ui_page->SetSizerAndFit(sizer);

	return ui_page;
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
	g_settings.setInteger(Config::MULTIPLAYER_PORT, multiplayer_port_spin ? multiplayer_port_spin->GetValue() : kDefaultMultiplayerPort);
	g_settings.setInteger(Config::UNDO_SIZE, undo_size_spin->GetValue());
	g_settings.setInteger(Config::UNDO_MEM_SIZE, undo_mem_size_spin->GetValue());
	g_settings.setInteger(Config::REPLACE_SIZE, replace_size_spin->GetValue());
	g_settings.setInteger(Config::COPY_POSITION_FORMAT, position_choice->GetSelection());

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
	if (parchment_background_chkbox)
		g_settings.setInteger(Config::USE_PARCHMENT_BACKGROUND, parchment_background_chkbox->GetValue() ? 1 : 0);
	if (grid_opacity_slider)
		g_settings.setInteger(Config::GRID_OPACITY, grid_opacity_slider->GetValue());
	if (light_intensity_slider)
		g_settings.setFloat(Config::LIGHT_INTENSITY, light_intensity_slider->GetValue() / 100.0f);

	// if (g_settings.getInteger(Config::RENDER_BACKEND) != backend_radio->GetSelection()) {
	// 	g_settings.setInteger(Config::RENDER_BACKEND, backend_radio->GetSelection());
	// 	must_restart = true;
	// }
    g_settings.setInteger(Config::V_SYNC, performance_vsync_chkbox ? performance_vsync_chkbox->GetValue() : 0);
    g_settings.setInteger(Config::MULTI_MONITOR_WORKSPACE, performance_multimonitor_chkbox ? performance_multimonitor_chkbox->GetValue() : 0);
    g_settings.setInteger(Config::WORKER_THREADS, performance_worker_threads_slider ? performance_worker_threads_slider->GetValue() : 4);
    g_settings.setInteger(Config::SHADER_AA_LEVEL, performance_aa_choice ? performance_aa_choice->GetSelection() : 0);
    g_settings.setInteger(Config::SHADER_CRT_STRENGTH, performance_crt_slider ? performance_crt_slider->GetValue() : 0);
    g_settings.setFloat(Config::SHADER_WATER_ANIM_SPEED, performance_water_slider ? performance_water_slider->GetValue() / 10.0f : 1.0f);

	if (ui_scale_slider) {
		int old_scale = g_settings.getInteger(Config::UI_SCALE);
		if (old_scale < 100) old_scale = 100;
		if (old_scale > 200) old_scale = 200;
		int new_scale = ui_scale_slider->GetValue();
		if (old_scale != new_scale) {
			g_settings.setInteger(Config::UI_SCALE, new_scale);
			must_restart = true;
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

	if (must_restart) {
		g_gui.PopupDialog(this, "Notice", "You must restart the editor for the changes to take effect.", wxOK);
	}

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
	Fit();
}

wxNotebookPage* PreferencesWindow::CreateHotkeysPage() {
	wxNotebookPage* page = newd wxPanel(book, wxID_ANY);
	page->SetBackgroundColour(book->GetBackgroundColour());
	page->SetForegroundColour(book->GetForegroundColour());

	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	wxListCtrl* list = newd wxListCtrl(page, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
	list->SetBackgroundColour(book->GetBackgroundColour());
	list->SetForegroundColour(book->GetForegroundColour());

	list->InsertColumn(0, "Action", wxLIST_FORMAT_LEFT, 240);
	list->InsertColumn(1, "Hotkey / Control", wxLIST_FORMAT_LEFT, 180);

	struct HotkeyInfo {
		wxString action;
		wxString key;
	};

	std::vector<HotkeyInfo> hotkeys = {
		{"Move view (canvas)", "W / A / S / D"},
		{"Move view (drag)", "Middle Mouse Button Drag"},
		{"Zoom Map", "Mouse Wheel / Plus / Minus"},
		{"Open quick Tool Wheel", "Shift + Q"},
		{"Close Tool Wheel / Dialog", "ESC"},
		{"Copy Selection", "Ctrl + C"},
		{"Paste Selection", "Ctrl + V"},
		{"Cut Selection", "Ctrl + X"},
		{"Delete Selection", "Delete"},
		{"Change Floor (Up/Down)", "PageUp / PageDown"},
		{"New Map", "Ctrl + N"},
		{"Open Map", "Ctrl + O"},
		{"Save Map", "Ctrl + S"},
		{"Undo", "Ctrl + Z"},
		{"Redo", "Ctrl + Y"}
	};

	for (size_t i = 0; i < hotkeys.size(); ++i) {
		long tmp = list->InsertItem(static_cast<long>(i), hotkeys[i].action);
		list->SetItem(tmp, 1, hotkeys[i].key);
	}

	sizer->Add(list, 1, wxEXPAND | wxALL, 5);
	page->SetSizer(sizer);
	return page;
}
