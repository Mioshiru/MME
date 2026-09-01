#include "radio_player.h"
#include "style_manager.h"
#include "gui.h"
#include <wx/statline.h>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <initguid.h>
#include <wmp.h>
#include <combaseapi.h>
#include <oleauto.h>
#endif

RadioManager& RadioManager::Get() {
	static RadioManager instance;
	return instance;
}

RadioManager::RadioManager() {
	// Setup available radio stations
	stations.push_back({
		"ALL",
		"The complete Rainwave playlist (All streams combined).",
		"http://allrelays.rainwave.cc/all.mp3",
		"https://rainwave.cc/all/"
	});
	stations.push_back({
		"GAME",
		"Original video game soundtracks (SNES & newer).",
		"http://allrelays.rainwave.cc/game.mp3",
		"https://rainwave.cc/game/"
	});
	stations.push_back({
		"CHIPTUNE",
		"Original and game chiptune tracks.",
		"http://allrelays.rainwave.cc/chiptune.mp3",
		"https://rainwave.cc/chiptune/"
	});
	stations.push_back({
		"OC REMIX",
		"Official OverClocked ReMix video game music tracks.",
		"http://allrelays.rainwave.cc/ocremix.mp3",
		"https://rainwave.cc/ocremix/"
	});
	stations.push_back({
		"COVERS",
		"Official and community-created game music covers.",
		"http://allrelays.rainwave.cc/covers.mp3",
		"https://rainwave.cc/covers/"
	});
	stations.push_back({
		"CHILL",
		"Calm music from games, and cozy covers.",
		"http://allrelays.rainwave.cc/chill.mp3",
		"https://rainwave.cc/chill/"
	});
	stations.push_back({
		"RPG",
		"RPGamers Radio - Epic roleplaying soundscapes & anthems.",
		"https://listen.rpgamers.net/rpgn",
		"https://www.rpgamers.net/radio/"
	});

	InitAudioBackend();
}

RadioManager::~RadioManager() {
	CleanupAudioBackend();
}

#ifdef _WIN32
static const CLSID CLSID_WMP = { 0x6BF52A52, 0x394A, 0x11D3, { 0xB1, 0x53, 0x00, 0xC0, 0x4F, 0x79, 0xFA, 0xA6 } };
static const IID IID_WMP4 = { 0x6C497D62, 0x8919, 0x413C, { 0x82, 0xDB, 0xE9, 0x35, 0xFB, 0x3E, 0xC5, 0x84 } };
#endif

void RadioManager::InitAudioBackend() {
#ifdef _WIN32
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (SUCCEEDED(hr) || hr == S_FALSE || hr == RPC_E_CHANGED_MODE) {
		m_comInitialized = true;
		IWMPPlayer4* player = nullptr;
		hr = CoCreateInstance(CLSID_WMP, NULL, CLSCTX_INPROC_SERVER, IID_WMP4, (void**)&player);
		if (SUCCEEDED(hr) && player) {
			m_player = (void*)player;
			IWMPSettings* settings = nullptr;
			if (SUCCEEDED(player->get_settings(&settings)) && settings) {
				settings->put_autoStart(VARIANT_TRUE);
				settings->put_volume(volume);
				settings->put_mute(isMuted ? VARIANT_TRUE : VARIANT_FALSE);
				settings->Release();
			}
		}
	}
#endif
}

void RadioManager::CleanupAudioBackend() {
	Stop();
#ifdef _WIN32
	if (m_player) {
		IWMPPlayer4* p = (IWMPPlayer4*)m_player;
		p->Release();
		m_player = nullptr;
	}
	if (m_comInitialized) {
		CoUninitialize();
		m_comInitialized = false;
	}
#endif
}

void RadioManager::PlayStreamUrl(const std::string& url) {
#ifdef _WIN32
	if (m_player) {
		IWMPPlayer4* p = (IWMPPlayer4*)m_player;
		BSTR bstrUrl = SysAllocString(wxString(url).wc_str());
		p->put_URL(bstrUrl);
		SysFreeString(bstrUrl);
		IWMPControls* controls = nullptr;
		if (SUCCEEDED(p->get_controls(&controls)) && controls) {
			controls->play();
			controls->Release();
		}
		isPlaying = true;
	}
#endif
}

static void NotifyRadioUI() {
	if (RadioPlayerWindow::GetPanelInstance()) {
		RadioPlayerWindow::GetPanelInstance()->UpdateUI();
	}
}

