#include "main.h"
#include "map_display.h"
#include "map_window.h"
#include "map_drawer.h"
#include "gui.h"
#include "editor.h"
#include "settings.h"
#include "complexitem.h"
#include "creature.h"
#include "town.h"
#include "live_server.h"
#include "live_socket.h"
#include "live_client.h"
#include "live_peer.h"
#include "wall_brush.h"
#include "carpet_brush.h"
#include "table_brush.h"
#include "doodad_brush.h"
#include "brush.h"
#include "ground_brush.h"
#include "raw_brush.h"
#include "spawn_brush.h"
#include "creature_brush.h"
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
	wxPaintDC dc(this); // Must always be created in EVT_PAINT to validate the region
	if (!drawer) {
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
#ifdef __WINDOWS__
	SetVSync(g_settings.getBoolean(Config::V_SYNC));
#endif
	static bool auto_scaled = false;
	if (!auto_scaled) {
		AutoScalePerformanceSettings();
		auto_scaled = true;
	}

	if (!imgui_context) {
		IMGUI_CHECKVERSION();
		imgui_context = ImGui::CreateContext();
		ImGui::SetCurrentContext(imgui_context);
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
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

		ImGui_ImplOpenGL3_Init(nullptr);
	} else {
		ImGui::SetCurrentContext(imgui_context);
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
	if (!tool_wheel_open && !screendragging && !boundbox_selection && g_gui.GetCurrentBrush() && !g_gui.IsSelectionMode()) {
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
	if (editor.IsLive() && g_settings.getBoolean(Config::SHOW_CHAT)) {
		static bool chat_minimized = false;
		static bool chat_docked_to_palette = false;
		static int last_seen_msg_count = 0;

		int total_msgs = (int)g_gui.chat_log.size();
		int unread_count = std::max(0, total_msgs - last_seen_msg_count);

		if (chat_minimized) {
			// Render a sleek button pill in the bottom status area
			ImGui::SetNextWindowPos(ImVec2(10, io.DisplaySize.y - 36), ImGuiCond_Always);
			ImGui::SetNextWindowBgAlpha(0.85f);
			ImGuiWindowFlags pill_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
				ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
			if (ImGui::Begin("##ChatMinimizedPill", nullptr, pill_flags)) {
				std::string label = unread_count > 0 
					? "💬 Team Chat (" + std::to_string(unread_count) + " new)"
					: "💬 Team Chat";
				
				if (unread_count > 0) {
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.45f, 0.1f, 0.9f));
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
				}
				if (ImGui::Button(label.c_str())) {
					chat_minimized = false;
					last_seen_msg_count = (int)g_gui.chat_log.size();
				}
				if (unread_count > 0) {
					ImGui::PopStyleColor(2);
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Click to restore Team Chat window");
				}
				ImGui::End();
			}
		} else {
			last_seen_msg_count = total_msgs;

			if (chat_docked_to_palette) {
				ImGui::SetNextWindowPos(ImVec2(std::max(10.0f, io.DisplaySize.x - 330.0f), std::max(10.0f, io.DisplaySize.y - 250.0f)), ImGuiCond_Always);
				ImGui::SetNextWindowSize(ImVec2(320, 240), ImGuiCond_Always);
			} else {
				ImGui::SetNextWindowPos(ImVec2(10, io.DisplaySize.y - 250), ImGuiCond_FirstUseEver);
				ImGui::SetNextWindowSize(ImVec2(320, 240), ImGuiCond_FirstUseEver);
			}
			ImGui::SetNextWindowSizeConstraints(ImVec2(220, 140), ImVec2(800, 600));

			bool open = true;
			if (ImGui::Begin("Team Chat", &open, ImGuiWindowFlags_None)) {
				// Header quick buttons
				ImGui::SameLine(ImGui::GetWindowWidth() - 95);
				if (ImGui::SmallButton(chat_docked_to_palette ? "⚓ Float" : "📌 Dock")) {
					chat_docked_to_palette = !chat_docked_to_palette;
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip(chat_docked_to_palette ? "Switch to free-floating window" : "Dock to right palette area");
				}
				ImGui::SameLine();
				if (ImGui::SmallButton(" _ ")) {
					chat_minimized = true;
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Minimize to bottom status bar");
				}

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
							if (lat <= 1) {
								ImGui::TextColored(col, " < 1 ms | %u%% loss | %s", peer->getPacketLoss(), nstr(peer->getConnectionStatus()).c_str());
							} else {
								ImGui::TextColored(col, " %u ms | %u%% loss | %s", lat, peer->getPacketLoss(), nstr(peer->getConnectionStatus()).c_str());
							}
						}
						ImGui::EndTooltip();
					}
				} else {
					LiveClient* client = editor.GetLiveClient();
					uint32_t lat = client->getLatency();
					ImVec4 col = (lat < 100) ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : (lat < 250 ? ImVec4(1.0f, 1.0f, 0.2f, 1.0f) : ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
					ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Join Mode | ");
					ImGui::SameLine();
					if (lat <= 1) {
						ImGui::TextColored(col, "%s | < 1 ms | %u%% loss", nstr(client->getConnectionStatus()).c_str(), client->getPacketLoss());
					} else {
						ImGui::TextColored(col, "%s | %u ms | %u%% loss", nstr(client->getConnectionStatus()).c_str(), lat, client->getPacketLoss());
					}
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
			}
			ImGui::End();

			if (!open) {
				g_settings.setInteger(Config::SHOW_CHAT, 0);
				if (g_gui.root) {
					g_gui.root->UpdateMenubar();
				}
			}
		}
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
				uint32_t icon_id;
			};
			
			std::vector<RadialTool> tools;
			if (tool_wheel_sub_menu == 1) { // Zones Sub-Menu
				tools = {
					{"PROTECTION ZONE", radial_tex_ids[4]},
					{"NO LOGOUT ZONE", radial_tex_ids[9]},
					{"NO PVP ZONE", radial_tex_ids[10]},
					{"PVP ZONE", radial_tex_ids[11]},
					{"BACK", radial_tex_ids[12]}
				};
			} else if (tool_wheel_sub_menu == 2) { // Doors Sub-Menu
				tools = {
					{"NORMAL DOOR", radial_tex_ids[5]},
					{"LOCKED DOOR", radial_tex_ids[5]},
					{"MAGIC DOOR", radial_tex_ids[5]},
					{"QUEST DOOR", radial_tex_ids[5]},
					{"BACK", radial_tex_ids[12]}
				};
			} else if (tool_wheel_sub_menu == 3) { // Windows Sub-Menu
				tools = {
					{"HATCH WINDOW", radial_tex_ids[8]},
					{"WINDOW", radial_tex_ids[8]},
					{"BACK", radial_tex_ids[12]}
				};
			} else { // Main Wheel
				tools = {
					{"SELECTION", radial_tex_ids[0]},
					{"PENCIL", radial_tex_ids[1]},
					{"BUCKET", radial_tex_ids[2]},
					{"ZONES", radial_tex_ids[4]},
					{"DOORS", radial_tex_ids[5]},
					{"WINDOWS", radial_tex_ids[8]},
					{"ERASER", radial_tex_ids[6]},
					{"PREFAB CREATOR", radial_tex_ids[7]}
				};
			}
			
			const int N = tools.size();
			ImVec2 mouse_pos = ImGui::GetMousePos();
			
			int scroll_x = 0, scroll_y = 0;
			if (GetParent()) {
				static_cast<MapWindow*>(GetParent())->GetViewStart(&scroll_x, &scroll_y);
			}
			int offset = (tool_wheel_tile_z <= 7) ? (7 - tool_wheel_tile_z) * TileSize : 0;
			float tile_cx = (((tool_wheel_tile_x + 0.5f) * TileSize - scroll_x) - offset) / zoom;
			float tile_cy = (((tool_wheel_tile_y + 0.5f) * TileSize - scroll_y) - offset) / zoom;
			ImVec2 center(tile_cx, tile_cy);
			
			float dx = mouse_pos.x - center.x;
			float dy = mouse_pos.y - center.y;
			float dist = std::sqrt(dx * dx + dy * dy);
			
			int hovered_slice = GetHoveredRadialSlice();
			
			// 1. Draw outer glowing ring (shadow)
			draw_list->AddCircle(center, r_max + 1.0f, IM_COL32(0, 0, 0, 120), 64, 4.0f);
			
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
				
				if (tools[i].icon_id != 0) {
					draw_list->AddImage(
						(ImTextureID)(intptr_t)tools[i].icon_id,
						ImVec2(icon_pos.x - 14.0f, icon_pos.y - 14.0f),
						ImVec2(icon_pos.x + 14.0f, icon_pos.y + 14.0f),
						ImVec2(0, 0), ImVec2(1, 1),
						icon_color
					);
				}
			}
			
			// 4. Draw central circle outline (Hollow center hole so highlighted map tile field is visible)
			draw_list->AddCircle(center, r_min - 1.0f, IM_COL32(255, 215, 80, 220), 64, 2.0f);
			draw_list->AddCircle(center, r_min - 4.0f, IM_COL32(180, 140, 50, 120), 64, 1.0f);

			// 5. Combined 2-Rectangle Description Box at Top-Center above the Wheel
			std::string selected_label = "SELECT TOOL";
			if (hovered_slice >= 0 && hovered_slice < N) {
				selected_label = tools[hovered_slice].label;
			} else {
				if (tool_wheel_sub_menu == 1) selected_label = "ZONES MENU";
				else if (tool_wheel_sub_menu == 2) selected_label = "DOORS MENU";
				else if (tool_wheel_sub_menu == 3) selected_label = "WINDOWS MENU";
				else selected_label = "MAIN TOOLS";
			}

			ImVec2 text_sz = ImGui::CalcTextSize(selected_label.c_str());
			float box_w = std::max(220.0f, text_sz.x + 48.0f);
			float box_h = 38.0f;
			float box_x = center.x - box_w * 0.5f;
			float box_y = center.y - r_max - 48.0f;

			// Rectangle 1: Outer Container Box
			ImVec2 r1_min(box_x, box_y);
			ImVec2 r1_max(box_x + box_w, box_y + box_h);
			draw_list->AddRectFilled(r1_min, r1_max, IM_COL32(10, 14, 24, 245), 6.0f);
			draw_list->AddRect(r1_min, r1_max, IM_COL32(180, 140, 50, 180), 6.0f, 0, 1.5f);

			// Rectangle 2: Inner Combined Accent Pill Box
			ImVec2 r2_min(box_x + 3.0f, box_y + 3.0f);
			ImVec2 r2_max(box_x + box_w - 3.0f, box_y + box_h - 3.0f);
			ImU32 r2_bg = (hovered_slice >= 0) ? IM_COL32(45, 35, 15, 230) : IM_COL32(18, 24, 38, 220);
			ImU32 r2_border = (hovered_slice >= 0) ? IM_COL32(255, 215, 80, 240) : IM_COL32(140, 110, 40, 140);
			draw_list->AddRectFilled(r2_min, r2_max, r2_bg, 4.0f);
			draw_list->AddRect(r2_min, r2_max, r2_border, 4.0f, 0, 1.0f);

			// Description text centered inside the combined rectangle box
			draw_list->AddText(
				ImVec2(center.x - text_sz.x * 0.5f, box_y + (box_h - text_sz.y) * 0.5f),
				IM_COL32(255, 225, 120, 255),
				selected_label.c_str()
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
	if ((g_settings.getBoolean(Config::SHOW_TOOLTIPS) || g_settings.getBoolean(Config::SHOW_TEXT_BUBBLES)) && g_gui.IsRenderingEnabled()) {
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

					struct ContainerItemInfo {
						uint16_t id;
						int count;
					};

					struct BubbleData {
						std::string header;
						std::string content;
						ImVec4 header_color;
						ImVec4 border_color;
						std::vector<ContainerItemInfo> container_items;
					};
					std::vector<BubbleData> bubbles;

					// 1. Containers / Chests with items inside
					auto check_container = [&](Item* item) {
						if (item && item->isContainer()) {
							// Filter out corpses, dead bodies, and slain monster remains
							if (item->typeExists()) {
								const std::string& lname = as_lower_str(item->getName());
								if (lname.find("corpse") != std::string::npos ||
									lname.find("dead") != std::string::npos ||
									lname.find("slain") != std::string::npos ||
									lname.find("remains") != std::string::npos ||
									lname.find("body") != std::string::npos) {
									return;
								}
							}

							Container* c = dynamic_cast<Container*>(item);
							if (c) {
								// Only show pop-up if container holds items or has action/unique ID
								if (c->getItemCount() == 0 && c->getActionID() == 0 && c->getUniqueID() == 0) {
									return;
								}

								BubbleData b;
								b.header = "Chest";
								b.header_color = ImVec4(0.95f, 0.70f, 0.30f, 1.0f); // Warm Gold/Amber
								b.border_color = ImVec4(0.85f, 0.55f, 0.20f, 1.0f);

								std::string content;
								uint16_t aid = c->getActionID();
								uint16_t uid = c->getUniqueID();
								if (aid > 0) {
									content += "Action ID: " + std::to_string(aid);
								}
								if (uid > 0) {
									if (!content.empty()) content += "\n";
									content += "Unique ID: " + std::to_string(uid);
								}
								b.content = content;

								for (size_t i = 0; i < c->getItemCount(); ++i) {
									Item* sub = c->getItem(i);
									if (sub) {
										b.container_items.push_back({ sub->getID(), sub->getCount() });
									}
								}
								bubbles.push_back(b);
							}
						}
					};
					check_container(tile->ground);
					for (Item* item : tile->items) {
						check_container(item);
					}

					// 2. Teleport Destination
					for (Item* item : tile->items) {
						if (item) {
							if (Teleport* tp = dynamic_cast<Teleport*>(item)) {
								const Position& dest = tp->getDestination();
								std::string dest_str = "Destination: " + std::to_string(dest.x) + ", " + std::to_string(dest.y) + ", " + std::to_string(dest.z);
								bubbles.push_back({ "Teleport", dest_str, ImVec4(0.35f, 0.75f, 0.95f, 1.0f), ImVec4(0.20f, 0.55f, 0.85f, 1.0f) });
							}
						}
					}

					// 3. Action ID / Unique ID (for non-containers)
					auto check_item_ids = [&](Item* item) {
						if (item && !item->isContainer()) {
							uint16_t aid = item->getActionID();
							uint16_t uid = item->getUniqueID();
							if (aid > 0 || uid > 0) {
								std::string aid_str = aid > 0 ? "Action ID: " + std::to_string(aid) : "";
								std::string uid_str = uid > 0 ? "Unique ID: " + std::to_string(uid) : "";
								std::string content = aid_str + (aid > 0 && uid > 0 ? "\n" : "") + uid_str;
								bubbles.push_back({ "Attributes", content, ImVec4(0.9f, 0.65f, 0.35f, 1.0f), ImVec4(0.75f, 0.45f, 0.2f, 1.0f) });
							}
						}
					};
					check_item_ids(tile->ground);
					for (Item* item : tile->items) {
						check_item_ids(item);
					}

					// 4. Sign / Book texts
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

					// 5. Town Spawn / Temple Name
					if (g_settings.getBoolean(Config::SHOW_TOWNS) && tile->isTownExit(editor.map)) {
						for (const auto& pair : editor.map.towns) {
							Town* town = pair.second;
							if (town && town->getTemplePosition() == tile->getPosition()) {
								std::string town_name = town->getName();
								if (town_name.empty()) {
									town_name = "Town #" + std::to_string(town->getID());
								}
								bubbles.push_back({ "Town Spawn", town_name, ImVec4(0.4f, 0.9f, 0.55f, 1.0f), ImVec4(0.25f, 0.7f, 0.35f, 1.0f) });
								break;
							}
						}
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
						ImVec2 text_size(0, 0);
						if (!text_to_draw.empty()) {
							text_size = ImGui::CalcTextSize(text_to_draw.c_str());
						}

						float header_height = 0.0f;
						ImVec2 header_size(0, 0);
						if (!bubble.header.empty()) {
							header_size = ImGui::CalcTextSize(bubble.header.c_str());
							header_height = header_size.y + 4.0f;
						}

						// Calculate Grid dimensions for Container Items
						float icon_size = 28.0f;
						float icon_gap = 4.0f;
						int items_per_row = 5;
						float grid_w = 0.0f;
						float grid_h = 0.0f;

						if (!bubble.container_items.empty()) {
							int total = (int)bubble.container_items.size();
							int cols = std::min(total, items_per_row);
							int rows = (total + items_per_row - 1) / items_per_row;
							grid_w = cols * icon_size + (cols - 1) * icon_gap;
							grid_h = rows * icon_size + (rows - 1) * icon_gap;
						}

						float box_w = std::max({ text_size.x, header_size.x, grid_w }) + 16.0f;
						float content_h = text_size.y + (text_size.y > 0.0f ? 4.0f : 0.0f) + (grid_h > 0.0f ? grid_h + 6.0f : 0.0f);
						float box_h = header_height + content_h + 8.0f;

						float box_x1 = bubble_x - box_w / 2.0f;
						float box_y1 = bubble_y - box_h;
						float box_x2 = bubble_x + box_w / 2.0f;
						float box_y2 = bubble_y;

						// Draw shadow
						draw_list->AddRectFilled(ImVec2(box_x1 + 2.5f, box_y1 + 2.5f), ImVec2(box_x2 + 2.5f, box_y2 + 2.5f), IM_COL32(0, 0, 0, 140), 5.0f);

						// Draw translucent background (glassmorphic dark look)
						draw_list->AddRectFilled(ImVec2(box_x1, box_y1), ImVec2(box_x2, box_y2), IM_COL32(18, 20, 26, 235), 5.0f);

						// Draw border
						ImU32 border_col = IM_COL32((int)(bubble.border_color.x * 255), (int)(bubble.border_color.y * 255), (int)(bubble.border_color.z * 255), 230);
						draw_list->AddRect(ImVec2(box_x1, box_y1), ImVec2(box_x2, box_y2), border_col, 5.0f, 0, 1.2f);

						float current_y = box_y1 + 4.0f;

						// Draw header if present
						if (!bubble.header.empty()) {
							ImU32 header_col = IM_COL32((int)(bubble.header_color.x * 255), (int)(bubble.header_color.y * 255), (int)(bubble.header_color.z * 255), 255);
							draw_list->AddText(ImVec2(box_x1 + 8.0f, current_y), header_col, bubble.header.c_str());

							// Draw separator line under header
							draw_list->AddLine(ImVec2(box_x1 + 6.0f, current_y + header_size.y + 2.0f), ImVec2(box_x2 - 6.0f, current_y + header_size.y + 2.0f), IM_COL32(80, 85, 100, 140), 1.0f);
							current_y += header_height;
						}

						// Draw body text if present
						if (!text_to_draw.empty()) {
							draw_list->AddText(ImVec2(box_x1 + 8.0f, current_y), IM_COL32(235, 235, 240, 255), text_to_draw.c_str());
							current_y += text_size.y + 4.0f;
						}

						// Draw Container Items Grid if present
						if (!bubble.container_items.empty()) {
							float start_grid_x = box_x1 + 8.0f;
							float start_grid_y = current_y + 2.0f;
							for (size_t i = 0; i < bubble.container_items.size(); ++i) {
								int r = (int)i / items_per_row;
								int c = (int)i % items_per_row;
								float ix1 = start_grid_x + c * (icon_size + icon_gap);
								float iy1 = start_grid_y + r * (icon_size + icon_gap);
								float ix2 = ix1 + icon_size;
								float iy2 = iy1 + icon_size;

								// Dark background slot for each item
								draw_list->AddRectFilled(ImVec2(ix1, iy1), ImVec2(ix2, iy2), IM_COL32(10, 12, 16, 220), 3.0f);
								draw_list->AddRect(ImVec2(ix1, iy1), ImVec2(ix2, iy2), IM_COL32(65, 70, 85, 180), 3.0f, 0, 1.0f);

								uint16_t item_id = bubble.container_items[i].id;
								int item_count = bubble.container_items[i].count;

								if (g_items.typeExists(item_id) && g_items[item_id].sprite) {
									GLuint texid = g_items[item_id].sprite->getHardwareID(0, 0, 0, -1, 0, 0, 0, 0);
									if (texid != 0) {
										draw_list->AddImage((ImTextureID)texid, ImVec2(ix1 + 2.0f, iy1 + 2.0f), ImVec2(ix2 - 2.0f, iy2 - 2.0f));
									}
								}

								// Count badge if > 1
								if (item_count > 1) {
									std::string count_str = "+" + std::to_string(item_count);
									ImVec2 csize = ImGui::CalcTextSize(count_str.c_str());
									float tx = ix2 - csize.x - 2.0f;
									float ty = iy2 - csize.y - 1.0f;
									draw_list->AddRectFilled(ImVec2(tx - 1.0f, ty - 1.0f), ImVec2(ix2, iy2), IM_COL32(0, 0, 0, 200), 2.0f);
									draw_list->AddText(ImVec2(tx, ty), IM_COL32(255, 215, 0, 255), count_str.c_str());
								}
							}
						}

						bubble_y -= box_h + 4.0f;
					}
				}
			}
			ImGui::End();
		}
	}

	// Render Pings & Map Notes on the canvas
	{
		ImGuiViewport* vp = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(vp->Pos);
		ImGui::SetNextWindowSize(vp->Size);
		ImGui::SetNextWindowViewport(vp->ID);
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoMouseInputs;

		if (ImGui::Begin("##MapNotesOverlay", nullptr, flags)) {
			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			int scroll_x = 0, scroll_y = 0;
			if (GetParent()) {
				static_cast<MapWindow*>(GetParent())->GetViewStart(&scroll_x, &scroll_y);
			}
			int offset = (floor <= 7) ? (7 - floor) * TileSize : 0;
			uint32_t now_ms = wxGetLocalTimeMillis().GetValue();

			// 1. Render Animated Pings
			for (auto it = active_pings.begin(); it != active_pings.end();) {
				float dt = (now_ms - it->start_time_ms) / 1000.0f;
				if (dt > 2.5f) {
					it = active_pings.erase(it);
					continue;
				}

				if (it->pos.z == floor) {
					float px = (((it->pos.x * TileSize - scroll_x) - offset) / zoom) + (TileSize * 0.5f / zoom);
					float py = (((it->pos.y * TileSize - scroll_y) - offset) / zoom) + (TileSize * 0.5f / zoom);

					float progress = dt / 2.5f;
					float r1 = 8.0f + 45.0f * progress;
					float r2 = 14.0f + 70.0f * progress;
					int alpha1 = static_cast<int>(255 * (1.0f - progress));
					int alpha2 = static_cast<int>(180 * (1.0f - progress));

					draw_list->AddCircle(ImVec2(px, py), r1, IM_COL32(0, 230, 255, alpha1), 32, 2.5f);
					draw_list->AddCircle(ImVec2(px, py), r2, IM_COL32(255, 215, 0, alpha2), 32, 1.8f);
					draw_list->AddCircleFilled(ImVec2(px, py), 5.0f, IM_COL32(255, 255, 255, alpha1));
					draw_list->AddText(ImVec2(px + 8.0f, py - 8.0f), IM_COL32(0, 255, 200, alpha1), "PING");
				}
				++it;
			}

			// 2. Render Map Notes with '!' symbol
			for (const auto& note : editor.map_notes) {
				if (note.pos.z != floor) continue;

				float nx = (((note.pos.x * TileSize - scroll_x) - offset) / zoom);
				float ny = (((note.pos.y * TileSize - scroll_y) - offset) / zoom);

				if (nx < -150 || ny < -50 || nx > vp->Size.x + 150 || ny > vp->Size.y + 50) continue;

				std::string badge_text = "Note: " + std::string(note.text.mb_str());
				if (badge_text.length() > 36) badge_text = badge_text.substr(0, 33) + "...";
				ImVec2 tsize = ImGui::CalcTextSize(badge_text.c_str());

				float bx1 = nx;
				float by1 = ny - tsize.y - 8.0f;
				float bx2 = bx1 + tsize.x + 28.0f;
				float by2 = by1 + tsize.y + 8.0f;

				// Background card
				draw_list->AddRectFilled(ImVec2(bx1, by1), ImVec2(bx2, by2), IM_COL32(15, 23, 42, 235), 4.0f);
				draw_list->AddRect(ImVec2(bx1, by1), ImVec2(bx2, by2), IM_COL32(245, 158, 11, 240), 4.0f, 0, 1.5f);

				// Amber '!' badge circle
				float cx = bx1 + 10.0f;
				float cy = by1 + (by2 - by1) * 0.5f;
				draw_list->AddCircleFilled(ImVec2(cx, cy), 6.5f, IM_COL32(245, 158, 11, 255));
				draw_list->AddText(ImVec2(cx - 2.5f, cy - 6.5f), IM_COL32(15, 23, 42, 255), "!");
				draw_list->AddText(ImVec2(bx1 + 22.0f, by1 + 4.0f), IM_COL32(248, 250, 252, 255), badge_text.c_str());
			}

			// 3. Render Waypoint Route Paths & Markers
			std::vector<std::pair<Position, std::string>> current_floor_wps;
			for (const auto& pair : editor.map.waypoints) {
				Waypoint* wp = pair.second;
				if (wp && wp->pos.z == floor) {
					current_floor_wps.push_back({wp->pos, wp->name});
				}
			}

			// Draw connecting route lines between consecutive waypoints
			if (current_floor_wps.size() >= 2) {
				for (size_t i = 0; i < current_floor_wps.size() - 1; ++i) {
					Position p1 = current_floor_wps[i].first;
					Position p2 = current_floor_wps[i + 1].first;

					float x1 = (((p1.x * TileSize - scroll_x) - offset) / zoom) + (TileSize * 0.5f / zoom);
					float y1 = (((p1.y * TileSize - scroll_y) - offset) / zoom) + (TileSize * 0.5f / zoom);
					float x2 = (((p2.x * TileSize - scroll_x) - offset) / zoom) + (TileSize * 0.5f / zoom);
					float y2 = (((p2.y * TileSize - scroll_y) - offset) / zoom) + (TileSize * 0.5f / zoom);

					// Draw dashed glowing route line
					draw_list->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), IM_COL32(59, 130, 246, 180), 2.5f);
				}
			}

			// Draw Waypoint nodes and labels
			for (const auto& wp_item : current_floor_wps) {
				float wx = (((wp_item.first.x * TileSize - scroll_x) - offset) / zoom) + (TileSize * 0.5f / zoom);
				float wy = (((wp_item.first.y * TileSize - scroll_y) - offset) / zoom) + (TileSize * 0.5f / zoom);

				if (wx < -100 || wy < -50 || wx > vp->Size.x + 100 || wy > vp->Size.y + 50) continue;

				// Glowing node
				draw_list->AddCircleFilled(ImVec2(wx, wy), 7.0f, IM_COL32(37, 99, 235, 230));
				draw_list->AddCircle(ImVec2(wx, wy), 9.0f, IM_COL32(147, 197, 253, 255), 16, 1.8f);

				// Waypoint Label
				std::string wp_name = wp_item.second;
				ImVec2 wsize = ImGui::CalcTextSize(wp_name.c_str());
				float lx1 = wx - wsize.x * 0.5f - 4.0f;
				float ly1 = wy - 22.0f;
				float lx2 = lx1 + wsize.x + 8.0f;
				float ly2 = ly1 + wsize.y + 2.0f;

				draw_list->AddRectFilled(ImVec2(lx1, ly1), ImVec2(lx2, ly2), IM_COL32(15, 23, 42, 220), 3.0f);
				draw_list->AddRect(ImVec2(lx1, ly1), ImVec2(lx2, ly2), IM_COL32(96, 165, 250, 200), 3.0f, 0, 1.0f);
				draw_list->AddText(ImVec2(lx1 + 4.0f, ly1 + 1.0f), IM_COL32(239, 246, 255, 255), wp_name.c_str());
			}

			// 4. Render On-Screen HUD Notification (e.g. Auto-Border Toggle)
			if (!hud_notification_text.empty()) {
				float elapsed_s = (now_ms - hud_notification_time_ms) / 1000.0f;
				if (elapsed_s < 2.5f) {
					float alpha = 1.0f;
					if (elapsed_s > 1.8f) {
						alpha = 1.0f - ((elapsed_s - 1.8f) / 0.7f);
					}
					int alpha_int = static_cast<int>(255 * alpha);

					ImVec2 txt_sz = ImGui::CalcTextSize(hud_notification_text.c_str());
					float n_w = txt_sz.x + 32.0f;
					float n_h = txt_sz.y + 16.0f;
					float n_x = (vp->Size.x - n_w) * 0.5f;
					float n_y = 48.0f;

					draw_list->AddRectFilled(ImVec2(n_x, n_y), ImVec2(n_x + n_w, n_y + n_h), IM_COL32(15, 23, 42, (int)(240 * alpha)), 8.0f);
					uint32_t base_col = (hud_notification_color != 0) ? hud_notification_color : 0xFFFBBF24;
					ImU32 border_col = IM_COL32((base_col >> 16) & 0xFF, (base_col >> 8) & 0xFF, base_col & 0xFF, alpha_int);
					draw_list->AddRect(ImVec2(n_x, n_y), ImVec2(n_x + n_w, n_y + n_h), border_col, 8.0f, 0, 2.0f);

					draw_list->AddText(ImVec2(n_x + 16.0f, n_y + 8.0f), border_col, hud_notification_text.c_str());
				} else {
					hud_notification_text.clear();
				}
			}

			ImGui::End();
		}
	}

	RenderCanvasContextMenu();

	ImGui::Render();
	{
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	// Swap buffer
	{
		SwapBuffers();
	}

	// Send new node requests
	editor.SendNodeRequests();
	
	g_gui.RefreshMinimapPanel();
}

