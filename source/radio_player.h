#pragma once

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/slider.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/bmpbuttn.h>
#include <wx/stattext.h>
#include <wx/dialog.h>
#include <wx/bitmap.h>
#include <vector>
#include <string>

struct RadioStation {
	std::string name;
	std::string description;
	std::string stream_url;
	std::string web_url;
};

class RadioManager {
public:
	static RadioManager& Get();

	RadioManager();
	~RadioManager();

	const std::vector<RadioStation>& GetStations() const { return stations; }
	int GetCurrentStationIndex() const { return currentStationIndex; }

	void Play(int stationIndex);
	void TogglePlay();
	void Stop();
	void SetVolume(int volumePercent); // 0 - 100
	int GetVolume() const { return volume; }
	void SetMute(bool mute);
	bool IsMuted() const { return isMuted; }
	bool IsPlaying() const { return isPlaying; }

	void OpenWebStation(int stationIndex);
	void OpenCurrentWebStation();

private:
	void InitAudioBackend();
	void CleanupAudioBackend();
	void PlayStreamUrl(const std::string& url);

	std::vector<RadioStation> stations;
	int currentStationIndex = 0;
	bool isPlaying = false;
	bool isMuted = false;
	int volume = 80;

	void* m_player = nullptr; // COM pointer storage (IWMPPlayer4)
	bool m_comInitialized = false;
};

class RadioPlayerPanel : public wxPanel {
public:
	RadioPlayerPanel(wxWindow* parent);
	virtual ~RadioPlayerPanel();

	void UpdateUI();

	void OnStationSelected(wxCommandEvent& event);
	void OnPlayClicked(wxCommandEvent& event);
	void OnStopClicked(wxCommandEvent& event);
	void OnMuteClicked(wxCommandEvent& event);
	void OnVolumeChanged(wxCommandEvent& event);
	void OnOpenWebClicked(wxCommandEvent& event);
	void OnOpacityChanged(wxCommandEvent& event);
	void OnDockToggleClicked(wxCommandEvent& event);

	static wxBitmap CreatePlayBitmap(bool is_pause);
	static wxBitmap CreateStopBitmap();
	static wxBitmap CreateMuteBitmap(bool is_muted);
	static wxBitmap CreateWebBitmap();
	static wxBitmap CreateDockBitmap(bool is_docked);

private:
	wxChoice* stationChoice = nullptr;
	wxBitmapButton* playButton = nullptr;
	wxBitmapButton* stopButton = nullptr;
	wxBitmapButton* muteButton = nullptr;
	wxSlider* volumeSlider = nullptr;
	wxSlider* opacitySlider = nullptr;
	wxStaticText* statusLabel = nullptr;
	wxButton* webButton = nullptr;
	wxBitmapButton* dockButton = nullptr;
	wxStaticText* opacityLabel = nullptr;
};

class RadioPlayerWindow : public wxDialog {
public:
	static void Toggle(wxWindow* parent);
	static void ShowDocked(bool dock);
	static bool IsDocked();
	static RadioPlayerWindow* GetInstance() { return s_instance; }
	static RadioPlayerPanel* GetPanelInstance() { return s_panelInstance; }

	RadioPlayerWindow(wxWindow* parent);
	virtual ~RadioPlayerWindow();

	void SetWindowTransparency(int percent); // 20 - 100

private:
	void OnClose(wxCloseEvent& event);

	static RadioPlayerWindow* s_instance;
	static RadioPlayerPanel* s_panelInstance;
	static RadioPlayerPanel* s_dockedPanelInstance;
	RadioPlayerPanel* playerPanel = nullptr;

	wxDECLARE_EVENT_TABLE();
};
