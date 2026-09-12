#include "mme_updater.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <wx/msgdlg.h>
#include <wx/progdlg.h>
#include <wx/utils.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/app.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <fstream>
#include <thread>
#include <sstream>
#include <vector>
#include <algorithm>

MMEUpdater& MMEUpdater::Instance() {
	static MMEUpdater instance;
	return instance;
}

MMEUpdater::MMEUpdater() :
	update_available(false) {
}

std::string MMEUpdater::GetCurrentVersion() const {
#ifdef __RME_VERSION__
	return std::string(__RME_VERSION__);
#else
	return "1.9.5";
#endif
}

// Parses string like "v1.8", "1.8.1", "1.8.1 (by Mioshiro)" into (major, minor, patch)
static std::vector<int> ParseVersion(const std::string& str) {
	std::vector<int> parts = { 0, 0, 0 };
	std::string clean;
	for (char c : str) {
		if (std::isdigit(c) || c == '.') {
			clean += c;
		} else if (!clean.empty()) {
			break;
		}
	}
	std::stringstream ss(clean);
	std::string token;
	int idx = 0;
	while (std::getline(ss, token, '.') && idx < 3) {
		try {
			parts[idx++] = std::stoi(token);
		} catch (...) {}
	}
	return parts;
}

static bool IsRemoteNewer(const std::string& remoteTag, const std::string& localVer) {
	std::vector<int> r = ParseVersion(remoteTag);
	std::vector<int> l = ParseVersion(localVer);

	for (int i = 0; i < 3; ++i) {
		if (r[i] > l[i]) return true;
		if (r[i] < l[i]) return false;
	}
	return false;
}

bool MMEUpdater::PerformCheck(std::string& out_tag, std::string& out_url, std::string& out_zip_url, std::string& out_notes) {
	try {
		cpr::Response r = cpr::Get(
			cpr::Url{ "https://api.github.com/repos/Mioshiru/MME/releases/latest" },
			cpr::Header{ { "User-Agent", "MME-MapEditor-Updater" }, { "Accept", "application/vnd.github.v3+json" } },
			cpr::Redirect{ 5L, true, false, cpr::PostRedirectFlags::POST_ALL },
			cpr::Timeout{ 15000 },
			cpr::ConnectTimeout{ 10000 }
		);

		if (r.status_code != 200 || r.text.empty()) {
			return false;
		}

		auto j = nlohmann::json::parse(r.text, nullptr, false);
		if (j.is_discarded() || !j.contains("tag_name")) {
			return false;
		}

		out_tag = j["tag_name"].get<std::string>();
		out_url = j.value("html_url", "https://github.com/Mioshiru/MME/releases");
		out_notes = j.value("body", "");

		if (j.contains("assets") && j["assets"].is_array()) {
			for (const auto& asset : j["assets"]) {
				std::string name = asset.value("name", "");
				std::string download_url = asset.value("browser_download_url", "");
				if (name.find(".zip") != std::string::npos && !download_url.empty()) {
					out_zip_url = download_url;
					break;
				}
			}
		}

		if (out_zip_url.empty()) {
			out_zip_url = j.value("zipball_url", "");
		}

		return true;
	} catch (...) {
		return false;
	}
}

void MMEUpdater::CheckForUpdatesAsync(wxWindow* parent) {
	std::thread([this, parent]() {
		std::string tag, url, zip_url, notes;
		if (PerformCheck(tag, url, zip_url, notes)) {
			std::string cur = GetCurrentVersion();
			if (IsRemoteNewer(tag, cur)) {
				update_available = true;
				latest_tag = tag;
				latest_url = url;
				latest_zip_url = zip_url;
			}
		}
	}).detach();
}

// Styled Corporate Design Dialog for Update Notification
class StyledUpdateDialog : public wxDialog {
public:
	StyledUpdateDialog(wxWindow* parent, const wxString& titleText, const wxString& msgText, bool isUpdateAvailable) :
		wxDialog(parent, wxID_ANY, titleText, wxDefaultPosition, wxSize(440, 250), wxDEFAULT_DIALOG_STYLE) {
		SetBackgroundColour(wxColour(12, 22, 38));
		SetForegroundColour(wxColour(240, 245, 255));

		wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);

		wxPanel* card = new wxPanel(this, wxID_ANY);
		card->SetBackgroundColour(wxColour(18, 32, 54));
		wxBoxSizer* cardSizer = new wxBoxSizer(wxVERTICAL);