void MapCanvas::RenderCanvasContextMenu() {
	if (!canvas_context_menu_open) return;

	if (canvas_context_menu_just_opened) {
		ImGui::OpenPopup("##CorporateCanvasContextMenu");
		canvas_context_menu_just_opened = false;
	}
	ImGui::SetNextWindowPos(ImVec2((float)canvas_context_menu_x, (float)canvas_context_menu_y), ImGuiCond_Appearing);

	// Style colors: Dark Sapphire (#0A1423 / #101C30), Gold (#B49632)
	ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.06f, 0.10f, 0.18f, 0.97f)); // Deep Sapphire #101C30
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.71f, 0.59f, 0.20f, 0.90f));  // Corporate Gold #B49632
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.12f, 0.20f, 0.35f, 0.80f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.18f, 0.30f, 0.50f, 0.95f)); // Highlight
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.71f, 0.59f, 0.20f, 0.80f));  // Gold Active
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.97f, 1.00f, 1.00f));
	ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.71f, 0.59f, 0.20f, 0.40f));     // Gold Separator

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 8.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 5.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 6.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.5f);

	if (ImGui::BeginPopup("##CorporateCanvasContextMenu")) {
		bool anything_selected = editor.selection.size() != 0;
		Tile* sel_tile = anything_selected ? editor.selection.getSelectedTile() : nullptr;
		if (!sel_tile) {
			sel_tile = editor.map.getTile(last_click_map_x, last_click_map_y, floor);
		}

		Item* topItem = nullptr;
		Item* topSelectedItem = nullptr;
		Creature* topCreature = sel_tile ? sel_tile->creature : nullptr;
		Spawn* topSpawn = sel_tile ? sel_tile->spawn : nullptr;
		bool hasWall = false;
		bool hasCarpet = false;
		bool hasTable = false;
		bool hasCollection = false;
		Brush* foundDoodadBrush = nullptr;
		Brush* foundDoorBrush = nullptr;

		if (sel_tile) {
			ItemVector selected_items = sel_tile->getSelectedItems();
			topSelectedItem = (selected_items.size() == 1 ? selected_items.back() : nullptr);
			for (auto* item : sel_tile->items) {
				if (item->isWall()) {
					Brush* wb = item->getWallBrush();
					if (wb && wb->visibleInPalette()) {
						hasWall = true;
						hasCollection = hasCollection || wb->hasCollection();
					}
				}
				if (item->isTable()) {
					Brush* tb = item->getTableBrush();
					if (tb && tb->visibleInPalette()) {
						hasTable = true;
						hasCollection = hasCollection || tb->hasCollection();
					}
				}
				if (item->isCarpet()) {
					Brush* cb = item->getCarpetBrush();
					if (cb && cb->visibleInPalette()) {
						hasCarpet = true;
						hasCollection = hasCollection || cb->hasCollection();
					}
				}
				if (Brush* db = item->getDoodadBrush()) {
					hasCollection = hasCollection || db->hasCollection();
				}
				if (item->isSelected()) {
					topItem = item;
				}
			}
			if (!topItem) topItem = sel_tile->ground;
			if (topSelectedItem && topSelectedItem->getDoodadBrush()) foundDoodadBrush = topSelectedItem->getDoodadBrush();
			else {
				for (auto it = sel_tile->items.rbegin(); it != sel_tile->items.rend(); ++it) {
					if ((*it)->getDoodadBrush()) { foundDoodadBrush = (*it)->getDoodadBrush(); break; }
				}
			}
			if (topSelectedItem && topSelectedItem->isBrushDoor() && topSelectedItem->getDoorBrush()) foundDoorBrush = topSelectedItem->getDoorBrush();
			else {
				for (auto it = sel_tile->items.rbegin(); it != sel_tile->items.rend(); ++it) {
					if ((*it)->isBrushDoor() && (*it)->getDoorBrush()) { foundDoorBrush = (*it)->getDoorBrush(); break; }
				}
			}
		}

		Item* rotatableItem = topSelectedItem ? topSelectedItem : topItem;

		auto MenuItemStyled = [](const char* label, const char* shortcut, bool enabled = true) -> bool {
			return ImGui::MenuItem(label, shortcut, false, enabled);
		};

		// 1. Brush Selection (Primary Quick Pickers at Very Top)
		bool has_brushes = (hasWall || hasCarpet || hasTable || foundDoodadBrush || foundDoorBrush || topItem || topCreature || topSpawn || (sel_tile && sel_tile->hasGround()));
		if (has_brushes) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.84f, 0.32f, 1.0f)); // Radiant Gold Accent

			if (sel_tile && sel_tile->hasGround() && sel_tile->getGroundBrush() && sel_tile->getGroundBrush()->visibleInPalette()) {
				if (MenuItemStyled("Select Groundbrush", "")) {
					wxCommandEvent ev; OnSelectGroundBrush(ev);
					canvas_context_menu_open = false; ImGui::CloseCurrentPopup();
				}
			}
			if (hasWall) {
				if (MenuItemStyled("Select Wallbrush", "")) {
					wxCommandEvent ev; OnSelectWallBrush(ev);
					canvas_context_menu_open = false; ImGui::CloseCurrentPopup();
				}
			}
			if (foundDoodadBrush && foundDoodadBrush->visibleInPalette()) {
				if (MenuItemStyled("Select Doodadbrush", "")) {
					wxCommandEvent ev; OnSelectDoodadBrush(ev);
					canvas_context_menu_open = false; ImGui::CloseCurrentPopup();
				}
			}
			if (foundDoorBrush) {
				if (MenuItemStyled("Select Doorbrush", "")) {
					wxCommandEvent ev; OnSelectDoorBrush(ev);
					canvas_context_menu_open = false; ImGui::CloseCurrentPopup();
				}
			}
			if (hasCarpet) {
				if (MenuItemStyled("Select Carpetbrush", "")) {
					wxCommandEvent ev; OnSelectCarpetBrush(ev);
					canvas_context_menu_open = false; ImGui::CloseCurrentPopup();
				}
			}
			if (hasTable) {
				if (MenuItemStyled("Select Tablebrush", "")) {
					wxCommandEvent ev; OnSelectTableBrush(ev);
					canvas_context_menu_open = false; ImGui::CloseCurrentPopup();
				}
			}
			if (topCreature) {
				if (MenuItemStyled("Select Creature", "")) {
					wxCommandEvent ev; OnSelectCreatureBrush(ev);
					canvas_context_menu_open = false; ImGui::CloseCurrentPopup();
				}
			}
			if (topSpawn) {
				if (MenuItemStyled("Select Spawn", "")) {
					wxCommandEvent ev; OnSelectSpawnBrush(ev);
					canvas_context_menu_open = false; ImGui::CloseCurrentPopup();
				}
			}
			if (topItem) {
				if (MenuItemStyled("Select RAW Brush", "")) {
					wxCommandEvent ev; OnSelectRAWBrush(ev);
					canvas_context_menu_open = false; ImGui::CloseCurrentPopup();
				}
			}
			if (hasCollection || (topSelectedItem && topSelectedItem->hasCollectionBrush()) || (sel_tile && sel_tile->getGroundBrush() && sel_tile->getGroundBrush()->hasCollection())) {
				if (MenuItemStyled("Select Collection", "")) {
					wxCommandEvent ev; OnSelectCollectionBrush(ev);
					canvas_context_menu_open = false; ImGui::CloseCurrentPopup();
				}
			}
			ImGui::PopStyleColor();
			ImGui::Separator();
		}

		// 2. Edit actions
		if (anything_selected) {
			if (MenuItemStyled("Cut", "Ctrl+X")) {
				wxCommandEvent ev; OnCut(ev);
				canvas_context_menu_open = false; ImGui::CloseCurrentPopup();
			}
			if (MenuItemStyled("Copy", "Ctrl+C")) {
				wxCommandEvent ev; OnCopy(ev);
				canvas_context_menu_open = false; ImGui::CloseCurrentPopup();
			}
		}
		if (editor.copybuffer.canPaste()) {
			if (MenuItemStyled("Paste", "Ctrl+V")) {
				wxCommandEvent ev; OnPaste(ev);
				canvas_context_menu_open = false; ImGui::CloseCurrentPopup();
			}
		}
		if (anything_selected) {
			if (MenuItemStyled("Delete", "Del")) {
				canvas_context_menu_open = false; ImGui::CloseCurrentPopup();
				wxTheApp->CallAfter([this]() { wxCommandEvent ev; OnDelete(ev); });
			}
			ImGui::Separator();
		}

		// 3. Transform actions
		if (sel_tile && (sel_tile->hasGround() || !sel_tile->empty())) {
			if (MenuItemStyled("Change Connected", "Alt+C")) {
				canvas_context_menu_open = false; ImGui::CloseCurrentPopup();
				wxTheApp->CallAfter([this]() { wxCommandEvent ev; OnChangeConnected(ev); });
			}
		}
		if (rotatableItem && rotatableItem->isRoteable()) {
			if (MenuItemStyled("Rotate Item", "Z")) {
				canvas_context_menu_open = false; ImGui::CloseCurrentPopup();
				wxTheApp->CallAfter([this]() { wxCommandEvent ev; OnRotateItem(ev); });
			}
		}

		bool can_use = false;
		if (topSelectedItem) {
			if (topSelectedItem->isBrushDoor() || getItemUseSwitchID(topSelectedItem) != 0 || topSelectedItem->isContainer()) can_use = true;
		} else if (topItem) {
			if (topItem->isBrushDoor() || getItemUseSwitchID(topItem) != 0 || topItem->isContainer()) can_use = true;
		}
		if (can_use) {
			if (MenuItemStyled("Use / Toggle Door", "Space")) {
				canvas_context_menu_open = false; ImGui::CloseCurrentPopup();
				wxTheApp->CallAfter([this]() { wxCommandEvent ev; OnSwitchDoor(ev); });
			}
		}

		// 4. Tools & Properties
		ImGui::Separator();

		int note_to_delete_id = -1;
		Position click_note_pos(last_click_map_x, last_click_map_y, floor);
		if (sel_tile) click_note_pos = sel_tile->getPosition();
		for (const auto& note : editor.map_notes) {
			if (note.pos == click_note_pos) {
				note_to_delete_id = static_cast<int>(note.id);
				break;
			}
		}

		if (note_to_delete_id != -1) {
			if (MenuItemStyled("Delete Note", "")) {
				uint32_t nid = static_cast<uint32_t>(note_to_delete_id);
				auto it = std::remove_if(editor.map_notes.begin(), editor.map_notes.end(), [nid](const MapEditor::MapNote& n) { return n.id == nid; });
				editor.map_notes.erase(it, editor.map_notes.end());

				if (editor.IsLiveClient()) {
					editor.GetLiveClient()->sendRemoveAnnotation(nid);
				} else if (editor.IsLiveServer()) {
					MapAnnotation ann;
					ann.id = nid;
					editor.GetLiveServer()->broadcastAnnotation(ann, true);
				}
				g_gui.SetStatusText(wxString::Format("Note #%d deleted.", nid));
				canvas_context_menu_open = false; ImGui::CloseCurrentPopup();
				Refresh();
			}
		}

		if (MenuItemStyled("Add Map Note", "")) {
			canvas_context_menu_open = false; ImGui::CloseCurrentPopup();
			wxTheApp->CallAfter([this]() { wxCommandEvent ev; OnAddAnnotation(ev); });
		}
		if (MenuItemStyled("Quick Ping Location", "")) {
			canvas_context_menu_open = false; ImGui::CloseCurrentPopup();
			wxTheApp->CallAfter([this]() { wxCommandEvent ev; OnQuickPing(ev); });
		}

		Town* clicked_town = nullptr;
		if (sel_tile) {
			Position click_pos = sel_tile->getPosition();
			for (const auto& pair : editor.map.towns) {
				if (pair.second->getTemplePosition() == click_pos) {
					clicked_town = pair.second;
					break;
				}
			}
		}
		if (clicked_town) {
			if (MenuItemStyled("Edit Town", "")) {
				canvas_context_menu_open = false; ImGui::CloseCurrentPopup();
				wxTheApp->CallAfter([this]() { wxCommandEvent ev; OnEditTown(ev); });
			}
		} else {
			if (MenuItemStyled("Create Town Here", "")) {
				canvas_context_menu_open = false; ImGui::CloseCurrentPopup();
				wxTheApp->CallAfter([this]() { wxCommandEvent ev; OnCreateTown(ev); });
			}
		}

		if (sel_tile && (sel_tile->hasGround() || topSelectedItem || topItem || topCreature || topSpawn)) {
			ImGui::Separator();
			if (MenuItemStyled("⭐ Add to Favorites", "")) {
				canvas_context_menu_open = false; ImGui::CloseCurrentPopup();
				wxTheApp->CallAfter([this]() { wxCommandEvent ev; OnAddFavorite(ev); });
			}
			if (MenuItemStyled("Attributes", "Alt+Enter")) {
				canvas_context_menu_open = false; ImGui::CloseCurrentPopup();
				wxTheApp->CallAfter([this]() { wxCommandEvent ev; OnProperties(ev); });
			}
		}

		ImGui::EndPopup();
	} else {
		canvas_context_menu_open = false;
	}

	ImGui::PopStyleVar(4);
	ImGui::PopStyleColor(7);
}

