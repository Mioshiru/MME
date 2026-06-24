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

WelcomeDialog::WelcomeDialog(const wxString& title_text, const wxString& version_text, const wxSize& size, const wxBitmap& rme_logo, const std::vector<wxString>& recent_files) :
	wxDialog(nullptr, wxID_ANY, "", wxDefaultPosition, size) {
	Centre();
	wxColour base_colour = wxColor(10, 15, 25); // Tiefes Dunkelblau/Schwarz
	m_welcome_dialog_panel = newd WelcomeDialogPanel(this, GetClientSize(), title_text, version_text, base_colour, wxBitmap(rme_logo.ConvertToImage().Scale(FROM_DIP(this, 48), FROM_DIP(this, 48))), recent_files);
	Bind(WELCOME_DIALOG_DELETE_RECENT, &WelcomeDialog::OnRemoveRecentRequested, this);
}

void WelcomeDialog::OnRemoveRecentRequested(wxCommandEvent& event) {
    wxString path = event.GetString();
    // Logik zum Entfernen aus der Registry/Config via MainMenuBar
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
    
    // Schließe den aktuellen Dialog sauber
    this->EndModal(wxID_CANCEL);
    
    // Starte die Landingpage in der nächsten Event-Loop-Iteration neu
    wxTheApp->CallAfter([]() {
        g_gui.ShowWelcomeDialog(wxNullBitmap);
    });
}

