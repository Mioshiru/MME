#include "main.h"
#include "map_display.h"
#include "map_window.h"
#include "map_drawer.h"
#include "gui.h"
#include "editor.h"
#include "settings.h"
#include "complexitem.h"
#include "creature.h"
#include "performance_logger.h"
#include "live_server.h"
#include "live_socket.h"
#include "live_client.h"
#include "live_peer.h"
#include <cmath>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <thread>
#include <chrono>
#include <time.h>
#include <wx/wfstream.h>
#include <wx/log.h>
#include <GL/gl.h>
#include "pngfiles.h"
#include "artprovider.h"
#include <wx/artprov.h>
#include <wx/mstream.h>
#include <wx/stdpaths.h>

static wxBitmap _wxGetBitmapFromMemoryRadial(const unsigned char* data, int length, const wxSize& target_size) {
	wxMemoryInputStream is(data, length);
	wxImage img(is, "image/png");
	if (!img.IsOk()) {
		return wxNullBitmap;
	}
	if (target_size.IsFullySpecified() && target_size.GetWidth() > 0 && target_size.GetHeight() > 0 &&
		(img.GetWidth() != target_size.GetWidth() || img.GetHeight() != target_size.GetHeight())) {
		img = img.Scale(target_size.GetWidth(), target_size.GetHeight(), wxIMAGE_QUALITY_HIGH);
	}
	return wxBitmap(img, -1);
}

static wxBitmap LoadBitmapFromCandidatesRadial(const wxSize& target_size, const std::vector<wxString>& candidates) {
	for (const auto& filepath : candidates) {
		wxImage img;
		if (img.LoadFile(filepath, wxBITMAP_TYPE_PNG)) {
			if (target_size.IsFullySpecified() && target_size.GetWidth() > 0 && target_size.GetHeight() > 0 &&
				(img.GetWidth() != target_size.GetWidth() || img.GetHeight() != target_size.GetHeight())) {
				img = img.Scale(target_size.GetWidth(), target_size.GetHeight(), wxIMAGE_QUALITY_HIGH);
			}
			return wxBitmap(img, -1);
		}
	}
	return wxNullBitmap;
}

