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
	return "1.9.0";
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
			cpr::Timeout{ 8000 }
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
		wxDialog(parent, wxID_ANY, titleText, wxDefaultPosition, wxSize(420, 240), wxDEFAULT_DIALOG_STYLE) {
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

			wxButton* noBtn = new wxButton(this, wxID_NO, "Later");
			noBtn->SetBackgroundColour(wxColour(22, 36, 58));
			noBtn->SetForegroundColour(wxColour(180, 190, 205));

			btnSizer->Add(yesBtn, 0, wxRIGHT, 8);
			btnSizer->Add(noBtn, 0);
		} else {
			wxButton* okBtn = new wxButton(this, wxID_OK, "OK");
			okBtn->SetBackgroundColour(wxColour(35, 75, 150));
			okBtn->SetForegroundColour(wxColour(240, 210, 120));
			okBtn->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
			btnSizer->Add(okBtn, 0);
		}

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

	wxProgressDialog progress("Downloading Update", "Downloading " + tag + " package from GitHub...", 100, parent, wxPD_APP_MODAL | wxPD_CAN_ABORT | wxPD_AUTO_HIDE);

	std::ofstream outFile(zipPath.ToStdString(), std::ios::binary);
	if (!outFile.is_open()) {
		StyledUpdateDialog dlg(parent, "Update Error", "Failed to create temporary update file.", false);
		dlg.ShowModal();
		return false;
	}

	bool aborted = false;
	cpr::Response r = cpr::Get(
		cpr::Url{ zip_url },
		cpr::Header{ { "User-Agent", "MME-MapEditor-Updater" } },
		cpr::ProgressCallback([&progress, &aborted](cpr::cpr_off_t downloadTotal, cpr::cpr_off_t downloadNow, cpr::cpr_off_t, cpr::cpr_off_t, intptr_t) -> bool {
			if (downloadTotal > 0) {
				int pct = (int)((downloadNow * 100) / downloadTotal);
				if (!progress.Update(std::min(99, pct), wxString::Format("Downloading update... %d%%", pct))) {
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
		cpr::Timeout{ 120000 }
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

	progress.Update(100, "Download completed!");

	StyledUpdateDialog confirmDlg(parent, "Update Ready", "Update downloaded successfully.\n\nRestart Mios Map Editor now to complete installation?", true);
	int res = confirmDlg.ShowModal();
	if (res != wxID_YES) {
		return true;
	}

	// Create updater batch script in safe temporary directory (never fails due to write permissions in Program Files)
	wxString appDir = wxPathOnly(wxStandardPaths::Get().GetExecutablePath());
	wxString scriptPath = tempDir + "\\update_mme_" + tag + ".bat";
	wxString exePath = wxStandardPaths::Get().GetExecutablePath();
	wxString exeName = wxFileName(exePath).GetFullName();

	std::ofstream script(scriptPath.ToStdString());
	if (script.is_open()) {
		script << "@echo off\n";
		script << "setlocal EnableDelayedExpansion\n";
		script << "title Mios Map Editor - Applying Update...\n";
		script << "echo Waiting for Mios Map Editor to close...\n";
		script << ":WAIT_LOOP\n";
		script << "tasklist /FI \"IMAGENAME eq " << exeName.ToStdString() << "\" 2>NUL | find /I /N \"" << exeName.ToStdString() << "\">NUL\n";
		script << "if \"%ERRORLEVEL%\"==\"0\" (\n";
		script << "    timeout /t 1 /nobreak >nul\n";
		script << "    goto WAIT_LOOP\n";
		script << ")\n";
		script << "timeout /t 1 /nobreak >nul\n";
		script << "echo Extracting update package...\n";
		script << "tar -xf \"" << zipPath.ToStdString() << "\" -C \"" << appDir.ToStdString() << "\" >nul 2>&1\n";
		script << "if %ERRORLEVEL% NEQ 0 (\n";
		script << "    powershell -NoProfile -ExecutionPolicy Bypass -Command \"Expand-Archive -Path '" << zipPath.ToStdString() << "' -DestinationPath '" << appDir.ToStdString() << "' -Force\"\n";
		script << ")\n";
		script << "del \"" << zipPath.ToStdString() << "\" >nul 2>&1\n";
		script << "echo Starting updated Mios Map Editor...\n";
		script << "start \"\" \"" << exePath.ToStdString() << "\"\n";
		script << "(goto) 2>nul & del \"%~f0\"\n";
		script.close();

#ifdef _WIN32
		// Launch updater script with normal or elevated privileges if needed
		HINSTANCE res_exec = ShellExecuteA(NULL, "open", scriptPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
		if ((INT_PTR)res_exec <= 32) {
			// If open failed (e.g. permission issue), try with UAC elevation
			ShellExecuteA(NULL, "runas", scriptPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
		}
#else
		wxExecute("cmd.exe /c \"" + scriptPath + "\"", wxEXEC_ASYNC);
#endif
		if (wxTheApp && wxTheApp->GetTopWindow()) {
			wxTheApp->GetTopWindow()->Close(true);
		}
	} else {
		StyledUpdateDialog dlg(parent, "Installation Error", "Failed to prepare updater helper. Please extract the downloaded zip file manually.", false);
		dlg.ShowModal();
	}

	return true;
}
