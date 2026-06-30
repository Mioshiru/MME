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
#include "welcome_dialog.h"
#include "settings.h"
#include "preferences.h"
#include "main_menubar.h"
#include <wx/app.h>
#include "gui.h"

wxDEFINE_EVENT(WELCOME_DIALOG_ACTION, wxCommandEvent);
wxDEFINE_EVENT(WELCOME_DIALOG_DELETE_RECENT, wxCommandEvent);

namespace {
wxBitmap LoadLandingPageBitmap(const wxBitmap& fallback_logo) {
	if (fallback_logo.IsOk()) {
		return fallback_logo;
	}

	wxArrayString candidates;
	wxString exe_dir = wxPathOnly(wxStandardPaths::Get().GetExecutablePath());
	wxString cwd = wxGetCwd();
	candidates.Add(exe_dir + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "editor_icon.png");
	candidates.Add(exe_dir + wxFILE_SEP_PATH + ".." + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "editor_icon.png");
	candidates.Add(exe_dir + wxFILE_SEP_PATH + ".." + wxFILE_SEP_PATH + ".." + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "editor_icon.png");
	candidates.Add(cwd + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "editor_icon.png");
	candidates.Add(cwd + wxFILE_SEP_PATH + ".." + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "editor_icon.png");
	candidates.Add(cwd + wxFILE_SEP_PATH + ".." + wxFILE_SEP_PATH + ".." + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "editor_icon.png");

	for (const wxString& candidate : candidates) {
		if (!wxFileExists(candidate)) {
			continue;
		}

		wxImage image;
		if (image.LoadFile(candidate, wxBITMAP_TYPE_PNG)) {
			wxBitmap bitmap(image);
			if (bitmap.IsOk()) {
				return bitmap;
			}
		}
	}

	return wxNullBitmap;
}
}

// Forward declarations for subclassed controls to prevent syntax errors
class TransparentStaticBitmap : public wxStaticBitmap {
public:
	TransparentStaticBitmap(wxWindow* parent, wxWindowID id, const wxBitmap& label, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize)
		: wxStaticBitmap(parent, id, label, pos, size) {
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		Bind(wxEVT_PAINT, &TransparentStaticBitmap::OnPaint, this);
		Bind(wxEVT_ERASE_BACKGROUND, &TransparentStaticBitmap::OnEraseBackground, this);
	}
	void OnEraseBackground(wxEraseEvent& event) {
		// Do nothing to avoid background erasing with system colors
	}
	void OnPaint(const wxPaintEvent&) {
		wxPaintDC dc(this);
		wxWindow* parent = GetParent();
		wxColour bg = wxColor(10, 15, 25); // Hardcoded fallback
		if (parent) {
			bg = parent->GetBackgroundColour();
		}
		dc.SetBrush(wxBrush(bg));
		dc.SetPen(wxPen(bg));
		dc.DrawRectangle(wxRect(wxPoint(0, 0), GetClientSize()));
		
		if (GetBitmap().IsOk()) {
			dc.DrawBitmap(GetBitmap(), wxPoint(0, 0), true);
		}
	}
};