		wxStaticText* header = new wxStaticText(card, wxID_ANY, titleText);
		header->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
		header->SetForegroundColour(wxColour(240, 210, 120));
		cardSizer->Add(header, 0, wxBOTTOM, 6);

		wxStaticText* body = new wxStaticText(card, wxID_ANY, msgText);
		body->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
		body->SetForegroundColour(wxColour(200, 215, 235));
		cardSizer->Add(body, 1, wxEXPAND);

		card->SetSizer(cardSizer);
		topsizer->Add(card, 1, wxEXPAND | wxALL, 12);

		wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
		if (isUpdateAvailable) {
			wxButton* yesBtn = new wxButton(this, wxID_YES, "Download & Install");
			yesBtn->SetBackgroundColour(wxColour(35, 75, 150));
			yesBtn->SetForegroundColour(wxColour(240, 210, 120));
			yesBtn->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
			yesBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_YES); });

			wxButton* noBtn = new wxButton(this, wxID_NO, "Later");
			noBtn->SetBackgroundColour(wxColour(22, 36, 58));
			noBtn->SetForegroundColour(wxColour(180, 190, 205));
			noBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_NO); });

			btnSizer->Add(yesBtn, 0, wxRIGHT, 8);
			btnSizer->Add(noBtn, 0);
		} else {
			wxButton* okBtn = new wxButton(this, wxID_OK, "OK");
			okBtn->SetBackgroundColour(wxColour(35, 75, 150));
			okBtn->SetForegroundColour(wxColour(240, 210, 120));
			okBtn->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
			okBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_OK); });
			btnSizer->Add(okBtn, 0);
		}

		Bind(wxEVT_CLOSE_WINDOW, [this, isUpdateAvailable](wxCloseEvent&) {
			EndModal(isUpdateAvailable ? wxID_NO : wxID_OK);
		});

		topsizer->Add(btnSizer, 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT | wxBOTTOM, 12);
		SetSizerAndFit(topsizer);
		Centre(wxBOTH);
	}
};

void MMEUpdater::CheckForUpdates(wxWindow* parent, bool user_initiated) {
	wxProgressDialog progress("Checking for Updates", "Connecting to GitHub releases...", 100, parent, wxPD_APP_MODAL | wxPD_AUTO_HIDE);
	progress.Pulse("Querying Mioshiru/MME repository...");

	std::string tag, url, zip_url, notes;
	bool success = PerformCheck(tag, url, zip_url, notes);

	progress.Update(100);

	if (!success) {
		if (user_initiated) {
			StyledUpdateDialog dlg(parent, "Update Check", "Could not check for updates.\nPlease verify your internet connection or visit GitHub releases.", false);
			dlg.ShowModal();
		}
		return;
	}

	std::string cur = GetCurrentVersion();
	bool is_newer = IsRemoteNewer(tag, cur);

	if (is_newer) {
		update_available = true;
		latest_tag = tag;
		latest_url = url;
		latest_zip_url = zip_url;

		wxString msg = wxString::Format(
			"A new version (%s) of Mios Map Editor is available!\n\n"
			"Current installed version: %s\n\n"
			"Would you like to download and install this update now?",
			tag.c_str(), cur.c_str()
		);

		StyledUpdateDialog dlg(parent, "Update Available", msg, true);
		int res = dlg.ShowModal();
		if (res == wxID_YES) {
			if (!zip_url.empty()) {
				DownloadAndInstall(parent, zip_url, tag);
			} else {
				wxLaunchDefaultBrowser(url);
			}
		}
	} else {
		update_available = false;
		if (user_initiated) {
			wxString msg = wxString::Format(
				"You are running the latest version of Mios Map Editor!\n\n"
				"Current installed version: %s\n"
				"Latest release on GitHub: %s",
				cur.c_str(), tag.c_str()
			);
			StyledUpdateDialog dlg(parent, "Up to Date", msg, false);
			dlg.ShowModal();
		}
	}
}

