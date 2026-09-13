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
#include "checklist_manager.h"
#include "tileset.h"
#include "materials.h"
#include "world_map_markers.h"
#include "radio_player.h"
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

static GLuint GetDayNightTexture() {
	static GLuint texture = 0;
	if (texture != 0) return texture;
	wxBitmap bitmap = LoadBitmapFromCandidatesRadial(wxSize(32, 32), {
		"icons/day-night.png",
		"../icons/day-night.png",
		"Map Editor/icons/day-night.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "day-night.png"
	});
	texture = ConvertBitmapToTexture(bitmap);
	return texture;
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

	if (!IsShownOnScreen()) {
		return;
	}

	wxGLContext* gl_ctx = g_gui.GetGLContext(this);
	if (!gl_ctx) {
		return;
	}
	SetCurrent(*gl_ctx);
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
		ImGuiIO& io_init = ImGui::GetIO();
		io_init.ConfigWindowsResizeFromEdges = true;
		// Note: Canvas overlays use free floating windows with magnetic edge snapping instead of intrusive dock nodes
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
	io.ConfigWindowsResizeFromEdges = true;
	imgui_last_time = imgui_current_time;

	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGuiWindowFlags dockspace_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMouseInputs;
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

	// Universal In-Canvas Overlay Window Snapping System
	struct OverlayWindowRect {
		std::string name;
		ImVec2 pos;
		ImVec2 size;
	};
	static std::vector<OverlayWindowRect> s_prevOverlayWindows;
	static std::vector<OverlayWindowRect> s_currentOverlayWindows;

	auto RegisterOverlayWindowRect = [](const std::string& name, ImVec2 pos, ImVec2 size) {
		s_currentOverlayWindows.push_back({ name, pos, size });
	};

	auto SnapOverlayWindow = [](const std::string& name, ImVec2 pos, ImVec2 size, ImGuiViewport* vp) -> ImVec2 {
		const float edgeMargin = 5.0f;
		const float windowGap = 5.0f;
		const float snapThreshold = 25.0f;

		ImVec2 targetPos = pos;
		bool snappedX = false;
		bool snappedY = false;

		// 1. Inter-window snapping (against other registered windows from previous frame with 5px gap)
		for (const auto& other : s_prevOverlayWindows) {
			if (other.name == name) continue;

			bool vOverlap = (pos.y + size.y > other.pos.y - snapThreshold) && (pos.y < other.pos.y + other.size.y + snapThreshold);
			bool hOverlap = (pos.x + size.x > other.pos.x - snapThreshold) && (pos.x < other.pos.x + other.size.x + snapThreshold);

			if (vOverlap && !snappedX) {
				// Snap our left edge to other's right edge (+5px gap)
				if (std::abs(pos.x - (other.pos.x + other.size.x + windowGap)) < snapThreshold) {
					targetPos.x = other.pos.x + other.size.x + windowGap;
					snappedX = true;
				}
				// Snap our right edge to other's left edge (-5px gap)
				else if (std::abs((pos.x + size.x) - (other.pos.x - windowGap)) < snapThreshold) {
					targetPos.x = other.pos.x - windowGap - size.x;
					snappedX = true;
				}
				// Snap alignment: Left-to-Left
				else if (std::abs(pos.x - other.pos.x) < snapThreshold) {
					targetPos.x = other.pos.x;
					snappedX = true;
				}
				// Snap alignment: Right-to-Right
				else if (std::abs((pos.x + size.x) - (other.pos.x + other.size.x)) < snapThreshold) {
					targetPos.x = other.pos.x + other.size.x - size.x;
					snappedX = true;
				}
			}

			if (hOverlap && !snappedY) {
				// Snap our top edge to other's bottom edge (+5px gap)
				if (std::abs(pos.y - (other.pos.y + other.size.y + windowGap)) < snapThreshold) {
					targetPos.y = other.pos.y + other.size.y + windowGap;
					snappedY = true;
				}
				// Snap our bottom edge to other's top edge (-5px gap)
				else if (std::abs((pos.y + size.y) - (other.pos.y - windowGap)) < snapThreshold) {
					targetPos.y = other.pos.y - windowGap - size.y;
					snappedY = true;
				}
				// Snap alignment: Top-to-Top
				else if (std::abs(pos.y - other.pos.y) < snapThreshold) {
					targetPos.y = other.pos.y;
					snappedY = true;
				}
				// Snap alignment: Bottom-to-Bottom
				else if (std::abs((pos.y + size.y) - (other.pos.y + other.size.y)) < snapThreshold) {
					targetPos.y = other.pos.y + other.size.y - size.y;
					snappedY = true;
				}
			}
		}

		// 2. Viewport Canvas Boundary Snapping (all 4 edges with 5px margin)
		if (vp) {
			// Left Edge
			if (!snappedX && std::abs(pos.x - (vp->Pos.x + edgeMargin)) < snapThreshold) {
				targetPos.x = vp->Pos.x + edgeMargin;
				snappedX = true;
			}
			// Right Edge
			if (!snappedX && std::abs((pos.x + size.x) - (vp->Pos.x + vp->Size.x - edgeMargin)) < snapThreshold) {
				targetPos.x = vp->Pos.x + vp->Size.x - size.x - edgeMargin;
				snappedX = true;
			}
			// Top Edge
			if (!snappedY && std::abs(pos.y - (vp->Pos.y + edgeMargin)) < snapThreshold) {
				targetPos.y = vp->Pos.y + edgeMargin;
				snappedY = true;
			}
			// Bottom Edge
			if (!snappedY && std::abs((pos.y + size.y) - (vp->Pos.y + vp->Size.y - edgeMargin)) < snapThreshold) {
				targetPos.y = vp->Pos.y + vp->Size.y - size.y - edgeMargin;
				snappedY = true;
			}
		}

		if ((snappedX || snappedY) && !ImGui::IsMouseDown(0)) {
			ImGui::SetWindowPos(targetPos, ImGuiCond_Always);
			return targetPos;
		}
		return pos;
	};

	s_prevOverlayWindows = s_currentOverlayWindows;
	s_currentOverlayWindows.clear();

	// Team Chat Window (Multiplayer & Local Log)
	if (g_settings.getBoolean(Config::SHOW_CHAT)) {
		static bool chat_minimized = false;
		static bool chat_docked_to_palette = false;
		static int last_seen_msg_count = 0;

		int total_msgs = (int)g_gui.chat_log.size();
		int unread_count = std::max(0, total_msgs - last_seen_msg_count);

		if (chat_minimized) {
			// Render a sleek button pill in the bottom status area with Corporate Design
			ImGui::SetNextWindowPos(ImVec2(10, io.DisplaySize.y - 36), ImGuiCond_Always);
			ImGui::SetNextWindowBgAlpha(0.92f);
			ImGuiWindowFlags pill_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
				ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.08f, 0.04f, 0.95f)); // Dark oak/leather
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.83f, 0.69f, 0.22f, 0.85f));   // Medieval Gold
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);

			if (ImGui::Begin("##ChatMinimizedPill", nullptr, pill_flags)) {
				std::string label = unread_count > 0 
					? "[Chat] (" + std::to_string(unread_count) + " new)"
					: "[Chat]";
				
				if (unread_count > 0) {
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.45f, 0.12f, 0.95f));
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
				} else {
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.24f, 0.16f, 0.09f, 0.90f));
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.96f, 0.90f, 0.76f, 1.0f));
				}
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.38f, 0.25f, 0.13f, 1.00f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.55f, 0.38f, 0.18f, 1.00f));

				if (ImGui::Button(label.c_str())) {
					chat_minimized = false;
					last_seen_msg_count = (int)g_gui.chat_log.size();
				}
				ImGui::PopStyleColor(4);

				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Click to restore Team Chat window");
				}
				ImGui::End();
			}
			ImGui::PopStyleVar(2);
			ImGui::PopStyleColor(2);
		} else {
			last_seen_msg_count = total_msgs;

			if (chat_docked_to_palette) {
				ImGui::SetNextWindowPos(ImVec2(std::max(10.0f, io.DisplaySize.x - 340.0f), std::max(10.0f, io.DisplaySize.y - 270.0f)), ImGuiCond_Always);
				ImGui::SetNextWindowSize(ImVec2(330, 260), ImGuiCond_Always);
			} else {
				ImGui::SetNextWindowPos(ImVec2(18, io.DisplaySize.y - 270), ImGuiCond_FirstUseEver);
				ImGui::SetNextWindowSize(ImVec2(330, 260), ImGuiCond_FirstUseEver);
			}
			ImGui::SetNextWindowSizeConstraints(ImVec2(240, 160), ImVec2(800, 600));

			// Corporate Medieval Palette Styles
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.11f, 0.08f, 0.04f, 0.94f));
			ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.18f, 0.12f, 0.06f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.25f, 0.16f, 0.08f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.83f, 0.69f, 0.22f, 0.90f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.96f, 0.91f, 0.78f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.22f, 0.15f, 0.08f, 0.85f));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.35f, 0.24f, 0.12f, 0.95f));
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.83f, 0.69f, 0.22f, 0.80f));
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.15f, 0.08f, 0.90f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.24f, 0.12f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.83f, 0.69f, 0.22f, 0.85f));
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.08f, 0.05f, 0.03f, 0.90f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.14f, 0.09f, 0.05f, 0.95f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.20f, 0.13f, 0.07f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.83f, 0.69f, 0.22f, 0.40f));

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 6.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.8f);

			bool open = true;
			if (ImGui::Begin("Team Chat", &open, ImGuiWindowFlags_NoCollapse)) {
				// Universal Magnetic Edge & Window Snapping
				ImVec2 w_pos = ImGui::GetWindowPos();
				ImVec2 w_size = ImGui::GetWindowSize();
				ImGuiViewport* vp = ImGui::GetMainViewport();
				if (vp && !chat_docked_to_palette) {
					w_pos = SnapOverlayWindow("Team Chat", w_pos, w_size, vp);
				}

				// Header quick buttons
				ImGui::SameLine(ImGui::GetWindowWidth() - 105);
				if (ImGui::SmallButton(chat_docked_to_palette ? "[Float]" : "[Dock]")) {
					chat_docked_to_palette = !chat_docked_to_palette;
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip(chat_docked_to_palette ? "Switch to free-floating window" : "Dock to right palette area");
				}
				ImGui::SameLine();
				if (ImGui::SmallButton("[-]")) {
					chat_minimized = true;
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Minimize to bottom status bar");
				}

				// Connection status sub-bar
				if (editor.IsLive()) {
					if (editor.IsLiveServer()) {
						LiveServer* srv = editor.GetLiveServer();
						int peers = srv ? (int)srv->clients.size() : 0;
						ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "* HOSTING (Port %d, %d Client%s)", 
							srv ? srv->getPort() : 7171, peers, peers == 1 ? "" : "s");
					} else if (editor.GetLiveClient()) {
						LiveClient* cli = editor.GetLiveClient();
						uint32_t lat = cli ? cli->getLatency() : 0;
						ImVec4 pingColor = lat < 50 ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f) :
										  lat < 120 ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f) :
													  ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
						ImGui::TextColored(pingColor, "* CONNECTED (Ping: %u ms)", lat);
					}
				} else {
					ImGui::TextColored(ImVec4(0.75f, 0.70f, 0.55f, 0.9f), "[LOCAL] Chat / Log Mode");
				}
				ImGui::Separator();

				// Collaborators & Teleport section
				if (editor.IsLive()) {
					if (ImGui::CollapsingHeader("[Team] Collaborators & Teleport", ImGuiTreeNodeFlags_None)) {
						LiveSocket& live = editor.GetLive();
						std::vector<LiveCursor> cursors = live.getCursorList();
						if (cursors.empty()) {
							ImGui::TextDisabled("No other collaborators connected yet.");
						} else {
							for (const auto& cur : cursors) {
								std::string name = wxString::Format("Collaborator #%u", cur.id).ToStdString();
								if (editor.IsLiveServer() && editor.GetLiveServer()) {
									auto it = editor.GetLiveServer()->clients.find(cur.id);
									if (it != editor.GetLiveServer()->clients.end() && it->second) {
										name = nstr(it->second->getClientName());
									}
								}
								ImGui::TextColored(ImVec4(cur.color.Red()/255.0f, cur.color.Green()/255.0f, cur.color.Blue()/255.0f, 1.0f), "[Player] %s", name.c_str());
								ImGui::SameLine();
								ImGui::TextDisabled("(%d, %d, %d)", cur.pos.x, cur.pos.y, cur.pos.z);
								ImGui::SameLine();
								std::string jump_btn_id = wxString::Format(">> Jump##jump_%u", cur.id).ToStdString();
								if (ImGui::SmallButton(jump_btn_id.c_str())) {
									if (cur.pos.isValid()) {
										ChangeFloor(cur.pos.z);
										g_gui.SetScreenCenterPosition(cur.pos, false);
										g_gui.SetStatusText(wxString::Format("Teleported to collaborator %s at (%d, %d, %d)", name.c_str(), cur.pos.x, cur.pos.y, cur.pos.z));
									}
								}
							}
						}
					}
					ImGui::Separator();
				}

				// Determine own name for highlighting
				std::string ownName = g_settings.getString(Config::MULTIPLAYER_NAME);
				if (ownName.empty()) ownName = "Mapper";
				if (editor.IsLiveClient() && editor.GetLiveClient()) {
					std::string cname = nstr(editor.GetLiveClient()->getName());
					if (!cname.empty()) ownName = cname;
				}

				// Chat history area
				float reserve_height = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing() + 6.0f;
				ImGui::BeginChild("ScrollingRegion", ImVec2(0, -reserve_height), false, ImGuiWindowFlags_HorizontalScrollbar);
				for (const auto& msg : g_gui.chat_log) {
					ImVec4 color = ImVec4(0.80f, 0.75f, 0.65f, 1.0f); // Parchment tone for others
					if (msg.sender == ownName) color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Bright ivory for self
					else if (msg.sender == "Host") color = ImVec4(0.5f, 1.0f, 0.5f, 1.0f); // Light green for Host
					else if (msg.sender == "Server") color = ImVec4(0.95f, 0.80f, 0.30f, 1.0f); // Gold for Server messages

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
				if (ImGui::InputTextWithHint("##ChatInput", "Type a message...", chat_input, IM_ARRAYSIZE(chat_input), ImGuiInputTextFlags_EnterReturnsTrue)) {
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

				RegisterOverlayWindowRect("Team Chat", w_pos, w_size);
			}
			ImGui::End();

			ImGui::PopStyleVar(5);
			ImGui::PopStyleColor(15);

			if (!open) {
				g_settings.setInteger(Config::SHOW_CHAT, 0);
				if (g_gui.root) {
					g_gui.root->UpdateMenubar();
				}
			}
		}
	}

	// Medieval Collaborative Checklist / Questpad Window
	if (g_settings.getBoolean(Config::SHOW_NOTEPAD)) {
		static bool notepad_minimized = false;
		static char new_task_input[256] = "";

		size_t activeCount = ChecklistManager::getInstance().getActiveCount();
		size_t completedCount = ChecklistManager::getInstance().getCompletedCount();

		if (notepad_minimized) {
			// Floating parchment minimized pill in the bottom-left area
			ImGui::SetNextWindowPos(ImVec2(10.0f, io.DisplaySize.y - 70.0f), ImGuiCond_Always);
			ImGui::SetNextWindowBgAlpha(0.92f);
			ImGuiWindowFlags pill_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
				ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.08f, 0.04f, 0.95f)); // Dark parchment wood
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.83f, 0.69f, 0.22f, 0.85f));   // Medieval Gold
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);

			if (ImGui::Begin("##NotepadMinimizedPill", nullptr, pill_flags)) {
				std::string label = "Notes (" + std::to_string(activeCount) + " open)";
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.24f, 0.16f, 0.09f, 0.90f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.38f, 0.25f, 0.13f, 1.00f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.55f, 0.38f, 0.18f, 1.00f));
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.96f, 0.90f, 0.76f, 1.0f));

				if (ImGui::Button(label.c_str())) {
					notepad_minimized = false;
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Click to expand Collaborative Medieval Notepad");
				}
				ImGui::PopStyleColor(4);
				ImGui::End();
			}
			ImGui::PopStyleVar(2);
			ImGui::PopStyleColor(2);
		} else {
			// Rich Medieval Parchment Dialog
			ImGui::SetNextWindowPos(ImVec2(18.0f, 60.0f), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(360.0f, 440.0f), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSizeConstraints(ImVec2(280.0f, 240.0f), ImVec2(700.0f, 900.0f));

			// Medieval Palette Styles (21 PushStyleColor calls)
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.11f, 0.08f, 0.04f, 0.94f));       // Aged Oak / Dark Leather
			ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.18f, 0.12f, 0.06f, 1.00f));        // Deep wood title
			ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.25f, 0.16f, 0.08f, 1.00f));  // Active Title
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.83f, 0.69f, 0.22f, 0.90f));         // Radiant Antique Gold
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.96f, 0.91f, 0.78f, 1.00f));           // Parchment Ivory Text
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.22f, 0.15f, 0.08f, 0.85f));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.35f, 0.24f, 0.12f, 0.95f));
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.83f, 0.69f, 0.22f, 0.80f));
			ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.16f, 0.11f, 0.06f, 0.90f));
			ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.32f, 0.22f, 0.11f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.26f, 0.18f, 0.09f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_TabUnfocused, ImVec4(0.14f, 0.09f, 0.05f, 0.85f));
			ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive, ImVec4(0.20f, 0.13f, 0.07f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.15f, 0.08f, 0.90f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.24f, 0.12f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.83f, 0.69f, 0.22f, 0.85f));
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.08f, 0.05f, 0.03f, 0.90f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.14f, 0.09f, 0.05f, 0.95f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.20f, 0.13f, 0.07f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.85f, 0.70f, 0.25f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.83f, 0.69f, 0.22f, 0.40f));

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 6.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.8f);

			static GLuint parchment_tex_id = 0;
			static bool parchment_loaded = false;
			if (!parchment_loaded) {
				parchment_loaded = true;
				wxBitmap parchment_bmp = LoadBitmapFromCandidatesRadial(wxDefaultSize, {
					"icons/parchment_bg.png", "../icons/parchment_bg.png", "Map Editor/icons/parchment_bg.png",
					wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "parchment_bg.png",
					wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "parchment_bg.png"
				});
				if (parchment_bmp.IsOk()) {
					parchment_tex_id = ConvertBitmapToTexture(parchment_bmp);
				}
			}

			bool notepad_open = true;
			if (ImGui::Begin("Quest Notepad & Checklist", &notepad_open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse)) {
				// If user collapsed the window or clicked minimize, switch to minimized floating pill
				if (ImGui::IsWindowCollapsed()) {
					ImGui::SetWindowCollapsed(false);
					notepad_minimized = true;
				}

				// Universal Magnetic Edge & Window Snapping
				ImVec2 w_pos = ImGui::GetWindowPos();
				ImVec2 w_size = ImGui::GetWindowSize();
				ImGuiViewport* vp = ImGui::GetMainViewport();
				if (vp) {
					w_pos = SnapOverlayWindow("Quest Notepad & Checklist", w_pos, w_size, vp);
				}

				// Draw parchment background texture inside window
				if (parchment_tex_id != 0) {
					ImDrawList* dl = ImGui::GetWindowDrawList();
					dl->AddImage((ImTextureID)(intptr_t)parchment_tex_id,
						ImVec2(w_pos.x + 2.0f, w_pos.y + 24.0f),
						ImVec2(w_pos.x + w_size.x - 2.0f, w_pos.y + w_size.y - 2.0f),
						ImVec2(0, 0), ImVec2(1, 1),
						IM_COL32(255, 255, 255, 45)); // Soft 18% opacity parchment texture overlay
				}

				// Custom Title Bar Minimize Button rendered right next to the 'X' close button
				{
					ImDrawList* dl = ImGui::GetWindowDrawList();
					float btnSize = 14.0f;
					// Position immediately to the left of the standard ImGui 'X' button (approx 36px from right edge)
					ImVec2 minBtnPos = ImVec2(w_pos.x + w_size.x - 42.0f, w_pos.y + 5.0f);
					ImVec2 mousePos = ImGui::GetMousePos();
					bool isHovered = mousePos.x >= minBtnPos.x && mousePos.x <= minBtnPos.x + btnSize &&
					                 mousePos.y >= minBtnPos.y && mousePos.y <= minBtnPos.y + btnSize;

					if (isHovered) {
						dl->AddRectFilled(minBtnPos, ImVec2(minBtnPos.x + btnSize, minBtnPos.y + btnSize),
							IM_COL32(200, 160, 60, 120), 3.0f);
						ImGui::SetTooltip("Minimize to floating pill");
						if (ImGui::IsMouseClicked(0)) {
							notepad_minimized = true;
						}
					}
					// Draw subtle '_' minimize dash icon in gold
					dl->AddLine(ImVec2(minBtnPos.x + 2.5f, minBtnPos.y + btnSize - 3.5f),
					            ImVec2(minBtnPos.x + btnSize - 2.5f, minBtnPos.y + btnSize - 3.5f),
					            IM_COL32(230, 205, 130, isHovered ? 255 : 200), 2.0f);
				}

				// Top Task Input Bar
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
				ImGui::Text("Inscribe Task:");
				ImGui::PopStyleColor();

				float btn_w = 70.0f;
				ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.04f, 0.03f, 0.02f, 0.95f));
				ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.85f, 0.72f, 0.25f, 0.90f));
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.2f);
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));
				ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - btn_w - 8.0f);
				bool enter_pressed = ImGui::InputTextWithHint("##NewTaskInput", "Example Text", new_task_input, IM_ARRAYSIZE(new_task_input), ImGuiInputTextFlags_EnterReturnsTrue);
				ImGui::PopItemWidth();
				ImGui::PopStyleVar(2);
				ImGui::PopStyleColor(2);

				ImGui::SameLine();
				bool add_clicked = ImGui::Button("+ Add", ImVec2(btn_w, 0));

				auto submit_task = [&]() {
					std::string task_text = new_task_input;
					// Trim whitespace
					size_t first = task_text.find_first_not_of(" \t\r\n");
					if (first != std::string::npos) {
						size_t last = task_text.find_last_not_of(" \t\r\n");
						task_text = task_text.substr(first, (last - first + 1));
					} else {
						task_text.clear();
					}

					if (!task_text.empty()) {
						std::string author_name = g_settings.getString(Config::MULTIPLAYER_NAME);
						if (author_name.empty()) author_name = "Mapper";
						if (editor.IsLiveClient() && editor.GetLiveClient()) {
							std::string cname = nstr(editor.GetLiveClient()->getName());
							if (!cname.empty()) author_name = cname;
						}

						uint32_t assignedId = ChecklistManager::getInstance().addItem(task_text, author_name, false);

						// Multiplayer broadcast / send
						if (editor.IsLiveServer() && editor.GetLiveServer()) {
							editor.GetLiveServer()->broadcastChecklistAdd(assignedId, task_text, author_name, false);
						} else if (editor.IsLiveClient() && editor.GetLiveClient()) {
							editor.GetLiveClient()->sendChecklistAdd(assignedId, task_text, author_name, false);
						}

						new_task_input[0] = '\0';
						ImGui::SetKeyboardFocusHere(-1);
					}
				};

				if (enter_pressed || add_clicked) {
					submit_task();
				}

				ImGui::Separator();

				// Tabs: Active Tasks vs Completed
				if (ImGui::BeginTabBar("##ChecklistTabs", ImGuiTabBarFlags_None)) {
					// TAB 1: ACTIVE TASKS
					std::string activeTabLabel = "Active (" + std::to_string(activeCount) + ")###ActiveTab";
					if (ImGui::BeginTabItem(activeTabLabel.c_str())) {
						auto activeItems = ChecklistManager::getInstance().getActiveItems();

						if (activeItems.empty()) {
							ImGui::Spacing();
							ImGui::TextColored(ImVec4(0.65f, 0.60f, 0.48f, 0.8f), "No active tasks. Inscribe a new quest above!");
						} else {
							ImGui::BeginChild("##ActiveListChild", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
							for (const auto& item : activeItems) {
								ImGui::PushID((int)item.id);

								bool completed_state = false;
								if (ImGui::Checkbox("##chk", &completed_state)) {
									// Mark completed!
									ChecklistManager::getInstance().toggleItem(item.id, true);
									if (editor.IsLiveServer() && editor.GetLiveServer()) {
										editor.GetLiveServer()->broadcastChecklistToggle(item.id, true);
									} else if (editor.IsLiveClient() && editor.GetLiveClient()) {
										editor.GetLiveClient()->sendChecklistToggle(item.id, true);
									}
								}
								if (ImGui::IsItemHovered()) {
									ImGui::SetTooltip("Check off this quest item");
								}

								ImGui::SameLine();
								ImGui::TextWrapped("%s", item.text.c_str());

								// Author stamp & Delete button
								ImGui::SameLine(ImGui::GetWindowWidth() - 56.0f);
								ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.10f, 0.10f, 0.70f));
								ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.60f, 0.15f, 0.15f, 0.90f));
								if (ImGui::SmallButton("X")) {
									ChecklistManager::getInstance().deleteItem(item.id);
									if (editor.IsLiveServer() && editor.GetLiveServer()) {
										editor.GetLiveServer()->broadcastChecklistDelete(item.id);
									} else if (editor.IsLiveClient() && editor.GetLiveClient()) {
										editor.GetLiveClient()->sendChecklistDelete(item.id);
									}
								}
								if (ImGui::IsItemHovered()) {
									ImGui::SetTooltip("Delete this task completely");
								}
								ImGui::PopStyleColor(2);

								// Subtle author stamp below item
								ImGui::TextColored(ImVec4(0.60f, 0.52f, 0.38f, 0.75f), "   by %s", item.author.c_str());
								ImGui::Separator();

								ImGui::PopID();
							}
							ImGui::EndChild();
						}
						ImGui::EndTabItem();
					}

					// TAB 2: COMPLETED TASKS
					std::string completedTabLabel = "Completed (" + std::to_string(completedCount) + ")###CompletedTab";
					if (ImGui::BeginTabItem(completedTabLabel.c_str())) {
						auto completedItems = ChecklistManager::getInstance().getCompletedItems();

						if (!completedItems.empty()) {
							if (ImGui::Button("Clear All Completed")) {
								ChecklistManager::getInstance().clearCompleted();
								if (editor.IsLiveServer() && editor.GetLiveServer()) {
									editor.GetLiveServer()->broadcastChecklistClearCompleted();
								} else if (editor.IsLiveClient() && editor.GetLiveClient()) {
									editor.GetLiveClient()->sendChecklistClearCompleted();
								}
							}
							ImGui::Separator();
						}

						if (completedItems.empty()) {
							ImGui::Spacing();
							ImGui::TextColored(ImVec4(0.65f, 0.60f, 0.48f, 0.8f), "No completed quests yet.");
						} else {
							ImGui::BeginChild("##CompletedListChild", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
							for (const auto& item : completedItems) {
								ImGui::PushID((int)item.id);

								bool checked_val = true;
								if (ImGui::Checkbox("##restoreChk", &checked_val)) {
									// Restore to active!
									ChecklistManager::getInstance().toggleItem(item.id, false);
									if (editor.IsLiveServer() && editor.GetLiveServer()) {
										editor.GetLiveServer()->broadcastChecklistToggle(item.id, false);
									} else if (editor.IsLiveClient() && editor.GetLiveClient()) {
										editor.GetLiveClient()->sendChecklistToggle(item.id, false);
									}
								}
								if (ImGui::IsItemHovered()) {
									ImGui::SetTooltip("Uncheck to restore this quest back to Active");
								}

								ImGui::SameLine();
								ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.50f, 0.85f), "%s (Done)", item.text.c_str());

								// Delete button
								ImGui::SameLine(ImGui::GetWindowWidth() - 56.0f);
								ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.10f, 0.10f, 0.70f));
								ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.60f, 0.15f, 0.15f, 0.90f));
								if (ImGui::SmallButton("X")) {
									ChecklistManager::getInstance().deleteItem(item.id);
									if (editor.IsLiveServer() && editor.GetLiveServer()) {
										editor.GetLiveServer()->broadcastChecklistDelete(item.id);
									} else if (editor.IsLiveClient() && editor.GetLiveClient()) {
										editor.GetLiveClient()->sendChecklistDelete(item.id);
									}
								}
								ImGui::PopStyleColor(2);

								ImGui::TextColored(ImVec4(0.50f, 0.45f, 0.35f, 0.65f), "   completed (by %s)", item.author.c_str());
								ImGui::Separator();

								ImGui::PopID();
							}
							ImGui::EndChild();
						}
						ImGui::EndTabItem();
					}

					ImGui::EndTabBar();
				}

				RegisterOverlayWindowRect("Quest Notepad & Checklist", w_pos, w_size);
			}
			ImGui::End();

			ImGui::PopStyleVar(5);
			ImGui::PopStyleColor(21);

			if (!notepad_open) {
				g_settings.setInteger(Config::SHOW_NOTEPAD, 0);
				if (g_gui.root) {
					g_gui.root->UpdateMenubar();
				}
			}
		}
	}

	// Radio Player In-Canvas ImGui Window
	if (g_settings.getBoolean(Config::SHOW_RADIO)) {
		static bool radio_minimized = false;
		static bool radio_docked_to_palette = false;

		RadioManager& rm = RadioManager::Get();
		const auto& stations = rm.GetStations();
		int curIdx = rm.GetCurrentStationIndex();
		std::string curStationName = (curIdx >= 0 && curIdx < (int)stations.size()) ? stations[curIdx].name : "None";

		if (radio_minimized) {
			// Render a sleek button pill in the bottom status area with Corporate Design
			ImGui::SetNextWindowPos(ImVec2(10.0f, io.DisplaySize.y - 104.0f), ImGuiCond_Always);
			ImGui::SetNextWindowBgAlpha(0.92f);
			ImGuiWindowFlags pill_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
				ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.08f, 0.04f, 0.95f));
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.83f, 0.69f, 0.22f, 0.85f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);

			if (ImGui::Begin("##RadioMinimizedPill", nullptr, pill_flags)) {
				std::string label = rm.IsPlaying()
					? "[Radio] > " + curStationName
					: "[Radio] (Stopped)";

				if (rm.IsPlaying()) {
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.35f, 0.18f, 0.95f));
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.5f, 1.0f));
				} else {
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.24f, 0.16f, 0.09f, 0.90f));
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.96f, 0.90f, 0.76f, 1.0f));
				}
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.38f, 0.25f, 0.13f, 1.00f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.55f, 0.38f, 0.18f, 1.00f));

				if (ImGui::Button(label.c_str())) {
					radio_minimized = false;
				}
				ImGui::PopStyleColor(4);

				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Click to restore Radio Player window");
				}
				ImGui::End();
			}
			ImGui::PopStyleVar(2);
			ImGui::PopStyleColor(2);
		} else {
			if (radio_docked_to_palette) {
				ImGui::SetNextWindowPos(ImVec2(std::max(10.0f, io.DisplaySize.x - 340.0f), std::max(10.0f, io.DisplaySize.y - 170.0f)), ImGuiCond_Always);
				ImGui::SetNextWindowSize(ImVec2(330, 160), ImGuiCond_Always);
			} else {
				ImGui::SetNextWindowPos(ImVec2(18.0f, io.DisplaySize.y - 440.0f), ImGuiCond_FirstUseEver);
				ImGui::SetNextWindowSize(ImVec2(330, 160), ImGuiCond_FirstUseEver);
			}
			ImGui::SetNextWindowSizeConstraints(ImVec2(260, 140), ImVec2(600, 300));

			// Corporate Medieval Palette Styles
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.11f, 0.08f, 0.04f, 0.94f));
			ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.18f, 0.12f, 0.06f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.25f, 0.16f, 0.08f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.83f, 0.69f, 0.22f, 0.90f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.96f, 0.91f, 0.78f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.22f, 0.15f, 0.08f, 0.85f));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.35f, 0.24f, 0.12f, 0.95f));
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.83f, 0.69f, 0.22f, 0.80f));
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.15f, 0.08f, 0.90f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.24f, 0.12f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.83f, 0.69f, 0.22f, 0.85f));
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.08f, 0.05f, 0.03f, 0.90f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.14f, 0.09f, 0.05f, 0.95f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.20f, 0.13f, 0.07f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.83f, 0.69f, 0.22f, 0.40f));
			ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.83f, 0.69f, 0.22f, 0.90f));
			ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.95f, 0.85f, 0.35f, 1.00f));

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 8.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 6.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.8f);

			bool radio_open = true;
			if (ImGui::Begin("Radio Player", &radio_open, ImGuiWindowFlags_NoCollapse)) {
				// Universal Magnetic Edge & Window Snapping
				ImVec2 w_pos = ImGui::GetWindowPos();
				ImVec2 w_size = ImGui::GetWindowSize();
				ImGuiViewport* vp = ImGui::GetMainViewport();
				if (vp && !radio_docked_to_palette) {
					w_pos = SnapOverlayWindow("Radio Player", w_pos, w_size, vp);
				}

				// Header quick buttons
				ImGui::SameLine(ImGui::GetWindowWidth() - 105);
				if (ImGui::SmallButton(radio_docked_to_palette ? "[Float]" : "[Dock]")) {
					radio_docked_to_palette = !radio_docked_to_palette;
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip(radio_docked_to_palette ? "Switch to free-floating window" : "Dock to right palette area");
				}
				ImGui::SameLine();
				if (ImGui::SmallButton("[-]")) {
					radio_minimized = true;
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Minimize to bottom status bar");
				}

				// Status sub-bar
				if (rm.IsPlaying()) {
					ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "[>] %s", curStationName.c_str());
				} else {
					ImGui::TextColored(ImVec4(0.70f, 0.65f, 0.55f, 0.85f), "[#] Stopped");
				}
				ImGui::SameLine(ImGui::GetWindowWidth() - 85);
				if (ImGui::SmallButton("Web Radio")) {
					rm.OpenCurrentWebStation();
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Open station web player in browser");
				}
				ImGui::Separator();

				// Station Selector Combo
				const char* preview_value = (curIdx >= 0 && curIdx < (int)stations.size()) ? stations[curIdx].name.c_str() : "Select...";
				ImGui::PushItemWidth(-1.0f);
				if (ImGui::BeginCombo("##StationCombo", preview_value)) {
					for (int i = 0; i < (int)stations.size(); ++i) {
						const bool is_selected = (curIdx == i);
						if (ImGui::Selectable(stations[i].name.c_str(), is_selected)) {
							rm.Play(i);
						}
						if (is_selected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
				// Mouse-wheel scroll to change station when hovering combo
				if (ImGui::IsItemHovered() && io.MouseWheel != 0.0f) {
					int newIdx = curIdx - (int)io.MouseWheel;
					if (newIdx < 0) newIdx = 0;
					if (newIdx >= (int)stations.size()) newIdx = (int)stations.size() - 1;
					if (newIdx != curIdx) {
						rm.Play(newIdx);
					}
				}
				ImGui::PopItemWidth();

				// Controls Row: Play, Stop, Mute, Volume
				if (rm.IsPlaying()) {
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.45f, 0.18f, 0.95f));
					if (ImGui::Button("[ > ]")) {
						rm.TogglePlay();
					}
					ImGui::PopStyleColor();
				} else {
					if (ImGui::Button("[ > ]")) {
						rm.TogglePlay();
					}
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip(rm.IsPlaying() ? "Pause/Play" : "Play Station");

				ImGui::SameLine();
				if (ImGui::Button("[ # ]")) {
					rm.Stop();
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stop Playback");

				ImGui::SameLine();
				bool isMuted = rm.IsMuted();
				if (isMuted) {
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.18f, 0.18f, 0.95f));
				}
				if (ImGui::Button(isMuted ? "[Mute]" : "[Vol]")) {
					rm.SetMute(!isMuted);
				}
				if (isMuted) {
					ImGui::PopStyleColor();
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip(isMuted ? "Unmute" : "Mute audio");

				ImGui::SameLine();
				int curVol = rm.GetVolume();
				ImGui::PushItemWidth(-1.0f);
				if (ImGui::SliderInt("##VolumeSlider", &curVol, 0, 100, "%d%%")) {
					rm.SetVolume(curVol);
				}
				ImGui::PopItemWidth();

				RegisterOverlayWindowRect("Radio Player", w_pos, w_size);
			}
			ImGui::End();

			ImGui::PopStyleVar(5);
			ImGui::PopStyleColor(17);

			if (!radio_open) {
				RadioManager::Get().Stop();
				g_settings.setInteger(Config::SHOW_RADIO, 0);
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

struct ToolbarIconCache {
	GLuint tex_pointer = 0;
	GLuint tex_pencil = 0;
	GLuint tex_bucket = 0;
	GLuint tex_magic_wand = 0;
	GLuint tex_eraser = 0;
	GLuint tex_autoborder = 0;
	GLuint tex_door = 0;
	GLuint tex_window = 0;
	GLuint tex_day_night = 0;
	GLuint tex_rect_brush = 0;
	GLuint tex_circle_brush = 0;
	GLuint tex_world_map = 0;
	GLuint tex_sizes[7] = {0, 0, 0, 0, 0, 0, 0};
	bool initialized = false;

	void Init() {
		if (initialized) return;
		initialized = true;

		auto load_tex = [](const std::vector<wxString>& candidates, wxSize sz = wxSize(24, 24)) -> GLuint {
			wxBitmap bmp = LoadBitmapFromCandidatesRadial(sz, candidates);
			return ConvertBitmapToTexture(bmp);
		};

		wxString exeDir = wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH;
		wxString cwdDir = wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH;

		tex_pointer = load_tex({"icons/pointer.png", "../icons/pointer.png", "Map Editor/icons/pointer.png", exeDir + "pointer.png", cwdDir + "pointer.png"});
		tex_pencil = load_tex({"icons/pencil.png", "../icons/pencil.png", "Map Editor/icons/pencil.png", exeDir + "pencil.png", cwdDir + "pencil.png"});
		tex_bucket = load_tex({"icons/bucket.png", "../icons/bucket.png", "Map Editor/icons/bucket.png", exeDir + "bucket.png", cwdDir + "bucket.png"});
		tex_magic_wand = load_tex({"icons/magic-wand.png", "../icons/magic-wand.png", "Map Editor/icons/magic-wand.png", exeDir + "magic-wand.png", cwdDir + "magic-wand.png"});
		tex_eraser = load_tex({"icons/eraser.png", "../icons/eraser.png", "Map Editor/icons/eraser.png", exeDir + "eraser.png", cwdDir + "eraser.png"});
		tex_autoborder = load_tex({"icons/auto_border.png", "../icons/auto_border.png", "Map Editor/icons/auto_border.png", exeDir + "auto_border.png", cwdDir + "auto_border.png"});
		tex_door = load_tex({"icons/door.png", "../icons/door.png", "Map Editor/icons/door.png", exeDir + "door.png", cwdDir + "door.png"});
		tex_window = load_tex({"icons/window.png", "../icons/window.png", "Map Editor/icons/window.png", exeDir + "window.png", cwdDir + "window.png"});
		tex_day_night = load_tex({"icons/day-night.png", "../icons/day-night.png", "Map Editor/icons/day-night.png", exeDir + "day-night.png", cwdDir + "day-night.png"});
		tex_world_map = load_tex({"icons/world-map.png", "../icons/world-map.png", "Map Editor/icons/world-map.png", exeDir + "world-map.png", cwdDir + "world-map.png"});
		tex_rect_brush = load_tex({"icons/rectangular_tileset.png", "../icons/rectangular_tileset.png", "Map Editor/icons/rectangular_tileset.png", exeDir + "rectangular_tileset.png", cwdDir + "rectangular_tileset.png"});
		tex_circle_brush = load_tex({"icons/circular_tileset.png", "../icons/circular_tileset.png", "Map Editor/icons/circular_tileset.png", exeDir + "circular_tileset.png", cwdDir + "circular_tileset.png"});

		for (int i = 0; i < 7; ++i) {
			wxString f = wxString::Format("rectangular_%d.png", i + 1);
			tex_sizes[i] = load_tex({"icons/" + f, "../icons/" + f, "Map Editor/icons/" + f, exeDir + f, cwdDir + f}, wxSize(18, 18));
		}
	}
};
static ToolbarIconCache s_toolbar_icons;

	// macOS Canvas Floating / Docked Toolbar in the exact style of the Canvas Info Overlay
	if (g_settings.getBoolean(Config::SHOW_TOOLBAR_BRUSHES)) {
		s_toolbar_icons.Init();

		const float ui_scale = std::clamp((float)g_settings.getInteger(Config::UI_SCALE) / 100.0f, 1.0f, 2.5f);
		const int dock_pos = std::clamp(g_settings.getInteger(Config::TOOLBAR_OVERLAY_POSITION), 0, 1);
		ImVec2 tb_anchor(io.DisplaySize.x * 0.5f, (dock_pos == 1) ? (io.DisplaySize.y - 12.0f) : 10.0f);
		ImVec2 tb_pivot(0.5f, (dock_pos == 1) ? 1.0f : 0.0f);
		ImGui::SetNextWindowPos(tb_anchor, ImGuiCond_Always, tb_pivot);
		ImGui::SetNextWindowBgAlpha(0.88f);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f * ui_scale);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f * ui_scale);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f * ui_scale, 5.0f * ui_scale));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f * ui_scale, 3.0f * ui_scale));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.07f, 0.11f, 0.90f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.55f, 0.45f, 0.25f, 0.65f));

		ImGuiWindowFlags tb_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

		bool toolbar_open = true;
		if (ImGui::Begin("##CanvasMacOSToolbar", &toolbar_open, tb_flags)) {
			const float tb_h = 24.0f * ui_scale;
			const ImVec2 tool_btn_sz(24.0f * ui_scale, tb_h);
			const ImVec2 tool_icon_sz(18.0f * ui_scale, 18.0f * ui_scale);
			const ImVec2 size_btn_sz(20.0f * ui_scale, tb_h);
			const ImVec2 size_icon_sz(14.0f * ui_scale, 14.0f * ui_scale);

			auto draw_icon_btn = [&](const char* id, GLuint tex, const char* fallback, bool active, const char* tooltip, const ImVec2& btn_sz, const ImVec2& icon_sz) -> bool {
				bool clicked = false;
				if (active) {
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.45f, 0.85f, 0.95f));
					ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.95f, 0.80f, 0.30f, 1.0f));
					ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f);
				} else {
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.15f, 0.22f, 0.75f));
					ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.35f, 0.38f, 0.48f, 0.50f));
					ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
				}

				ImVec2 p0 = ImGui::GetCursorScreenPos();
				if (ImGui::Button(id, btn_sz)) {
					clicked = true;
				}

				ImDrawList* dl = ImGui::GetWindowDrawList();
				if (tex != 0) {
					ImVec2 img_min(p0.x + (btn_sz.x - icon_sz.x) * 0.5f, p0.y + (btn_sz.y - icon_sz.y) * 0.5f);
					ImVec2 img_max(img_min.x + icon_sz.x, img_min.y + icon_sz.y);
					dl->AddImage((ImTextureID)(intptr_t)tex, img_min, img_max);
				} else if (fallback && fallback[0]) {
					ImVec2 txt_sz = ImGui::CalcTextSize(fallback);
					ImVec2 txt_pos(p0.x + (btn_sz.x - txt_sz.x) * 0.5f, p0.y + (btn_sz.y - txt_sz.y) * 0.5f);
					dl->AddText(txt_pos, IM_COL32(230, 230, 240, 255), fallback);
				}

				ImGui::PopStyleVar();
				ImGui::PopStyleColor(2);

				if (ImGui::IsItemHovered() && tooltip) {
					ImGui::SetTooltip("%s", tooltip);
				}
				return clicked;
			};

			// 1. Selection Tool
			const bool is_sel = g_gui.IsSelectionMode();
			if (draw_icon_btn("##tool_sel", s_toolbar_icons.tex_pointer, "Sel", is_sel, "Selection Tool (Pointer)", tool_btn_sz, tool_icon_sz)) {
				g_gui.SetFillBrushMode(false);
				g_gui.SetSelectionMode();
			}

			ImGui::SameLine();
			// 2. Pencil Tool
			const bool is_pencil = (!g_gui.IsSelectionMode() && !g_gui.IsFillBrushMode() && g_gui.GetCurrentBrush() != g_gui.eraser && g_gui.GetCurrentBrush() != g_gui.optional_brush && g_gui.GetCurrentBrush() != g_gui.normal_door_brush && g_gui.GetCurrentBrush() != g_gui.locked_door_brush && g_gui.GetCurrentBrush() != g_gui.magic_door_brush && g_gui.GetCurrentBrush() != g_gui.quest_door_brush && g_gui.GetCurrentBrush() != g_gui.archway_door_brush && g_gui.GetCurrentBrush() != g_gui.window_door_brush && g_gui.GetCurrentBrush() != g_gui.hatch_door_brush);
			if (draw_icon_btn("##tool_pencil", s_toolbar_icons.tex_pencil, "Pen", is_pencil, "Pencil Drawing Tool", tool_btn_sz, tool_icon_sz)) {
				g_gui.SetFillBrushMode(false);
				g_gui.SetDrawingMode();
			}

			ImGui::SameLine();
			// 3. Bucket Fill Tool
			const bool is_fill = g_gui.IsFillBrushMode();
			if (draw_icon_btn("##tool_fill", s_toolbar_icons.tex_bucket, "Fill", is_fill, "Bucket Fill Tool", tool_btn_sz, tool_icon_sz)) {
				g_gui.SetDrawingMode();
				g_gui.SetFillBrushMode(!is_fill);
			}

			ImGui::SameLine();
			// 4. Eraser Tool
			const bool is_eraser = (g_gui.GetCurrentBrush() == g_gui.eraser);
			if (draw_icon_btn("##tool_eraser", s_toolbar_icons.tex_eraser, "Era", is_eraser, "Eraser Tool", tool_btn_sz, tool_icon_sz)) {
				g_gui.SetFillBrushMode(false);
				g_gui.SelectBrush(g_gui.eraser);
			}

			ImGui::SameLine();
			// 5. Autoborder Tool
			const bool is_border = (g_gui.GetCurrentBrush() == g_gui.optional_brush);
			if (draw_icon_btn("##tool_border", s_toolbar_icons.tex_autoborder, "Brd", is_border, "Autoborder Tool", tool_btn_sz, tool_icon_sz)) {
				g_gui.SetFillBrushMode(false);
				g_gui.SelectBrush(g_gui.optional_brush);
			}

			ImGui::SameLine();
			// 6. Doors Tool (Click to select or choose variant)
			const bool is_door = (g_gui.GetCurrentBrush() == g_gui.normal_door_brush || g_gui.GetCurrentBrush() == g_gui.locked_door_brush || g_gui.GetCurrentBrush() == g_gui.magic_door_brush || g_gui.GetCurrentBrush() == g_gui.quest_door_brush || g_gui.GetCurrentBrush() == g_gui.archway_door_brush);
			if (draw_icon_btn("##tool_door", s_toolbar_icons.tex_door, "Dor", is_door, "Doors Tool (Click for Door Options)", tool_btn_sz, tool_icon_sz)) {
				ImGui::OpenPopup("##DoorSubMenu");
			}

			if (ImGui::BeginPopup("##DoorSubMenu")) {
				ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.35f, 1.0f), "Select Door Type");
				ImGui::Separator();
				if (ImGui::Selectable("Normal Door", g_gui.GetCurrentBrush() == g_gui.normal_door_brush)) {
					g_gui.SetFillBrushMode(false);
					g_gui.SelectBrush(g_gui.normal_door_brush);
				}
				if (ImGui::Selectable("Locked Door", g_gui.GetCurrentBrush() == g_gui.locked_door_brush)) {
					g_gui.SetFillBrushMode(false);
					g_gui.SelectBrush(g_gui.locked_door_brush);
				}
				if (ImGui::Selectable("Magic Door", g_gui.GetCurrentBrush() == g_gui.magic_door_brush)) {
					g_gui.SetFillBrushMode(false);
					g_gui.SelectBrush(g_gui.magic_door_brush);
				}
				if (ImGui::Selectable("Quest Door", g_gui.GetCurrentBrush() == g_gui.quest_door_brush)) {
					g_gui.SetFillBrushMode(false);
					g_gui.SelectBrush(g_gui.quest_door_brush);
				}
				if (ImGui::Selectable("Archway", g_gui.GetCurrentBrush() == g_gui.archway_door_brush)) {
					g_gui.SetFillBrushMode(false);
					g_gui.SelectBrush(g_gui.archway_door_brush);
				}
				ImGui::EndPopup();
			}

			ImGui::SameLine();
			// 7. Windows Tool (Click to select or choose variant)
			const bool is_window = (g_gui.GetCurrentBrush() == g_gui.window_door_brush || g_gui.GetCurrentBrush() == g_gui.hatch_door_brush);
			if (draw_icon_btn("##tool_window", s_toolbar_icons.tex_window, "Win", is_window, "Windows Tool (Click for Window Options)", tool_btn_sz, tool_icon_sz)) {
				ImGui::OpenPopup("##WindowSubMenu");
			}

			if (ImGui::BeginPopup("##WindowSubMenu")) {
				ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.35f, 1.0f), "Select Window Type");
				ImGui::Separator();
				if (ImGui::Selectable("Window", g_gui.GetCurrentBrush() == g_gui.window_door_brush)) {
					g_gui.SetFillBrushMode(false);
					g_gui.SelectBrush(g_gui.window_door_brush);
				}
				if (ImGui::Selectable("Hatch Window", g_gui.GetCurrentBrush() == g_gui.hatch_door_brush)) {
					g_gui.SetFillBrushMode(false);
					g_gui.SelectBrush(g_gui.hatch_door_brush);
				}
				ImGui::EndPopup();
			}

			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.45f, 0.48f, 0.55f, 1.0f), "|");
			ImGui::SameLine();

			// 8. Floor Selector (Dropdown matching toolbar height exactly, scrollable with mouse wheel)
			ImGui::SetNextItemWidth(76.0f * ui_scale);
			int current_floor = floor;
			const char* floor_items[] = { "Floor 0", "Floor 1", "Floor 2", "Floor 3", "Floor 4", "Floor 5", "Floor 6", "Floor 7",
			                              "Floor 8", "Floor 9", "Floor 10", "Floor 11", "Floor 12", "Floor 13", "Floor 14", "Floor 15" };
			float floor_pad_y = std::max(0.0f, (tb_h - ImGui::GetFontSize()) * 0.5f);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f * ui_scale, floor_pad_y));
			if (ImGui::Combo("##FloorCombo", &current_floor, floor_items, IM_ARRAYSIZE(floor_items), 16)) {
				g_gui.ChangeFloor(current_floor);
			}
			ImGui::PopStyleVar();

			if (ImGui::IsItemHovered()) {
				if (io.MouseWheel != 0.0f) {
					int new_f = std::clamp(current_floor - (int)io.MouseWheel, 0, 15);
					if (new_f != current_floor) {
						g_gui.ChangeFloor(new_f);
					}
				}
				ImGui::SetTooltip("Z-Level / Floor (0-15)\nScroll mouse wheel to change floor");
			}

			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.45f, 0.48f, 0.55f, 1.0f), "|");
			ImGui::SameLine();

			// 9. Brush Shape Toggle (Square / Circle) with matching height
			const bool is_square = (g_gui.GetBrushShape() == BRUSHSHAPE_SQUARE);
			ImVec2 btn_pos = ImGui::GetCursorScreenPos();

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.15f, 0.22f, 0.75f));
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.95f, 0.80f, 0.30f, 0.80f));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.2f);

			if (ImGui::Button("##ShapeToggleBtn", tool_btn_sz)) {
				g_gui.SetBrushShape(is_square ? BRUSHSHAPE_CIRCLE : BRUSHSHAPE_SQUARE);
				Refresh(false);
			}
			ImGui::PopStyleVar();
			ImGui::PopStyleColor(2);

			ImDrawList* tb_dl = ImGui::GetWindowDrawList();
			ImVec2 shape_center(btn_pos.x + tool_btn_sz.x * 0.5f, btn_pos.y + tool_btn_sz.y * 0.5f);
			if (is_square) {
				float half_side = 5.5f * ui_scale;
				tb_dl->AddRectFilled(ImVec2(shape_center.x - half_side, shape_center.y - half_side), ImVec2(shape_center.x + half_side, shape_center.y + half_side), IM_COL32(255, 215, 60, 220), 1.5f);
				tb_dl->AddRect(ImVec2(shape_center.x - half_side, shape_center.y - half_side), ImVec2(shape_center.x + half_side, shape_center.y + half_side), IM_COL32(255, 245, 160, 255), 1.5f, 0, 1.2f);
			} else {
				float rad = 6.0f * ui_scale;
				tb_dl->AddCircleFilled(shape_center, rad, IM_COL32(255, 215, 60, 220), 16);
				tb_dl->AddCircle(shape_center, rad, IM_COL32(255, 245, 160, 255), 16, 1.2f);
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Brush Shape: %s (Click to toggle)", is_square ? "Square" : "Circular");
			}

			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.45f, 0.48f, 0.55f, 1.0f), "|");
			ImGui::SameLine();

			// 10. Brush Sizes 1..7 with uniform height
			int current_size = g_gui.GetBrushSize();
			for (int sz = 1; sz <= 7; ++sz) {
				char btn_id[16];
				snprintf(btn_id, sizeof(btn_id), "##sz_%d", sz);
				char sz_fallback[8];
				snprintf(sz_fallback, sizeof(sz_fallback), "%d", sz);
				char tip[32];
				snprintf(tip, sizeof(tip), "Brush Size %d", sz);
				const bool is_curr_sz = (current_size == sz);
				if (draw_icon_btn(btn_id, s_toolbar_icons.tex_sizes[sz - 1], sz_fallback, is_curr_sz, tip, size_btn_sz, size_icon_sz)) {
					g_gui.SetBrushSize(sz);
				}
				if (sz < 7) ImGui::SameLine();
			}

			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.45f, 0.48f, 0.55f, 1.0f), "|");
			ImGui::SameLine();

			// 11. Day/Night switch with uniform height
			const bool show_lights = g_settings.getBoolean(Config::SHOW_LIGHTS);
			if (draw_icon_btn("##tool_daynight", s_toolbar_icons.tex_day_night, "D/N", show_lights, "Toggle Day/Night Ambient Light", tool_btn_sz, tool_icon_sz)) {
				g_settings.setInteger(Config::SHOW_LIGHTS, show_lights ? 0 : 1);
				Refresh(false);
			}

			ImGui::SameLine();
			// 12. World Map / Weltkarte Button (Hotkey: M)
			const bool is_world_map = g_settings.getBoolean(Config::SHOW_WORLD_MAP);
			if (draw_icon_btn("##tool_worldmap", s_toolbar_icons.tex_world_map, "Map", is_world_map, "World Map / Weltkarte (Hotkey: M)\nExplore full world, set custom markers & fast travel", tool_btn_sz, tool_icon_sz)) {
				g_settings.setInteger(Config::SHOW_WORLD_MAP, is_world_map ? 0 : 1);
				Refresh(false);
			}

			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.45f, 0.48f, 0.55f, 1.0f), "|");
			ImGui::SameLine();

			// Live Multiplayer Indicator Badge in Toolbar
			if (editor.IsLive()) {
				ImGui::SameLine(0.0f, 6.0f * ui_scale);
				bool is_host = editor.IsLiveServer();
				int peer_count = (is_host && editor.GetLiveServer()) ? (int)editor.GetLiveServer()->clients.size() : 1;
				
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.22f, 0.16f, 0.85f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.12f, 0.35f, 0.25f, 0.95f));
				ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.30f, 0.85f, 0.50f, 0.80f));
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f * ui_scale);

				std::string badge_txt = is_host 
					? wxString::Format("🟢 Host (%d)", peer_count).ToStdString()
					: "🟢 Live";

				if (ImGui::SmallButton(badge_txt.c_str())) {
					g_settings.setInteger(Config::SHOW_CHAT, g_settings.getInteger(Config::SHOW_CHAT) ? 0 : 1);
				}
				if (ImGui::IsItemHovered()) {
					if (is_host) {
						ImGui::SetTooltip("Live Host Active: %d Connected Peer(s)\nClick to toggle Multiplayer Chat & User Hub", peer_count);
					} else {
						LiveClient* client = editor.GetLiveClient();
						uint32_t lat = client ? client->getLatency() : 0;
						ImGui::SetTooltip("Live Connected (%u ms)\nClick to toggle Multiplayer Chat & User Hub", lat);
					}
				}

				ImGui::PopStyleVar(2);
				ImGui::PopStyleColor(3);
			}

			ImGui::SameLine(0.0f, 4.0f * ui_scale);

			// 12. Move Toolbar Gem Control (Green circle: "Change Position")
			{
				const float gem_radius = 5.0f * ui_scale;
				const float gem_btn_sz = 14.0f * ui_scale;
				ImVec2 gpos = ImGui::GetCursorScreenPos();
				ImGui::InvisibleButton("##tb_pos_toggle", ImVec2(gem_btn_sz, gem_btn_sz));
				bool ghovered = ImGui::IsItemHovered();
				bool gactive = ImGui::IsItemActive();
				if (ImGui::IsItemClicked()) {
					int cur = g_settings.getInteger(Config::TOOLBAR_OVERLAY_POSITION);
					g_settings.setInteger(Config::TOOLBAR_OVERLAY_POSITION, (cur == 0) ? 1 : 0);
				}
				if (ghovered) ImGui::SetTooltip("Change Position");

				ImVec2 center(gpos.x + gem_btn_sz * 0.5f, gpos.y + gem_btn_sz * 0.5f);
				ImU32 col = gactive ? IM_COL32(30, 160, 90, 255) : (ghovered ? IM_COL32(55, 215, 130, 255) : IM_COL32(40, 180, 100, 220));
				ImDrawList* tbdl = ImGui::GetWindowDrawList();
				tbdl->AddCircleFilled(center, gem_radius, col, 16);
				tbdl->AddCircle(center, gem_radius, IM_COL32(200, 255, 220, ghovered ? 220 : 100), 16, 1.0f);
				if (ghovered) {
					tbdl->AddCircleFilled(center, gem_radius * 0.45f, IM_COL32(255, 255, 255, 230), 8);
				}
			}

			ImGui::SameLine(0.0f, 4.0f * ui_scale);

			// 13. Close Toolbar Gem Control (Red circle: "Close")
			{
				const float gem_radius = 5.0f * ui_scale;
				const float gem_btn_sz = 14.0f * ui_scale;
				ImVec2 rpos = ImGui::GetCursorScreenPos();
				ImGui::InvisibleButton("##tb_close", ImVec2(gem_btn_sz, gem_btn_sz));
				bool rhovered = ImGui::IsItemHovered();
				bool ractive = ImGui::IsItemActive();
				if (ImGui::IsItemClicked()) {
					g_settings.setInteger(Config::SHOW_TOOLBAR_BRUSHES, 0);
					if (g_gui.root) g_gui.root->UpdateMenubar();
				}
				if (rhovered) ImGui::SetTooltip("Close");

				ImVec2 center(rpos.x + gem_btn_sz * 0.5f, rpos.y + gem_btn_sz * 0.5f);
				ImU32 col = ractive ? IM_COL32(190, 40, 40, 255) : (rhovered ? IM_COL32(255, 80, 80, 255) : IM_COL32(220, 60, 60, 220));
				ImDrawList* tbdl = ImGui::GetWindowDrawList();
				tbdl->AddCircleFilled(center, gem_radius, col, 16);
				tbdl->AddCircle(center, gem_radius, IM_COL32(255, 210, 210, rhovered ? 220 : 100), 16, 1.0f);
				if (rhovered) {
					float hw = gem_radius * 0.45f;
					tbdl->AddLine(ImVec2(center.x - hw, center.y - hw), ImVec2(center.x + hw, center.y + hw), IM_COL32(255, 255, 255, 230), 1.2f);
					tbdl->AddLine(ImVec2(center.x - hw, center.y + hw), ImVec2(center.x + hw, center.y - hw), IM_COL32(255, 255, 255, 230), 1.2f);
				}
			}
		}
		ImGui::End();
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar(4);
	}

	enum class WindowControlAction {
		None,
		Dock,
		Minimize,
		Close
	};

	auto RenderFantasyWindowControls = [](const char* prefix, bool allow_minimize = false, bool is_minimized = false, const char* dock_tooltip = "Change Position", bool allow_dock = true) -> WindowControlAction {
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		const float radius = 5.0f;
		const float btn_size = 13.0f;
		const float spacing = 4.0f;
		
		WindowControlAction action = WindowControlAction::None;

		int count = (allow_dock ? 1 : 0) + (allow_minimize ? 1 : 0) + 1;
		float total_w = count * btn_size + (count - 1) * spacing;
		float win_w = ImGui::GetWindowWidth();
		float start_x = std::max(10.0f, win_w - total_w - 6.0f);
		ImGui::SameLine(start_x);

		// 1. Emerald / Rune Green Anchor Dot (Move)
		if (allow_dock) {
			ImVec2 pos = ImGui::GetCursorScreenPos();
			ImGui::InvisibleButton(wxString::Format("##%s_dock", prefix).c_str(), ImVec2(btn_size, btn_size));
			bool hovered = ImGui::IsItemHovered();
			bool active = ImGui::IsItemActive();
			if (ImGui::IsItemClicked()) action = WindowControlAction::Dock;
			if (hovered && dock_tooltip) ImGui::SetTooltip("%s", dock_tooltip);

			ImVec2 center(pos.x + btn_size * 0.5f, pos.y + btn_size * 0.5f);
			ImU32 col = active ? IM_COL32(30, 160, 90, 255) : (hovered ? IM_COL32(55, 215, 130, 255) : IM_COL32(40, 180, 100, 220));
			draw_list->AddCircleFilled(center, radius, col, 16);
			draw_list->AddCircle(center, radius, IM_COL32(200, 255, 220, hovered ? 220 : 100), 16, 1.0f);
			if (hovered) {
				draw_list->AddCircleFilled(center, radius * 0.45f, IM_COL32(255, 255, 255, 230), 8);
			}
		}

		// 2. Amber / Topaz Gold Minimize Dot (if allowed)
		if (allow_minimize) {
			if (allow_dock) ImGui::SameLine(0.0f, spacing);
			ImVec2 pos = ImGui::GetCursorScreenPos();
			ImGui::InvisibleButton(wxString::Format("##%s_min", prefix).c_str(), ImVec2(btn_size, btn_size));
			bool hovered = ImGui::IsItemHovered();
			bool active = ImGui::IsItemActive();
			if (ImGui::IsItemClicked()) action = WindowControlAction::Minimize;
			if (hovered) ImGui::SetTooltip(is_minimized ? "Expand Palette" : "Collapse Palette");

			ImVec2 center(pos.x + btn_size * 0.5f, pos.y + btn_size * 0.5f);
			ImU32 col = active ? IM_COL32(200, 140, 20, 255) : (hovered ? IM_COL32(255, 205, 55, 255) : IM_COL32(230, 170, 35, 220));
			draw_list->AddCircleFilled(center, radius, col, 16);
			draw_list->AddCircle(center, radius, IM_COL32(255, 245, 190, hovered ? 220 : 100), 16, 1.0f);
			if (hovered || is_minimized) {
				float hw = radius * 0.55f;
				draw_list->AddLine(ImVec2(center.x - hw, center.y), ImVec2(center.x + hw, center.y), IM_COL32(255, 255, 255, 230), 1.2f);
				if (is_minimized) {
					draw_list->AddLine(ImVec2(center.x, center.y - hw), ImVec2(center.x, center.y + hw), IM_COL32(255, 255, 255, 230), 1.2f);
				}
			}
		}

		// 3. Ruby / Dragon Red Close Dot
		{
			if (allow_dock || allow_minimize) ImGui::SameLine(0.0f, spacing);
			ImVec2 pos = ImGui::GetCursorScreenPos();
			ImGui::InvisibleButton(wxString::Format("##%s_close", prefix).c_str(), ImVec2(btn_size, btn_size));
			bool hovered = ImGui::IsItemHovered();
			bool active = ImGui::IsItemActive();
			if (ImGui::IsItemClicked()) action = WindowControlAction::Close;
			if (hovered) ImGui::SetTooltip("Close");

			ImVec2 center(pos.x + btn_size * 0.5f, pos.y + btn_size * 0.5f);
			ImU32 col = active ? IM_COL32(190, 40, 40, 255) : (hovered ? IM_COL32(255, 80, 80, 255) : IM_COL32(220, 60, 60, 220));
			draw_list->AddCircleFilled(center, radius, col, 16);
			draw_list->AddCircle(center, radius, IM_COL32(255, 210, 210, hovered ? 220 : 100), 16, 1.0f);
			if (hovered) {
				float hw = radius * 0.45f;
				draw_list->AddLine(ImVec2(center.x - hw, center.y - hw), ImVec2(center.x + hw, center.y + hw), IM_COL32(255, 255, 255, 230), 1.2f);
				draw_list->AddLine(ImVec2(center.x - hw, center.y + hw), ImVec2(center.x + hw, center.y - hw), IM_COL32(255, 255, 255, 230), 1.2f);
			}
		}

		return action;
	};

	// macOS Canvas Floating / Docked Tileset Palette in filigree fantasy look
	static bool pal_minimized = false;
	static int last_applied_dock_side = -1;
	const bool tb_active = g_settings.getBoolean(Config::SHOW_TOOLBAR_BRUSHES);
	const int tb_dock = std::clamp(g_settings.getInteger(Config::TOOLBAR_OVERLAY_POSITION), 0, 1);
	const float pal_width = 310.0f;

	if (g_settings.getBoolean(Config::SHOW_PALETTE)) {
		const int pal_dock = std::clamp(g_settings.getInteger(Config::PALETTE_DOCK_SIDE), 0, 1);
		const float pal_top = (tb_active && tb_dock == 0) ? 48.0f : 10.0f;
		const float pal_bottom_margin = (tb_active && tb_dock == 1) ? 52.0f : 10.0f;
		const float pal_h = std::clamp(io.DisplaySize.y - pal_top - pal_bottom_margin, 260.0f, 900.0f);

		ImVec2 pal_pos = (pal_dock == 0)
			? ImVec2(8.0f, pal_top)
			: ImVec2(io.DisplaySize.x - pal_width - 8.0f, pal_top);

		if (pal_minimized) {
			ImGui::SetNextWindowSize(ImVec2(240.0f, 32.0f), ImGuiCond_Always);
		} else {
			ImGui::SetNextWindowSize(ImVec2(pal_width, pal_h), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSizeConstraints(ImVec2(240.0f, 200.0f), ImVec2(500.0f, std::max(200.0f, io.DisplaySize.y - 35.0f)));
		}

		if (pal_dock != last_applied_dock_side) {
			ImGui::SetNextWindowPos(pal_pos, ImGuiCond_Always);
			last_applied_dock_side = pal_dock;
		} else {
			ImGui::SetNextWindowPos(pal_pos, ImGuiCond_FirstUseEver);
		}

		ImGui::SetNextWindowBgAlpha(0.88f);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 4.0f));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.07f, 0.11f, 0.90f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.55f, 0.45f, 0.25f, 0.65f));
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.18f, 0.23f, 0.35f, 0.85f));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.25f, 0.35f, 0.55f, 0.95f));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.30f, 0.45f, 0.70f, 1.00f));
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.13f, 0.20f, 0.85f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.16f, 0.20f, 0.30f, 0.95f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.22f, 0.28f, 0.40f, 1.00f));
		ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.10f, 0.13f, 0.20f, 0.85f));
		ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.25f, 0.35f, 0.55f, 0.95f));
		ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.20f, 0.30f, 0.48f, 1.00f));

		ImGuiWindowFlags pal_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
		if (pal_minimized) pal_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar;
		bool pal_open = true;

		if (ImGui::Begin("##CanvasPaletteFiligree", &pal_open, pal_flags)) {
			// Magnetic side snapping to left and right canvas edge:
			ImVec2 pal_curr_pos = ImGui::GetWindowPos();
			ImVec2 pal_curr_sz = ImGui::GetWindowSize();
			const float snapThreshold = 35.0f;
			const float snapMargin = 8.0f;
			if (pal_curr_pos.x < snapThreshold) {
				if (!ImGui::IsMouseDown(0)) {
					ImGui::SetWindowPos(ImVec2(snapMargin, pal_curr_pos.y), ImGuiCond_Always);
					g_settings.setInteger(Config::PALETTE_DOCK_SIDE, 0);
				} else {
					ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(0, 0), ImVec2(5.0f, io.DisplaySize.y), IM_COL32(80, 200, 255, 180));
				}
			} else if (io.DisplaySize.x - (pal_curr_pos.x + pal_curr_sz.x) < snapThreshold) {
				if (!ImGui::IsMouseDown(0)) {
					ImGui::SetWindowPos(ImVec2(io.DisplaySize.x - pal_curr_sz.x - snapMargin, pal_curr_pos.y), ImGuiCond_Always);
					g_settings.setInteger(Config::PALETTE_DOCK_SIDE, 1);
				} else {
					ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(io.DisplaySize.x - 5.0f, 0), ImVec2(io.DisplaySize.x, io.DisplaySize.y), IM_COL32(80, 200, 255, 180));
				}
			}

			// Header Title & Fantasy Gem Controls (Amber Minimize, Red Close)
			ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.35f, 1.0f), "Tileset Palette");

			WindowControlAction ctrl_act = RenderFantasyWindowControls("pal", true, pal_minimized, "Change Position", false);
			if (ctrl_act == WindowControlAction::Minimize) {
				pal_minimized = !pal_minimized;
			} else if (ctrl_act == WindowControlAction::Close) {
				g_settings.setInteger(Config::SHOW_PALETTE, 0);
				if (g_gui.root) g_gui.root->UpdateMenubar();
			}

			if (!pal_minimized) {
				ImGui::Separator();

				// Category Switcher (Favorites first, then classic order)
				static int current_cat_idx = 1; // Default to Terrain
				static int last_seen_cat_idx = -1;
				const char* categories[] = {
					"Favorites", "Terrain", "Doodads", "Items", "RAW", "Creatures", "Houses", "Waypoints", "Prefabs"
				};
				const TilesetCategoryType cat_types[] = {
					TILESET_FAVORITE, TILESET_TERRAIN, TILESET_DOODAD, TILESET_ITEM, TILESET_RAW, TILESET_CREATURE, TILESET_HOUSE, TILESET_WAYPOINT, TILESET_PREFAB
				};

				// Synchronize with active brush selected from canvas right-click
				Brush* cur_active_brush = g_gui.GetCurrentBrush();
				static Brush* s_last_synced_brush = nullptr;
				static bool s_need_scroll_to_brush = false;

				if (cur_active_brush && cur_active_brush != s_last_synced_brush) {
					s_last_synced_brush = cur_active_brush;
					s_need_scroll_to_brush = true;

					// Locate which category contains this brush
					bool found_cat = false;
					for (int c = 0; c < (int)IM_ARRAYSIZE(cat_types); ++c) {
						TilesetCategoryType ctype = cat_types[c];
						if (ctype == TILESET_FAVORITE) continue;
						for (auto& pair : g_materials.tilesets) {
							if (!pair.second) continue;
							if (pair.second->name == "Favorites" || pair.second->name == "Host-Favorites") continue;
							if (const TilesetCategory* cat = pair.second->getCategory(ctype)) {
								if (cat->containsBrush(cur_active_brush)) {
									current_cat_idx = c;
									found_cat = true;
									break;
								}
							}
						}
						if (found_cat) break;
					}
				}

				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				if (ImGui::Combo("##PalCategory", &current_cat_idx, categories, IM_ARRAYSIZE(categories))) {
					// manual switch
				}
				if (ImGui::IsItemHovered() && io.MouseWheel != 0.0f) {
					current_cat_idx = std::clamp(current_cat_idx - (int)io.MouseWheel, 0, (int)IM_ARRAYSIZE(categories) - 1);
				}

				TilesetCategoryType active_cat_type = cat_types[current_cat_idx];

				// Subcategory / Tileset Selector exactly matching classic palette (NO "All Tilesets")
				std::vector<std::pair<std::string, const TilesetCategory*>> available_tilesets;

				if (active_cat_type == TILESET_FAVORITE) {
					Tileset* favs = nullptr;
					auto fit = g_materials.tilesets.find("Favorites");
					if (fit != g_materials.tilesets.end()) favs = fit->second;

					if (favs) {
						struct FavCatDef {
							TilesetCategoryType type;
							const char* name;
						} fav_subcats[] = {
							{ TILESET_FAVORITE, "All Favorites" },
							{ TILESET_TERRAIN, "Terrain" },
							{ TILESET_DOODAD, "Doodads" },
							{ TILESET_ITEM, "Items" },
							{ TILESET_CREATURE, "Monsters" },
							{ TILESET_NPC, "NPCs" }
						};
						for (const auto& sc : fav_subcats) {
							if (const TilesetCategory* tcg = favs->getCategory(sc.type)) {
								if (tcg->size() > 0 || sc.type == TILESET_FAVORITE) {
									available_tilesets.push_back({sc.name, tcg});
								}
							}
						}
					}

					auto hfit = g_materials.tilesets.find("Host-Favorites");
					if (hfit != g_materials.tilesets.end() && hfit->second) {
						if (const TilesetCategory* htcg = hfit->second->getCategory(TILESET_FAVORITE)) {
							if (htcg->size() > 0) {
								available_tilesets.push_back({"Host-Favorites (All)", htcg});
							}
						}
					}
				} else {
					std::vector<Tileset*> sorted_tilesets;
					for (auto& pair : g_materials.tilesets) {
						if (!pair.second) continue;
						if (pair.second->name == "Favorites" || pair.second->name == "Host-Favorites") continue;
						sorted_tilesets.push_back(pair.second);
					}

					auto getTilesetRank = [](const std::string& name) -> int {
						if (name == "Nature") return 0;
						if (name == "City Grounds") return 1;
						return 10;
					};

					std::sort(sorted_tilesets.begin(), sorted_tilesets.end(), [&getTilesetRank](Tileset* a, Tileset* b) {
						if (!a && !b) return false;
						if (!a) return false;
						if (!b) return true;
						int rankA = getTilesetRank(a->name);
						int rankB = getTilesetRank(b->name);
						if (rankA != rankB) return rankA < rankB;
						return a->name < b->name;
					});

					for (Tileset* ts : sorted_tilesets) {
						if (const TilesetCategory* cat = ts->getCategory(active_cat_type)) {
							if (cat->size() > 0) {
								available_tilesets.push_back({ts->name, cat});
							}
						}
					}
				}

				static int selected_tileset_idx = 0;
				if (current_cat_idx != last_seen_cat_idx) {
					selected_tileset_idx = 0;
					last_seen_cat_idx = current_cat_idx;
				}

				// If syncing brush, pick the tileset that contains it
				if (s_need_scroll_to_brush && cur_active_brush) {
					for (size_t i = 0; i < available_tilesets.size(); ++i) {
						if (available_tilesets[i].second && available_tilesets[i].second->containsBrush(cur_active_brush)) {
							selected_tileset_idx = (int)i;
							break;
						}
					}
				}

				if (selected_tileset_idx >= (int)available_tilesets.size()) {
					selected_tileset_idx = 0;
				}

				std::vector<const char*> tileset_names;
				for (const auto& entry : available_tilesets) {
					tileset_names.push_back(entry.first.c_str());
				}

				if (!tileset_names.empty()) {
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
					ImGui::Combo("##PalTileset", &selected_tileset_idx, tileset_names.data(), (int)tileset_names.size());
					if (ImGui::IsItemHovered() && io.MouseWheel != 0.0f) {
						selected_tileset_idx = std::clamp(selected_tileset_idx - (int)io.MouseWheel, 0, (int)tileset_names.size() - 1);
					}
				}

				// Search Box with complete keyboard input support
				static char search_buf[128] = "";
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				ImGui::InputTextWithHint("##PalSearch", "Search by name or ID...", search_buf, sizeof(search_buf));

				std::string search_str = search_buf;
				for (auto& c : search_str) c = (char)tolower(c);

				// Scrollable Brush Grid / List with Collapsible Section Headers and Large Perspective Previews
				ImGui::BeginChild("##PalBrushList", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

				if (!available_tilesets.empty() && (size_t)selected_tileset_idx < available_tilesets.size()) {
					const auto& brushlist = available_tilesets[selected_tileset_idx].second->brushlist;

					bool current_section_collapsed = false;
					std::vector<Brush*> section_brushes;
					SeparatorBrush* current_sep = nullptr;

					// Composite texture cache for mountain assets, large doodads, roofs, walls, and items
					static std::unordered_map<uint32_t, GLuint> s_brush_composite_cache;

					auto get_brush_preview_tex = [](Brush* b) -> GLuint {
						if (!b) return 0;
						if (b->isCreature()) {
							CreatureBrush* cb = b->asCreature();
							CreatureType* ct = cb ? cb->getType() : nullptr;
							if (ct) {
								GameSprite* spr = g_gui.gfx.getCreatureSprite(ct->outfit.lookType);
								if (spr) {
									return spr->getHardwareID(0, 0, 2, ct->outfit.lookAddon, 0, ct->outfit, 0);
								}
							}
							return 0;
						}

						int look_id = b->getLookID();
						if (look_id == 0 && b->isWall()) {
							WallBrush* wb = b->asWall();
							look_id = wb->getWallItem(WALL_HORIZONTAL);
							if (look_id == 0) look_id = wb->getWallItem(WALL_VERTICAL);
						}

						if (look_id > 0) {
							auto it = s_brush_composite_cache.find((uint32_t)look_id);
							if (it != s_brush_composite_cache.end()) {
								return it->second;
							}

							GameSprite* gspr = nullptr;
							if (g_items.typeExists(look_id)) {
								gspr = g_items[look_id].sprite ? g_items[look_id].sprite : dynamic_cast<GameSprite*>(g_gui.gfx.getSprite(g_items[look_id].clientID));
							} else {
								gspr = dynamic_cast<GameSprite*>(g_gui.gfx.getSprite(look_id));
							}

							if (gspr) {
								wxBitmap* bmp = gspr->getBitmap(SPRITE_SIZE_32x32, false);
								if (bmp && bmp->IsOk()) {
									GLuint tex = ConvertBitmapToTexture(*bmp);
									if (tex != 0) {
										s_brush_composite_cache[(uint32_t)look_id] = tex;
										return tex;
									}
								}
								GLuint tex = gspr->getHardwareID(0, 0, 0, -1, 0, 0, 0, 0);
								if (tex != 0) {
									s_brush_composite_cache[(uint32_t)look_id] = tex;
									return tex;
								}
							}
						}
						return 0;
					};

					auto flush_section_tiles = [&](const std::vector<Brush*>& brushes) {
						if (brushes.empty()) return;
						float avail_w = ImGui::GetContentRegionAvail().x;
						float btn_dim = 48.0f;
						float btn_spacing = 4.0f;
						int cols = std::max(1, (int)((avail_w + btn_spacing) / (btn_dim + btn_spacing)));

						Brush* cur_active_brush = g_gui.GetCurrentBrush();

						for (size_t i = 0; i < brushes.size(); ++i) {
							Brush* b = brushes[i];
							if (!b || b->isSeparator()) continue;

							ImGui::PushID(b);

							bool is_sel = (cur_active_brush == b);
							if (is_sel) {
								ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.45f, 0.85f, 0.95f));
								ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.95f, 0.80f, 0.30f, 1.0f));
								ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f);

								if (s_need_scroll_to_brush) {
									ImGui::SetScrollHereY(0.5f);
									s_need_scroll_to_brush = false;
								}
							} else {
								ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f, 0.13f, 0.20f, 0.80f));
								ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.35f, 0.38f, 0.48f, 0.50f));
								ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
							}

							GLuint spr_tex = get_brush_preview_tex(b);

							if (spr_tex != 0) {
								if (ImGui::ImageButton("##tile_btn", (ImTextureID)(intptr_t)spr_tex, ImVec2(btn_dim - 6.0f, btn_dim - 6.0f))) {
									g_gui.SelectBrush(b, active_cat_type);
									g_gui.SelectBrushInternal(b);
								}
							} else {
								std::string short_label = b->getName().substr(0, std::min<size_t>(4, b->getName().size()));
								if (ImGui::Button(short_label.c_str(), ImVec2(btn_dim, btn_dim))) {
									g_gui.SelectBrush(b, active_cat_type);
									g_gui.SelectBrushInternal(b);
								}
							}

							ImGui::PopStyleVar();
							ImGui::PopStyleColor(2);

							if (ImGui::IsItemHovered()) {
								ImGui::SetTooltip("%s (ID: %d)", b->getName().c_str(), b->getLookID());
							}

							if ((i + 1) % cols != 0 && (i + 1) < brushes.size()) {
								ImGui::SameLine();
							}

							ImGui::PopID();
						}
					};

					if (!search_str.empty()) {
						bool any_found = false;
						for (const auto& ts_entry : available_tilesets) {
							std::vector<Brush*> matched;
							for (Brush* b : ts_entry.second->brushlist) {
								if (!b || b->isSeparator()) continue;
								std::string bname = b->getName();
								for (auto& c : bname) c = (char)tolower(c);
								std::string id_str = std::to_string(b->getLookID());
								if (bname.find(search_str) != std::string::npos || id_str.find(search_str) != std::string::npos) {
									matched.push_back(b);
								}
							}
							if (!matched.empty()) {
								any_found = true;
								ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.35f, 1.0f), "v %s", ts_entry.first.c_str());
								flush_section_tiles(matched);
							}
						}
						if (!any_found) {
							ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.7f, 0.8f), "No matching tiles found in this category.");
						}
					} else {
						for (Brush* b : brushlist) {
							if (!b) continue;
							if (b->isSeparator()) {
								if (!current_section_collapsed && !section_brushes.empty()) {
									flush_section_tiles(section_brushes);
									section_brushes.clear();
								} else {
									section_brushes.clear();
								}

								current_sep = b->asSeparator();
								current_section_collapsed = current_sep ? current_sep->isCollapsed() : false;

								if (current_sep && current_section_collapsed && s_need_scroll_to_brush && cur_active_brush) {
									current_sep->toggleCollapsed();
									current_section_collapsed = false;
								}

								std::string header_label = (current_sep && !current_sep->getName().empty()) ? current_sep->getName() : "Section";
								std::string arrow = current_section_collapsed ? "> " : "v ";
								std::string full_title = arrow + header_label;

								ImGui::PushID(b);
								ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.16f, 0.24f, 0.80f));
								ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.28f, 0.42f, 0.95f));
								ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.82f, 0.35f, 1.0f));
								ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));

								if (ImGui::Button(full_title.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 22.0f))) {
									if (current_sep) {
										current_sep->toggleCollapsed();
									}
								}

								ImGui::PopStyleVar();
								ImGui::PopStyleColor(3);
								ImGui::PopID();
							} else {
								if (!current_section_collapsed) {
									section_brushes.push_back(b);
								}
							}
						}
						if (!current_section_collapsed && !section_brushes.empty()) {
							flush_section_tiles(section_brushes);
						}
					}
				}
				ImGui::EndChild();
			}
		}
		ImGui::End();

		ImGui::PopStyleColor(11);
		ImGui::PopStyleVar(4);
	}

	if (ui_toolbar && ui_toolbar->isVisible() && drawer->GetNanoVGContext()) {
		nvgBeginFrame(drawer->GetNanoVGContext(), (float)w, (float)h, (float)GetContentScaleFactor());
		ui_toolbar->render(drawer->GetNanoVGContext());
		nvgEndFrame(drawer->GetNanoVGContext());
	}

	// Interactive World Map (Weltkarte) System (Hotkey: M)
	RenderWorldMapOverlay();

	// Keep coordinates and hovered item information parallel and exactly 5px above the bottom editor edge
	if (g_settings.getInteger(Config::CANVAS_INFO_CORNER) >= 0) {
		const Position info_pos = GetCursorPosition();
		wxString info_text = wxString::Format("X: %d  Y: %d  Z: %d", info_pos.x, info_pos.y, info_pos.z);
		wxString item_text = "Item ID: -";
		if (Tile* info_tile = editor.map.getTile(info_pos)) {
			if (Item* info_item = info_tile->getTopItem()) {
				if (!info_item->getName().empty()) {
					item_text = wxString::Format("Item ID: %d (%s)", info_item->getID(), wxstr(info_item->getName()));
				} else {
					item_text = wxString::Format("Item ID: %d", info_item->getID());
				}
			}
		}

		int corner = std::clamp(g_settings.getInteger(Config::CANVAS_INFO_CORNER), 0, 3);
		const bool minimap_on = g_settings.getBoolean(Config::MINIMAP_VISIBLE);
		const int minimap_corner = std::clamp(g_settings.getInteger(Config::MINIMAP_CORNER), 0, 3);

		// Guarantee Minimap and Coordinates NEVER share the same corner
		if (minimap_on && corner == minimap_corner) {
			corner = (minimap_corner == 0) ? 3 : ((minimap_corner + 2) % 4);
		}

		const float margin_x = 10.0f;
		const float margin_bottom = 5.0f; // Exactly 5px distance to bottom editor edge
		const float margin_top = (tb_active && tb_dock == 0) ? 48.0f : 10.0f;

		ImVec2 pivot(1.0f, 0.0f);
		ImVec2 anchor(io.DisplaySize.x - margin_x, margin_top);

		if (corner == 0) { // Top-Right
			anchor = ImVec2(io.DisplaySize.x - margin_x, margin_top);
			pivot = ImVec2(1.0f, 0.0f);
		} else if (corner == 1) { // Top-Left
			anchor = ImVec2(margin_x, margin_top);
			pivot = ImVec2(0.0f, 0.0f);
		} else if (corner == 2) { // Bottom-Right (parallel 5px from bottom)
			anchor = ImVec2(io.DisplaySize.x - margin_x, io.DisplaySize.y - margin_bottom);
			pivot = ImVec2(1.0f, 1.0f);
		} else if (corner == 3) { // Bottom-Left (parallel 5px from bottom)
			anchor = ImVec2(margin_x, io.DisplaySize.y - margin_bottom);
			pivot = ImVec2(0.0f, 1.0f);
		}

		ImGui::SetNextWindowBgAlpha(0.85f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 6.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 2.0f));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.07f, 0.11f, 0.92f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.55f, 0.45f, 0.25f, 0.65f));

		ImGuiWindowFlags info_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
			ImGuiWindowFlags_NoInputs;

		ImGui::SetNextWindowPos(anchor, ImGuiCond_Always, pivot);
		if (ImGui::Begin("##CanvasInfoOverlay", nullptr, info_flags)) {
			ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.35f, 1.0f), "%s", info_text.ToUTF8().data());
			ImGui::TextColored(ImVec4(0.75f, 0.85f, 0.98f, 1.0f), "%s", item_text.ToUTF8().data());
		}
		ImGui::End();
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar(4);
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
							// For doors locked via Action ID 100 show a simple "Locked" popup
							if (item->isDoor() && aid == 100) {
								bubbles.push_back({ "Locked", std::string("Locked"), ImVec4(0.9f, 0.45f, 0.45f, 1.0f), ImVec4(0.75f, 0.35f, 0.35f, 1.0f) });
							} else if (aid > 0 || uid > 0) {
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

			// 1. Render Animated Radar Pings (Local & Network)
			for (auto it = active_pings.begin(); it != active_pings.end();) {
				float dt = (now_ms - it->start_time_ms) / 1000.0f;
				if (dt > 2.8f) {
					it = active_pings.erase(it);
					continue;
				}

				if (it->pos.z == floor) {
					float px = (((it->pos.x * TileSize - scroll_x) - offset) / zoom) + (TileSize * 0.5f / zoom);
					float py = (((it->pos.y * TileSize - scroll_y) - offset) / zoom) + (TileSize * 0.5f / zoom);

					float progress = dt / 2.8f;
					float r1 = 6.0f + 45.0f * progress;
					float r2 = 12.0f + 75.0f * progress;
					float r3 = 18.0f + 105.0f * progress;
					int a1 = static_cast<int>(255 * (1.0f - progress));
					int a2 = static_cast<int>(190 * (1.0f - progress));
					int a3 = static_cast<int>(120 * (1.0f - progress));

					// Concentric radar waves
					draw_list->AddCircle(ImVec2(px, py), r1, IM_COL32(0, 240, 255, a1), 36, 2.5f);
					draw_list->AddCircle(ImVec2(px, py), r2, IM_COL32(255, 215, 50, a2), 36, 2.0f);
					draw_list->AddCircle(ImVec2(px, py), r3, IM_COL32(0, 200, 255, a3), 36, 1.2f);

					// Diamond center
					const float dsz = 5.0f;
					draw_list->AddQuadFilled(ImVec2(px, py - dsz), ImVec2(px + dsz, py), ImVec2(px, py + dsz), ImVec2(px - dsz, py), IM_COL32(255, 255, 255, a1));
					draw_list->AddText(ImVec2(px + 10.0f, py - 10.0f), IM_COL32(0, 255, 200, a1), "PING");
				}
				++it;
			}

			if (editor.IsLive()) {
				LiveSocket& live = editor.GetLive();
				for (auto it = live.activePings.begin(); it != live.activePings.end();) {
					float dt = (now_ms - it->timestamp) / 1000.0f;
					if (dt > 2.8f) {
						it = live.activePings.erase(it);
						continue;
					}

					if (it->pos.z == floor) {
						float px = (((it->pos.x * TileSize - scroll_x) - offset) / zoom) + (TileSize * 0.5f / zoom);
						float py = (((it->pos.y * TileSize - scroll_y) - offset) / zoom) + (TileSize * 0.5f / zoom);

						float progress = dt / 2.8f;
						float r1 = 6.0f + 45.0f * progress;
						float r2 = 12.0f + 75.0f * progress;
						float r3 = 18.0f + 105.0f * progress;
						int a1 = static_cast<int>(255 * (1.0f - progress));
						int a2 = static_cast<int>(190 * (1.0f - progress));
						int a3 = static_cast<int>(120 * (1.0f - progress));

						draw_list->AddCircle(ImVec2(px, py), r1, IM_COL32(0, 240, 255, a1), 36, 2.5f);
						draw_list->AddCircle(ImVec2(px, py), r2, IM_COL32(255, 215, 50, a2), 36, 2.0f);
						draw_list->AddCircle(ImVec2(px, py), r3, IM_COL32(0, 200, 255, a3), 36, 1.2f);

						const float dsz = 5.0f;
						draw_list->AddQuadFilled(ImVec2(px, py - dsz), ImVec2(px + dsz, py), ImVec2(px, py + dsz), ImVec2(px - dsz, py), IM_COL32(255, 255, 255, a1));

						std::string sender = it->senderName.empty() ? "PING" : ("PING: " + nstr(it->senderName));
						draw_list->AddText(ImVec2(px + 10.0f, py - 10.0f), IM_COL32(255, 230, 100, a1), sender.c_str());
					}
					++it;
				}
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

void MapCanvas::RenderWorldMapOverlay() {
	if (!g_settings.getBoolean(Config::SHOW_WORLD_MAP)) {
		return;
	}

	ImGuiIO& io = ImGui::GetIO();
	Editor& editor = *g_gui.GetCurrentEditor();
	const int map_w = editor.map.getWidth();
	const int map_h = editor.map.getHeight();
	if (map_w <= 0 || map_h <= 0) return;

	// Load Table.jpg texture
	static GLuint s_table_tex_id = 0;
	static bool s_table_tex_loaded = false;
	if (!s_table_tex_loaded) {
		s_table_tex_loaded = true;
		wxImage img;
		wxString exeDir = wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH;
		wxString cwdDir = wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH;
		std::vector<wxString> paths = {
			"icons/Table.jpg", "../icons/Table.jpg", "Map Editor/icons/Table.jpg",
			exeDir + "Table.jpg", cwdDir + "Table.jpg",
			"icons/table.jpg", "../icons/table.jpg", exeDir + "table.jpg", cwdDir + "table.jpg"
		};
		for (const auto& p : paths) {
			if (wxFileExists(p) && img.LoadFile(p)) {
				wxBitmap bmp(img);
				s_table_tex_id = ConvertBitmapToTexture(bmp);
				break;
			}
		}
	}

	// Synchronize markers with current map file
	static std::string last_synced_map_path;
	std::string cur_map_path = editor.map.hasFile() ? editor.map.getFilename() : "";
	if (cur_map_path != last_synced_map_path) {
		WorldMapMarkerManager::GetInstance().SetCurrentMapPath(cur_map_path);
		last_synced_map_path = cur_map_path;
	}

	// Update texture for active world map floor
	UpdateWorldMapTexture(world_map_floor);

	// Proportional Window Dimensions matching 16:9 Table.jpg perfectly
	const float default_win_h = std::clamp(io.DisplaySize.y * 0.88f, 560.0f, 960.0f);
	const float default_table_h = default_win_h - 75.0f;
	const float default_table_w = default_table_h * (16.0f / 9.0f);
	const float default_win_w = std::min(default_table_w + 270.0f + 30.0f, io.DisplaySize.x * 0.96f);

	ImGui::SetNextWindowSize(ImVec2(default_win_w, default_win_h), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - default_win_w) * 0.5f, (io.DisplaySize.y - default_win_h) * 0.5f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowBgAlpha(0.96f);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 6.0f));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.06f, 0.09f, 0.97f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.85f, 0.70f, 0.32f, 0.90f)); // Radiant Antique Gold

	bool wm_open = true;
	ImGuiWindowFlags wm_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

	static char search_filter[128] = "";
	static bool show_pins = true;
	static bool show_towns = true;
	static bool show_viewport_box = true;
	static bool show_peers = true;
	static int active_tab = 0; // 0: Pins, 1: Towns, 2: Multiplayer

	// Marker Category Definitions & Stylized Colored Badges (Pure ASCII to avoid '?' font missing glyphs)
	struct MarkerCatInfo {
		const char* name;
		const char* badge;
		ImU32 fill_color;
		ImU32 border_color;
		ImU32 text_color;
		const char* icon;
	};

	static const MarkerCatInfo s_cat_info[8] = {
		{ "Waypoint", "[WP]", IM_COL32(14, 165, 233, 255),  IM_COL32(56, 189, 248, 255),  IM_COL32(186, 230, 253, 255), "*" }, // Cyan Waypoint
		{ "Town",     "[TW]", IM_COL32(217, 119, 6, 255),    IM_COL32(251, 191, 36, 255),  IM_COL32(254, 240, 138, 255), "+" }, // Amber Castle/Town
		{ "Quest",    "[QS]", IM_COL32(234, 88, 12, 255),    IM_COL32(249, 115, 22, 255),  IM_COL32(254, 215, 170, 255), "!" }, // Orange Quest
		{ "Dungeon",  "[DG]", IM_COL32(147, 51, 234, 255),   IM_COL32(192, 132, 252, 255), IM_COL32(233, 213, 255, 255), "@" }, // Violet Dungeon
		{ "Treasure", "[TR]", IM_COL32(202, 138, 4, 255),    IM_COL32(250, 204, 21, 255),  IM_COL32(254, 240, 138, 255), "$" }, // Gold Treasure
		{ "Base",     "[BS]", IM_COL32(22, 163, 74, 255),    IM_COL32(74, 222, 128, 255),  IM_COL32(187, 247, 208, 255), "#" }, // Green Base
		{ "Special",  "[SP]", IM_COL32(219, 39, 119, 255),   IM_COL32(244, 114, 182, 255), IM_COL32(251, 207, 232, 255), "~" }, // Magenta Portal
		{ "Danger",   "[DN]", IM_COL32(220, 38, 38, 255),    IM_COL32(248, 113, 113, 255), IM_COL32(254, 202, 202, 255), "X" }  // Crimson Danger
	};

	// Modal state for adding/editing marker
	static bool modal_marker_open = false;
	static WorldMapMarker editing_marker;
	static bool is_editing_existing = false;
	static char marker_title_buf[128] = "";
	static char marker_desc_buf[256] = "";

	if (ImGui::Begin("WELTKARTE / WORLD MAP", &wm_open, wm_flags)) {
		ImDrawList* wdl = ImGui::GetWindowDrawList();
		ImVec2 wpos = ImGui::GetWindowPos();
		ImVec2 wsz = ImGui::GetWindowSize();

		// Corner Golden Rivets
		wdl->AddCircleFilled(ImVec2(wpos.x + 8, wpos.y + 8), 3.0f, IM_COL32(255, 215, 80, 255));
		wdl->AddCircleFilled(ImVec2(wpos.x + wsz.x - 8, wpos.y + 8), 3.0f, IM_COL32(255, 215, 80, 255));
		wdl->AddCircleFilled(ImVec2(wpos.x + 8, wpos.y + wsz.y - 8), 3.0f, IM_COL32(255, 215, 80, 255));
		wdl->AddCircleFilled(ImVec2(wpos.x + wsz.x - 8, wpos.y + wsz.y - 8), 3.0f, IM_COL32(255, 215, 80, 255));

		// Top Controls Row
		ImGui::SetNextItemWidth(200.0f);
		ImGui::InputTextWithHint("##WMSearchFilter", "Search Pins & Towns...", search_filter, sizeof(search_filter));

		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.0f), "|");
		ImGui::SameLine();

		// Floor Dropdown Combo with Mousewheel hover scrolling (without < & > buttons)
		const char* floor_names[16] = {
			"Floor 0 (Sky 7)",
			"Floor 1 (Sky 6)",
			"Floor 2 (Sky 5)",
			"Floor 3 (Sky 4)",
			"Floor 4 (Sky 3)",
			"Floor 5 (Sky 2)",
			"Floor 6 (Sky 1)",
			"Floor 7 (Surface / Ground)",
			"Floor 8 (Underground -1)",
			"Floor 9 (Underground -2)",
			"Floor 10 (Underground -3)",
			"Floor 11 (Underground -4)",
			"Floor 12 (Underground -5)",
			"Floor 13 (Underground -6)",
			"Floor 14 (Underground -7)",
			"Floor 15 (Underground -8)"
		};

		ImGui::SetNextItemWidth(215.0f);
		if (ImGui::Combo("##WMFloorCombo", &world_map_floor, floor_names, 16)) {
			UpdateWorldMapTexture(world_map_floor);
		}
		if (ImGui::IsItemHovered() && io.MouseWheel != 0.0f) {
			int new_floor = std::clamp(world_map_floor - (int)io.MouseWheel, 0, 15);
			if (new_floor != world_map_floor) {
				world_map_floor = new_floor;
				UpdateWorldMapTexture(world_map_floor);
			}
		}

		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.0f), "|");
		ImGui::SameLine();

		if (ImGui::Button("Fit World##WMFit", ImVec2(74, 22))) {
			world_map_zoom = 1.0f;
			world_map_center_x = (float)(map_w / 2);
			world_map_center_y = (float)(map_h / 2);
			UpdateWorldMapTexture(world_map_floor);
		}
		ImGui::SameLine();
		if (ImGui::Button("Focus 1:1##WMFocusCenter", ImVec2(74, 22))) {
			int cx = map_w / 2, cy = map_h / 2;
			GetScreenCenter(&cx, &cy);
			world_map_center_x = (float)cx;
			world_map_center_y = (float)cy;
			world_map_zoom = 256.0f / (float)std::max(map_w, map_h);
			UpdateWorldMapTexture(world_map_floor);
		}

		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.0f), "|");
		ImGui::SameLine();

		ImGui::Checkbox("Pins", &show_pins);
		ImGui::SameLine();
		ImGui::Checkbox("Towns", &show_towns);
		ImGui::SameLine();
		ImGui::Checkbox("Viewport", &show_viewport_box);

		if (editor.IsLiveClient()) {
			ImGui::SameLine();
			ImGui::Checkbox("Multiplayer Live", &show_peers);
		}

		ImGui::Separator();

		// Main Content Area: Split Sidebar (Left) and Interactive World Map Canvas (Right)
		const float avail_h = ImGui::GetContentRegionAvail().y - 4.0f;
		const float sidebar_w = 265.0f;

		// --- Left Sidebar ---
		ImGui::BeginChild("##WMSidebar", ImVec2(sidebar_w, avail_h), true);
		{
			auto& markerMgr = WorldMapMarkerManager::GetInstance();
			auto& markerList = markerMgr.GetMarkers();

			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
			if (ImGui::Button("Pins", ImVec2(74, 24))) active_tab = 0;
			ImGui::SameLine();
			if (ImGui::Button("Towns", ImVec2(74, 24))) active_tab = 1;
			if (editor.IsLiveClient()) {
				ImGui::SameLine();
				if (ImGui::Button("Live MP", ImVec2(74, 24))) active_tab = 2;
			}
			ImGui::PopStyleVar();
			ImGui::Separator();

			std::string filter_str = search_filter;
			std::transform(filter_str.begin(), filter_str.end(), filter_str.begin(), ::tolower);

			if (active_tab == 0) {
				ImGui::TextColored(ImVec4(0.85f, 0.75f, 0.35f, 1.0f), "Custom Pins (%zu)", markerList.size());
				ImGui::Separator();

				for (size_t i = 0; i < markerList.size(); ++i) {
					const auto& m = markerList[i];
					std::string title_lower = m.title;
					std::transform(title_lower.begin(), title_lower.end(), title_lower.begin(), ::tolower);
					if (!filter_str.empty() && title_lower.find(filter_str) == std::string::npos) continue;

					int cat_idx = std::clamp(m.category, 0, 7);
					const auto& info = s_cat_info[cat_idx];

					ImGui::PushID((int)m.id);
					ImVec4 tag_col((float)(info.fill_color & 0xFF) / 255.0f,
					               (float)((info.fill_color >> 8) & 0xFF) / 255.0f,
					               (float)((info.fill_color >> 16) & 0xFF) / 255.0f, 1.0f);
					ImGui::TextColored(tag_col, "%s %s", info.icon, info.badge);
					ImGui::SameLine();
					ImGui::TextColored(ImVec4(0.95f, 0.90f, 0.75f, 1.0f), "%s", m.title.c_str());

					ImGui::TextColored(ImVec4(0.60f, 0.65f, 0.75f, 0.85f), "Pos: %d, %d, %d", m.x, m.y, m.z);
					if (!m.description.empty()) {
						ImGui::TextWrapped("%s", m.description.c_str());
					}

					if (ImGui::SmallButton("Go")) {
						g_gui.SetScreenCenterPosition(Position(m.x, m.y, m.z), false);
						world_map_floor = m.z;
						world_map_center_x = (float)m.x;
						world_map_center_y = (float)m.y;
						UpdateWorldMapTexture(world_map_floor);
						Refresh(false);
					}
					ImGui::SameLine();
					if (ImGui::SmallButton("Edit")) {
						editing_marker = m;
						strncpy(marker_title_buf, m.title.c_str(), sizeof(marker_title_buf));
						strncpy(marker_desc_buf, m.description.c_str(), sizeof(marker_desc_buf));
						is_editing_existing = true;
						modal_marker_open = true;
					}
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.40f, 0.15f, 0.15f, 0.70f));
					if (ImGui::SmallButton("Del")) {
						markerMgr.RemoveMarker(m.id);
						ImGui::PopStyleColor();
						ImGui::PopID();
						break;
					}
					ImGui::PopStyleColor();
					ImGui::Separator();
					ImGui::PopID();
				}
			} else if (active_tab == 1) {
				// Towns Tab
				const Towns& towns = editor.map.towns;
				ImGui::TextColored(ImVec4(0.85f, 0.75f, 0.35f, 1.0f), "Towns & Temples (%u)", towns.count());
				ImGui::Separator();

				for (auto it = towns.begin(); it != towns.end(); ++it) {
					Town* town = it->second;
					if (!town || town->getName().empty()) continue;
					std::string name_lower = town->getName();
					std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
					if (!filter_str.empty() && name_lower.find(filter_str) == std::string::npos) continue;

					Position tpos = town->getTemplePosition();
					ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.40f, 1.0f), "[Town] %s", town->getName().c_str());
					ImGui::TextColored(ImVec4(0.60f, 0.65f, 0.75f, 0.85f), "Temple: %d, %d, %d", tpos.x, tpos.y, tpos.z);

					ImGui::PushID(town->getID());
					if (ImGui::SmallButton("Go to Temple")) {
						g_gui.SetScreenCenterPosition(tpos, false);
						world_map_floor = tpos.z;
						world_map_center_x = (float)tpos.x;
						world_map_center_y = (float)tpos.y;
						UpdateWorldMapTexture(world_map_floor);
						Refresh(false);
					}
					ImGui::PopID();
					ImGui::Separator();
				}
			} else if (active_tab == 2 && editor.IsLiveClient()) {
				// Multiplayer Peers Tab
				LiveClient* lc = editor.GetLiveClient();
				auto cursors = lc ? lc->getCursorList() : std::vector<LiveCursor>();
				ImGui::TextColored(ImVec4(0.40f, 0.85f, 0.45f, 1.0f), "Live Session Co-Editors (%zu)", cursors.size());
				ImGui::Separator();

				for (const auto& cur : cursors) {
					ImGui::PushID((int)cur.id);
					ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.40f, 1.0f), "Co-Editor #%u", cur.id);
					ImGui::TextColored(ImVec4(0.60f, 0.65f, 0.75f, 0.85f), "Pos: %d, %d, %d", cur.pos.x, cur.pos.y, cur.pos.z);
					if (ImGui::SmallButton("Jump to Player")) {
						g_gui.SetScreenCenterPosition(cur.pos, false);
						world_map_floor = cur.pos.z;
						world_map_center_x = (float)cur.pos.x;
						world_map_center_y = (float)cur.pos.y;
						UpdateWorldMapTexture(world_map_floor);
						Refresh(false);
					}
					ImGui::Separator();
					ImGui::PopID();
				}
			}
		}
		ImGui::EndChild();

		ImGui::SameLine();

		// --- Right Area: Table Texture Background with Centered Wooden Map Desk (16:9 Aspect Ratio) ---
		const float avail_w = ImGui::GetContentRegionAvail().x;
		const float avail_h_desk = avail_h;

		// Exact 16:9 proportional fitting so Table.jpg is never stretched or squashed
		float map_view_w = avail_w;
		float map_view_h = map_view_w * (9.0f / 16.0f);
		if (map_view_h > avail_h_desk) {
			map_view_h = avail_h_desk;
			map_view_w = map_view_h * (16.0f / 9.0f);
		}
		float offset_x = (avail_w - map_view_w) * 0.5f;
		float offset_y = (avail_h_desk - map_view_h) * 0.5f;
		ImVec2 cur_pos = ImGui::GetCursorScreenPos();
		ImVec2 map_pos(cur_pos.x + offset_x, cur_pos.y + offset_y);
		ImVec2 map_sz(map_view_w, map_view_h);

		ImGui::SetCursorScreenPos(map_pos);
		ImGui::InvisibleButton("##WorldMapCanvasInteractive", map_sz);
		bool wm_hovered = ImGui::IsItemHovered();
		bool wm_active = ImGui::IsItemActive();
		bool is_window_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

		ImDrawList* mdl = ImGui::GetWindowDrawList();

		// 1. Draw Adventure Desk Background (Table.jpg)
		if (s_table_tex_id != 0) {
			mdl->AddImage((ImTextureID)(intptr_t)s_table_tex_id, map_pos, ImVec2(map_pos.x + map_sz.x, map_pos.y + map_sz.y));
		} else {
			mdl->AddRectFilled(map_pos, ImVec2(map_pos.x + map_sz.x, map_pos.y + map_sz.y), IM_COL32(24, 18, 12, 255), 6.0f);
		}

		// 2. Define the Inner Center Desk Area where the map is placed (2% smaller, strictly inside wooden surface, no gold border)
		ImVec2 desk_min(map_pos.x + map_sz.x * 0.262f, map_pos.y + map_sz.y * 0.220f);
		ImVec2 desk_max(map_pos.x + map_sz.x * 0.748f, map_pos.y + map_sz.y * 0.782f);
		ImVec2 desk_sz(desk_max.x - desk_min.x, desk_max.y - desk_min.y);

		// Navigation: WASD smooth panning while window is active
		if (is_window_focused) {
			float pan_delta = 450.0f * io.DeltaTime;
			if (io.KeyShift) pan_delta *= 2.5f;
			bool moved = false;

			if (ImGui::IsKeyDown(ImGuiKey_W) || ImGui::IsKeyDown(ImGuiKey_UpArrow)) {
				world_map_center_y -= (float)world_map_span_h * pan_delta * 0.002f;
				moved = true;
			}
			if (ImGui::IsKeyDown(ImGuiKey_S) || ImGui::IsKeyDown(ImGuiKey_DownArrow)) {
				world_map_center_y += (float)world_map_span_h * pan_delta * 0.002f;
				moved = true;
			}
			if (ImGui::IsKeyDown(ImGuiKey_A) || ImGui::IsKeyDown(ImGuiKey_LeftArrow)) {
				world_map_center_x -= (float)world_map_span_w * pan_delta * 0.002f;
				moved = true;
			}
			if (ImGui::IsKeyDown(ImGuiKey_D) || ImGui::IsKeyDown(ImGuiKey_RightArrow)) {
				world_map_center_x += (float)world_map_span_w * pan_delta * 0.002f;
				moved = true;
			}

			if (moved) {
				world_map_center_x = std::clamp(world_map_center_x, 0.0f, (float)map_w);
				world_map_center_y = std::clamp(world_map_center_y, 0.0f, (float)map_h);
				UpdateWorldMapTexture(world_map_floor);
			}
		}

		// Navigation: Mouse Drag with Middle Mouse Button (Wheel Press), Left Drag or Right Drag
		if (wm_active && (ImGui::IsMouseDragging(2, 0.0f) || ImGui::IsMouseDragging(0, 4.0f) || ImGui::IsMouseDragging(1, 4.0f))) {
			ImVec2 drag_delta = io.MouseDelta;
			float move_scale_x = (float)world_map_span_w / desk_sz.x;
			float move_scale_y = (float)world_map_span_h / desk_sz.y;
			world_map_center_x -= drag_delta.x * move_scale_x;
			world_map_center_y -= drag_delta.y * move_scale_y;
			world_map_center_x = std::clamp(world_map_center_x, 0.0f, (float)map_w);
			world_map_center_y = std::clamp(world_map_center_y, 0.0f, (float)map_h);
			UpdateWorldMapTexture(world_map_floor);
		}

		// Navigation: Deep Zooming with Mouse Wheel
		if (wm_hovered && io.MouseWheel != 0.0f) {
			if (io.MouseWheel > 0.0f) world_map_zoom = std::clamp(world_map_zoom * 0.80f, 0.0005f, 2.0f);
			else world_map_zoom = std::clamp(world_map_zoom * 1.25f, 0.0005f, 2.0f);
			UpdateWorldMapTexture(world_map_floor);
		}

		// 1. Outer Ambient Occlusion & Multi-Layer Drop Shadow onto the table wood
		mdl->AddRectFilled(ImVec2(desk_min.x - 7.0f, desk_min.y - 7.0f), ImVec2(desk_max.x + 7.0f, desk_max.y + 7.0f), IM_COL32(0, 0, 0, 35), 6.0f);
		mdl->AddRectFilled(ImVec2(desk_min.x - 5.0f, desk_min.y - 5.0f), ImVec2(desk_max.x + 5.0f, desk_max.y + 5.0f), IM_COL32(0, 0, 0, 60), 5.0f);
		mdl->AddRectFilled(ImVec2(desk_min.x - 3.0f, desk_min.y - 3.0f), ImVec2(desk_max.x + 3.0f, desk_max.y + 3.0f), IM_COL32(0, 0, 0, 95), 4.0f);
		mdl->AddRectFilled(ImVec2(desk_min.x - 1.0f, desk_min.y - 1.0f), ImVec2(desk_max.x + 1.0f, desk_max.y + 1.0f), IM_COL32(0, 0, 0, 140), 3.0f);

		// Push Clipping Rect strictly to the center wooden desk
		mdl->PushClipRect(desk_min, desk_max, true);

		// Render Map Texture in Center Desk
		if (world_map_tex_id != 0) {
			mdl->AddImage((ImTextureID)(intptr_t)world_map_tex_id, desk_min, desk_max);
		}

		// 2. Inner Aged Parchment Vignette Shading (Soft feathered edge falloff marrying map to the wooden desk)
		const float v_size = 18.0f;
		mdl->AddRectFilledMultiColor(desk_min, ImVec2(desk_max.x, desk_min.y + v_size),
			IM_COL32(24, 15, 8, 210), IM_COL32(24, 15, 8, 210), IM_COL32(24, 15, 8, 0), IM_COL32(24, 15, 8, 0));
		mdl->AddRectFilledMultiColor(ImVec2(desk_min.x, desk_max.y - v_size), desk_max,
			IM_COL32(24, 15, 8, 0), IM_COL32(24, 15, 8, 0), IM_COL32(24, 15, 8, 210), IM_COL32(24, 15, 8, 210));
		mdl->AddRectFilledMultiColor(desk_min, ImVec2(desk_min.x + v_size, desk_max.y),
			IM_COL32(24, 15, 8, 210), IM_COL32(24, 15, 8, 0), IM_COL32(24, 15, 8, 0), IM_COL32(24, 15, 8, 210));
		mdl->AddRectFilledMultiColor(ImVec2(desk_max.x - v_size, desk_min.y), desk_max,
			IM_COL32(24, 15, 8, 0), IM_COL32(24, 15, 8, 210), IM_COL32(24, 15, 8, 210), IM_COL32(24, 15, 8, 0));

		// 3. Subtle Antique Parchment Border & Brass Corner Rivets
		mdl->AddRect(desk_min, desk_max, IM_COL32(140, 95, 45, 160), 0.0f, 0, 1.5f);

		auto DrawCornerPin = [&](ImVec2 pos) {
			mdl->AddCircleFilled(ImVec2(pos.x + 1.5f, pos.y + 1.5f), 4.5f, IM_COL32(0, 0, 0, 160));
			mdl->AddCircleFilled(pos, 3.5f, IM_COL32(185, 140, 55, 255));
			mdl->AddCircle(pos, 3.5f, IM_COL32(245, 210, 110, 240), 12, 1.0f);
			mdl->AddCircleFilled(ImVec2(pos.x - 0.8f, pos.y - 0.8f), 1.0f, IM_COL32(255, 255, 220, 220));
		};
		DrawCornerPin(ImVec2(desk_min.x + 6.0f, desk_min.y + 6.0f));
		DrawCornerPin(ImVec2(desk_max.x - 6.0f, desk_min.y + 6.0f));
		DrawCornerPin(ImVec2(desk_min.x + 6.0f, desk_max.y - 6.0f));
		DrawCornerPin(ImVec2(desk_max.x - 6.0f, desk_max.y - 6.0f));

		// Coordinate Transformation Helpers
		auto MapToDeskScreen = [&](int mx, int my) -> ImVec2 {
			float rx = (float)(mx - world_map_start_x) / (float)world_map_span_w;
			float ry = (float)(my - world_map_start_y) / (float)world_map_span_h;
			return ImVec2(desk_min.x + rx * desk_sz.x, desk_min.y + ry * desk_sz.y);
		};

		auto DeskScreenToMap = [&](const ImVec2& spos, int* mx, int* my) {
			float rx = std::clamp((spos.x - desk_min.x) / desk_sz.x, 0.0f, 1.0f);
			float ry = std::clamp((spos.y - desk_min.y) / desk_sz.y, 0.0f, 1.0f);
			*mx = std::clamp(world_map_start_x + (int)(rx * (float)world_map_span_w), 0, map_w - 1);
			*my = std::clamp(world_map_start_y + (int)(ry * (float)world_map_span_h), 0, map_h - 1);
		};

		// Draw Exact Viewport Box & Crosshair Reticle
		if (show_viewport_box) {
			int cam_x1, cam_y1, cam_x2, cam_y2;
			int scr_w, scr_h;
			static_cast<MapWindow*>(GetParent())->GetClientSize(&scr_w, &scr_h);
			ScreenToMap(0, 0, &cam_x1, &cam_y1);
			ScreenToMap(scr_w, scr_h, &cam_x2, &cam_y2);

			ImVec2 vp1 = MapToDeskScreen(cam_x1, cam_y1);
			ImVec2 vp2 = MapToDeskScreen(cam_x2, cam_y2);
			vp1.x = std::clamp(vp1.x, desk_min.x, desk_max.x);
			vp1.y = std::clamp(vp1.y, desk_min.y, desk_max.y);
			vp2.x = std::clamp(vp2.x, desk_min.x, desk_max.x);
			vp2.y = std::clamp(vp2.y, desk_min.y, desk_max.y);

			mdl->AddRect(vp1, vp2, IM_COL32(255, 215, 60, 240), 0.0f, 0, 1.8f);
			ImVec2 vp_mid((vp1.x + vp2.x) * 0.5f, (vp1.y + vp2.y) * 0.5f);
			mdl->AddLine(ImVec2(vp_mid.x - 6, vp_mid.y), ImVec2(vp_mid.x + 7, vp_mid.y), IM_COL32(255, 235, 120, 255), 1.5f);
			mdl->AddLine(ImVec2(vp_mid.x, vp_mid.y - 6), ImVec2(vp_mid.x, vp_mid.y + 7), IM_COL32(255, 235, 120, 255), 1.5f);
		}

		// Render Towns
		if (show_towns) {
			const Towns& towns = editor.map.towns;
			for (auto it = towns.begin(); it != towns.end(); ++it) {
				Town* town = it->second;
				if (!town || town->getName().empty()) continue;
				Position tpos = town->getTemplePosition();
				if (tpos.z != world_map_floor) continue;

				ImVec2 pt = MapToDeskScreen(tpos.x, tpos.y);
				if (pt.x >= desk_min.x && pt.x <= desk_max.x && pt.y >= desk_min.y && pt.y <= desk_max.y) {
					mdl->AddCircleFilled(ImVec2(pt.x + 1.5f, pt.y + 2.0f), 7.5f, IM_COL32(0, 0, 0, 160));
					mdl->AddCircleFilled(pt, 6.0f, IM_COL32(217, 119, 6, 255));
					mdl->AddCircle(pt, 6.0f, IM_COL32(251, 191, 36, 255), 20, 1.5f);
					ImVec2 isz = ImGui::CalcTextSize("+");
					mdl->AddText(ImVec2(pt.x - isz.x * 0.5f, pt.y - isz.y * 0.5f - 1.0f), IM_COL32(255, 255, 255, 245), "+");

					std::string tlabel = "[Town] " + town->getName();
					ImVec2 tsz = ImGui::CalcTextSize(tlabel.c_str());
					ImVec2 pill_min(pt.x + 10.0f, pt.y - tsz.y * 0.5f - 2.0f);
					ImVec2 pill_max(pill_min.x + tsz.x + 8.0f, pill_min.y + tsz.y + 4.0f);
					mdl->AddRectFilled(pill_min, pill_max, IM_COL32(12, 16, 24, 225), 4.0f);
					mdl->AddRect(pill_min, pill_max, IM_COL32(251, 191, 36, 180), 4.0f, 0, 1.0f);
					mdl->AddText(ImVec2(pill_min.x + 4.0f, pill_min.y + 2.0f), IM_COL32(254, 240, 138, 255), tlabel.c_str());
				}
			}
		}

		// Render Custom Markers (Pins) with Distinct Colored Category Icons
		int hovered_marker_id = -1;
		auto& markerMgr = WorldMapMarkerManager::GetInstance();
		auto& markerList = markerMgr.GetMarkers();

		if (show_pins) {
			for (const auto& m : markerList) {
				if (m.z != world_map_floor) continue;
				ImVec2 pt = MapToDeskScreen(m.x, m.y);
				if (pt.x >= desk_min.x - 20 && pt.x <= desk_max.x + 20 && pt.y >= desk_min.y - 20 && pt.y <= desk_max.y + 20) {
					ImVec2 mpos = io.MousePos;
					float dist = std::sqrt((mpos.x - pt.x) * (mpos.x - pt.x) + (mpos.y - pt.y) * (mpos.y - pt.y));
					if (dist < 14.0f) {
						hovered_marker_id = m.id;
					}

					bool is_h = (hovered_marker_id == m.id);
					int c = std::clamp(m.category, 0, 7);
					const auto& info = s_cat_info[c];
					float rad = is_h ? 9.0f : 7.0f;

					// Drop shadow
					mdl->AddCircleFilled(ImVec2(pt.x + 1.5f, pt.y + 2.0f), rad + 1.5f, IM_COL32(0, 0, 0, 160));

					// Glow halo if hovered
					if (is_h) {
						mdl->AddCircleFilled(pt, rad + 4.0f, info.border_color & IM_COL32(255, 255, 255, 100));
					}

					// Badge Body & Golden / Category Rim
					mdl->AddCircleFilled(pt, rad, info.fill_color);
					mdl->AddCircle(pt, rad, is_h ? IM_COL32(255, 245, 160, 255) : info.border_color, 24, is_h ? 2.0f : 1.5f);

					// Inner Icon Glyph centered
					ImVec2 isz = ImGui::CalcTextSize(info.icon);
					mdl->AddText(ImVec2(pt.x - isz.x * 0.5f, pt.y - isz.y * 0.5f - 1.0f), IM_COL32(255, 255, 255, 245), info.icon);

					// Label badge with sleek dark background pill
					std::string full_label = std::string(info.badge) + " " + m.title;
					ImVec2 tsz = ImGui::CalcTextSize(full_label.c_str());
					ImVec2 pill_min(pt.x + rad + 4.0f, pt.y - tsz.y * 0.5f - 2.0f);
					ImVec2 pill_max(pill_min.x + tsz.x + 8.0f, pill_min.y + tsz.y + 4.0f);

					mdl->AddRectFilled(pill_min, pill_max, IM_COL32(12, 16, 24, 225), 4.0f);
					mdl->AddRect(pill_min, pill_max, is_h ? IM_COL32(255, 215, 80, 230) : info.border_color & IM_COL32(255, 255, 255, 160), 4.0f, 0, 1.0f);
					mdl->AddText(ImVec2(pill_min.x + 4.0f, pill_min.y + 2.0f), is_h ? IM_COL32(255, 245, 180, 255) : info.text_color, full_label.c_str());
				}
			}
		}

		// Render Multiplayer Co-Editor Cursors & Positions
		if (show_peers && editor.IsLiveClient()) {
			LiveClient* lc = editor.GetLiveClient();
			if (lc) {
				for (const auto& peer_cur : lc->getCursorList()) {
					if (peer_cur.pos.z != world_map_floor) continue;
					ImVec2 ppt = MapToDeskScreen(peer_cur.pos.x, peer_cur.pos.y);
					if (ppt.x >= desk_min.x && ppt.x <= desk_max.x && ppt.y >= desk_min.y && ppt.y <= desk_max.y) {
						ImU32 pcolor = IM_COL32(peer_cur.color.Red(), peer_cur.color.Green(), peer_cur.color.Blue(), 255);
						mdl->AddCircleFilled(ppt, 6.0f, pcolor);
						mdl->AddCircle(ppt, 8.0f, IM_COL32(255, 255, 255, 220), 16, 1.5f);
						std::string peer_label = "User #" + std::to_string(peer_cur.id);
						mdl->AddText(ImVec2(ppt.x + 10, ppt.y - 6), IM_COL32(0, 0, 0, 255), peer_label.c_str());
						mdl->AddText(ImVec2(ppt.x + 9, ppt.y - 7), pcolor, peer_label.c_str());
					}
				}
			}
		}

		// Pop Desk Clipping
		mdl->PopClipRect();

		// Hover Coordinate & Tooltip
		int hover_map_x = 0, hover_map_y = 0;
		bool in_desk = (io.MousePos.x >= desk_min.x && io.MousePos.x <= desk_max.x && io.MousePos.y >= desk_min.y && io.MousePos.y <= desk_max.y);
		if (wm_hovered && in_desk) {
			DeskScreenToMap(io.MousePos, &hover_map_x, &hover_map_y);

			if (hovered_marker_id != -1) {
				for (const auto& m : markerList) {
					if (m.id == hovered_marker_id) {
						ImGui::SetTooltip("%s\n%s\nLocation: [X: %d, Y: %d, Z: %d]\n(Left-Click: Teleport | Right-Click: Edit/Del)",
							m.title.c_str(), m.description.empty() ? "(No notes)" : m.description.c_str(), m.x, m.y, m.z);
						break;
					}
				}
			} else {
				ImGui::SetTooltip("Location: [X: %d, Y: %d, Z: %d]\n(Click: Teleport | Drag/WASD: Pan | Right-Click: Add Marker)", hover_map_x, hover_map_y, world_map_floor);
			}
		}

		// Left-Click Travel Navigation inside Desk
		if (wm_hovered && in_desk && ImGui::IsMouseClicked(0) && !ImGui::IsMouseDragging(0, 4.0f)) {
			int click_map_x, click_map_y;
			DeskScreenToMap(io.MousePos, &click_map_x, &click_map_y);

			if (hovered_marker_id != -1) {
				for (const auto& m : markerList) {
					if (m.id == hovered_marker_id) {
						g_gui.SetScreenCenterPosition(Position(m.x, m.y, m.z), false);
						Refresh(false);
						break;
					}
				}
			} else {
				g_gui.SetScreenCenterPosition(Position(click_map_x, click_map_y, world_map_floor), false);
				Refresh(false);
			}
		}

		// Right-Click Context Menu for Adding or Editing Pins
		static int ctx_map_x = 0, ctx_map_y = 0, ctx_marker_id = -1;
		if (wm_hovered && in_desk && ImGui::IsMouseClicked(1)) {
			DeskScreenToMap(io.MousePos, &ctx_map_x, &ctx_map_y);
			ctx_marker_id = hovered_marker_id;
			ImGui::OpenPopup("##WMCanvasContextMenu");
		}

		if (ImGui::BeginPopup("##WMCanvasContextMenu")) {
			if (ctx_marker_id != -1) {
				for (const auto& m : markerList) {
					if (m.id == ctx_marker_id) {
						ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.40f, 1.0f), "Pin: %s", m.title.c_str());
						ImGui::Separator();
						if (ImGui::MenuItem("Teleport Here")) {
							g_gui.SetScreenCenterPosition(Position(m.x, m.y, m.z), false);
							Refresh(false);
						}
						if (ImGui::MenuItem("Edit Pin...")) {
							editing_marker = m;
							strncpy(marker_title_buf, m.title.c_str(), sizeof(marker_title_buf));
							strncpy(marker_desc_buf, m.description.c_str(), sizeof(marker_desc_buf));
							is_editing_existing = true;
							modal_marker_open = true;
						}
						if (ImGui::MenuItem("Delete Pin")) {
							markerMgr.RemoveMarker(m.id);
						}
						break;
					}
				}
			} else {
				ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.40f, 1.0f), "World Location: [%d, %d, %d]", ctx_map_x, ctx_map_y, world_map_floor);
				ImGui::Separator();
				if (ImGui::MenuItem("Add Custom Marker Here...")) {
					editing_marker = WorldMapMarker();
					editing_marker.x = ctx_map_x;
					editing_marker.y = ctx_map_y;
					editing_marker.z = world_map_floor;
					editing_marker.title = "New Marker";
					editing_marker.category = 0;
					editing_marker.color = 0xFFE5C158;
					strncpy(marker_title_buf, editing_marker.title.c_str(), sizeof(marker_title_buf));
					marker_desc_buf[0] = '\0';
					is_editing_existing = false;
					modal_marker_open = true;
				}
				if (ImGui::MenuItem("Teleport Editor Here")) {
					g_gui.SetScreenCenterPosition(Position(ctx_map_x, ctx_map_y, world_map_floor), false);
					Refresh(false);
				}
			}
			ImGui::EndPopup();
		}

		// --- Modal: Add / Edit Marker ---
		if (modal_marker_open) {
			ImGui::OpenPopup("Add / Edit World Marker##MarkerModal");
		}

		ImVec2 center_pos = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
		ImGui::SetNextWindowPos(center_pos, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(380, 320), ImGuiCond_Always);

		if (ImGui::BeginPopupModal("Add / Edit World Marker##MarkerModal", &modal_marker_open, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.35f, 1.0f), is_editing_existing ? "Edit World Marker" : "Create New World Marker");
			ImGui::Separator();

			ImGui::Text("Title:");
			ImGui::SetNextItemWidth(340.0f);
			ImGui::InputText("##MTitle", marker_title_buf, sizeof(marker_title_buf));

			ImGui::Text("Description / Notes:");
			ImGui::SetNextItemWidth(340.0f);
			ImGui::InputText("##MDesc", marker_desc_buf, sizeof(marker_desc_buf));

			ImGui::Text("Category:");
			int cur_cat = std::clamp(editing_marker.category, 0, 7);
			const auto& cur_info = s_cat_info[cur_cat];
			std::string combo_preview = std::string(cur_info.icon) + " " + cur_info.badge + " " + cur_info.name;

			ImGui::SetNextItemWidth(340.0f);
			if (ImGui::BeginCombo("##MCat", combo_preview.c_str())) {
				for (int n = 0; n < 8; n++) {
					const auto& info = s_cat_info[n];
					bool is_selected = (editing_marker.category == n);
					ImGui::PushID(n);

					ImVec4 tag_col((float)(info.fill_color & 0xFF) / 255.0f,
					               (float)((info.fill_color >> 8) & 0xFF) / 255.0f,
					               (float)((info.fill_color >> 16) & 0xFF) / 255.0f, 1.0f);

					std::string desc_suffix = (n == 0 ? " (Navigation)" :
					                           n == 1 ? " (Settlement)" :
					                           n == 2 ? " (Objective)" :
					                           n == 3 ? " (Cave / Crypt)" :
					                           n == 4 ? " (Loot / Chest)" :
					                           n == 5 ? " (Camp / Outpost)" :
					                           n == 6 ? " (Portal / Arcane)" : " (Boss / Hazard)");

					if (ImGui::Selectable("##cat_sel", is_selected)) {
						editing_marker.category = n;
					}
					ImGui::SameLine(10);
					ImGui::TextColored(tag_col, "%s %s", info.icon, info.badge);
					ImGui::SameLine();
					ImGui::TextColored(ImVec4(0.95f, 0.90f, 0.75f, 1.0f), "%s%s", info.name, desc_suffix.c_str());

					if (is_selected) {
						ImGui::SetItemDefaultFocus();
					}
					ImGui::PopID();
				}
				ImGui::EndCombo();
			}

			ImGui::Text("Position (X, Y, Z):");
			ImGui::SetNextItemWidth(100.0f);
			ImGui::InputInt("##MX", &editing_marker.x);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(100.0f);
			ImGui::InputInt("##MY", &editing_marker.y);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(80.0f);
			ImGui::InputInt("##MZ", &editing_marker.z);

			ImGui::Separator();
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.40f, 0.22f, 0.90f));
			if (ImGui::Button("Save Marker", ImVec2(160, 26))) {
				editing_marker.title = marker_title_buf;
				editing_marker.description = marker_desc_buf;
				if (editing_marker.title.empty()) editing_marker.title = "Marker";
				if (is_editing_existing) {
					WorldMapMarkerManager::GetInstance().UpdateMarker(editing_marker);
				} else {
					WorldMapMarkerManager::GetInstance().AddMarker(editing_marker);
				}
				modal_marker_open = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::PopStyleColor();

			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(100, 26))) {
				modal_marker_open = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}
	ImGui::End();
	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(4);

	if (!wm_open) {
		g_settings.setInteger(Config::SHOW_WORLD_MAP, 0);
		if (g_gui.root) g_gui.root->UpdateMenubar();
	}
}
