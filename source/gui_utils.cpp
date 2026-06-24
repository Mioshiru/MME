#include "main.h"
#include "gui.h"
#include "application.h"
#include "editor.h"
#include "live_server.h"
#include "loading_window.h"
#include <imgui.h>
#include <deque>

// Note: PopupDialog, ListDialog, ShowTextBox, SetHotkey, GetHotkey,
// SaveHotkeys, LoadHotkeys, SetStatusText, SetTitle, UpdateTitle,
// CreateLoadBar, SetLoadDone, DestroyLoadBar, operator<<, and operator>>
// for Hotkey are all defined in gui.cpp. This file only contains
// utility methods that are unique to this translation unit.

static std::deque<std::pair<std::string, ImVec4>> g_console_logs;
static bool g_scroll_to_bottom = false;

void GUI::AddDebugLog(const std::string& msg, bool isError) {
    ImVec4 color = isError ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f) : ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
    g_console_logs.push_back({ msg, color });
    if (g_console_logs.size() > 200) g_console_logs.pop_front();
    g_scroll_to_bottom = true;
}

void GUI::DrawDebugConsole() {
    if (!ImGui::Begin("Lua & System Debug", nullptr)) {
        ImGui::End();
        return;
    }
    if (ImGui::Button("Clear")) g_console_logs.clear();
    ImGui::Separator();
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    for (auto& log : g_console_logs) {
        ImGui::TextColored(log.second, "%s", log.first.c_str());
    }
    if (g_scroll_to_bottom) ImGui::SetScrollHereY(1.0f);
    g_scroll_to_bottom = false;
    ImGui::EndChild();
    ImGui::End();
}

void GUI::ToggleMinimapHUD() {
    bool current = g_settings.getInteger(Config::SHOW_MINIMAP_HUD) != 0;
    g_settings.setInteger(Config::SHOW_MINIMAP_HUD, current ? 0 : 1);
    RefreshView();
    UpdateMenus();
}

uint32_t GUI::GetHoveredPaletteItemID() const { return m_palette_hover_id; }
Item* GUI::GetSelectedPropertiesItem() { return m_imgui_properties_item; }
void GUI::OpenProperties(Item* item) { m_imgui_properties_item = item; }
void GUI::CloseProperties() { m_imgui_properties_item = nullptr; }

void GUI::AddChatMessage(const std::string& sender, const std::string& text) {
	ChatMessage msg { sender, text, time(nullptr) };
	chat_log.push_back(msg);
	if (chat_log.size() > 100) chat_log.erase(chat_log.begin());
	RefreshView();
}

void GUI::SendChat(const std::string& text) {
	if (text.empty()) return;
	Editor* ed = GetCurrentEditor();
	if (ed && ed->IsLive()) {
		if (ed->IsLiveServer()) ed->GetLiveServer()->broadcastChat("Host", wxstr(text));
		else {
			NetworkMessage msg; msg.write<uint8_t>(0x07); msg.write<std::string>(text);
			ed->GetLive().send(msg); AddChatMessage("Me", text);
		}
	}
}