class TransparentStaticText : public wxStaticText {
public:
	TransparentStaticText(wxWindow* parent, wxWindowID id, const wxString& label, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0)
		: wxStaticText(parent, id, label, pos, size, style) {
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		Bind(wxEVT_PAINT, &TransparentStaticText::OnPaint, this);
		Bind(wxEVT_ERASE_BACKGROUND, &TransparentStaticText::OnEraseBackground, this);
	}
	void OnEraseBackground(wxEraseEvent& event) {
		// Do nothing to avoid background erasing with system colors
	}
	void OnPaint(const wxPaintEvent&) {
		wxPaintDC dc(this);
		wxWindow* parent = GetParent();
		wxColour bg = wxColor(10, 15, 25); // Hardcoded fallback for absolute color fidelity
		if (parent) {
			bg = parent->GetBackgroundColour();
		}
		dc.SetBrush(wxBrush(bg));
		dc.SetPen(wxPen(bg));
		dc.DrawRectangle(wxRect(wxPoint(0, 0), GetClientSize()));

		dc.SetFont(GetFont());
		dc.SetTextForeground(GetForegroundColour());
		dc.SetBackgroundMode(wxTRANSPARENT);
		
		int width, height;
		GetClientSize(&width, &height);
		wxSize text_size = dc.GetTextExtent(GetLabel());
		
		int x = 0;
		if (GetWindowStyle() & wxALIGN_CENTER_HORIZONTAL) {
			x = (width - text_size.x) / 2;
			if (x < 0) x = 0;
		}
		int y = (height - text_size.y) / 2;
		if (y < 0) y = 0;
		
		dc.DrawText(GetLabel(), wxPoint(x, y));
	}
};

WelcomeDialog::WelcomeDialog(const wxString& title_text, const wxString& version_text, const wxSize& size, const wxBitmap& rme_logo, const std::vector<wxString>& recent_files) :
	wxDialog(nullptr, wxID_ANY, "", wxDefaultPosition, size, wxSTAY_ON_TOP | wxDEFAULT_DIALOG_STYLE) {
	Centre();
	wxColour base_colour = wxColor(10, 15, 25); // Tiefes Dunkelblau/Schwarz
	SetBackgroundColour(base_colour);
	
	// Robustly load the specified icon from the user's explicit path
	wxBitmap landing_logo;
	wxString explicit_path = "C:\\Users\\weber\\Dokumente\\Projekt\\In Arbeit\\Map Editor\\icons\\editor_icon.png";
	if (wxFileExists(explicit_path)) {
		wxImage img;
		if (img.LoadFile(explicit_path, wxBITMAP_TYPE_PNG)) {
			// Rescale beautifully for the game menu
			landing_logo = wxBitmap(img.Scale(140, 140, wxIMAGE_QUALITY_HIGH));
		}
	}
	if (!landing_logo.IsOk() && rme_logo.IsOk()) {
		landing_logo = wxBitmap(rme_logo.ConvertToImage().Scale(140, 140, wxIMAGE_QUALITY_HIGH));
	} else if (!landing_logo.IsOk()) {
		landing_logo = LoadLandingPageBitmap(rme_logo);
		if (landing_logo.IsOk()) {
			landing_logo = wxBitmap(landing_logo.ConvertToImage().Scale(140, 140, wxIMAGE_QUALITY_HIGH));
		}
	}

	m_welcome_dialog_panel = newd WelcomeDialogPanel(this, GetClientSize(), title_text, version_text, base_colour, landing_logo, recent_files);
	Bind(WELCOME_DIALOG_DELETE_RECENT, &WelcomeDialog::OnRemoveRecentRequested, this);
}

