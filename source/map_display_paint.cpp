#include "main.h"
#include "map_display.h"
#include "map_window.h"
#include "map_drawer.h"
#include "gui.h"
#include "editor.h"
#include "settings.h"
#include "performance_logger.h"
#include "live_server.h"
#include "live_socket.h"
#include "live_client.h"
#include "live_peer.h"
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <thread>
#include <time.h>
#include <wx/wfstream.h>
#include <wx/log.h>
#include <GL/gl.h>

#ifdef __WINDOWS__
#include <windows.h>
typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int interval);
static void SetVSync(bool enabled) {
	static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = nullptr;
	static bool resolved = false;
	if (!resolved) {
		wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
		resolved = true;
	}
	if (wglSwapIntervalEXT) {
		wglSwapIntervalEXT(enabled ? 1 : 0);
	}
}
#endif

void AutoScalePerformanceSettings() {
	bool high_end_ram = false;
#ifdef __WINDOWS__
	MEMORYSTATUSEX memInfo;
	memInfo.dwLength = sizeof(MEMORYSTATUSEX);
	if (GlobalMemoryStatusEx(&memInfo)) {
		DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
		double total_gb = (double)totalPhysMem / (1024.0 * 1024.0 * 1024.0);
		if (total_gb >= 8.0) {
			high_end_ram = true;
		}
		wxLogDebug("Hardware Profiler: Total RAM: %.2f GB (%s)", total_gb, high_end_ram ? "High-End" : "Standard");
	}
#else
	high_end_ram = std::thread::hardware_concurrency() >= 8;
#endif

	int cpu_cores = std::thread::hardware_concurrency();
	wxLogDebug("Hardware Profiler: CPU Cores: %d", cpu_cores);

	const char* gl_renderer = (const char*)glGetString(GL_RENDERER);
	bool high_end_gpu = false;
	if (gl_renderer) {
		std::string renderer_str(gl_renderer);
		std::transform(renderer_str.begin(), renderer_str.end(), renderer_str.begin(), ::tolower);
		wxLogDebug("Hardware Profiler: GPU Renderer: %s", gl_renderer);
		if (renderer_str.find("nvidia") != std::string::npos ||
			renderer_str.find("geforce") != std::string::npos ||
			renderer_str.find("rtx") != std::string::npos ||
			renderer_str.find("radeon") != std::string::npos ||
			renderer_str.find("amd") != std::string::npos) {
			high_end_gpu = true;
		}
	}

	if (high_end_ram) {
		wxLogDebug("Hardware Profiler: High-end RAM detected.");
	}
	
	if (high_end_gpu) {
		wxLogDebug("Hardware Profiler: Modern GPU detected. Full performance options unlocked.");
	} else {
		g_settings.setInteger(Config::HIDE_ITEMS_WHEN_ZOOMED, 1);
		wxLogDebug("Hardware Profiler: Low-end GPU. Auto-enabled HIDE_ITEMS_WHEN_ZOOMED to preserve framerate.");
	}
}