void RadioManager::Play(int stationIndex) {
	if (stationIndex < 0 || stationIndex >= (int)stations.size()) return;
	currentStationIndex = stationIndex;
	PlayStreamUrl(stations[stationIndex].stream_url);
	NotifyRadioUI();
	g_gui.SetStatusText("Radio: Playing " + wxString(stations[stationIndex].name));
}

void RadioManager::TogglePlay() {
	if (isPlaying) {
		Stop();
	} else {
		Play(currentStationIndex);
	}
}

void RadioManager::Stop() {
#ifdef _WIN32
	if (m_player) {
		IWMPPlayer4* p = (IWMPPlayer4*)m_player;
		IWMPControls* controls = nullptr;
		if (SUCCEEDED(p->get_controls(&controls)) && controls) {
			controls->stop();
			controls->Release();
		}
	}
#endif
	isPlaying = false;
	NotifyRadioUI();
	g_gui.SetStatusText("Radio: Stopped");
}

void RadioManager::SetVolume(int volumePercent) {
	volume = std::clamp(volumePercent, 0, 100);
#ifdef _WIN32
	if (m_player) {
		IWMPPlayer4* p = (IWMPPlayer4*)m_player;
		IWMPSettings* settings = nullptr;
		if (SUCCEEDED(p->get_settings(&settings)) && settings) {
			settings->put_volume(volume);
			settings->Release();
		}
	}
#endif
	NotifyRadioUI();
}

void RadioManager::SetMute(bool mute) {
	isMuted = mute;
#ifdef _WIN32
	if (m_player) {
		IWMPPlayer4* p = (IWMPPlayer4*)m_player;
		IWMPSettings* settings = nullptr;
		if (SUCCEEDED(p->get_settings(&settings)) && settings) {
			settings->put_mute(isMuted ? VARIANT_TRUE : VARIANT_FALSE);
			settings->Release();
		}
	}
#endif
	NotifyRadioUI();
}

void RadioManager::OpenWebStation(int stationIndex) {
	if (stationIndex >= 0 && stationIndex < (int)stations.size()) {
		wxLaunchDefaultBrowser(stations[stationIndex].web_url);
	}
}

void RadioManager::OpenCurrentWebStation() {
	OpenWebStation(currentStationIndex);
}

// ============================================================================
// Radio Player Window UI & Icons
// ============================================================================

#include "gui.h"
#include <wx/aui/aui.h>
#include <wx/dcmemory.h>
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/statline.h>
#include <wx/bmpbuttn.h>

RadioPlayerWindow* RadioPlayerWindow::s_instance = nullptr;
RadioPlayerPanel* RadioPlayerWindow::s_panelInstance = nullptr;
RadioPlayerPanel* RadioPlayerWindow::s_dockedPanelInstance = nullptr;

static const wxString RADIO_DOCK_PANE_NAME = "Radio_Player_Pane";

enum {
	RADIO_ID_STATION_CHOICE = wxID_HIGHEST + 1000,
	RADIO_ID_BTN_PLAY,
	RADIO_ID_BTN_STOP,
	RADIO_ID_BTN_MUTE,
	RADIO_ID_SLIDER_VOL,
	RADIO_ID_SLIDER_OPACITY,
	RADIO_ID_BTN_WEB,
	RADIO_ID_BTN_DOCK
};

// Procedurally generate high-DPI vector bitmaps for controls (Play, Pause, Stop, Volume, Mute, Dock)
wxBitmap RadioPlayerPanel::CreatePlayBitmap(bool is_pause) {
	wxBitmap bmp(22, 22);
	wxMemoryDC dc(bmp);
	dc.SetBackground(wxBrush(wxColour(20, 45, 80)));
	dc.Clear();

	dc.SetPen(*wxTRANSPARENT_PEN);
	if (is_pause) {
		dc.SetBrush(wxBrush(wxColour(240, 210, 120)));
		dc.DrawRectangle(6, 4, 3, 14);
		dc.DrawRectangle(13, 4, 3, 14);
	} else {
		dc.SetBrush(wxBrush(wxColour(120, 240, 120)));
		wxPoint pts[3] = { wxPoint(6, 4), wxPoint(6, 18), wxPoint(17, 11) };
		dc.DrawPolygon(3, pts);
	}
	dc.SelectObject(wxNullBitmap);
	return bmp;
}