void WelcomeDialog::OnSlotAction(wxCommandEvent& event) {
	if (event.GetId() == wxID_NEW) {
		if (event.GetString().empty()) {
			wxArrayString choices;
			choices.Add("Create a new map");
			choices.Add("Open an existing map");
			wxSingleChoiceDialog dialog(this, "What should this empty save slot do?", "Empty save slot", choices);
			if (dialog.ShowModal() != wxID_OK) {
				return;
			}

			if (dialog.GetStringSelection() == "Create a new map") {
				// Intercept and load the specific versions choose dialog directly
				wxCommandEvent trigger_event(WELCOME_DIALOG_ACTION);
				trigger_event.SetId(wxID_NEW);
				g_gui.OnWelcomeDialogAction(trigger_event);
				return;
			} else {
				wxString wildcard = g_settings.getInteger(Config::USE_OTGZ) != 0 ? "(*.otbm;*.otgz)|*.otbm;*.otgz" : "(*.otbm)|*.otbm|Compressed OpenTibia Binary Map (*.otgz)|*.otgz";
				wxFileDialog file_dialog(this, "Open map file", "", "", wildcard, wxFD_OPEN | wxFD_FILE_MUST_EXIST);
				if (file_dialog.ShowModal() != wxID_OK) {
					return;
				}
				event.SetString(file_dialog.GetPath());
				event.SetId(wxID_OPEN);
			}
		}

		if (event.GetId() == wxID_NEW) {
			// Trigger a custom Welcome Dialog action that prompts for the version exactly like GUI::OnWelcomeDialogAction does
			wxCommandEvent trigger_event(WELCOME_DIALOG_ACTION);
			trigger_event.SetId(wxID_NEW);
			g_gui.OnWelcomeDialogAction(trigger_event);
			return;
		}
	}

	wxString map_path = event.GetString();
	if (map_path.empty()) {
		wxString wildcard = g_settings.getInteger(Config::USE_OTGZ) != 0 ? "(*.otbm;*.otgz)|*.otbm;*.otgz" : "(*.otbm)|*.otbm|Compressed OpenTibia Binary Map (*.otgz)|*.otgz";
		wxFileDialog file_dialog(this, "Open map file", "", "", wildcard, wxFD_OPEN | wxFD_FILE_MUST_EXIST);
		if (file_dialog.ShowModal() != wxID_OK) {
			return;
		}
		map_path = file_dialog.GetPath();
	}

	ClientVersionID version_id = g_settings.getInteger(Config::DEFAULT_CLIENT_VERSION);
	if (version_id == CLIENT_VERSION_NONE) {
		ClientVersion* latest = ClientVersion::getLatestVersion();
		if (latest) {
			version_id = latest->getID();
		}
	}
	if (version_id != CLIENT_VERSION_NONE && version_id != g_gui.GetCurrentVersionID()) {
		wxString error;
		wxArrayString warnings;
		if (!g_gui.LoadVersion(version_id, error, warnings, true)) {
			g_gui.PopupDialog("Asset warning", error, wxOK);
			if (!warnings.empty()) {
				g_gui.ListDialog("Warnings", warnings);
			}
			PreferencesWindow preferences_window(this, true);
			preferences_window.ShowModal();
			return;
		}
	}
	g_gui.LoadMap(FileName(map_path));
}

void WelcomeDialog::OnRemoveRecentRequested(wxCommandEvent& event) {
	wxString path = event.GetString();
	MainMenuBar* menuBar = g_gui.root ? g_gui.root->menu_bar : nullptr;
	if (menuBar) {
		std::vector<wxString> recent = menuBar->GetRecentFiles();
		for (size_t i = 0; i < recent.size(); ++i) {
			if (recent[i] == path) {
				menuBar->RemoveRecentFile(i);
				break;
			}
		}
	}

	this->EndModal(wxID_CANCEL);
	wxTheApp->CallAfter([this]() {
		g_gui.ShowWelcomeDialog(wxNullBitmap);
	});
}

void WelcomeDialog::OnButtonClicked(const wxMouseEvent& event) {
	auto* button = dynamic_cast<WelcomeDialogButton*>(event.GetEventObject());
	if (!button) return;
	wxSize button_size = button->GetSize();
	wxPoint click_point = event.GetPosition();
	if (click_point.x > 0 && click_point.x < button_size.x && click_point.y > 0 && click_point.y < button_size.y) {
		if (button->GetAction() == wxID_PREFERENCES) {
			// Disable signature checking temporarily during/before we load the settings dialog (to prevent "Signatures are incorrect" on clients.xml load inside Settings dialog)
			int old_check_sigs = g_settings.getInteger(Config::CHECK_SIGNATURES);
			g_settings.setInteger(Config::CHECK_SIGNATURES, 0);

			PreferencesWindow preferences_window(this, true);
			preferences_window.ShowModal();

			// Restore the signature checking setting after the Settings window closes
			g_settings.setInteger(Config::CHECK_SIGNATURES, old_check_sigs);

			m_welcome_dialog_panel->updateInputs();
		} else {
			wxCommandEvent action_event(WELCOME_DIALOG_ACTION);
			if (button->GetAction() == wxID_OPEN) {
				wxString wildcard = g_settings.getInteger(Config::USE_OTGZ) != 0 ? "(*.otbm;*.otgz)|*.otbm;*.otgz" : "(*.otbm)|*.otbm|Compressed OpenTibia Binary Map (*.otgz)|*.otgz";
				wxFileDialog file_dialog(this, "Open map file", "", "", wildcard, wxFD_OPEN | wxFD_FILE_MUST_EXIST);
				if (file_dialog.ShowModal() == wxID_OK) {
					action_event.SetString(file_dialog.GetPath());
				} else {
					return;
				}
			}
			action_event.SetId(button->GetAction());
			ProcessWindowEvent(action_event);
		}
	}
}