void MapCanvas::OnPaint(wxPaintEvent& event) {
	PerformanceLogger::BeginFrame();
	wxPaintDC dc(this); // Must always be created in EVT_PAINT to validate the region
	if (!drawer) {
		// drawer not yet initialized (very early paint event before constructor
		// completes). Skip rendering but keep the DC alive to avoid infinite repaints.
		PerformanceLogger::EndFrame();
		return;
	}
	SetCurrent(*g_gui.GetGLContext(this));

#ifdef __WINDOWS__
	SetVSync(g_settings.getBoolean(Config::V_SYNC));
#endif

	static bool auto_scaled = false;
	if (!auto_scaled) {
		AutoScalePerformanceSettings();
		auto_scaled = true;
	}

	static bool imgui_initialized = false;
	if (!imgui_initialized) {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable; // <--- THIS WAS MISSING AND CAUSED THE CRASH
		ImGui::StyleColorsDark();
		
		// Custom styling for a premium look
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowRounding = 6.0f;
		style.ChildRounding = 4.0f;
		style.FrameRounding = 4.0f;
		style.PopupRounding = 4.0f;
		style.ScrollbarRounding = 9.0f;
		style.GrabRounding = 4.0f;
		style.TabRounding = 4.0f;

		style.WindowBorderSize = 1.0f;
		style.ChildBorderSize = 1.0f;
		style.PopupBorderSize = 1.0f;
		style.FrameBorderSize = 0.0f;
		style.TabBorderSize = 0.0f;

		// Colors
		style.Colors[ImGuiCol_Text]                   = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		style.Colors[ImGuiCol_WindowBg]               = ImVec4(0.09f, 0.10f, 0.15f, 0.90f); // Sleek dark blue-gray with transparency
		style.Colors[ImGuiCol_ChildBg]                = ImVec4(0.12f, 0.13f, 0.18f, 0.00f);
		style.Colors[ImGuiCol_PopupBg]                = ImVec4(0.09f, 0.10f, 0.15f, 0.95f);
		style.Colors[ImGuiCol_Border]                 = ImVec4(0.20f, 0.22f, 0.29f, 1.00f);
		style.Colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_FrameBg]                = ImVec4(0.15f, 0.16f, 0.23f, 1.00f);
		style.Colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.20f, 0.22f, 0.31f, 1.00f);
		style.Colors[ImGuiCol_FrameBgActive]          = ImVec4(0.24f, 0.26f, 0.37f, 1.00f);
		style.Colors[ImGuiCol_TitleBg]                = ImVec4(0.12f, 0.13f, 0.18f, 1.00f);
		style.Colors[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.18f, 0.25f, 1.00f);
		style.Colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.09f, 0.10f, 0.15f, 1.00f);
		style.Colors[ImGuiCol_MenuBarBg]              = ImVec4(0.12f, 0.13f, 0.18f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.09f, 0.10f, 0.15f, 0.50f);
		style.Colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.24f, 0.26f, 0.37f, 0.80f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.30f, 0.32f, 0.45f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.37f, 0.40f, 0.55f, 1.00f);
		style.Colors[ImGuiCol_CheckMark]              = ImVec4(0.35f, 0.55f, 0.95f, 1.00f);
		style.Colors[ImGuiCol_SliderGrab]             = ImVec4(0.35f, 0.55f, 0.95f, 1.00f);
		style.Colors[ImGuiCol_SliderGrabActive]        = ImVec4(0.45f, 0.65f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_Button]                 = ImVec4(0.18f, 0.20f, 0.28f, 1.00f);
		style.Colors[ImGuiCol_ButtonHovered]          = ImVec4(0.24f, 0.27f, 0.38f, 1.00f);
		style.Colors[ImGuiCol_ButtonActive]           = ImVec4(0.30f, 0.34f, 0.47f, 1.00f);
		style.Colors[ImGuiCol_Header]                 = ImVec4(0.18f, 0.20f, 0.28f, 1.00f);
		style.Colors[ImGuiCol_HeaderHovered]          = ImVec4(0.24f, 0.27f, 0.38f, 1.00f);
		style.Colors[ImGuiCol_HeaderActive]           = ImVec4(0.30f, 0.34f, 0.47f, 1.00f);
		style.Colors[ImGuiCol_Separator]              = ImVec4(0.20f, 0.22f, 0.29f, 1.00f);
		style.Colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.30f, 0.32f, 0.45f, 1.00f);
		style.Colors[ImGuiCol_SeparatorActive]        = ImVec4(0.35f, 0.55f, 0.95f, 1.00f);
		style.Colors[ImGuiCol_ResizeGrip]             = ImVec4(0.20f, 0.22f, 0.29f, 1.00f);
		style.Colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.30f, 0.32f, 0.45f, 1.00f);
		style.Colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.35f, 0.55f, 0.95f, 1.00f);
		style.Colors[ImGuiCol_Tab]                    = ImVec4(0.12f, 0.13f, 0.18f, 1.00f);
		style.Colors[ImGuiCol_TabHovered]             = ImVec4(0.20f, 0.22f, 0.31f, 1.00f);
		style.Colors[ImGuiCol_TabActive]              = ImVec4(0.18f, 0.20f, 0.28f, 1.00f);
		style.Colors[ImGuiCol_TabUnfocused]           = ImVec4(0.12f, 0.13f, 0.18f, 1.00f);
		style.Colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.16f, 0.18f, 0.25f, 1.00f);

		ImGui_ImplOpenGL3_Init("#version 450");
		imgui_initialized = true;
	}

	if (g_gui.IsRenderingEnabled()) {
		DrawingOptions& options = drawer->getOptions(); // Access through unique_ptr
		if (screenshot_buffer) {
			options.SetIngame();
		} else {
			options.transparent_floors = g_settings.getBoolean(Config::TRANSPARENT_FLOORS);
			options.transparent_items = g_settings.getBoolean(Config::TRANSPARENT_ITEMS);
			options.show_ingame_box = g_settings.getBoolean(Config::SHOW_INGAME_BOX);
			options.show_lights = g_settings.getBoolean(Config::SHOW_LIGHTS);
			options.show_light_str = g_settings.getBoolean(Config::SHOW_LIGHT_STR);
			options.show_tech_items = g_settings.getBoolean(Config::SHOW_TECHNICAL_ITEMS);
			options.show_waypoints = g_settings.getBoolean(Config::SHOW_WAYPOINTS);
			options.show_grid = g_settings.getInteger(Config::SHOW_GRID);
			options.ingame = !g_settings.getBoolean(Config::SHOW_EXTRA);
			options.show_all_floors = g_settings.getBoolean(Config::SHOW_ALL_FLOORS);
			options.show_creatures = g_settings.getBoolean(Config::SHOW_CREATURES);
			options.show_spawns = g_settings.getBoolean(Config::SHOW_SPAWNS);
			options.show_houses = g_settings.getBoolean(Config::SHOW_HOUSES);
			options.show_shade = g_settings.getBoolean(Config::SHOW_SHADE);
			options.show_special_tiles = g_settings.getBoolean(Config::SHOW_SPECIAL_TILES);
			options.show_items = g_settings.getBoolean(Config::SHOW_ITEMS);
			options.highlight_items = g_settings.getBoolean(Config::HIGHLIGHT_ITEMS);
			options.highlight_locked_doors = g_settings.getBoolean(Config::HIGHLIGHT_LOCKED_DOORS);
			options.show_blocking = g_settings.getBoolean(Config::SHOW_BLOCKING);
			options.show_tooltips = g_settings.getBoolean(Config::SHOW_TOOLTIPS);
			options.show_as_minimap = g_settings.getBoolean(Config::SHOW_AS_MINIMAP);
			options.show_only_colors = g_settings.getBoolean(Config::SHOW_ONLY_TILEFLAGS);
			options.show_only_modified = g_settings.getBoolean(Config::SHOW_ONLY_MODIFIED_TILES);
			options.show_preview = g_settings.getBoolean(Config::SHOW_PREVIEW);
			options.show_hooks = g_settings.getBoolean(Config::SHOW_WALL_HOOKS);
			options.hide_items_when_zoomed = g_settings.getBoolean(Config::HIDE_ITEMS_WHEN_ZOOMED);
			options.show_towns = g_settings.getBoolean(Config::SHOW_TOWNS);
			options.always_show_zones = g_settings.getBoolean(Config::ALWAYS_SHOW_ZONES);
			options.extended_house_shader = g_settings.getBoolean(Config::EXT_HOUSE_SHADER);

			options.experimental_fog = g_settings.getBoolean(Config::EXPERIMENTAL_FOG);
		}

		options.dragging = boundbox_selection;

		drawer->SetupVars();
		drawer->SetupGL();
		drawer->Draw();

		if (screenshot_buffer) {
			drawer->TakeScreenshot(screenshot_buffer);
		}

		drawer->Release();
	}

	// Clean unused textures
	g_gui.gfx.garbageCollection();

	// Render ImGui
	ImGuiIO& io = ImGui::GetIO();
	int w, h;
	GetClientSize(&w, &h);
	io.DisplaySize = ImVec2((float)w, (float)h);
	static wxLongLong lastTime = wxGetLocalTimeMillis();
	wxLongLong currentTime = wxGetLocalTimeMillis();
	float deltaTime = (currentTime - lastTime).ToLong() / 1000.0f;
	if (deltaTime <= 0.0f) deltaTime = 0.00001f;
	io.DeltaTime = deltaTime;
	lastTime = currentTime;

	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	// Bulletproof Dockspace definition
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	// Overlay for FPS
	if (g_settings.getBoolean(Config::SHOW_FPS)) {
		ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
		ImGui::SetNextWindowBgAlpha(0.35f);
		if (ImGui::Begin("Overlay", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav)) {
			ImGui::Text("FPS: %.1f (%.3f ms/frame)", io.Framerate, 1000.0f / (io.Framerate > 0 ? io.Framerate : 1.0f));
			ImGui::End();
		}
	}
	// Team Chat Window (Multiplayer only)
	if (editor.IsLive()) {
		ImGui::SetNextWindowPos(ImVec2(10, io.DisplaySize.y - 220), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
		ImGui::Begin("Team Chat", nullptr, ImGuiWindowFlags_NoCollapse);

		// Network Latency Display
		if (editor.IsLiveServer()) {
			LiveServer* server = editor.GetLiveServer();
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Host Mode | Clients: %d", (int)server->clients.size());
			if (ImGui::IsItemHovered() && !server->clients.empty()) {
				ImGui::BeginTooltip();
				for (auto& clientEntry : server->clients) {
					LivePeer* peer = clientEntry.second;
					uint32_t lat = g_gui.latencies[peer];
					ImVec4 col = (lat < 100) ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : (lat < 250 ? ImVec4(1.0f, 1.0f, 0.2f, 1.0f) : ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
					ImGui::Text("%s:", nstr(peer->getName()).c_str());
					ImGui::SameLine();
					ImGui::TextColored(col, " %d ms", lat);
				}
				ImGui::EndTooltip();
			}
		} else {
			uint32_t lat = g_gui.latencies[editor.GetLiveClient()];
			ImVec4 col = (lat < 100) ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : (lat < 250 ? ImVec4(1.0f, 1.0f, 0.2f, 1.0f) : ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Latency: ");
			ImGui::SameLine();
			ImGui::TextColored(col, "%d ms", lat);
		}
		ImGui::Separator();

		// Chat history area
		float reserve_height = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
		ImGui::BeginChild("ScrollingRegion", ImVec2(0, -reserve_height), false, ImGuiWindowFlags_HorizontalScrollbar);
		for (const auto& msg : g_gui.chat_log) {
			ImVec4 color = ImVec4(0.7f, 0.7f, 0.9f, 1.0f); // Default silver
			if (msg.sender == "Host") color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f); // Green for Host
			else if (msg.sender == "Me") color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White for self

			ImGui::TextColored(color, "[%s]: ", msg.sender.c_str());
			ImGui::SameLine();
			ImGui::TextWrapped("%s", msg.text.c_str());
		}
		if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			ImGui::SetScrollHereY(1.0f);
		ImGui::EndChild();

		ImGui::Separator();

		// Input field
		static char chat_input[256] = "";
		bool reclaim_focus = false;
		ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_EnterReturnsTrue;
		if (ImGui::InputText("##ChatInput", chat_input, IM_ARRAYSIZE(chat_input), input_flags)) {
			std::string t = chat_input;
			if (!t.empty()) {
				g_gui.SendChat(t);
				chat_input[0] = '\0';
			}
			reclaim_focus = true;
		}
		
		// Auto-focus on window appearance or message send
		ImGui::SetItemDefaultFocus();
		if (reclaim_focus || ImGui::IsWindowAppearing()) {
			ImGui::SetKeyboardFocusHere(-1);
		}

		ImGui::End();
	}

	// Graphics Error Log Overlay
	if (!g_gui.m_graphicsErrorLog.empty()) {
		ImGui::SetNextWindowSize(ImVec2(450, 200), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Graphics Error Log", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Serious rendering issue detected:");
			ImGui::Separator();
			ImGui::TextWrapped("%s", g_gui.m_graphicsErrorLog.c_str());
			ImGui::Spacing();
			if (ImGui::Button("Clear and Acknowledge")) { g_gui.m_graphicsErrorLog.clear(); }
			ImGui::End();
		}
	}

	// In-Canvas Minimap HUD in the top-right corner
	if (g_settings.getBoolean(Config::MINIMAP_VISIBLE)) {
		UpdateMinimapTexture();
		float canvas_width = io.DisplaySize.x;
		ImGui::SetNextWindowPos(ImVec2(canvas_width - 200, 10), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(196, 196), ImGuiCond_Always);
		ImGui::SetNextWindowBgAlpha(0.6f); // 40% transparent (60% opacity)
		ImGuiWindowFlags minimap_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
		ImGui::Begin("Minimap HUD", nullptr, minimap_flags);
		if (minimap_tex_id != 0) {
			ImVec2 pos = ImGui::GetCursorScreenPos();
			ImGui::Image((void*)(intptr_t)minimap_tex_id, ImVec2(180, 180));

			if (ImGui::IsItemHovered()) {
				if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
					ImVec2 mouse_pos = ImGui::GetMousePos();
					float rel_x = (mouse_pos.x - pos.x) / 180.0f;
					float rel_y = (mouse_pos.y - pos.y) / 180.0f;
					int click_map_x = minimap_start_x + (int)(rel_x * 180.0f);
					int click_map_y = minimap_start_y + (int)(rel_y * 180.0f);
					g_gui.SetScreenCenterPosition(Position(click_map_x, click_map_y, floor));
					last_minimap_update_time = 0; // immediate update
					Refresh();
				}
			}

			if (g_settings.getInteger(Config::MINIMAP_VIEW_BOX)) {
				int screensize_x, screensize_y;
				int view_scroll_x, view_scroll_y;
				GetViewBox(&view_scroll_x, &view_scroll_y, &screensize_x, &screensize_y);

				int tile_size = int(TileSize / GetZoom());
				int floor_offset = (floor > GROUND_LAYER ? 0 : (GROUND_LAYER - floor));

				int view_start_x = view_scroll_x / TileSize + floor_offset;
				int view_start_y = view_scroll_y / TileSize + floor_offset;
				int view_end_x = view_start_x + screensize_x / tile_size + 1;
				int view_end_y = view_start_y + screensize_y / tile_size + 1;

				float p_start_x = pos.x + (view_start_x - minimap_start_x);
				float p_start_y = pos.y + (view_start_y - minimap_start_y);
				float p_end_x = pos.x + (view_end_x - minimap_start_x);
				float p_end_y = pos.y + (view_end_y - minimap_start_y);

				p_start_x = std::max(p_start_x, pos.x);
				p_start_y = std::max(p_start_y, pos.y);
				p_end_x = std::min(p_end_x, pos.x + 180.0f);
				p_end_y = std::min(p_end_y, pos.y + 180.0f);

				if (p_start_x < p_end_x && p_start_y < p_end_y) {
					ImDrawList* draw_list = ImGui::GetWindowDrawList();
					draw_list->AddRect(ImVec2(p_start_x, p_start_y), ImVec2(p_end_x, p_end_y), IM_COL32(255, 255, 255, 255), 0.0f, 0, 1.5f);
				}
			}
		}
		ImGui::End();
	}

	ImGui::Render();
	{
		PROFILE_SCOPE("ImGui::RenderDrawData");
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	// Swap buffer
	{
		PROFILE_SCOPE("SwapBuffers");
		SwapBuffers();
	}

	// Send new node requests
	editor.SendNodeRequests();
	
	PerformanceLogger::EndFrame();
}

