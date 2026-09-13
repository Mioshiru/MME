#include "radio_player.h"
#include "gui.h"
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
		"Rivendell",
		"Radio Rivendell - Fantasy music 24/7 (Orchestral, Neofolk & Ambient).",
		"https://play.radiorivendell.com/radio/8000/radio.mp3",
		"https://radiorivendell.com/"
	});
	stations.push_back({
		"The Green Dragon",
		"Radio Rivendell - The Green Dragon (Tavern & Folk ambience).",
		"https://play.radiorivendell.com/radio/8010/radio.mp3",
		"https://radiorivendell.com/"
	});
	stations.push_back({
		"Lorien",
		"Radio Rivendell - Lorien (Atmospheric & Elven soundscapes).",
		"https://play.radiorivendell.com/radio/8020/radio.mp3",
		"https://radiorivendell.com/"
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

void RadioManager::Play(int stationIndex) {
	if (stationIndex < 0 || stationIndex >= (int)stations.size()) return;
	currentStationIndex = stationIndex;
	PlayStreamUrl(stations[stationIndex].stream_url);
	g_gui.SetStatusText("Radio: Playing " + wxString(stations[stationIndex].name));
	g_gui.RefreshView();
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
	g_gui.SetStatusText("Radio: Stopped");
	g_gui.RefreshView();
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
	g_gui.RefreshView();
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
	g_gui.RefreshView();
}

void RadioManager::OpenWebStation(int stationIndex) {
	if (stationIndex >= 0 && stationIndex < (int)stations.size()) {
		wxLaunchDefaultBrowser(stations[stationIndex].web_url);
	}
}

void RadioManager::OpenCurrentWebStation() {
	OpenWebStation(currentStationIndex);
}