void WelcomeDialog::OnButtonClicked(const wxMouseEvent& event) {
	auto* button = dynamic_cast<WelcomeDialogButton*>(event.GetEventObject());
	wxSize button_size = button->GetSize();
	wxPoint click_point = event.GetPosition();
	if (click_point.x > 0 && click_point.x < button_size.x && click_point.y > 0 && click_point.y < button_size.x) {
		if (button->GetAction() == wxID_PREFERENCES) {
			PreferencesWindow preferences_window(m_welcome_dialog_panel, true);
			preferences_window.ShowModal();
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

void WelcomeDialog::OnRecentItemClicked(const wxMouseEvent& event) {
	auto* recent_item = dynamic_cast<RecentItem*>(event.GetEventObject());
	wxSize button_size = recent_item->GetSize();
	wxPoint click_point = event.GetPosition();
	if (click_point.x > 0 && click_point.x < button_size.x && click_point.y > 0 && click_point.y < button_size.x) {
		wxCommandEvent action_event(WELCOME_DIALOG_ACTION);
		action_event.SetString(recent_item->GetText());
		action_event.SetId(wxID_OPEN);
		ProcessWindowEvent(action_event);
	}
}

WelcomeDialogPanel::WelcomeDialogPanel(WelcomeDialog* dialog, const wxSize& size, const wxString& title_text, const wxString& version_text, const wxColour& base_colour, const wxBitmap& rme_logo, const std::vector<wxString>& recent_files) :
	wxPanel(dialog),
	m_rme_logo(rme_logo),
	m_title_text(title_text),
	m_version_text(version_text),
	m_text_colour(wxColor(218, 165, 32)), // Edles RPG-Gold
	m_background_colour(base_colour) {

	auto* recent_maps_panel = newd RecentMapsPanel(this, dialog, base_colour, recent_files);
	recent_maps_panel->SetBackgroundColour(wxColor(10, 15, 25)); // Dunkler Listen-Hintergrund

	wxSize button_size = FROM_DIP(this, wxSize(150, 35));
	wxColour button_base_colour = wxColor(30, 45, 70);

	int button_pos_center_x = size.x / 4 - button_size.x / 2;
	int button_pos_center_y = size.y / 2;

	wxPoint newMapButtonPoint(button_pos_center_x, button_pos_center_y);
	auto* new_map_button = newd WelcomeDialogButton(this, wxDefaultPosition, button_size, button_base_colour, "New");
	new_map_button->SetAction(wxID_NEW);
	new_map_button->Bind(wxEVT_LEFT_UP, &WelcomeDialog::OnButtonClicked, dialog);

	auto* open_map_button = newd WelcomeDialogButton(this, wxDefaultPosition, button_size, button_base_colour, "Open");
	open_map_button->SetAction(wxID_OPEN);
	open_map_button->Bind(wxEVT_LEFT_UP, &WelcomeDialog::OnButtonClicked, dialog);
	auto* preferences_button = newd WelcomeDialogButton(this, wxDefaultPosition, button_size, button_base_colour, "Settings");
	preferences_button->SetAction(wxID_PREFERENCES);
	preferences_button->Bind(wxEVT_LEFT_UP, &WelcomeDialog::OnButtonClicked, dialog);

	Bind(wxEVT_PAINT, &WelcomeDialogPanel::OnPaint, this);

	wxClientDC dc(this);
	wxFont font = GetFont();
	font.SetPointSize(20);
	font.SetWeight(wxFONTWEIGHT_BOLD);
	dc.SetFont(font);
	wxSize header_size = dc.GetTextExtent(m_title_text);
	int title_left_x = (size.x / 4) - (header_size.x / 2);

	wxSizer* rootSizer = newd wxBoxSizer(wxHORIZONTAL);
	wxSizer* buttons_sizer = newd wxBoxSizer(wxVERTICAL);
	buttons_sizer->AddSpacer(size.y / 2);
	buttons_sizer->Add(new_map_button, 0, wxALIGN_CENTER | wxTOP, FROM_DIP(this, 10));
	buttons_sizer->Add(open_map_button, 0, wxALIGN_CENTER | wxTOP, FROM_DIP(this, 10));
	buttons_sizer->Add(preferences_button, 0, wxALIGN_CENTER | wxTOP, FROM_DIP(this, 10));

	wxSizer* vertical_sizer = newd wxBoxSizer(wxVERTICAL);
	wxSizer* horizontal_sizer = newd wxBoxSizer(wxHORIZONTAL);

	m_show_welcome_dialog_checkbox = newd wxCheckBox(this, wxID_ANY, "Show this dialog on startup");
	m_show_welcome_dialog_checkbox->SetValue(g_settings.getInteger(Config::WELCOME_DIALOG) == 1);
	m_show_welcome_dialog_checkbox->Bind(wxEVT_CHECKBOX, &WelcomeDialog::OnCheckboxClicked, dialog);
	m_show_welcome_dialog_checkbox->SetForegroundColour(m_text_colour);
	horizontal_sizer->AddSpacer(button_pos_center_x);
	horizontal_sizer->Add(m_show_welcome_dialog_checkbox, 0, wxALIGN_CENTER_VERTICAL);
	vertical_sizer->Add(buttons_sizer, 0, wxEXPAND | wxCENTER);
	vertical_sizer->Add(horizontal_sizer, 1, wxEXPAND);

	rootSizer->Add(vertical_sizer, 1, wxEXPAND);
	rootSizer->Add(recent_maps_panel, 1, wxEXPAND | wxALL, FROM_DIP(this, 15));
	SetSizer(rootSizer);
}

void WelcomeDialogPanel::updateInputs() {
	m_show_welcome_dialog_checkbox->SetValue(g_settings.getInteger(Config::WELCOME_DIALOG) == 1);
}

void WelcomeDialogPanel::OnPaint(const wxPaintEvent& event) {
	wxPaintDC dc(this);

	dc.SetBrush(wxBrush(m_background_colour));
	dc.SetPen(wxPen(m_background_colour));
	dc.DrawRectangle(wxRect(wxPoint(0, 0), GetClientSize()));

    // Goldener Rahmen
    dc.SetPen(wxPen(wxColor(180, 140, 50, 180), 2));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(GetClientSize());

	dc.DrawBitmap(m_rme_logo, wxPoint(GetSize().x / 4 - m_rme_logo.GetWidth() / 2, FROM_DIP(this, 50)), true);

	wxFont font = GetFont();
	font.SetPointSize(20);
    font.SetWeight(wxFONTWEIGHT_BOLD);
	dc.SetFont(font);
	wxSize header_size = dc.GetTextExtent(m_title_text);
	wxSize header_point(GetSize().x / 4, GetSize().y / 4);
	dc.SetTextForeground(m_text_colour);
	dc.DrawText(m_title_text, wxPoint(header_point.x - header_size.x / 2, header_point.y));

	dc.SetFont(GetFont());
	wxSize version_size = dc.GetTextExtent(m_version_text);
	dc.SetTextForeground(m_text_colour.ChangeLightness(110));
	dc.DrawText(m_version_text, wxPoint(header_point.x - version_size.x / 2, header_point.y + header_size.y + 10));
}

WelcomeDialogButton::WelcomeDialogButton(wxWindow* parent, const wxPoint& pos, const wxSize& size, const wxColour& base_colour, const wxString& text) :
	wxPanel(parent, wxID_ANY, pos, size),
	m_action(wxID_CLOSE),
	m_text(text),
	m_text_colour(wxColor(255, 255, 255)),
	m_background(wxColor(35, 45, 70)),
	m_background_hover(wxColor(180, 140, 50)), // Goldenes Highlight beim Hover
	m_is_hover(false) {
	Bind(wxEVT_PAINT, &WelcomeDialogButton::OnPaint, this);
	Bind(wxEVT_ENTER_WINDOW, &WelcomeDialogButton::OnMouseEnter, this);
	Bind(wxEVT_LEAVE_WINDOW, &WelcomeDialogButton::OnMouseLeave, this);
}

void WelcomeDialogButton::OnPaint(const wxPaintEvent& event) {
	wxPaintDC dc(this);

	wxColour colour = m_is_hover ? m_background_hover : m_background;
	dc.SetBrush(wxBrush(colour));
	dc.SetPen(wxPen(m_is_hover ? wxColor(255, 255, 200) : wxColor(150, 120, 40), 1));
	dc.DrawRectangle(wxRect(wxPoint(0, 0), GetClientSize()));

	dc.SetFont(GetFont());
	dc.SetTextForeground(m_is_hover ? *wxBLACK : m_text_colour); // Text schwarz auf gold beim Hover
	wxSize text_size = dc.GetTextExtent(m_text);
	dc.DrawText(m_text, wxPoint(GetSize().x / 2 - text_size.x / 2, GetSize().y / 2 - text_size.y / 2));
}

void WelcomeDialogButton::OnMouseEnter(const wxMouseEvent& event) {
	m_is_hover = true;
	Refresh();
}

void WelcomeDialogButton::OnMouseLeave(const wxMouseEvent& event) {
	m_is_hover = false;
	Refresh();
}

RecentMapsPanel::RecentMapsPanel(wxWindow* parent, WelcomeDialog* dialog, const wxColour& base_colour, const std::vector<wxString>& recent_files) :
	wxPanel(parent, wxID_ANY) {
	wxBoxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
	sizer->AddStretchSpacer(1);
	for (const wxString& file : recent_files) {
		auto* recent_item = newd RecentItem(this, base_colour, file);
		sizer->Add(recent_item, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
		recent_item->Bind(wxEVT_LEFT_UP, &WelcomeDialog::OnRecentItemClicked, dialog);
	}
	sizer->AddStretchSpacer(1);
	SetSizer(sizer);
}

RecentItem::RecentItem(wxWindow* parent, const wxColour& base_colour, const wxString& item_name) :
	wxPanel(parent, wxID_ANY),
	m_text_colour(wxColor(180, 180, 190)),
	m_text_colour_hover(wxColor(255, 215, 0)),
	m_item_text(item_name) {
	SetBackgroundColour(wxColor(20, 25, 40)); // Dunkler Slot-Hintergrund
	m_title = newd wxStaticText(this, wxID_ANY, wxFileNameFromPath(m_item_text));
	m_title->SetFont(GetFont().Bold());
	m_title->SetForegroundColour(m_text_colour);
	m_title->SetToolTip(m_item_text);
	m_file_path = newd wxStaticText(this, wxID_ANY, m_item_text, wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_START);
	m_file_path->SetToolTip(m_item_text);
	m_file_path->SetFont(GetFont().Smaller());
	m_file_path->SetForegroundColour(m_text_colour);

    m_delete_button = newd wxPanel(this, wxID_ANY, wxDefaultPosition, FROM_DIP(this, wxSize(20, 20)));
    m_delete_button->SetBackgroundColour(wxColor(80, 20, 20));
    m_delete_button->SetToolTip("Entfernen");
    
    // Zeichne ein kleines 'X' auf den Löschen-Button
    m_delete_button->Bind(wxEVT_PAINT, [this](wxPaintEvent& evt){
        wxPaintDC dc(m_delete_button);
        dc.SetPen(wxPen(wxColor(200, 50, 50), 2));
        dc.DrawLine(4, 4, 16, 16);
        dc.DrawLine(16, 4, 4, 16);
    });

	wxBoxSizer* mainSizer = newd wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
	sizer->Add(m_title);
	sizer->Add(m_file_path, 0, wxTOP, 2);
	mainSizer->Add(sizer, 1, wxEXPAND | wxALL, 10);
    mainSizer->Add(m_delete_button, 0, wxALIGN_CENTER | wxRIGHT, 15);

	Bind(wxEVT_ENTER_WINDOW, &RecentItem::OnMouseEnter, this);
	Bind(wxEVT_LEAVE_WINDOW, &RecentItem::OnMouseLeave, this);
	m_title->Bind(wxEVT_LEFT_UP, &RecentItem::PropagateItemClicked, this);
	m_file_path->Bind(wxEVT_LEFT_UP, &RecentItem::PropagateItemClicked, this);
    m_delete_button->Bind(wxEVT_LEFT_UP, &RecentItem::OnDeleteClicked, this);

	SetSizerAndFit(mainSizer);
}

void RecentItem::OnDeleteClicked(const wxMouseEvent& event) {
    wxCommandEvent del_event(WELCOME_DIALOG_DELETE_RECENT);
    del_event.SetString(m_item_text);
    if (g_gui.welcomeDialog) {
        g_gui.welcomeDialog->GetEventHandler()->ProcessEvent(del_event);
    }
}
void RecentItem::PropagateItemClicked(wxMouseEvent& event) {
	event.ResumePropagation(1);
	event.SetEventObject(this);
	event.Skip();
}

void RecentItem::OnMouseEnter(const wxMouseEvent& event) {
	if (GetScreenRect().Contains(ClientToScreen(event.GetPosition()))
		&& m_title->GetForegroundColour() != m_text_colour_hover) {
		m_title->SetForegroundColour(m_text_colour_hover);
		m_file_path->SetForegroundColour(m_text_colour_hover);
		m_title->Refresh();
		m_file_path->Refresh();
	}
}

void RecentItem::OnMouseLeave(const wxMouseEvent& event) {
	if (!GetScreenRect().Contains(ClientToScreen(event.GetPosition()))
		&& m_title->GetForegroundColour() != m_text_colour) {
		m_title->SetForegroundColour(m_text_colour);
		m_file_path->SetForegroundColour(m_text_colour);
		m_title->Refresh();
		m_file_path->Refresh();
	}
}