void WelcomeDialog::OnCheckboxClicked(const wxCommandEvent& event) {
	g_settings.setInteger(Config::WELCOME_DIALOG, event.GetInt());
}

WelcomeDialogPanel::WelcomeDialogPanel(WelcomeDialog* dialog, const wxSize& size, const wxString& title_text, const wxString& version_text, const wxColour& base_colour, const wxBitmap& rme_logo, const std::vector<wxString>& recent_files) :
	wxPanel(dialog),
	m_rme_logo(rme_logo),
	m_title_text(title_text),
	m_version_text(version_text),
	m_text_colour(wxColor(218, 165, 32)),
	m_background_colour(base_colour) {

	SetBackgroundColour(base_colour);
	// Make the panel background and we set transparent background flags so the static bitmaps/text blend elegantly
	SetBackgroundStyle(wxBG_STYLE_PAINT);

	Bind(wxEVT_PAINT, &WelcomeDialogPanel::OnPaint, this);

	wxSizer* rootSizer = newd wxBoxSizer(wxVERTICAL);


	// Let's create the top header elements as real controls so they are automatically positioned by sizers!
	rootSizer->AddSpacer(FROM_DIP(this, 15));

	// 1. Logo
	if (m_rme_logo.IsOk()) {
		auto* logo_ctrl = newd TransparentStaticBitmap(this, wxID_ANY, m_rme_logo);
		rootSizer->Add(logo_ctrl, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, FROM_DIP(this, 10));
	}

	// 2. Title Text
	auto* title_ctrl = newd TransparentStaticText(this, wxID_ANY, m_title_text, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
	
	// Create a vintage, hand-written curly game title font
	wxFont title_font = GetFont();
	title_font.SetPointSize(24);
	title_font.SetWeight(wxFONTWEIGHT_BOLD);
	title_font.SetStyle(wxFONTSTYLE_ITALIC);
	// Use classical fantasy serif/script font families if available, otherwise fall back to system serif
	title_font.SetFaceName("Georgia");
	
	title_ctrl->SetFont(title_font);
	title_ctrl->SetForegroundColour(m_text_colour);
	rootSizer->Add(title_ctrl, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, FROM_DIP(this, 15));

	// 3. Version Text
	if (!m_version_text.empty()) {
		auto* version_ctrl = newd TransparentStaticText(this, wxID_ANY, m_version_text, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
		wxFont version_font = GetFont().Smaller();
		version_ctrl->SetFont(version_font);
		version_ctrl->SetForegroundColour(m_text_colour.ChangeLightness(110));
		rootSizer->Add(version_ctrl, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, FROM_DIP(this, 20));
	} else {
		// Just spacing
		rootSizer->AddSpacer(FROM_DIP(this, 10));
	}


	// Recent items list (Save Slots) in a nice clean sizer
	wxBoxSizer* slotsSizer = newd wxBoxSizer(wxVERTICAL);
	std::vector<wxString> slots = recent_files;
	if (slots.size() < 3) {
		slots.resize(3);
	}
	for (size_t i = 0; i < 3; ++i) {
		wxString file;
		if (i < recent_files.size()) {
			file = recent_files[i];
		}
		auto* recent_item = newd RecentItem(this, dialog, base_colour, file, static_cast<int>(i));
		slotsSizer->Add(recent_item, 0, wxEXPAND | wxBOTTOM, FROM_DIP(this, 10));
	}
	rootSizer->Add(slotsSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, FROM_DIP(this, 40));

	rootSizer->AddSpacer(FROM_DIP(this, 15));

	// Bottom bar with Settings and the Checkbox
	wxBoxSizer* bottomSizer = newd wxBoxSizer(wxHORIZONTAL);
	
	wxSize button_size = FROM_DIP(this, wxSize(64, 48)); // Width is smaller because we only want a big gear wheel icon
	wxColour button_base_colour = wxColor(32, 48, 78);
	auto* preferences_button = newd WelcomeDialogButton(this, wxDefaultPosition, button_size, button_base_colour, wxString::FromUTF8("⚙"));
	preferences_button->SetAction(wxID_PREFERENCES);
	preferences_button->Bind(wxEVT_LEFT_UP, &WelcomeDialog::OnButtonClicked, dialog);
	bottomSizer->Add(preferences_button, 0, wxALIGN_CENTER_VERTICAL);


	bottomSizer->AddSpacer(FROM_DIP(this, 30));

	m_show_welcome_dialog_checkbox = newd wxCheckBox(this, wxID_ANY, "Show this dialog on startup");
	m_show_welcome_dialog_checkbox->SetValue(g_settings.getInteger(Config::WELCOME_DIALOG) == 1);
	m_show_welcome_dialog_checkbox->Bind(wxEVT_CHECKBOX, &WelcomeDialog::OnCheckboxClicked, dialog);
	m_show_welcome_dialog_checkbox->SetForegroundColour(m_text_colour);
	bottomSizer->Add(m_show_welcome_dialog_checkbox, 0, wxALIGN_CENTER_VERTICAL);

	rootSizer->Add(bottomSizer, 0, wxALIGN_CENTER_HORIZONTAL);
	rootSizer->AddSpacer(FROM_DIP(this, 15));

	SetSizer(rootSizer);
}

void WelcomeDialogPanel::updateInputs() {
	if (m_show_welcome_dialog_checkbox) {
		m_show_welcome_dialog_checkbox->SetValue(g_settings.getInteger(Config::WELCOME_DIALOG) == 1);
	}
}

void WelcomeDialogPanel::OnPaint(const wxPaintEvent& event) {
	wxPaintDC dc(this);

	dc.SetBrush(wxBrush(m_background_colour));
	dc.SetPen(wxPen(m_background_colour));
	dc.DrawRectangle(wxRect(wxPoint(0, 0), GetClientSize()));

	// Draw golden double border like in the Ziel-UI screenshot
	dc.SetPen(wxPen(wxColor(255, 215, 0), 2));
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	
	// Outer border
	dc.DrawRectangle(wxRect(wxPoint(0, 0), GetClientSize()));
	
	// Inner thin golden highlight border
	dc.SetPen(wxPen(wxColor(180, 140, 50, 150), 1));
	dc.DrawRectangle(wxRect(wxPoint(3, 3), GetClientSize() - wxSize(6, 6)));
}




WelcomeDialogButton::WelcomeDialogButton(wxWindow* parent, const wxPoint& pos, const wxSize& size, const wxColour& base_colour, const wxString& text) :
	wxPanel(parent, wxID_ANY, pos, size),
	m_action(wxID_CLOSE),
	m_text(text),
	m_text_colour(wxColor(255, 255, 255)),
	m_background(wxColor(22, 33, 55)),
	m_background_hover(wxColor(255, 215, 0)), // Goldenes Highlight beim Hover
	m_is_hover(false) {
	Bind(wxEVT_PAINT, &WelcomeDialogButton::OnPaint, this);
	Bind(wxEVT_ENTER_WINDOW, &WelcomeDialogButton::OnMouseEnter, this);
	Bind(wxEVT_LEAVE_WINDOW, &WelcomeDialogButton::OnMouseLeave, this);
}

void WelcomeDialogButton::OnPaint(const wxPaintEvent& event) {
	wxPaintDC dc(this);

	wxColour colour = m_is_hover ? m_background_hover : m_background;
	dc.SetBrush(wxBrush(colour));
	dc.SetPen(wxPen(m_is_hover ? wxColor(255, 255, 255) : wxColor(140, 110, 40), m_is_hover ? 2 : 1));
	dc.DrawRectangle(wxRect(wxPoint(0, 0), GetClientSize()));

	// Use larger font for the gear symbol
	wxFont button_font = GetFont();
	button_font.SetPointSize(16);
	dc.SetFont(button_font);
	dc.SetTextForeground(m_is_hover ? *wxBLACK : wxColour(255, 215, 0)); // Text gold im Standard, schwarz auf gold beim Hover
	wxSize text_size = dc.GetTextExtent(m_text);
	dc.DrawText(m_text, wxPoint(GetSize().x / 2 - text_size.x / 2, GetSize().y / 2 - text_size.y / 2 - 2));
}

void WelcomeDialogButton::OnMouseEnter(const wxMouseEvent& event) {
	m_is_hover = true;
	Refresh();
}

void WelcomeDialogButton::OnMouseLeave(const wxMouseEvent& event) {
	m_is_hover = false;
	Refresh();
}

RecentItem::RecentItem(wxWindow* parent, WelcomeDialog* dialog, const wxColour& base_colour, const wxString& item_name, int slot_index) :
	wxPanel(parent, wxID_ANY),
	m_dialog(dialog),
	m_text_colour(wxColor(180, 180, 190)),
	m_text_colour_hover(wxColor(255, 215, 0)),
	m_item_text(item_name),
	m_slot_index(slot_index),
	m_is_hover(false) {
	
	SetBackgroundColour(wxColor(20, 25, 40));
	SetMinSize(FROM_DIP(this, wxSize(460, 64)));

	Bind(wxEVT_PAINT, &RecentItem::OnPaint, this);
	Bind(wxEVT_ENTER_WINDOW, &RecentItem::OnMouseEnter, this);
	Bind(wxEVT_LEAVE_WINDOW, &RecentItem::OnMouseLeave, this);
	Bind(wxEVT_LEFT_UP, &RecentItem::OnMouseClick, this);
	Bind(wxEVT_RIGHT_DOWN, &RecentItem::OnRightClick, this);
}

void RecentItem::OnPaint(const wxPaintEvent& event) {
	wxPaintDC dc(this);

	// Custom paint resembling a game slot (Golden frame, clean retro or sci-fi borders)
	wxColour bg_col = m_is_hover ? wxColour(25, 35, 60) : wxColour(13, 18, 30);
	wxColour border_col = m_is_hover ? wxColour(255, 215, 0) : wxColour(160, 130, 45);

	dc.SetBrush(wxBrush(bg_col));
	dc.SetPen(wxPen(border_col, m_is_hover ? 2 : 1));
	dc.DrawRectangle(wxRect(wxPoint(0, 0), GetClientSize()));

	// Draw slot label/number index and title
	dc.SetTextForeground(m_is_hover ? wxColour(255, 215, 0) : wxColour(218, 165, 32));
	wxFont label_font = GetFont();
	label_font.SetPointSize(12);
	label_font.SetWeight(wxFONTWEIGHT_BOLD);
	dc.SetFont(label_font);

	wxString slot_num = wxString::Format("SLOT %d:  ", m_slot_index + 1);
	wxString title_text = m_item_text.empty() ? wxString("EMPTY SLOT") : wxFileNameFromPath(m_item_text);
	
	wxSize label_size = dc.GetTextExtent(slot_num);
	dc.DrawText(slot_num, wxPoint(FROM_DIP(this, 15), GetSize().y / 2 - label_size.y / 2 - 8));

	// Make the text color stand out even more nicely on hollow slots
	if (m_item_text.empty()) {
		dc.SetTextForeground(m_is_hover ? wxColour(230, 235, 255) : wxColour(140, 150, 175));
	} else {
		dc.SetTextForeground(m_is_hover ? wxColour(255, 255, 255) : wxColour(210, 215, 230));
	}
	dc.DrawText(title_text, wxPoint(FROM_DIP(this, 15) + label_size.x, GetSize().y / 2 - label_size.y / 2 - 8));

	// Draw file path or hint text
	wxFont path_font = GetFont().Smaller();
	dc.SetFont(path_font);
	dc.SetTextForeground(m_is_hover ? wxColour(220, 200, 150) : wxColour(130, 130, 140));
	wxString sub_text = m_item_text.empty() ? wxString("Click to select an action") : m_item_text;
	wxSize sub_size = dc.GetTextExtent(sub_text);
	dc.DrawText(sub_text, wxPoint(FROM_DIP(this, 15), GetSize().y / 2 + 6));

	// Context menu tip for non-empty ones
	if (!m_item_text.empty()) {
		dc.SetTextForeground(wxColour(110, 110, 120));
		wxString hint = "Right-click to Clear";
		wxSize hint_size = dc.GetTextExtent(hint);
		dc.DrawText(hint, wxPoint(GetSize().x - hint_size.x - FROM_DIP(this, 15), GetSize().y / 2 - hint_size.y / 2));
	}
}

void RecentItem::OnMouseClick(const wxMouseEvent& event) {
	wxCommandEvent action_event(WELCOME_DIALOG_ACTION);
	if (m_item_text.empty()) {
		// prompt on empty slot
		wxArrayString choices;
		choices.Add("New Map");
		choices.Add("Load Map");
		wxSingleChoiceDialog choice_dlg(m_dialog, "What would you like to do with this Save Slot?", "Select Action", choices);
		if (choice_dlg.ShowModal() != wxID_OK) {
			return;
		}

		if (choice_dlg.GetStringSelection() == "New Map") {
			action_event.SetId(wxID_NEW);
			action_event.SetString(""); // trigger new map flow with choice
		} else {
			wxString wildcard = g_settings.getInteger(Config::USE_OTGZ) != 0 ? "(*.otbm;*.otgz)|*.otbm;*.otgz" : "(*.otbm)|*.otbm|Compressed OpenTibia Binary Map (*.otgz)|*.otgz";
			wxFileDialog file_dialog(m_dialog, "Open map file", "", "", wildcard, wxFD_OPEN | wxFD_FILE_MUST_EXIST);
			if (file_dialog.ShowModal() != wxID_OK) {
				return;
			}
			action_event.SetId(wxID_OPEN);
			action_event.SetString(file_dialog.GetPath());
		}
	} else {
		// Load the map immediately on populated slot
		action_event.SetId(wxID_OPEN);
		action_event.SetString(m_item_text);
	}
	m_dialog->GetEventHandler()->ProcessEvent(action_event);
}


void RecentItem::OnRightClick(const wxMouseEvent& event) {
	if (m_item_text.empty()) {
		return;
	}
	int answer = wxMessageBox("Remove this save slot entry from the recent list?", "Confirm", wxYES_NO | wxICON_QUESTION);
	if (answer != wxYES) {
		return;
	}
	wxCommandEvent del_event(WELCOME_DIALOG_DELETE_RECENT);
	del_event.SetString(m_item_text);
	m_dialog->GetEventHandler()->ProcessEvent(del_event);
}

void RecentItem::OnMouseEnter(const wxMouseEvent& event) {
	m_is_hover = true;
	Refresh();
}

void RecentItem::OnMouseLeave(const wxMouseEvent& event) {
	m_is_hover = false;
	Refresh();
}

