#ifndef RME_WORLD_MAP_MARKERS_H
#define RME_WORLD_MAP_MARKERS_H

#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include "position.h"
#include <wx/filefn.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>

struct WorldMapMarker {
    int id = 0;
    std::string title;
    std::string description;
    int x = 0;
    int y = 0;
    int z = 7;
    int category = 0; // 0: 🚩 Waypoint, 1: 🏰 Town/City, 2: ⚔️ Quest/Combat, 3: 💀 Dungeon, 4: 💎 Treasure, 5: 🏠 Base/House, 6: ⭐ Special/POI, 7: ⚠️ Danger
    uint32_t color = 0xFFE5C158; // Default corporate gold ARGB
};

class WorldMapMarkerManager {
public:
    static WorldMapMarkerManager& GetInstance() {
        static WorldMapMarkerManager instance;
        return instance;
    }

    std::vector<WorldMapMarker>& GetMarkers() {
        return markers;
    }

    const std::vector<WorldMapMarker>& GetMarkers() const {
        return markers;
    }

    void AddMarker(const WorldMapMarker& marker) {
        WorldMapMarker m = marker;
        if (m.id <= 0) {
            m.id = next_id++;
        } else {
            next_id = std::max(next_id, m.id + 1);
        }
        markers.push_back(m);
        SaveToFile();
    }

    bool RemoveMarker(int id) {
        auto it = std::remove_if(markers.begin(), markers.end(), [id](const WorldMapMarker& m) {
            return m.id == id;
        });
        if (it != markers.end()) {
            markers.erase(it, markers.end());
            SaveToFile();
            return true;
        }
        return false;
    }

    void UpdateMarker(const WorldMapMarker& updated) {
        for (auto& m : markers) {
            if (m.id == updated.id) {
                m = updated;
                SaveToFile();
                return;
            }
        }
    }

    void ClearAllMarkers() {
        markers.clear();
        SaveToFile();
    }

    void SetCurrentMapPath(const std::string& path) {
        current_map_path = path;
        LoadFromFile();
    }

    void SaveToFile() {
        std::string filePath = GetMarkerFilePath();
        if (filePath.empty()) return;

        std::ofstream out(filePath);
        if (!out.is_open()) return;

        out << markers.size() << "\n";
        for (const auto& m : markers) {
            out << m.id << "\n";
            out << m.x << " " << m.y << " " << m.z << " " << m.category << " " << m.color << "\n";
            std::string t = m.title;
            std::replace(t.begin(), t.end(), '\n', ' ');
            out << (t.empty() ? "Marker" : t) << "\n";
            std::string d = m.description;
            std::replace(d.begin(), d.end(), '\n', ' ');
            out << (d.empty() ? "-" : d) << "\n";
        }
    }

    void LoadFromFile() {
        markers.clear();
        std::string filePath = GetMarkerFilePath();
        if (filePath.empty() || !wxFileExists(filePath)) return;

        std::ifstream in(filePath);
        if (!in.is_open()) return;

        size_t count = 0;
        if (!(in >> count)) return;

        for (size_t i = 0; i < count; ++i) {
            WorldMapMarker m;
            if (!(in >> m.id >> m.x >> m.y >> m.z >> m.category >> m.color)) break;
            std::string dummy;
            std::getline(in, dummy); // consume newline
            std::getline(in, m.title);
            std::getline(in, m.description);
            if (m.description == "-") m.description.clear();

            next_id = std::max(next_id, m.id + 1);
            markers.push_back(m);
        }
    }

private:
    WorldMapMarkerManager() {
        LoadFromFile();
    }

    std::string GetMarkerFilePath() const {
        if (!current_map_path.empty()) {
            wxFileName fn(current_map_path);
            fn.SetExt("markers.dat");
            return fn.GetFullPath().ToStdString();
        }
        wxString configDir = wxStandardPaths::Get().GetUserConfigDir() + "/.rme";
        if (!wxDirExists(configDir)) wxMkdir(configDir);
        return (configDir + "/global_world_markers.dat").ToStdString();
    }

    std::vector<WorldMapMarker> markers;
    std::string current_map_path;
    int next_id = 1;
};

#endif // RME_WORLD_MAP_MARKERS_H