bool MMEUpdater::DownloadAndInstall(wxWindow* parent, const std::string& zip_url, const std::string& tag) {
	wxString tempDir = wxStandardPaths::Get().GetTempDir();
	wxString zipPath = tempDir + "\\mme_update_" + tag + ".zip";

	wxProgressDialog progress("Downloading Update", "Connecting to download server...", 100, parent, wxPD_APP_MODAL | wxPD_CAN_ABORT | wxPD_AUTO_HIDE);

	// Resolve any HTTP 301/302 redirects first to obtain the direct asset URL (AWS S3/GitHub CDN link)
	// This prevents the 302 redirect response body from being written into the ZIP stream.
	std::string direct_url = zip_url;
	try {
		cpr::Response r_redir = cpr::Get(
			cpr::Url{ zip_url },
			cpr::Header{ { "User-Agent", "MME-MapEditor-Updater" } },
			cpr::Redirect{ 5L, true, false, cpr::PostRedirectFlags::POST_ALL },
			cpr::Timeout{ 15000 },
			cpr::ConnectTimeout{ 10000 }
		);
		if (!r_redir.url.str().empty()) {
			direct_url = r_redir.url.str();
		}
	} catch (...) {
		// Fall back to original URL
		direct_url = zip_url;
	}

	std::ofstream outFile(zipPath.ToStdString(), std::ios::binary | std::ios::trunc);
	if (!outFile.is_open()) {
		StyledUpdateDialog dlg(parent, "Update Error", "Failed to create temporary update file.", false);
		dlg.ShowModal();
		return false;
	}

	bool aborted = false;
	cpr::Response r = cpr::Get(
		cpr::Url{ direct_url },
		cpr::Header{ { "User-Agent", "MME-MapEditor-Updater" } },
		cpr::Redirect{ 10L, true, false, cpr::PostRedirectFlags::POST_ALL },
		cpr::ProgressCallback([&progress, &aborted](cpr::cpr_off_t downloadTotal, cpr::cpr_off_t downloadNow, cpr::cpr_off_t, cpr::cpr_off_t, intptr_t) -> bool {
			if (downloadTotal > 0) {
				int pct = (int)((downloadNow * 100) / downloadTotal);
				long long mbNow = downloadNow / (1024 * 1024);
				long long mbTotal = downloadTotal / (1024 * 1024);
				if (!progress.Update(std::min(99, pct), wxString::Format("Downloading update... %d%% (%lld MB / %lld MB)", pct, mbNow, mbTotal))) {
					aborted = true;
					return false;
				}
			} else {
				progress.Pulse("Downloading update data...");
			}
			return true;
		}),
		cpr::WriteCallback([&outFile](std::string_view data, intptr_t) -> bool {
			outFile.write(data.data(), data.size());
			return true;
		}),
		cpr::Timeout{ 900000 }, // 15 minutes timeout for large packages
		cpr::ConnectTimeout{ 15000 }
	);

	outFile.close();

	if (aborted || r.status_code != 200) {
		wxRemoveFile(zipPath);
		if (!aborted) {
			StyledUpdateDialog dlg(parent, "Download Failed", "Download failed. Please check https://github.com/Mioshiru/MME/releases manually.", false);
			dlg.ShowModal();
		}
		return false;
	}

	// Verify ZIP magic header (0x50, 0x4B, 0x03, 0x04)
	std::ifstream checkFile(zipPath.ToStdString(), std::ios::binary);
	char magic[4] = { 0 };
	if (checkFile.is_open()) {
		checkFile.read(magic, 4);
		checkFile.close();
	}
	bool is_valid_zip = (magic[0] == 'P' && magic[1] == 'K' && magic[2] == 0x03 && magic[3] == 0x04);
	if (!is_valid_zip) {
		wxRemoveFile(zipPath);
		StyledUpdateDialog dlg(parent, "Download Corrupted", "The downloaded package appears to be corrupted or invalid.\nPlease download the update manually from GitHub releases.", false);
		dlg.ShowModal();
		return false;
	}

	progress.Update(100, "Download completed!");

	StyledUpdateDialog confirmDlg(parent, "Update Ready", "Update package downloaded successfully!\n\nRestart Mios Map Editor now to complete installation?", true);
	int res = confirmDlg.ShowModal();
	if (res != wxID_YES) {
		return true;
	}

	// Create updater batch script in temporary directory
	wxString appDir = wxPathOnly(wxStandardPaths::Get().GetExecutablePath());
	wxString scriptPath = tempDir + "\\update_mme_" + tag + ".bat";
	wxString exePath = wxStandardPaths::Get().GetExecutablePath();
	wxString exeName = wxFileName(exePath).GetFullName();

	std::ofstream script(scriptPath.ToStdString());
	if (script.is_open()) {
		script << "@echo off\n";
		script << "setlocal enabledelayedexpansion\n";
		script << "title Mios Map Editor - Applying Update " << tag << "...\n";
		script << "echo ========================================================\n";
		script << "echo        Applying Mios Map Editor Update " << tag << "\n";
		script << "echo ========================================================\n";
		script << "echo.\n";
		script << "echo Waiting for Mios Map Editor to shut down...\n";
		script << "set /a WAIT_COUNT=0\n";
		script << ":WAIT_LOOP\n";
		script << "tasklist /FI \"IMAGENAME eq " << exeName.ToStdString() << "\" 2>nul | find /I /N \"" << exeName.ToStdString() << "\">nul\n";
		script << "if !ERRORLEVEL! EQU 0 (\n";
		script << "    set /a WAIT_COUNT+=1\n";
		script << "    if !WAIT_COUNT! GTR 10 (\n";
		script << "        echo Closing active process...\n";
		script << "        taskkill /F /IM \"" << exeName.ToStdString() << "\" >nul 2>&1\n";
		script << "        timeout /t 1 /nobreak >nul\n";
		script << "    ) else (\n";
		script << "        timeout /t 1 /nobreak >nul\n";
		script << "        goto WAIT_LOOP\n";
		script << "    )\n";
		script << ")\n";
		script << "timeout /t 1 /nobreak >nul\n";
		script << "echo Extracting update package...\n";
		script << "tar -xf \"" << zipPath.ToStdString() << "\" -C \"" << appDir.ToStdString() << "\" >nul 2>&1\n";
		script << "set EXTRACT_STATUS=!ERRORLEVEL!\n";
		script << "if !EXTRACT_STATUS! NEQ 0 (\n";
		script << "    echo Using PowerShell extraction engine...\n";
		script << "    powershell -NoProfile -ExecutionPolicy Bypass -Command \"$ErrorActionPreference = 'Stop'; [System.Reflection.Assembly]::LoadWithPartialName('System.IO.Compression.FileSystem') | Out-Null; $zip = [System.IO.Compression.ZipFile]::OpenRead('" << zipPath.ToStdString() << "'); foreach ($entry in $zip.Entries) { $target = [System.IO.Path]::Combine('" << appDir.ToStdString() << "', $entry.FullName); if ($entry.FullName.EndsWith('/') -or $entry.FullName.EndsWith('\\')) { [System.IO.Directory]::CreateDirectory($target) | Out-Null; } else { $parent = [System.IO.Path]::GetDirectoryName($target); if (-not [System.IO.Directory]::Exists($parent)) { [System.IO.Directory]::CreateDirectory($parent) | Out-Null; } [System.IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $target, $true); } }; $zip.Dispose()\" >nul 2>&1\n";
		script << ")\n";
		script << "if exist \"" << zipPath.ToStdString() << "\" del /f /q \"" << zipPath.ToStdString() << "\" >nul 2>&1\n";
		script << "echo Starting updated Mios Map Editor...\n";
		script << "cd /d \"" << appDir.ToStdString() << "\"\n";
		script << "start \"\" /D \"" << appDir.ToStdString() << "\" \"" << exePath.ToStdString() << "\"\n";
		script << "(goto) 2>nul & del \"%~f0\"\n";
		script.close();

#ifdef _WIN32
		// Launch updater script explicitly with cmd.exe
		HINSTANCE res_exec = ShellExecuteA(NULL, "open", "cmd.exe", ("/c \"" + scriptPath.ToStdString() + "\"").c_str(), appDir.ToStdString().c_str(), SW_SHOWNORMAL);
		if ((INT_PTR)res_exec <= 32) {
			ShellExecuteA(NULL, "runas", "cmd.exe", ("/c \"" + scriptPath.ToStdString() + "\"").c_str(), appDir.ToStdString().c_str(), SW_SHOWNORMAL);
		}
#else
		wxExecute("cmd.exe /c \"" + scriptPath + "\"", wxEXEC_ASYNC);
#endif
		if (wxTheApp) {
			wxTheApp->ExitMainLoop();
		}
		exit(0);
	} else {
		StyledUpdateDialog dlg(parent, "Installation Error", "Failed to prepare updater helper. Please extract the downloaded zip file manually.", false);
		dlg.ShowModal();
	}

	return true;
}