static GLuint ConvertBitmapToTexture(const wxBitmap& bitmap) {
	if (!bitmap.IsOk()) return 0;
	wxImage img = bitmap.ConvertToImage();
	int w = img.GetWidth();
	int h = img.GetHeight();
	unsigned char* rgb = img.GetData();
	unsigned char* alpha = img.HasAlpha() ? img.GetAlpha() : nullptr;
	
	std::vector<unsigned char> rgba(w * h * 4);
	for (int i = 0; i < w * h; ++i) {
		rgba[i * 4 + 0] = rgb[i * 3 + 0];
		rgba[i * 4 + 1] = rgb[i * 3 + 1];
		rgba[i * 4 + 2] = rgb[i * 3 + 2];
		rgba[i * 4 + 3] = alpha ? alpha[i] : 255;
	}
	
	GLuint tex_id = 0;
	glGenTextures(1, &tex_id);
	glBindTexture(GL_TEXTURE_2D, tex_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
	glBindTexture(GL_TEXTURE_2D, 0);
	return tex_id;
}


#ifdef __WINDOWS__
#include <windows.h>
typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int interval);
static void SetVSync(bool enabled) {
	static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = nullptr;
	static bool resolved = false;
	static int current_interval = -1;
	if (!resolved) {
		wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
		resolved = true;
	}
	int target_interval = enabled ? 1 : 0;
	if (wglSwapIntervalEXT && current_interval != target_interval) {
		wglSwapIntervalEXT(target_interval);
		current_interval = target_interval;
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
		PerformanceLogger::EndFrame();
		return;
	}

	// Update physics with high-precision std::chrono dt
	static auto last_time = std::chrono::high_resolution_clock::now();
	auto current_time = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = current_time - last_time;
	double dt = elapsed.count();
	if (dt <= 0.0 || dt > 0.1) {
		dt = 1.0 / 60.0;
	}
	last_time = current_time;

	if (GetParent()) {
		static_cast<MapWindow*>(GetParent())->UpdateSmoothScroll();
	}
	UpdateKineticScroll();
	UpdateSmoothZoom();

	SetCurrent(*g_gui.GetGLContext(this));

// Enable VSync for smooth, tear-free rendering (butterweich)
#ifdef __WINDOWS__
SetVSync(true);
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
	static auto imgui_last_time = std::chrono::high_resolution_clock::now();
	auto imgui_current_time = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float> imgui_elapsed = imgui_current_time - imgui_last_time;
	float deltaTime = imgui_elapsed.count();
	if (deltaTime <= 0.0f) deltaTime = 0.00001f;
	io.DeltaTime = deltaTime;
	imgui_last_time = imgui_current_time;

	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGuiWindowFlags dockspace_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;
	ImGui::Begin("##MapDockSpaceHost", nullptr, dockspace_flags);
	ImGuiID dockspace_id = ImGui::GetID("MapEditorDockSpace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
	ImGui::End();

	// Render Brush Hover Preview Overlay
	if (!screendragging && !boundbox_selection && g_gui.GetCurrentBrush() && !g_gui.IsSelectionMode()) {
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(io.DisplaySize);
		ImGui::SetNextWindowBgAlpha(0.0f);
		ImGuiWindowFlags preview_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground;
		if (ImGui::Begin("##BrushHoverPreviewOverlay", nullptr, preview_flags)) {
			int scroll_x = 0, scroll_y = 0;
			if (GetParent()) {
				static_cast<MapWindow*>(GetParent())->GetViewStart(&scroll_x, &scroll_y);
			}
			int map_x = last_cursor_map_x;
			int map_y = last_cursor_map_y;
			int size = g_gui.GetBrushSize();
			BrushShape shape = g_gui.GetBrushShape();
			ImVec2 pos = ImGui::GetWindowPos();
			
			int offset = (floor <= 7) ? (7 - floor) * TileSize : 0;
			if (shape == BRUSHSHAPE_SQUARE) {
				double x1 = pos.x + (((map_x - size) * TileSize - scroll_x) - offset) / zoom;
				double y1 = pos.y + (((map_y - size) * TileSize - scroll_y) - offset) / zoom;
				double x2 = pos.x + (((map_x + size + 1) * TileSize - scroll_x) - offset) / zoom;
				double y2 = pos.y + (((map_y + size + 1) * TileSize - scroll_y) - offset) / zoom;
				
				ImGui::GetWindowDrawList()->AddRectFilled(ImVec2((float)x1, (float)y1), ImVec2((float)x2, (float)y2), ImColor(60, 120, 220, 76), 4.0f);
				ImGui::GetWindowDrawList()->AddRect(ImVec2((float)x1, (float)y1), ImVec2((float)x2, (float)y2), ImColor(180, 150, 50, 204), 4.0f, 0, 2.0f);
			} else if (shape == BRUSHSHAPE_CIRCLE) {
				double cx = pos.x + (((map_x + 0.5) * TileSize - scroll_x) - offset) / zoom;
				double cy = pos.y + (((map_y + 0.5) * TileSize - scroll_y) - offset) / zoom;
				double r = ((size + 0.5) * TileSize) / zoom;
				
				ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2((float)cx, (float)cy), (float)r, ImColor(60, 120, 220, 76), 32);
				ImGui::GetWindowDrawList()->AddCircle(ImVec2((float)cx, (float)cy), (float)r, ImColor(180, 150, 50, 204), 32, 2.0f);
			}
			ImGui::End();
		}
	}

	// Bulletproof Dockspace definition
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
					uint32_t lat = peer->getLatency();
					ImVec4 col = (lat < 100) ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : (lat < 250 ? ImVec4(1.0f, 1.0f, 0.2f, 1.0f) : ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
					ImGui::Text("%s:", nstr(peer->getName()).c_str());
					ImGui::SameLine();
					ImGui::TextColored(col, " %d ms | %u%% loss | %s", lat, peer->getPacketLoss(), nstr(peer->getConnectionStatus()).c_str());
				}
				ImGui::EndTooltip();
			}
		} else {
			LiveClient* client = editor.GetLiveClient();
			uint32_t lat = client->getLatency();
			ImVec4 col = (lat < 100) ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : (lat < 250 ? ImVec4(1.0f, 1.0f, 0.2f, 1.0f) : ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Join Mode | ");
			ImGui::SameLine();
			ImGui::TextColored(col, "%s | %d ms | %u%% loss", nstr(client->getConnectionStatus()).c_str(), lat, client->getPacketLoss());
		}
		ImGui::Separator();

		// Determine own name for highlighting
		std::string ownName;
		if (editor.IsLiveServer()) {
			ownName = "Host";
		} else if (editor.GetLiveClient()) {
			ownName = nstr(editor.GetLiveClient()->getName());
		}

		// Chat history area
		float reserve_height = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
		ImGui::BeginChild("ScrollingRegion", ImVec2(0, -reserve_height), false, ImGuiWindowFlags_HorizontalScrollbar);
		for (const auto& msg : g_gui.chat_log) {
			ImVec4 color = ImVec4(0.7f, 0.7f, 0.9f, 1.0f); // Default silver for other players
			if (msg.sender == ownName) color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White for self
			else if (msg.sender == "Host") color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f); // Green for Host
			else if (msg.sender == "Server") color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f); // Gold for Server messages

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
		ImGui::PushItemWidth(-1.0f);
		if (ImGui::IsWindowAppearing()) {
			ImGui::SetKeyboardFocusHere();
		}
		if (ImGui::InputText("##ChatInput", chat_input, IM_ARRAYSIZE(chat_input), ImGuiInputTextFlags_EnterReturnsTrue)) {
			std::string t = chat_input;
			if (!t.empty()) {
				g_gui.SendChat(t);
				chat_input[0] = '\0';
			}
			reclaim_focus = true;
		}
		ImGui::PopItemWidth();
		
		if (reclaim_focus) {
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

	if (ui_toolbar && ui_toolbar->isVisible() && drawer->GetNanoVGContext()) {
		nvgBeginFrame(drawer->GetNanoVGContext(), (float)w, (float)h, (float)GetContentScaleFactor());
		ui_toolbar->render(drawer->GetNanoVGContext());
		nvgEndFrame(drawer->GetNanoVGContext());
	}

	// Movable minimap window
	if (g_settings.getBoolean(Config::MINIMAP_VISIBLE) && g_settings.getInteger(Config::MINIMAP_DOCK_STYLE) == 0) {
		bool minimap_open = true;

		UpdateMinimapTexture();
		ImGui::SetNextWindowSize(ImVec2(200.0f, 260.0f), ImGuiCond_Always);
		ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 210.0f, 10.0f), ImGuiCond_Always);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 5.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 4.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(10.0f / 255.0f, 15.0f / 255.0f, 25.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(10.0f / 255.0f, 15.0f / 255.0f, 25.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(10.0f / 255.0f, 15.0f / 255.0f, 25.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(10.0f / 255.0f, 15.0f / 255.0f, 25.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(180.0f / 255.0f, 140.0f / 255.0f, 50.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(180.0f / 255.0f, 140.0f / 255.0f, 50.0f / 255.0f, 1.0f));
		ImGuiWindowFlags minimap_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
		const bool minimap_visible = ImGui::Begin("Minimap", &minimap_open, minimap_flags);
		if (!minimap_open) {
			g_settings.setInteger(Config::MINIMAP_VISIBLE, 0);
			g_gui.RefreshPalettes();
			g_gui.UpdateMenus();
		}

		if (minimap_visible && minimap_tex_id != 0) {
			ImVec2 avail = ImVec2(180.0f, 180.0f);

			ImVec2 pos = ImGui::GetCursorScreenPos();
			ImGui::Image((void*)(intptr_t)minimap_tex_id, avail);

			// Draw compass, zoom, and floor overlay buttons on top of the minimap image
			int center_map_x, center_map_y;
			GetScreenCenter(&center_map_x, &center_map_y);

			auto draw_button = [&](ImVec2 b_pos, const char* label, std::function<void()> action) {
				ImGui::SetCursorScreenPos(b_pos);
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(20.0f/255.0f, 22.0f/255.0f, 28.0f/255.0f, 0.85f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(180.0f/255.0f, 140.0f/255.0f, 50.0f/255.0f, 0.85f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(180.0f/255.0f, 140.0f/255.0f, 50.0f/255.0f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(180.0f/255.0f, 140.0f/255.0f, 50.0f/255.0f, 0.85f));
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

				if (ImGui::Button(label, ImVec2(20, 20))) {
					action();
					last_minimap_update_time = 0;
					Refresh();
				}

				ImGui::PopStyleVar(2);
				ImGui::PopStyleColor(4);
			};

			// North
			draw_button(ImVec2(pos.x + 90 - 10, pos.y + 4), "N", [&](){
				g_gui.SetScreenCenterPosition(Position(center_map_x, center_map_y - 10, floor));
			});
			// South
			draw_button(ImVec2(pos.x + 90 - 10, pos.y + 180 - 24), "S", [&](){
				g_gui.SetScreenCenterPosition(Position(center_map_x, center_map_y + 10, floor));
			});
			// West
			draw_button(ImVec2(pos.x + 4, pos.y + 90 - 10), "W", [&](){
				g_gui.SetScreenCenterPosition(Position(center_map_x - 10, center_map_y, floor));
			});
			// East
			draw_button(ImVec2(pos.x + 180 - 24, pos.y + 90 - 10), "E", [&](){
				g_gui.SetScreenCenterPosition(Position(center_map_x + 10, center_map_y, floor));
			});

			// Zoom In (+)
			draw_button(ImVec2(pos.x + 180 - 46, pos.y + 180 - 24), "+", [&](){
				minimap_zoom /= 1.2f;
				float max_zoom = std::max(4.0f, (float)std::max(editor.map.getWidth(), editor.map.getHeight()) / 180.0f);
				minimap_zoom = std::clamp(minimap_zoom, 0.25f, max_zoom);
				minimap_span_w = (int)(180.0f * minimap_zoom);
				minimap_span_h = (int)(180.0f * minimap_zoom);
			});
			// Zoom Out (-)
			draw_button(ImVec2(pos.x + 180 - 24, pos.y + 180 - 24), "-", [&](){
				minimap_zoom *= 1.2f;
				float max_zoom = std::max(4.0f, (float)std::max(editor.map.getWidth(), editor.map.getHeight()) / 180.0f);
				minimap_zoom = std::clamp(minimap_zoom, 0.25f, max_zoom);
				minimap_span_w = (int)(180.0f * minimap_zoom);
				minimap_span_h = (int)(180.0f * minimap_zoom);
			});

			// Floor Up (U)
			draw_button(ImVec2(pos.x + 4, pos.y + 180 - 46), "U", [&](){
				if (floor > 0) {
					floor--;
				}
			});
			// Floor Down (D)
			draw_button(ImVec2(pos.x + 4, pos.y + 180 - 24), "D", [&](){
				if (floor < 15) {
					floor++;
				}
			});

			// Reset cursor position below minimap image for coordinates/controls
			ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + 184));
			ImGui::Text("X: %d Y: %d Z: %d", center_map_x, center_map_y, floor);

			// Handle mouse click and drag to reposition viewport on the main minimap area
			ImGui::SetCursorScreenPos(pos);
			ImGui::Dummy(avail);
			if (ImGui::IsItemHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
				ImVec2 mouse_pos = ImGui::GetMousePos();
				float rel_x = (mouse_pos.x - pos.x) / avail.x;
				float rel_y = (mouse_pos.y - pos.y) / avail.y;
				rel_x = std::clamp(rel_x, 0.0f, 1.0f);
				rel_y = std::clamp(rel_y, 0.0f, 1.0f);
				int click_map_x = minimap_start_x + (int)(rel_x * (float)std::max(1, minimap_span_w - 1));
				int click_map_y = minimap_start_y + (int)(rel_y * (float)std::max(1, minimap_span_h - 1));
				g_gui.SetScreenCenterPosition(Position(click_map_x, click_map_y, floor));
				last_minimap_update_time = 0; // immediate update
				Refresh();
			}

			// Handle mouse wheel zoom
			if (ImGui::IsItemHovered()) {
				float wheel = ImGui::GetIO().MouseWheel;
				if (wheel != 0.0f) {
					if (wheel > 0.0f) minimap_zoom /= 1.2f;
					else minimap_zoom *= 1.2f;
					float max_zoom = std::max(4.0f, (float)std::max(editor.map.getWidth(), editor.map.getHeight()) / 180.0f);
					minimap_zoom = std::clamp(minimap_zoom, 0.25f, max_zoom);
					minimap_span_w = (int)(180.0f * minimap_zoom);
					minimap_span_h = (int)(180.0f * minimap_zoom);
					last_minimap_update_time = 0;
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

				const float sx = avail.x / (float)std::max(1, minimap_span_w);
				const float sy = avail.y / (float)std::max(1, minimap_span_h);
				float p_start_x = pos.x + (view_start_x - minimap_start_x) * sx;
				float p_start_y = pos.y + (view_start_y - minimap_start_y) * sy;
				float p_end_x = pos.x + (view_end_x - minimap_start_x) * sx;
				float p_end_y = pos.y + (view_end_y - minimap_start_y) * sy;

				p_start_x = std::max(p_start_x, pos.x);
				p_start_y = std::max(p_start_y, pos.y);
				p_end_x = std::min(p_end_x, pos.x + avail.x);
				p_end_y = std::min(p_end_y, pos.y + avail.y);

				if (p_start_x < p_end_x && p_start_y < p_end_y) {
					ImDrawList* draw_list = ImGui::GetWindowDrawList();
					draw_list->AddRect(ImVec2(p_start_x, p_start_y), ImVec2(p_end_x, p_end_y), IM_COL32(255, 255, 255, 255), 0.0f, 0, 1.5f);
				}
			}

			// Jump to Town dropdown
			ImGui::SetNextItemWidth(180.0f);
			if (ImGui::BeginCombo("##JumpTown", "Go to...")) {
				// Map Center option
				if (ImGui::Selectable("Map Center")) {
					int map_w = editor.map.getWidth();
					int map_h = editor.map.getHeight();
					g_gui.SetScreenCenterPosition(Position(map_w / 2, map_h / 2, floor));
					last_minimap_update_time = 0;
					Refresh();
				}

				// Towns
				const Towns& towns = editor.map.towns;
				for (TownMap::const_iterator it = towns.begin(); it != towns.end(); ++it) {
					Town* town = it->second;
					if (town && !town->getName().empty()) {
						if (ImGui::Selectable(town->getName().c_str())) {
							g_gui.SetScreenCenterPosition(town->getTemplePosition());
							last_minimap_update_time = 0;
							Refresh();
						}
					}
				}
				ImGui::EndCombo();
			}

			// Dock button: show "Palette" when on canvas
			if (ImGui::Button("Palette", ImVec2(180.0f, 0.0f))) {
				g_settings.setInteger(Config::MINIMAP_DOCK_STYLE, 1);
				g_gui.RefreshPalettes();
			}
		}
		ImGui::End();
		ImGui::PopStyleColor(6);
		ImGui::PopStyleVar(6);
	}

	// [UI] Old ImGui Tool Wheel replaced with premium ImGui circular selection wheel.
	if (tool_wheel_open) {
		LoadRadialTextures();
		ImGuiViewport* vp = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(vp->Pos);
		ImGui::SetNextWindowSize(vp->Size);
		ImGui::SetNextWindowViewport(vp->ID);
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoMouseInputs;
		
		if (ImGui::Begin("##RadialMenuFullscreen", &tool_wheel_open, flags)) {
			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			
			const float r_min = 45.0f;
			const float r_max = 145.0f;
			
			struct RadialTool {
				std::string label;
			};
			
			static const std::vector<RadialTool> tools = {
				{"SELECTION"},
				{"PENCIL"},
				{"BUCKET"},
				{"PROTECTION ZONE"},
				{"NORMAL DOOR"},
				{"LOCKED DOOR"},
				{"MAGIC DOOR"},
				{"HATCH WINDOW"},
				{"ERASER"},
				{"PREFAB CREATOR"}
			};
			
			const int N = tools.size();
			ImVec2 mouse_pos = ImGui::GetMousePos();
			ImVec2 center(vp->Pos.x + tool_wheel_x, vp->Pos.y + tool_wheel_y);
			
			float dx = mouse_pos.x - center.x;
			float dy = mouse_pos.y - center.y;
			float dist = std::sqrt(dx * dx + dy * dy);
			
			int hovered_slice = GetHoveredRadialSlice();
			
			// 1. Draw outer glowing ring (shadow)
			draw_list->AddCircle(center, r_max + 1.0f, IM_COL32(0, 0, 0, 100), 64, 4.0f);
			
			// 2. Draw slices
			for (int i = 0; i < N; ++i) {
				float angle_start = (i * 2.0f * PI / N) - PI / 2.0f - (PI / N);
				float angle_end = ((i + 1) * 2.0f * PI / N) - PI / 2.0f - (PI / N);
				
				bool is_hovered = (hovered_slice == i);
				
				ImU32 fill_color = is_hovered ? IM_COL32(180, 140, 50, 180) : IM_COL32(12, 16, 26, 235);
				ImU32 border_color = is_hovered ? IM_COL32(255, 220, 100, 255) : IM_COL32(180, 140, 50, 100);
				
				// Fill segment
				draw_list->PathClear();
				draw_list->PathArcTo(center, r_max, angle_start, angle_end, 16);
				draw_list->PathArcTo(center, r_min, angle_end, angle_start, 16);
				draw_list->PathFillConvex(fill_color);
				
				// Stroke borders
				draw_list->PathClear();
				draw_list->PathArcTo(center, r_max, angle_start, angle_end, 16);
				draw_list->PathArcTo(center, r_min, angle_end, angle_start, 16);
				draw_list->PathStroke(border_color, ImDrawFlags_Closed, is_hovered ? 1.5f : 1.0f);
				
				// Draw separator line
				draw_list->AddLine(
					ImVec2(center.x + r_min * std::cos(angle_start), center.y + r_min * std::sin(angle_start)),
					ImVec2(center.x + r_max * std::cos(angle_start), center.y + r_max * std::sin(angle_start)),
					IM_COL32(180, 140, 50, 60), 1.0f
				);
				
				// 3. Render texture icon
				float angle_mid = (angle_start + angle_end) / 2.0f;
				float r_mid = (r_min + r_max) / 2.0f;
				ImVec2 icon_pos(center.x + r_mid * std::cos(angle_mid), center.y + r_mid * std::sin(angle_mid));
				
				ImU32 icon_color = is_hovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(245, 215, 120, 255);
				
				if (radial_tex_ids[i] != 0) {
					draw_list->AddImage(
						(ImTextureID)(intptr_t)radial_tex_ids[i],
						ImVec2(icon_pos.x - 12.0f, icon_pos.y - 12.0f),
						ImVec2(icon_pos.x + 12.0f, icon_pos.y + 12.0f),
						ImVec2(0, 0), ImVec2(1, 1),
						icon_color
					);
				}
			}
			
			// 4. Draw central circle
			draw_list->AddCircleFilled(center, r_min - 2.0f, IM_COL32(8, 10, 18, 255));
			draw_list->AddCircle(center, r_min - 2.0f, IM_COL32(180, 140, 50, 255), 64, 2.0f);
			
			// 5. Draw center text label
			std::string center_text = "TOOLS";
			if (hovered_slice >= 0 && hovered_slice < N) {
				center_text = tools[hovered_slice].label;
			}
			
			if (hovered_slice >= 0) {
				draw_list->AddCircle(center, r_min - 5.0f, IM_COL32(180, 140, 50, 40), 64, 1.0f);
			}
			
			ImVec2 text_size = ImGui::CalcTextSize(center_text.c_str());
			draw_list->AddText(
				ImVec2(center.x - text_size.x * 0.5f, center.y - text_size.y * 0.5f),
				IM_COL32(180, 140, 50, 255),
				center_text.c_str()
			);
		}
		ImGui::End();
	}
	if (rubber_band_mode) {
		ImGuiViewport* vp = ImGui::GetMainViewport();
		ImVec2 p_min = ImVec2(vp->Pos.x + std::min(rubber_start_x, rubber_end_x), vp->Pos.y + std::min(rubber_start_y, rubber_end_y));
		ImVec2 p_max = ImVec2(vp->Pos.x + std::max(rubber_start_x, rubber_end_x), vp->Pos.y + std::max(rubber_start_y, rubber_end_y));
		
		ImDrawList* draw_list = ImGui::GetForegroundDrawList();
		draw_list->AddRectFilled(p_min, p_max, IM_COL32(255, 215, 0, 45));
		draw_list->AddRect(p_min, p_max, IM_COL32(255, 215, 0, 220), 0.0f, 0, 1.5f);
	}

	// Rendering stacked speech bubbles / tooltips for sign texts, teleport destinations, creature names, action/unique IDs
	if (g_settings.getBoolean(Config::SHOW_TEXT_BUBBLES) && g_gui.IsRenderingEnabled()) {
		ImGuiIO& io = ImGui::GetIO();
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(io.DisplaySize);
		ImGui::SetNextWindowBgAlpha(0.0f);
		ImGuiWindowFlags tooltip_overlay_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground;
		if (ImGui::Begin("##MapTooltipBubbleOverlay", nullptr, tooltip_overlay_flags)) {
			int scroll_x = 0, scroll_y = 0;
			if (GetParent()) {
				static_cast<MapWindow*>(GetParent())->GetViewStart(&scroll_x, &scroll_y);
			}
			ImVec2 win_pos = ImGui::GetWindowPos();
			int offset = (floor <= 7) ? (7 - floor) * TileSize : 0;
			int w, h;
			GetClientSize(&w, &h);

			int start_map_x = (scroll_x + offset) / TileSize - 1;
			int start_map_y = (scroll_y + offset) / TileSize - 1;
			int end_map_x = start_map_x + (int)(w * zoom) / TileSize + 3;
			int end_map_y = start_map_y + (int)(h * zoom) / TileSize + 3;

			start_map_x = std::max(0, start_map_x);
			start_map_y = std::max(0, start_map_y);
			end_map_x = std::min(editor.map.getWidth(), end_map_x);
			end_map_y = std::min(editor.map.getHeight(), end_map_y);

			for (int y = start_map_y; y < end_map_y; ++y) {
				for (int x = start_map_x; x < end_map_x; ++x) {
					Tile* tile = editor.map.getTile(x, y, floor);
					if (!tile) continue;

					struct BubbleData {
						std::string header;
						std::string content;
						ImVec4 header_color;
						ImVec4 border_color;
					};
					std::vector<BubbleData> bubbles;

					// 1. Sign / Book texts
					auto check_item_text = [&](Item* item) {
						if (item) {
							std::string text = item->getText();
							if (!text.empty()) {
								bubbles.push_back({ "Text", text, ImVec4(0.85f, 0.75f, 0.45f, 1.0f), ImVec4(0.7f, 0.55f, 0.2f, 1.0f) });
							}
						}
					};
					check_item_text(tile->ground);
					for (Item* item : tile->items) {
						check_item_text(item);
					}

					// 2. Teleport Destination
					for (Item* item : tile->items) {
						if (item) {
							if (Teleport* tp = dynamic_cast<Teleport*>(item)) {
								const Position& dest = tp->getDestination();
								std::string dest_str = std::to_string(dest.x) + ", " + std::to_string(dest.y) + ", " + std::to_string(dest.z);
								bubbles.push_back({ "Destination", dest_str, ImVec4(0.8f, 0.5f, 0.9f, 1.0f), ImVec4(0.6f, 0.3f, 0.7f, 1.0f) });
							}
						}
					}

					// 3. Action ID / Unique ID
					auto check_item_ids = [&](Item* item) {
						if (item) {
							uint16_t aid = item->getActionID();
							uint16_t uid = item->getUniqueID();
							if (aid > 0 || uid > 0) {
								std::string aid_str = aid > 0 ? "Action ID: " + std::to_string(aid) : "";
								std::string uid_str = uid > 0 ? "Unique ID: " + std::to_string(uid) : "";
								std::string content = aid_str + (aid > 0 && uid > 0 ? "\n" : "") + uid_str;
								bubbles.push_back({ "Attributes", content, ImVec4(0.9f, 0.6f, 0.4f, 1.0f), ImVec4(0.7f, 0.45f, 0.25f, 1.0f) });
							}
						}
					};
					check_item_ids(tile->ground);
					for (Item* item : tile->items) {
						check_item_ids(item);
					}

					// 4. Placed NPC / Monster name
					if (tile->creature) {
						bubbles.push_back({ "", tile->creature->getName(), ImVec4(0.6f, 0.8f, 1.0f, 1.0f), ImVec4(0.3f, 0.6f, 0.8f, 1.0f) });
					}

					if (bubbles.empty()) continue;

					// Compute screen position of the tile
					double tile_screen_x = win_pos.x + (((x * TileSize) - scroll_x) - offset) / zoom;
					double tile_screen_y = win_pos.y + (((y * TileSize) - scroll_y) - offset) / zoom;
					
					// Draw stacked bubbles starting above the tile
					float bubble_y = (float)tile_screen_y - 4.0f;
					float bubble_x = (float)tile_screen_x + (TileSize / 2.0f) / (float)zoom;

					ImDrawList* draw_list = ImGui::GetWindowDrawList();

					for (const auto& bubble : bubbles) {
						std::string text_to_draw = bubble.content;
						ImVec2 text_size = ImGui::CalcTextSize(text_to_draw.c_str());
						
						float header_height = 0.0f;
						ImVec2 header_size(0, 0);
						if (!bubble.header.empty()) {
							header_size = ImGui::CalcTextSize(bubble.header.c_str());
							header_height = header_size.y + 4.0f;
						}

						float box_w = std::max(text_size.x, header_size.x) + 12.0f;
						float box_h = text_size.y + header_height + 8.0f;

						float box_x1 = bubble_x - box_w / 2.0f;
						float box_y1 = bubble_y - box_h;
						float box_x2 = bubble_x + box_w / 2.0f;
						float box_y2 = bubble_y;

						// Draw shadow
						draw_list->AddRectFilled(ImVec2(box_x1 + 2.0f, box_y1 + 2.0f), ImVec2(box_x2 + 2.0f, box_y2 + 2.0f), IM_COL32(0, 0, 0, 120), 4.0f);

						// Draw translucent background (glassmorphic dark look)
						draw_list->AddRectFilled(ImVec2(box_x1, box_y1), ImVec2(box_x2, box_y2), IM_COL32(20, 22, 28, 225), 4.0f);

						// Draw border
						ImU32 border_col = IM_COL32((int)(bubble.border_color.x * 255), (int)(bubble.border_color.y * 255), (int)(bubble.border_color.z * 255), 255);
						draw_list->AddRect(ImVec2(box_x1, box_y1), ImVec2(box_x2, box_y2), border_col, 4.0f, 0, 1.0f);

						// Draw header if present
						float current_y = box_y1 + 4.0f;
						if (!bubble.header.empty()) {
							ImU32 header_col = IM_COL32((int)(bubble.header_color.x * 255), (int)(bubble.header_color.y * 255), (int)(bubble.header_color.z * 255), 255);
							draw_list->AddText(ImVec2(box_x1 + 6.0f, current_y), header_col, bubble.header.c_str());
							
							// Draw separator line under header
							draw_list->AddLine(ImVec2(box_x1 + 4.0f, current_y + header_size.y + 1.0f), ImVec2(box_x2 - 4.0f, current_y + header_size.y + 1.0f), border_col, 0.8f);
							current_y += header_height;
						}

						// Draw body text
						draw_list->AddText(ImVec2(box_x1 + 6.0f, current_y), IM_COL32(240, 240, 240, 255), text_to_draw.c_str());

						bubble_y -= box_h + 4.0f;
					}
				}
			}
			ImGui::End();
		}
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
	
	g_gui.RefreshMinimapPanel();
	
	
	PerformanceLogger::EndFrame();
}

int MapCanvas::GetHoveredRadialSlice() const {
	if (!tool_wheel_open) return -1;
	
	float dx = cursor_x - tool_wheel_x;
	float dy = cursor_y - tool_wheel_y;
	float dist = std::sqrt(dx * dx + dy * dy);
	
	const float r_min = 45.0f;
	const float r_max = 145.0f;
	
	if (dist < r_min || dist > r_max) {
		return -1;
	}
	
	float angle = std::atan2(dy, dx);
	if (angle < 0) angle += 2.0f * PI;
	
	const int N = 10;
	float adjusted_angle = angle + PI / 2.0f + (PI / N);
	if (adjusted_angle >= 2.0f * PI) adjusted_angle -= 2.0f * PI;
	
	int slice = (int)(adjusted_angle / (2.0f * PI / N)) % N;
	return slice;
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

void MapCanvas::LoadRadialTextures() {
	if (radial_textures_loaded) return;
	
	wxSize size = wxSize(32, 32);
	
	// Selection
	wxBitmap pointer_bmp = LoadBitmapFromCandidatesRadial(size, {
		"icons/pointer.png", "../icons/pointer.png", "Map Editor/icons/pointer.png", "../Map Editor/icons/pointer.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "pointer.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "pointer.png"
	});
	radial_tex_ids[0] = ConvertBitmapToTexture(pointer_bmp);
	
	// Pencil
	wxBitmap pencil_bmp = LoadBitmapFromCandidatesRadial(size, {
		"icons/pencil.png", "../icons/pencil.png", "Map Editor/icons/pencil.png", "../Map Editor/icons/pencil.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "pencil.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "pencil.png"
	});
	radial_tex_ids[1] = ConvertBitmapToTexture(pencil_bmp);
	
	// Bucket
	wxBitmap bucket_bmp = LoadBitmapFromCandidatesRadial(size, {
		"icons/bucket.png", "../icons/bucket.png", "Map Editor/icons/bucket.png", "../Map Editor/icons/bucket.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "bucket.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "bucket.png"
	});
	radial_tex_ids[2] = ConvertBitmapToTexture(bucket_bmp);
	
	// Protection Zone (Shield)
	wxBitmap pz_bmp = LoadBitmapFromCandidatesRadial(size, {
		"icons/protected_zone.png", "../icons/protected_zone.png", "Map Editor/icons/protected_zone.png", "../Map Editor/icons/protected_zone.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "protected_zone.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "protected_zone.png"
	});
	if (!pz_bmp.IsOk()) {
		pz_bmp = wxArtProvider::GetBitmap(ART_PZ_BRUSH, wxART_TOOLBAR, size);
	}
	radial_tex_ids[3] = ConvertBitmapToTexture(pz_bmp);
	
	// Normal Door
	wxBitmap normal_door_bmp = wxArtProvider::GetBitmap(ART_DOOR_NORMAL_SMALL, wxART_TOOLBAR, size);
	radial_tex_ids[4] = ConvertBitmapToTexture(normal_door_bmp);
	
	// Locked Door
	wxBitmap locked_door_bmp = wxArtProvider::GetBitmap(ART_DOOR_LOCKED_SMALL, wxART_TOOLBAR, size);
	radial_tex_ids[5] = ConvertBitmapToTexture(locked_door_bmp);
	
	// Magic Door
	wxBitmap magic_door_bmp = wxArtProvider::GetBitmap(ART_DOOR_MAGIC_SMALL, wxART_TOOLBAR, size);
	radial_tex_ids[6] = ConvertBitmapToTexture(magic_door_bmp);
	
	// Hatch Window
	wxBitmap hatch_bmp = _wxGetBitmapFromMemoryRadial(window_hatch_small_png, sizeof(window_hatch_small_png), size);
	radial_tex_ids[7] = ConvertBitmapToTexture(hatch_bmp);
	
	// Eraser
	wxBitmap eraser_bmp = _wxGetBitmapFromMemoryRadial(eraser_small_png, sizeof(eraser_small_png), size);
	radial_tex_ids[8] = ConvertBitmapToTexture(eraser_bmp);
	
	// Prefab Creator (Blueprint)
	wxBitmap prefab_bmp = LoadBitmapFromCandidatesRadial(size, {
		"icons/prefab.png", "../icons/prefab.png", "Map Editor/icons/prefab.png", "../Map Editor/icons/prefab.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "prefab.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "prefab.png"
	});
	radial_tex_ids[9] = ConvertBitmapToTexture(prefab_bmp);
	
	radial_textures_loaded = true;
}