wxBitmap RadioPlayerPanel::CreateStopBitmap() {
	wxBitmap bmp(22, 22);
	wxMemoryDC dc(bmp);
	dc.SetBackground(wxBrush(wxColour(20, 45, 80)));
	dc.Clear();

	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(wxBrush(wxColour(240, 100, 100)));
	dc.DrawRectangle(5, 5, 12, 12);

	dc.SelectObject(wxNullBitmap);
	return bmp;
}

wxBitmap RadioPlayerPanel::CreateMuteBitmap(bool is_muted) {
	wxBitmap bmp(22, 22);
	wxMemoryDC dc(bmp);
	dc.SetBackground(wxBrush(wxColour(20, 45, 80)));
	dc.Clear();

	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(wxBrush(is_muted ? wxColour(180, 180, 180) : wxColour(240, 210, 120)));

	// Speaker body
	dc.DrawRectangle(4, 8, 4, 6);
	wxPoint cone[3] = { wxPoint(8, 8), wxPoint(8, 14), wxPoint(13, 18) };
	wxPoint cone_top[3] = { wxPoint(8, 8), wxPoint(13, 4), wxPoint(13, 18) };
	dc.DrawPolygon(3, cone);
	dc.DrawPolygon(3, cone_top);

	// Sound waves / X
	dc.SetPen(wxPen(is_muted ? wxColour(240, 80, 80) : wxColour(240, 210, 120), 2));
	if (is_muted) {
		dc.DrawLine(15, 7, 19, 15);
		dc.DrawLine(19, 7, 15, 15);
	} else {
		dc.DrawEllipticArc(10, 6, 8, 10, -50, 50);
	}

	dc.SelectObject(wxNullBitmap);
	return bmp;
}

wxBitmap RadioPlayerPanel::CreateDockBitmap(bool is_docked) {
	wxBitmap bmp(22, 22);
	wxMemoryDC dc(bmp);
	dc.SetBackground(wxBrush(wxColour(20, 45, 80)));
	dc.Clear();

	dc.SetPen(wxPen(wxColour(240, 210, 120), 1));
	dc.SetBrush(wxBrush(wxColour(30, 60, 100)));
	dc.DrawRectangle(3, 3, 16, 16);

	if (is_docked) {
		// Icon showing window popping out
		dc.SetPen(wxPen(wxColour(120, 220, 255), 2));
		dc.DrawLine(10, 6, 15, 6);
		dc.DrawLine(15, 6, 15, 11);
		dc.DrawLine(9, 12, 15, 6);
	} else {
		// Icon showing docking into sidebar
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.SetBrush(wxBrush(wxColour(120, 220, 255)));
		dc.DrawRectangle(4, 4, 5, 14);
	}

	dc.SelectObject(wxNullBitmap);
	return bmp;
}