void MapCanvas::TakeScreenshot(wxFileName path, wxString format) {
	int screensize_x, screensize_y;
	GetViewBox(&view_scroll_x, &view_scroll_y, &screensize_x, &screensize_y);

	delete[] screenshot_buffer; // unique_ptr verwaltet dies jetzt
	screenshot_buffer = newd uint8_t[3 * screensize_x * screensize_y]; 

	// Draw the window
	Refresh();
	wxGLCanvas::Update(); // Forces immediate redraws the window.

	// screenshot_buffer should now contain the screenbuffer
	if (screenshot_buffer == nullptr) {
		g_gui.PopupDialog("Capture failed", "Image capture failed. Old Video Driver?", wxOK);
	} else {
		// We got the shit
		int screensize_x, screensize_y;
		static_cast<MapWindow*>(GetParent())->GetViewSize(&screensize_x, &screensize_y);
		wxImage screenshot(screensize_x, screensize_y, screenshot_buffer);

		time_t t = time(nullptr);
		struct tm* current_time = localtime(&t);
		ASSERT(current_time);

		wxString date;
		date << "screenshot_" << (1900 + current_time->tm_year);
		if (current_time->tm_mon < 9) {
			date << "-"
				 << "0" << current_time->tm_mon + 1;
		} else {
			date << "-" << current_time->tm_mon + 1;
		}
		date << "-" << current_time->tm_mday;
		date << "-" << current_time->tm_hour;
		date << "-" << current_time->tm_min;
		date << "-" << current_time->tm_sec;

		int type = 0;
		path.SetName(date);
		if (format == "bmp") {
			path.SetExt(format);
			type = wxBITMAP_TYPE_BMP;
		} else if (format == "png") {
			path.SetExt(format);
			type = wxBITMAP_TYPE_PNG;
		} else if (format == "jpg" || format == "jpeg") {
			path.SetExt(format);
			type = wxBITMAP_TYPE_JPEG;
		} else if (format == "tga") {
			path.SetExt(format);
			type = wxBITMAP_TYPE_TGA;
		} else {
			g_gui.SetStatusText("Unknown screenshot format \'" + format + "\", switching to default (png)");
			path.SetExt("png");
			;
			type = wxBITMAP_TYPE_PNG;
		}

		path.Mkdir(0755, wxPATH_MKDIR_FULL);
		wxFileOutputStream of(path.GetFullPath());
		if (of.IsOk()) {
			if (screenshot.SaveFile(of, static_cast<wxBitmapType>(type))) {
				g_gui.SetStatusText("Took screenshot and saved as " + path.GetFullName());
			} else {
				g_gui.PopupDialog("File error", "Couldn't save image file correctly.", wxOK);
			}
		} else {
			g_gui.PopupDialog("File error", "Couldn't open file " + path.GetFullPath() + " for writing.", wxOK);
		}
	}

	Refresh();

	screenshot_buffer = nullptr;
}