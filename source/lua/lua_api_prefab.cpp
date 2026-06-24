#include "main.h"
#include "lua_api.h"
#include "map.h"
#include "gui.h"
#include "../snapshot_action.h"
#include <string>
#include <vector>
#include <wx/clipbrd.h>

namespace LuaAPI {
    // Interne Base64 Helfer für shareable Strings
    static const std::string base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    static std::vector<uint8_t> current_ghost_data;

    std::string base64_encode(const uint8_t* bytes_to_encode, size_t in_len) {
        std::string ret;
        int i = 0, j = 0;
        uint8_t char_array_3[3], char_array_4[4];
        while (in_len--) {
            char_array_3[i++] = *(bytes_to_encode++);
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;
                for(i = 0; (i <4) ; i++) ret += base64_chars[char_array_4[i]];
                i = 0;
            }
        }
        if (i) {
            for(j = i; j < 3; j++) char_array_3[j] = '\0';
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            for (j = 0; (j < i + 1); j++) ret += base64_chars[char_array_4[j]];
            while((i++ < 3)) ret += '=';
        }
        return ret;
    }

    static inline bool is_base64(uint8_t c) {
        return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || (c == '+') || (c == '/'));
    }

    std::vector<uint8_t> base64_decode(const std::string& encoded_string) {
        size_t in_len = encoded_string.size();
        int i = 0;
        int j = 0;
        int in_ = 0;
        uint8_t char_array_4[4], char_array_3[3];
        std::vector<uint8_t> ret;

        while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
            char_array_4[i++] = encoded_string[in_]; in_++;
            if (i == 4) {
                for (i = 0; i < 4; i++)
                    char_array_4[i] = base64_chars.find(char_array_4[i]);

                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

                for (i = 0; (i < 3); i++)
                    ret.push_back(char_array_3[i]);
                i = 0;
            }
        }

        if (i) {
            for (j = i; j < 4; j++)
                char_array_4[j] = 0;

            for (j = 0; j < 4; j++)
                char_array_4[j] = base64_chars.find(char_array_4[j]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
        }

        return ret;
    }

    const std::vector<uint8_t>& getGhostData() {
        return current_ghost_data;
    }

    void registerPrefab(sol::state& lua) {
        sol::table prefab = lua.create_table();

        // prefab.capture(x, y, w, h) -> string (base64 encoded prefab)
        prefab.set_function("capture", [](int x, int y, int w, int h) -> std::string {
            Map& map = g_gui.GetCurrentMap();
            auto data = map.createRegionSnapshot(x, y, w, h);
            return base64_encode(data.data(), data.size());
        });

        // prefab.place(x, y, prefabString)
        prefab.set_function("place", [](int x, int y, const std::string& prefabString) {
            Map& map = g_gui.GetCurrentMap();
            auto data = base64_decode(prefabString);
            
            int w = 0, h = 0;
            size_t offset = 0;
            while(offset < data.size() && data[offset] != 0xFF) {
                if (offset + 5 > data.size()) break;
                int dx = data[offset++];
                int dy = data[offset++];
                offset++; // tz
                uint16_t flags = data[offset] | (data[offset+1] << 8);
                offset += 2;
                uint16_t itemCount = data[offset] | (data[offset+1] << 8);
                offset += 2;
                offset += itemCount * 6; // id, aid, uid
                if (dx + 1 > w) w = dx + 1;
                if (dy + 1 > h) h = dy + 1;
            }

            Editor* editor = g_gui.GetCurrentEditor();
                    if (!editor) return;
                    RME::Core::SnapshotAction* action = newd RME::Core::SnapshotAction(*editor, map, x, y, w, h);
            map.restoreRegionSnapshot(x, y, data);
            action->captureAfter();
            editor->addAction(action);
            
            g_gui.RefreshView();
        });

        // prefab.setGhost(base64String) -> Aktiviert die Ghost-Vorschau
        prefab.set_function("setGhost", [](const std::string& prefabString) {
            current_ghost_data = base64_decode(prefabString);
            g_gui.RefreshView();
        });

        // prefab.clearGhost() -> Deaktiviert die Ghost-Vorschau
        prefab.set_function("clearGhost", []() {
            current_ghost_data.clear();
            g_gui.RefreshView();
        });

        // prefab.importFromClipboard(x, y) -> bool
        prefab.set_function("importFromClipboard", [](int x, int y) -> bool {
            if (wxTheClipboard->Open()) {
                if (wxTheClipboard->IsSupported(wxDF_TEXT)) {
                    wxTextDataObject data;
                    wxTheClipboard->GetData(data);
                    std::string prefabString = data.GetText().ToStdString();
                    
                    Map& map = g_gui.GetCurrentMap();
                    auto decoded = base64_decode(prefabString);
                    map.restoreRegionSnapshot(x, y, decoded);
                    g_gui.RefreshView();
                    wxTheClipboard->Close();
                    return true;
                }
                wxTheClipboard->Close();
            }
            return false;
        });

        // prefab.saveToFile(path, prefabString)
        prefab.set_function("saveToFile", [](const std::string& path, const std::string& data) {
            std::ofstream f(path, std::ios::binary);
            if(f.is_open()) {
                f << data;
                return true;
            }
            return false;
        });

        // prefab.addToPalette(name, base64Data, iconName)
        prefab.set_function("addToPalette", [](const std::string& name, const std::string& data, const std::string& iconName) {
            // Registriert das Prefab intern als "Virtual Item" für die Palette
            g_gui.RegisterVirtualBrush(name, data, iconName); 
            g_gui.RefreshPalettes();
        });

        lua["prefab"] = prefab;
    }
}