int MapCanvas::GetHoveredRadialSlice() const {
	if (!tool_wheel_open) return -1;
	
	int scroll_x = 0, scroll_y = 0;
	if (GetParent()) {
		const_cast<MapWindow*>(static_cast<const MapWindow*>(GetParent()))->GetViewStart(&scroll_x, &scroll_y);
	}
	int offset = (tool_wheel_tile_z <= 7) ? (7 - tool_wheel_tile_z) * TileSize : 0;
	float tile_cx = (((tool_wheel_tile_x + 0.5f) * TileSize - scroll_x) - offset) / zoom;
	float tile_cy = (((tool_wheel_tile_y + 0.5f) * TileSize - scroll_y) - offset) / zoom;

	float dx = cursor_x - tile_cx;
	float dy = cursor_y - tile_cy;
	float dist = std::sqrt(dx * dx + dy * dy);
	
	const float r_min = 45.0f;
	const float r_max = 145.0f;
	
	if (dist < r_min || dist > r_max) {
		return -1;
	}
	
	float angle = std::atan2(dy, dx);
	if (angle < 0) angle += 2.0f * static_cast<float>(PI);
	
	int N = 9;
	if (tool_wheel_sub_menu == 1) N = 5;
	else if (tool_wheel_sub_menu == 2) N = 5;
	else if (tool_wheel_sub_menu == 3) N = 3;

	float adjusted_angle = angle + static_cast<float>(PI) / 2.0f + (static_cast<float>(PI) / N);
	if (adjusted_angle >= 2.0f * static_cast<float>(PI)) adjusted_angle -= 2.0f * static_cast<float>(PI);
	
	int slice = (int)(adjusted_angle / (2.0f * static_cast<float>(PI) / N)) % N;
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
	
	// Selection (0)
	wxBitmap pointer_bmp = LoadBitmapFromCandidatesRadial(size, {
		"icons/pointer.png", "../icons/pointer.png", "Map Editor/icons/pointer.png", "../Map Editor/icons/pointer.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "pointer.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "pointer.png"
	});
	radial_tex_ids[0] = ConvertBitmapToTexture(pointer_bmp);
	
	// Pencil (1)
	wxBitmap pencil_bmp = LoadBitmapFromCandidatesRadial(size, {
		"icons/pencil.png", "../icons/pencil.png", "Map Editor/icons/pencil.png", "../Map Editor/icons/pencil.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "pencil.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "pencil.png"
	});
	radial_tex_ids[1] = ConvertBitmapToTexture(pencil_bmp);
	
	// Bucket (2)
	wxBitmap bucket_bmp = LoadBitmapFromCandidatesRadial(size, {
		"icons/bucket.png", "../icons/bucket.png", "Map Editor/icons/bucket.png", "../Map Editor/icons/bucket.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "bucket.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "bucket.png"
	});
	radial_tex_ids[2] = ConvertBitmapToTexture(bucket_bmp);
	
	// Magic Wand (3)
	wxBitmap wand_bmp = LoadBitmapFromCandidatesRadial(size, {
		"icons/magic-wand.png", "../icons/magic-wand.png", "Map Editor/icons/magic-wand.png", "../Map Editor/icons/magic-wand.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "magic-wand.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "magic-wand.png"
	});
	radial_tex_ids[3] = ConvertBitmapToTexture(wand_bmp);

	// Protection Zone (4)
	wxBitmap pz_bmp = LoadBitmapFromCandidatesRadial(size, {
		"icons/protected_zone.png", "../icons/protected_zone.png", "Map Editor/icons/protected_zone.png", "../Map Editor/icons/protected_zone.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "protected_zone.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "protected_zone.png"
	});
	if (!pz_bmp.IsOk()) {
		pz_bmp = wxArtProvider::GetBitmap(ART_PZ_BRUSH, wxART_TOOLBAR, size);
	}
	radial_tex_ids[4] = ConvertBitmapToTexture(pz_bmp);
	
	// Normal Door (5)
	wxBitmap normal_door_bmp = LoadBitmapFromCandidatesRadial(size, {
		"icons/door.png", "../icons/door.png", "Map Editor/icons/door.png", "../Map Editor/icons/door.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "door.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "door.png"
	});
	if (!normal_door_bmp.IsOk()) {
		normal_door_bmp = wxArtProvider::GetBitmap(ART_DOOR_NORMAL_SMALL, wxART_TOOLBAR, size);
	}
	radial_tex_ids[5] = ConvertBitmapToTexture(normal_door_bmp);
	
	// Eraser (6)
	wxBitmap eraser_bmp = LoadBitmapFromCandidatesRadial(size, {
		"icons/eraser.png", "../icons/eraser.png", "Map Editor/icons/eraser.png", "../Map Editor/icons/eraser.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "eraser.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "eraser.png"
	});
	if (!eraser_bmp.IsOk()) {
		eraser_bmp = _wxGetBitmapFromMemoryRadial(eraser_small_png, sizeof(eraser_small_png), size);
	}
	radial_tex_ids[6] = ConvertBitmapToTexture(eraser_bmp);
	
	// Prefab Creator (7)
	wxBitmap prefab_bmp = LoadBitmapFromCandidatesRadial(size, {
		"icons/prefab.png", "../icons/prefab.png", "Map Editor/icons/prefab.png", "../Map Editor/icons/prefab.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "prefab.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "prefab.png"
	});
	radial_tex_ids[7] = ConvertBitmapToTexture(prefab_bmp);

	// Window (8)
	wxBitmap window_bmp = LoadBitmapFromCandidatesRadial(size, {
		"icons/window.png", "../icons/window.png", "Map Editor/icons/window.png", "../Map Editor/icons/window.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "window.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "window.png"
	});
	if (!window_bmp.IsOk()) {
		window_bmp = _wxGetBitmapFromMemoryRadial(window_hatch_small_png, sizeof(window_hatch_small_png), size);
	}
	radial_tex_ids[8] = ConvertBitmapToTexture(window_bmp);

	// No Logout Zone (9)
	wxBitmap nologout_bmp = LoadBitmapFromCandidatesRadial(size, {
		"icons/nologout_zone.png", "../icons/nologout_zone.png", "Map Editor/icons/nologout_zone.png", "../Map Editor/icons/nologout_zone.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "nologout_zone.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "nologout_zone.png"
	});
	if (!nologout_bmp.IsOk()) nologout_bmp = wxArtProvider::GetBitmap(ART_NOLOOUT_BRUSH, wxART_TOOLBAR, size);
	radial_tex_ids[9] = ConvertBitmapToTexture(nologout_bmp);

	// No PVP Zone (10)
	wxBitmap nopvp_bmp = LoadBitmapFromCandidatesRadial(size, {
		"icons/nopvp_zone.png", "../icons/nopvp_zone.png", "Map Editor/icons/nopvp_zone.png", "../Map Editor/icons/nopvp_zone.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "nopvp_zone.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "nopvp_zone.png"
	});
	if (!nopvp_bmp.IsOk()) nopvp_bmp = wxArtProvider::GetBitmap(ART_NOPVP_BRUSH, wxART_TOOLBAR, size);
	radial_tex_ids[10] = ConvertBitmapToTexture(nopvp_bmp);

	// PVP Zone (11)
	wxBitmap pvp_bmp = LoadBitmapFromCandidatesRadial(size, {
		"icons/pvp_zone.png", "../icons/pvp_zone.png", "Map Editor/icons/pvp_zone.png", "../Map Editor/icons/pvp_zone.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "pvp_zone.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "pvp_zone.png"
	});
	if (!pvp_bmp.IsOk()) pvp_bmp = wxArtProvider::GetBitmap(ART_PVP_BRUSH, wxART_TOOLBAR, size);
	radial_tex_ids[11] = ConvertBitmapToTexture(pvp_bmp);

	// Back Icon (12)
	wxBitmap back_bmp = LoadBitmapFromCandidatesRadial(size, {
		"icons/back.png", "../icons/back.png", "Map Editor/icons/back.png", "../Map Editor/icons/back.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "back.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "back.png"
	});
	if (back_bmp.IsOk()) {
		radial_tex_ids[12] = ConvertBitmapToTexture(back_bmp);
	}
	
	radial_textures_loaded = true;
}
