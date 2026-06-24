#include "main.h"
#include "map_drawer.h"
#include "map_display.h"
#include "gui.h"
#include "editor.h"
#include "settings.h"
#include "performance_logger.h"
#include "light_drawer.h"

#ifdef __APPLE__
	#include <GLUT/glut.h>
#else
	#include <GL/glut.h>
#endif

// VBO & Batching Globals (Internal to Drawer System)
std::vector<RME_Rendering::MapVertex> g_vbo_vertices;
std::vector<DrawBatch> g_vbo_batches;
std::map<uint32_t, std::vector<DrawBatch>> g_floor_batches;
bool g_vbo_building = false;
float g_vbo_current_shader_flag = 0.0f;
std::map<uint32_t, std::vector<DoodadInstance>> g_pending_instances;

MapDrawer::MapDrawer(MapCanvas* canvas) :
	canvas(canvas), editor(canvas->editor), current_vbo_revision(1) {
	light_drawer = std::make_shared<LightDrawer>();
}

MapDrawer::~MapDrawer() {
	Release();
}

void MapDrawer::SetupVars() {
	int old_screensize_x = screensize_x;
	int old_screensize_y = screensize_y;

	canvas->MouseToMap(&mouse_map_x, &mouse_map_y);
	canvas->GetViewBox(&view_scroll_x, &view_scroll_y, &screensize_x, &screensize_y);

	if (old_screensize_x > 0 && old_screensize_y > 0 && 
		(old_screensize_x != screensize_x || old_screensize_y != screensize_y)) {
		if (auto* backend = g_gui.GetRenderBackend()) {
			backend->Resize(screensize_x, screensize_y);
		}
		editor.map.root.markDirty(-1);
	}

	dragging = canvas->dragging;
	dragging_draw = canvas->dragging_draw;
	zoom = (float)canvas->GetZoom();
	tile_size = int(TileSize / zoom);
	floor = canvas->GetFloor();

	if (options.show_all_floors) {
		start_z = (floor <= GROUND_LAYER) ? GROUND_LAYER : std::min(MAP_MAX_LAYER, floor + 2);
	} else {
		start_z = floor;
	}

	end_z = floor;
	superend_z = (floor > GROUND_LAYER ? 8 : 0);
	start_x = view_scroll_x / TileSize;
	start_y = view_scroll_y / TileSize;
	if (floor > GROUND_LAYER) { start_x -= 2; start_y -= 2; }
	end_x = start_x + screensize_x / tile_size + 2;
	end_y = start_y + screensize_y / tile_size + 2;
}

void MapDrawer::SetupGL() {
	glViewport(0, 0, screensize_x, screensize_y);
	int vPort[4];
	glGetIntegerv(GL_VIEWPORT, vPort);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, vPort[2] * zoom, vPort[3] * zoom, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glTranslatef(0.375f, 0.375f, 0.0f);
}

void MapDrawer::Draw() {
	PROFILE_SCOPE("MapDrawer::DrawTotal");
	last_bound_texture = -1;
	DrawBackground();
	DrawMap();
	
	if (options.isDrawLight()) {
		PROFILE_SCOPE("MapDrawer::DrawLight");
		DrawLight();
	}

	DrawDraggingShadow();
	DrawHigherFloors();

	bool overlayHasTooltips = false;
	if (options.dragging) DrawSelectionBox();
	
	DrawLiveCursors();
	DrawBrush();
	if (options.show_grid) DrawGrid();
	if (options.show_ingame_box) DrawIngameBox();
	
	if (g_luaScripts.isInitialized()) {
		std::vector<MapOverlayCommand> overlayCommands;
		g_luaScripts.collectMapOverlayCommands(getViewInfo(), overlayCommands);
		overlayHasTooltips = drawOverlayCommands(overlayCommands) || overlayHasTooltips;

		const MapOverlayHoverState& hoverState = g_luaScripts.getMapOverlayHover();
		if (hoverState.valid) {
			overlayHasTooltips = addOverlayTooltips(hoverState.tooltips) || overlayHasTooltips;
			overlayHasTooltips = drawOverlayCommands(hoverState.commands) || overlayHasTooltips;
		}
	}
	if (options.show_tooltips || overlayHasTooltips) DrawTooltips();

	// Update Shader Time Uniform for Water/Lava
	static float s_gpu_anim_time = 0.0f;
	s_gpu_anim_time += 0.016f * g_settings.getFloat(Config::SHADER_WATER_ANIM_SPEED);
	// Hier erfolgt der Upload (Mockup-Aufruf für vcpkg/bgfx Integration)
	// bgfx::setUniform(u_time_handle, &s_gpu_anim_time);
}

