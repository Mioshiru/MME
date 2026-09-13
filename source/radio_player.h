#pragma once

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