RadioPlayerPanel::RadioPlayerPanel(wxWindow* parent)
	: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize) {

	SetBackgroundColour(wxColour(18, 36, 62));
	SetForegroundColour(wxColour(240, 245, 255));

	wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);

	// Status & Dock Header Bar
	wxPanel* headerPanel = new wxPanel(this, wxID_ANY);
	headerPanel->SetBackgroundColour(wxColour(12, 24, 42));
	wxBoxSizer* headerSizer = new wxBoxSizer(wxHORIZONTAL);

	statusLabel = new wxStaticText(headerPanel, wxID_ANY, "● Ready");
	statusLabel->SetForegroundColour(wxColour(120, 220, 120));
	wxFont statusFont = statusLabel->GetFont();
	statusFont.SetWeight(wxFONTWEIGHT_BOLD);
	statusLabel->SetFont(statusFont);
	headerSizer->Add(statusLabel, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);

	webButton = new wxButton(headerPanel, RADIO_ID_BTN_WEB, "Web Radio", wxDefaultPosition, wxSize(76, 22));
	webButton->SetToolTip("Open Web Voting & Station in Browser");
	webButton->SetBackgroundColour(wxColour(24, 48, 80));
	webButton->SetForegroundColour(wxColour(240, 210, 120));
	headerSizer->Add(webButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

	dockButton = new wxBitmapButton(headerPanel, RADIO_ID_BTN_DOCK, CreateDockBitmap(RadioPlayerWindow::IsDocked()), wxDefaultPosition, wxSize(24, 22));
	dockButton->SetToolTip(RadioPlayerWindow::IsDocked() ? "Float / Undock window" : "Dock into editor panels");
	dockButton->SetBackgroundColour(wxColour(20, 45, 80));
	headerSizer->Add(dockButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);

	headerPanel->SetSizer(headerSizer);
	topSizer->Add(headerPanel, 0, wxEXPAND);

	// Middle Controls Row
	wxBoxSizer* ctrlRow = new wxBoxSizer(wxHORIZONTAL);

	// Station dropdown
	const auto& stations = RadioManager::Get().GetStations();
	wxArrayString stationChoices;
	for (const auto& s : stations) {
		stationChoices.Add(s.name);
	}
	stationChoice = new wxChoice(this, RADIO_ID_STATION_CHOICE, wxDefaultPosition, wxSize(100, -1), stationChoices);
	stationChoice->SetSelection(RadioManager::Get().GetCurrentStationIndex());
	stationChoice->SetToolTip("Select Radio Station");
	stationChoice->SetBackgroundColour(wxColour(15, 32, 56));
	stationChoice->SetForegroundColour(wxColour(255, 255, 255));
	ctrlRow->Add(stationChoice, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 6);

	// Divider
	wxStaticLine* sep = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxSize(2, 24), wxLI_VERTICAL);
	ctrlRow->Add(sep, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);

	// Media buttons
	playButton = new wxBitmapButton(this, RADIO_ID_BTN_PLAY, CreatePlayBitmap(RadioManager::Get().IsPlaying()), wxDefaultPosition, wxSize(30, 26));
	playButton->SetToolTip("Play / Pause");
	playButton->SetBackgroundColour(wxColour(20, 45, 80));
	ctrlRow->Add(playButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

	stopButton = new wxBitmapButton(this, RADIO_ID_BTN_STOP, CreateStopBitmap(), wxDefaultPosition, wxSize(30, 26));
	stopButton->SetToolTip("Stop");
	stopButton->SetBackgroundColour(wxColour(20, 45, 80));
	ctrlRow->Add(stopButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

	muteButton = new wxBitmapButton(this, RADIO_ID_BTN_MUTE, CreateMuteBitmap(RadioManager::Get().IsMuted()), wxDefaultPosition, wxSize(30, 26));
	muteButton->SetToolTip("Mute / Unmute");
	muteButton->SetBackgroundColour(wxColour(20, 45, 80));
	ctrlRow->Add(muteButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);

	// Volume slider
	volumeSlider = new wxSlider(this, RADIO_ID_SLIDER_VOL, RadioManager::Get().GetVolume(), 0, 100, wxDefaultPosition, wxSize(75, -1));
	volumeSlider->SetToolTip("Volume");
	volumeSlider->SetBackgroundColour(wxColour(18, 36, 62));
	ctrlRow->Add(volumeSlider, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	topSizer->Add(ctrlRow, 0, wxEXPAND | wxTOP | wxBOTTOM, 6);

	// Bottom Row: Transparency Slider
	wxBoxSizer* transRow = new wxBoxSizer(wxHORIZONTAL);
	opacityLabel = new wxStaticText(this, wxID_ANY, "Opacity:");
	opacityLabel->SetForegroundColour(wxColour(170, 195, 225));
	wxFont smallFont = opacityLabel->GetFont();
	smallFont.SetPointSize(8);
	opacityLabel->SetFont(smallFont);
	transRow->Add(opacityLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);

	opacitySlider = new wxSlider(this, RADIO_ID_SLIDER_OPACITY, 100, 25, 100, wxDefaultPosition, wxSize(90, -1));
	opacitySlider->SetToolTip("Window Opacity / Transparency (25% - 100%)");
	opacitySlider->SetBackgroundColour(wxColour(18, 36, 62));
	transRow->Add(opacitySlider, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 6);

	topSizer->Add(transRow, 0, wxEXPAND | wxBOTTOM, 4);

	SetSizer(topSizer);

	// Event Bindings
	stationChoice->Bind(wxEVT_CHOICE, &RadioPlayerPanel::OnStationSelected, this);
	playButton->Bind(wxEVT_BUTTON, &RadioPlayerPanel::OnPlayClicked, this);
	stopButton->Bind(wxEVT_BUTTON, &RadioPlayerPanel::OnStopClicked, this);
	muteButton->Bind(wxEVT_BUTTON, &RadioPlayerPanel::OnMuteClicked, this);
	volumeSlider->Bind(wxEVT_SLIDER, &RadioPlayerPanel::OnVolumeChanged, this);
	webButton->Bind(wxEVT_BUTTON, &RadioPlayerPanel::OnOpenWebClicked, this);
	opacitySlider->Bind(wxEVT_SLIDER, &RadioPlayerPanel::OnOpacityChanged, this);
	dockButton->Bind(wxEVT_BUTTON, &RadioPlayerPanel::OnDockToggleClicked, this);

	UpdateUI();
}

RadioPlayerPanel::~RadioPlayerPanel() {
}

void RadioPlayerPanel::UpdateUI() {
	RadioManager& rm = RadioManager::Get();
	int curIdx = rm.GetCurrentStationIndex();

	if (stationChoice && stationChoice->GetSelection() != curIdx) {
		stationChoice->SetSelection(curIdx);
	}

	if (playButton) {
		playButton->SetBitmap(CreatePlayBitmap(rm.IsPlaying()));
	}

	if (muteButton) {
		muteButton->SetBitmap(CreateMuteBitmap(rm.IsMuted()));
	}

	if (statusLabel) {
		if (rm.IsPlaying()) {
			const auto& stations = rm.GetStations();
			statusLabel->SetLabel("Playing: " + stations[curIdx].name);
			statusLabel->SetForegroundColour(wxColour(120, 240, 120));
		} else {
			statusLabel->SetLabel("Stopped");
			statusLabel->SetForegroundColour(wxColour(180, 180, 180));
		}
	}

	if (dockButton) {
		dockButton->SetBitmap(CreateDockBitmap(RadioPlayerWindow::IsDocked()));
		dockButton->SetToolTip(RadioPlayerWindow::IsDocked() ? "Float / Undock window" : "Dock into editor panels");
	}
}

void RadioPlayerPanel::OnStationSelected(wxCommandEvent& WXUNUSED(event)) {
	int sel = stationChoice->GetSelection();
	if (sel != wxNOT_FOUND) {
		RadioManager::Get().Play(sel);
	}
}

void RadioPlayerPanel::OnPlayClicked(wxCommandEvent& WXUNUSED(event)) {
	RadioManager::Get().TogglePlay();
}

void RadioPlayerPanel::OnStopClicked(wxCommandEvent& WXUNUSED(event)) {
	RadioManager::Get().Stop();
}

void RadioPlayerPanel::OnMuteClicked(wxCommandEvent& WXUNUSED(event)) {
	RadioManager::Get().SetMute(!RadioManager::Get().IsMuted());
}

void RadioPlayerPanel::OnVolumeChanged(wxCommandEvent& WXUNUSED(event)) {
	RadioManager::Get().SetVolume(volumeSlider->GetValue());
}

void RadioPlayerPanel::OnOpenWebClicked(wxCommandEvent& WXUNUSED(event)) {
	RadioManager::Get().OpenCurrentWebStation();
}

void RadioPlayerPanel::OnOpacityChanged(wxCommandEvent& WXUNUSED(event)) {
	if (RadioPlayerWindow::GetInstance()) {
		RadioPlayerWindow::GetInstance()->SetWindowTransparency(opacitySlider->GetValue());
	}
}

void RadioPlayerPanel::OnDockToggleClicked(wxCommandEvent& WXUNUSED(event)) {
	RadioPlayerWindow::ShowDocked(!RadioPlayerWindow::IsDocked());
}

// ============================================================================
// RadioPlayerWindow Container Dialog
// ============================================================================

wxBEGIN_EVENT_TABLE(RadioPlayerWindow, wxDialog)
	EVT_CLOSE(RadioPlayerWindow::OnClose)
wxEND_EVENT_TABLE()

bool RadioPlayerWindow::IsDocked() {
	if (!g_gui.GetAuiManager()) return false;
	wxAuiPaneInfo& pane = g_gui.GetAuiManager()->GetPane(RADIO_DOCK_PANE_NAME);
	return pane.IsOk() && pane.IsShown();
}

void RadioPlayerWindow::ShowDocked(bool dock) {
	wxAuiManager* aui = g_gui.GetAuiManager();
	if (!aui) return;

	static bool s_paneCloseBound = false;
	if (!s_paneCloseBound) {
		aui->Bind(wxEVT_AUI_PANE_CLOSE, [](wxAuiManagerEvent& evt) {
			if (evt.GetPane() && evt.GetPane()->name == RADIO_DOCK_PANE_NAME) {
				RadioManager::Get().Stop();
			}
			evt.Skip();
		});
		s_paneCloseBound = true;
	}

	if (!s_dockedPanelInstance) {
		s_dockedPanelInstance = new RadioPlayerPanel(g_gui.root);
		aui->AddPane(s_dockedPanelInstance, wxAuiPaneInfo()
			.Name(RADIO_DOCK_PANE_NAME)
			.Caption("Radio Player")
			.Right()
			.Layer(1)
			.Position(1)
			.CloseButton(true)
			.Floatable(true)
			.Dockable(true)
			.LeftDockable(true)
			.RightDockable(true)
			.TopDockable(true)
			.BottomDockable(true)
			.BestSize(320, 110)
			.MinSize(wxSize(260, 90))
			.Show(true));
	}

	wxAuiPaneInfo& pane = aui->GetPane(s_dockedPanelInstance);
	if (dock) {
		pane.Show(true);
		if (s_instance) {
			s_instance->Hide();
		}
	} else {
		// Float within AUI
		pane.Show(true).Float();
	}
	aui->Update();
	s_dockedPanelInstance->UpdateUI();
}

void RadioPlayerWindow::Toggle(wxWindow* parent) {
	wxAuiManager* aui = g_gui.GetAuiManager();
	if (!aui) return;

	static bool s_paneCloseBound = false;
	if (!s_paneCloseBound) {
		aui->Bind(wxEVT_AUI_PANE_CLOSE, [](wxAuiManagerEvent& evt) {
			if (evt.GetPane() && evt.GetPane()->name == RADIO_DOCK_PANE_NAME) {
				RadioManager::Get().Stop();
			}
			evt.Skip();
		});
		s_paneCloseBound = true;
	}

	if (!s_dockedPanelInstance) {
		s_dockedPanelInstance = new RadioPlayerPanel(g_gui.root);
		aui->AddPane(s_dockedPanelInstance, wxAuiPaneInfo()
			.Name(RADIO_DOCK_PANE_NAME)
			.Caption("Radio Player")
			.Right()
			.Layer(1)
			.Position(1)
			.CloseButton(true)
			.Floatable(true)
			.Dockable(true)
			.LeftDockable(true)
			.RightDockable(true)
			.TopDockable(true)
			.BottomDockable(true)
			.BestSize(320, 110)
			.MinSize(wxSize(260, 90))
			.Show(true));
		aui->Update();
		s_dockedPanelInstance->UpdateUI();
		return;
	}

	wxAuiPaneInfo& pane = aui->GetPane(s_dockedPanelInstance);
	if (pane.IsShown()) {
		RadioManager::Get().Stop();
		pane.Show(false);
	} else {
		pane.Show(true);
	}
	aui->Update();
	s_dockedPanelInstance->UpdateUI();
}

RadioPlayerWindow::RadioPlayerWindow(wxWindow* parent)
	: wxDialog(parent, wxID_ANY, "Radio Player", wxDefaultPosition, wxSize(360, 130),
	           wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxSTAY_ON_TOP) {

	SetBackgroundColour(wxColour(18, 36, 62));
	SetForegroundColour(wxColour(240, 245, 255));

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	playerPanel = new RadioPlayerPanel(this);
	s_panelInstance = playerPanel;
	sizer->Add(playerPanel, 1, wxEXPAND);
	SetSizer(sizer);

	SetWindowTransparency(100);
}

RadioPlayerWindow::~RadioPlayerWindow() {
	s_instance = nullptr;
	s_panelInstance = nullptr;
}

void RadioPlayerWindow::SetWindowTransparency(int percent) {
#ifdef _WIN32
	HWND hwnd = (HWND)GetHWND();
	if (hwnd) {
		LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
		if (!(exStyle & WS_EX_LAYERED)) {
			SetWindowLong(hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);
		}
		BYTE alpha = (BYTE)((std::clamp(percent, 20, 100) * 255) / 100);
		SetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);
	}
#endif
}

void RadioPlayerWindow::OnClose(wxCloseEvent& WXUNUSED(event)) {
	RadioManager::Get().Stop();
	Hide();
}