void MapDrawer::DrawMap() {
	bool only_colors = options.show_as_minimap || options.show_only_colors;
	if (!only_colors) glEnable(GL_TEXTURE_2D);
	if (options != last_options) { current_vbo_revision++; last_options = options; }

	for (int map_z = start_z; map_z >= superend_z; map_z--) {
		if (map_z == end_z && start_z != end_z && options.show_shade) {
			if (!only_colors) glDisable(GL_TEXTURE_2D);
			glColor4ub(0, 0, 0, 128);
			glBegin(GL_QUADS);
			glVertex2f(0, int(screensize_y * zoom));
			glVertex2f(int(screensize_x * zoom), int(screensize_y * zoom));
			glVertex2f(int(screensize_x * zoom), 0);
			glVertex2f(0, 0);
			glEnd();
			if (!only_colors) glEnable(GL_TEXTURE_2D);
		}

		if (map_z >= end_z) {
			int nd_start_x = start_x & ~3;
			int nd_start_y = start_y & ~3;
			int nd_end_x = (end_x & ~3) + 4; 
			int nd_end_y = (end_y & ~3) + 4;

			for (int nd_x = nd_start_x; nd_x <= nd_end_x; nd_x += 4) {
				for (int nd_y = nd_start_y; nd_y <= nd_end_y; nd_y += 4) {
					QTreeNode* nd = editor.map.getLeaf(nd_x, nd_y);
					if (!nd) continue;

					Floor* f = nd->getFloor(map_z);
					if (!f) continue;

void MapDrawer::RebuildFloorVBO(QTreeNode* nd, int map_z) {
    Floor* f = nd->getFloor(map_z);
    if (f->vbo_id == 0) glGenBuffers(1, &f->vbo_id);

    g_vbo_vertices.clear();
    g_vbo_batches.clear();
    // Vorallozierung für 4x4 Tiles à durchschnittlich 4 Vertices pro Item
    g_vbo_vertices.reserve(16 * 4 * 4); 
    
    g_vbo_building = true;
    for (int map_x = 0; map_x < 4; ++map_x) {
        for (int map_y = 0; map_y < 4; ++map_y) {
            DrawTile(nd->getTile(map_x, map_y, map_z));
        }
    }
    g_vbo_building = false;

    if (!g_vbo_vertices.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, f->vbo_id);
        glBufferData(GL_ARRAY_BUFFER, g_vbo_vertices.size() * sizeof(RME_Rendering::MapVertex), g_vbo_vertices.data(), GL_STATIC_DRAW);
        g_floor_batches[f->vbo_id] = g_vbo_batches;
    }
}

					// Execute VBO Draw Call
					RenderFloorVBO(f);

					// Light Pass (Dynamic)
					if (options.isDrawLight()) {
						for (int lx = 0; lx < 4; ++lx) {
							for (int ly = 0; ly < 4; ++ly) {
								AddLight(nd->getTile(lx, ly, map_z));
							}
						}
					}
				}
			}
		}
	}
}

void MapDrawer::bindTexture(int texture_number) {
	if (last_bound_texture != texture_number) {
		glBindTexture(GL_TEXTURE_2D, texture_number);
		last_bound_texture = texture_number;
	}
}

void MapDrawer::glColor(wxColor color) {
	glColor4ub(color.Red(), color.Green(), color.Blue(), color.Alpha());
}

void MapDrawer::glColorCheck(Brush* brush, const Position& pos) {
	if (brush->canDraw(&editor.map, pos)) glColor(COLOR_VALID);
	else glColor(COLOR_INVALID);
}