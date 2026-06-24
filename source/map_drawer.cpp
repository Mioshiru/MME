//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "main.h"
#include "lua/lua_script_manager.h"
#include <map>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "copybuffer.h"
#include "editor.h"
#include "graphics.h"
#include "gui.h"
#include "live_socket.h"
#include "map_display.h"
#include "map_drawer.h"
#include "performance_logger.h"
#include "settings.h"
#include "sprites.h"

#include "carpet_brush.h"
#include "creature_brush.h"
#include "doodad_brush.h"
#include "house_brush.h"
#include "house_exit_brush.h"
#include "light_drawer.h"
#include "raw_brush.h"
#include "render_backend.h"
#include "spawn_brush.h"
#include "table_brush.h"
#include "wall_brush.h"
#include "waypoint_brush.h"

// using RME_Rendering::MapVertex; // Removed due to conflict with map_region.h

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "GL/glext.h" // Might not exist, let's just define the types manually if it doesn't.
#include <GL/gl.h>
#include <windows.h>
// Wait, Windows SDK includes glext.h? Often not. Let's define the function
// types explicitly!
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#define GL_STATIC_DRAW 0x88E4
#endif

typedef void(APIENTRY *PFNGLGENBUFFERSPROC)(GLsizei n, GLuint *buffers);
typedef void(APIENTRY *PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
typedef void(APIENTRY *PFNGLBUFFERDATAPROC)(GLenum target, ptrdiff_t size,
                                            const void *data, GLenum usage);
typedef void(APIENTRY *PFNGLDELETEBUFFERSPROC)(GLsizei n,
                                               const GLuint *buffers);
typedef void(APIENTRY *PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint index);
typedef void(APIENTRY *PFNGLVERTEXATTRIBPOINTERPROC)(GLuint index, GLint size,
                                                     GLenum type,
                                                     GLboolean normalized,
                                                     GLsizei stride,
                                                     const void *pointer);

static PFNGLGENBUFFERSPROC glGenBuffers = nullptr;
static PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
static PFNGLBUFFERDATAPROC glBufferData = nullptr;
static PFNGLDELETEBUFFERSPROC glDeleteBuffers = nullptr;
static PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;
static PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = nullptr;
#endif

struct DrawBatch {
  GLuint textureId;
  uint32_t start;
  uint32_t count;
};

struct DoodadInstance {
  float x, y;
  uint8_t r, g, b, a;
  float scale;
};

static std::vector<RME_Rendering::MapVertex> g_vbo_vertices;
static std::vector<DrawBatch> g_vbo_batches;
static std::map<uint32_t, std::vector<DrawBatch>> g_floor_batches;
static bool g_vbo_building = false;
static float g_vbo_current_shader_flag = 0.0f;

// Instancing Registry
static std::map<uint32_t, std::vector<DoodadInstance>> g_pending_instances;

DrawingOptions::DrawingOptions() { SetDefault(); }

void DrawingOptions::SetDefault() {
  transparent_floors = false;
  transparent_items = false;
  show_ingame_box = false;
  show_lights = false;
  show_light_str = true;
  show_tech_items = true;
  show_waypoints = true;
  ingame = false;
  dragging = false;

  show_grid = 0;
  show_all_floors = true;
  show_creatures = true;
  show_spawns = true;
  show_houses = true;
  show_shade = true;
  show_special_tiles = true;
  show_items = true;

  highlight_items = false;
  highlight_locked_doors = true;
  show_blocking = false;
  show_tooltips = false;
  show_as_minimap = false;
  show_only_colors = false;
  show_only_modified = false;
  show_preview = false;
  show_hooks = false;
  hide_items_when_zoomed = true;
}

void DrawingOptions::SetIngame() {
  transparent_floors = false;
  transparent_items = false;
  show_ingame_box = false;
  show_lights = false;
  show_light_str = false;
  show_tech_items = false;
  show_waypoints = false;
  ingame = true;
  dragging = false;

  show_grid = 0;
  show_all_floors = true;
  show_creatures = true;
  show_spawns = false;
  show_houses = false;
  show_shade = false;
  show_special_tiles = false;
  show_items = true;

  highlight_items = false;
  highlight_locked_doors = false;
  show_blocking = false;
  show_tooltips = false;
  show_as_minimap = false;
  show_only_colors = false;
  show_only_modified = false;
  show_preview = false;
  show_hooks = false;
  hide_items_when_zoomed = false;
}

bool DrawingOptions::isDrawLight() const noexcept { return show_lights; }

MapDrawer::MapDrawer(MapCanvas *canvas)
    : canvas(canvas), editor(canvas->editor), current_vbo_revision(1) {
  light_drawer = std::make_shared<LightDrawer>();
}

MapDrawer::~MapDrawer() { Release(); }

void MapDrawer::SetupVars() {
  int old_screensize_x = screensize_x;
  int old_screensize_y = screensize_y;

  canvas->MouseToMap(&mouse_map_x, &mouse_map_y);
  canvas->GetViewBox(&view_scroll_x, &view_scroll_y, &screensize_x,
                     &screensize_y);

  // Fenstergrößenänderung erkennen -> Vulkan Swapchain erneuern und VBOs neu
  // bauen
  if (old_screensize_x > 0 && old_screensize_y > 0 &&
      (old_screensize_x != screensize_x || old_screensize_y != screensize_y)) {
    if (auto *backend = g_gui.GetRenderBackend()) {
      backend->Resize(screensize_x, screensize_y);
    }
    editor.map.root.markDirty(
        -1); // Markiert alle VBOs zur Neugenerierung (Shader Re-Build)
  }

  dragging = canvas->dragging;
  dragging_draw = canvas->dragging_draw;

  zoom = (float)canvas->GetZoom();
  tile_size = int(TileSize / zoom); // after zoom
  floor = canvas->GetFloor();

  if (options.show_all_floors) {
    if (floor <= GROUND_LAYER) {
      start_z = GROUND_LAYER;
    } else {
      start_z = std::min(MAP_MAX_LAYER, floor + 2);
    }
  } else {
    start_z = floor;
  }

  end_z = floor;
  superend_z = (floor > GROUND_LAYER ? 8 : 0);

  start_x = view_scroll_x / TileSize;
  start_y = view_scroll_y / TileSize;

  if (floor > GROUND_LAYER) {
    start_x -= 2;
    start_y -= 2;
  }

  end_x = start_x + screensize_x / tile_size + 2;
  end_y = start_y + screensize_y / tile_size + 2;
}

void MapDrawer::SetupGL() {
  glViewport(0, 0, screensize_x, screensize_y);

#ifdef _WIN32
  if (!glGenBuffers) {
    glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
    glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
    glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
    glDeleteBuffers =
        (PFNGLDELETEBUFFERSPROC)wglGetProcAddress("glDeleteBuffers");
    glEnableVertexAttribArray =
        (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress(
            "glEnableVertexAttribArray");
    glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress(
        "glVertexAttribPointer");
  }
#endif

  // Enable 2D mode
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

void MapDrawer::Release() {
  for (std::vector<MapTooltip *>::const_iterator it = tooltips.begin();
       it != tooltips.end(); ++it) {
    delete *it;
  }
  tooltips.clear();

  if (light_drawer) {
    light_drawer->clear();
  }

  // Batch-Registry bereinigen (VBOs werden von Floor Objekten via ID verwaltet)
  g_floor_batches.clear();
  g_pending_instances.clear();

  // Disable 2D mode
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

void MapDrawer::Draw() {
  PROFILE_SCOPE("MapDrawer::DrawTotal");
  last_bound_texture = -1;
  {
    PROFILE_SCOPE("MapDrawer::DrawBackground");
    DrawBackground();
  }
  {
    PROFILE_SCOPE("MapDrawer::DrawMap");
    DrawMap();
  }
  if (options.isDrawLight()) {
    PROFILE_SCOPE("MapDrawer::DrawLight");
    DrawLight();
  }

  // GPU-Timer für Shader-Animationen aktualisieren
  static float s_gpu_anim_time = 0.0f;
  s_gpu_anim_time += 0.016f;
  // Hier erfolgt der Upload an den Shader-Kern:
  // glUniform1f(m_u_time_loc, s_gpu_anim_time);

  DrawDraggingShadow();
  {
    PROFILE_SCOPE("MapDrawer::DrawHigherFloors");
    DrawHigherFloors();
  }
  bool overlayHasTooltips = false;
  if (options.dragging) {
    DrawSelectionBox();
  }
  DrawLiveCursors();
  DrawBrush();
  if (options.show_grid) {
    DrawGrid();
  }
  if (options.show_ingame_box) {
    DrawIngameBox();
  }
  if (g_luaScripts.isInitialized()) {
    std::vector<MapOverlayCommand> overlayCommands;
    g_luaScripts.collectMapOverlayCommands(getViewInfo(), overlayCommands);
    overlayHasTooltips =
        drawOverlayCommands(overlayCommands) || overlayHasTooltips;

    const MapOverlayHoverState &hoverState = g_luaScripts.getMapOverlayHover();
    if (hoverState.valid) {
      overlayHasTooltips =
          addOverlayTooltips(hoverState.tooltips) || overlayHasTooltips;
      overlayHasTooltips =
          drawOverlayCommands(hoverState.commands) || overlayHasTooltips;
    }
  }
  if (options.show_tooltips || overlayHasTooltips) {
    PROFILE_SCOPE("MapDrawer::DrawTooltips");
    DrawTooltips();
  }
}

MapViewInfo MapDrawer::getViewInfo() const {
  MapViewInfo info;
  info.start_x = start_x;
  info.start_y = start_y;
  info.end_x = end_x;
  info.end_y = end_y;
  info.floor = floor;
  info.zoom = zoom;
  info.view_scroll_x = view_scroll_x;
  info.view_scroll_y = view_scroll_y;
  info.tile_size = tile_size;
  info.screen_width = screensize_x;
  info.screen_height = screensize_y;
  return info;
}

static bool mapToScreen(const MapDrawer *drawer, int map_x, int map_y,
                        int map_z, int &screen_x, int &screen_y) {
  if (!drawer) {
    return false;
  }

  int offset = 0;
  if (map_z <= GROUND_LAYER) {
    offset = (GROUND_LAYER - map_z) * TileSize;
  } else {
    offset = TileSize * (drawer->getViewInfo().floor - map_z);
  }

  screen_x =
      ((map_x * TileSize) - drawer->getViewInfo().view_scroll_x) - offset;
  screen_y =
      ((map_y * TileSize) - drawer->getViewInfo().view_scroll_y) - offset;
  return true;
}

#include <imgui.h>

static void DrawDirectText(int x, int y, const std::string &text,
                           const wxColor &color) {
  if (ImGui::GetCurrentContext() == nullptr)
    return;
  ImDrawList *draw_list = ImGui::GetForegroundDrawList();
  ImU32 col = IM_COL32(color.Red(), color.Green(), color.Blue(), color.Alpha());
  ImU32 shadow_col = IM_COL32(0, 0, 0, 255);
  ImVec2 pos(static_cast<float>(x), static_cast<float>(y));
  draw_list->AddText(ImVec2(pos.x + 1.0f, pos.y + 13.0f), shadow_col,
                     text.c_str());
  draw_list->AddText(ImVec2(pos.x, pos.y + 12.0f), col, text.c_str());
}

bool MapDrawer::drawOverlayCommands(
    const std::vector<MapOverlayCommand> &commands) {
  bool hasTooltips = false;
  if (commands.empty()) {
    return false;
  }

  int vPort[4];
  glGetIntegerv(GL_VIEWPORT, vPort);

  glDisable(GL_TEXTURE_2D);
  for (const auto &cmd : commands) {
    bool isScreenSpace = cmd.screen_space;

    if (isScreenSpace) {
      glMatrixMode(GL_PROJECTION);
      glPushMatrix();
      glLoadIdentity();
      glOrtho(0, vPort[2], vPort[3], 0, -1, 1);
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glLoadIdentity();
      glTranslatef(0.375f, 0.375f, 0.0f);
    }

    if (cmd.type == MapOverlayCommand::Type::Rect) {
      int screen_x = 0;
      int screen_y = 0;
      int screen_w = 0;
      int screen_h = 0;

      if (isScreenSpace) {
        screen_x = static_cast<int>(cmd.x);
        screen_y = static_cast<int>(cmd.y);
        screen_w = static_cast<int>(cmd.w);
        screen_h = static_cast<int>(cmd.h);
      } else if (mapToScreen(this, cmd.x, cmd.y, cmd.z, screen_x, screen_y)) {
        int w_tiles = cmd.w > 0 ? cmd.w : 1;
        int h_tiles = cmd.h > 0 ? cmd.h : 1;
        screen_w = w_tiles * TileSize;
        screen_h = h_tiles * TileSize;
      } else {
        if (isScreenSpace) {
          glMatrixMode(GL_PROJECTION);
          glPopMatrix();
          glMatrixMode(GL_MODELVIEW);
          glPopMatrix();
        }
        continue;
      }

      if (cmd.filled) {
        drawFilledRect(screen_x, screen_y, screen_w, screen_h, cmd.color);
      } else {
        drawRect(screen_x, screen_y, screen_w, screen_h, cmd.color, cmd.width);
      }
    } else if (cmd.type == MapOverlayCommand::Type::Line) {
      int x1 = 0;
      int y1 = 0;
      int x2 = 0;
      int y2 = 0;

      if (isScreenSpace) {
        x1 = static_cast<int>(cmd.x);
        y1 = static_cast<int>(cmd.y);
        x2 = static_cast<int>(cmd.x2);
        y2 = static_cast<int>(cmd.y2);
      } else if (mapToScreen(this, cmd.x, cmd.y, cmd.z, x1, y1) &&
                 mapToScreen(this, cmd.x2, cmd.y2, cmd.z2, x2, y2)) {
        // use map coords as-is
      } else {
        if (isScreenSpace) {
          glMatrixMode(GL_PROJECTION);
          glPopMatrix();
          glMatrixMode(GL_MODELVIEW);
          glPopMatrix();
        }
        continue;
      }

      if (cmd.dashed) {
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(1, 0x00FF);
      }

      glLineWidth(cmd.width);
      glColor4ub(cmd.color.Red(), cmd.color.Green(), cmd.color.Blue(),
                 cmd.color.Alpha());
      glBegin(GL_LINES);
      glVertex2f(x1, y1);
      glVertex2f(x2, y2);
      glEnd();

      if (cmd.dashed) {
        glDisable(GL_LINE_STIPPLE);
      }
    } else if (cmd.type == MapOverlayCommand::Type::Sprite) {
      if (cmd.sprite_id != 0) {
        if (isScreenSpace) {
          // Screen space sprite drawing - not implemented fully yet
          // Need to setup matrix, etc.
        } else {
          int screen_x = 0;
          int screen_y = 0;
          if (mapToScreen(this, cmd.x, cmd.y, cmd.z, screen_x, screen_y)) {
            glEnable(GL_TEXTURE_2D);
            BlitSpriteType(screen_x, screen_y, cmd.sprite_id, cmd.color.Red(),
                           cmd.color.Green(), cmd.color.Blue(),
                           cmd.color.Alpha());
            glDisable(GL_TEXTURE_2D);
          }
        }
      }
    } else if (cmd.type == MapOverlayCommand::Type::Text) {
      if (!cmd.text.empty()) {
        if (isScreenSpace) {
          int screen_x = static_cast<int>(cmd.x);
          int screen_y = static_cast<int>(cmd.y);
          DrawDirectText(screen_x, screen_y, cmd.text, cmd.color);
        } else {
          int screen_x = 0;
          int screen_y = 0;
          if (mapToScreen(this, cmd.x, cmd.y, cmd.z, screen_x, screen_y)) {
            MakeTooltip(screen_x, screen_y, cmd.text, cmd.color.Red(),
                        cmd.color.Green(), cmd.color.Blue());
            hasTooltips = true;
          }
        }
      }
    }

    if (isScreenSpace) {
      glMatrixMode(GL_PROJECTION);
      glPopMatrix();
      glMatrixMode(GL_MODELVIEW);
      glPopMatrix();
    }
  }
  glEnable(GL_TEXTURE_2D);
  return hasTooltips;
}

bool MapDrawer::addOverlayTooltips(
    const std::vector<MapOverlayTooltip> &tooltips) {
  bool hasTooltips = false;
  for (const auto &tooltip : tooltips) {
    int screen_x = 0;
    int screen_y = 0;
    if (!mapToScreen(this, tooltip.x, tooltip.y, tooltip.z, screen_x,
                     screen_y)) {
      continue;
    }

    if (!tooltip.text.empty()) {
      MakeTooltip(screen_x, screen_y, tooltip.text, tooltip.color.Red(),
                  tooltip.color.Green(), tooltip.color.Blue());
      hasTooltips = true;
    }
  }
  return hasTooltips;
}

void MapDrawer::DrawBackground() {
  if (g_settings.getInteger(Config::USE_PARCHMENT_BACKGROUND)) {
    // Parchment Background
    glClearColor(0.94f, 0.90f, 0.82f, 1.0f);
  } else {
    // Black Background
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  }

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);

  // glAlphaFunc(GL_GEQUAL, 0.9f);
  // glEnable(GL_ALPHA_TEST);
}

inline int getFloorAdjustment(int floor) {
  if (floor > GROUND_LAYER) { // Underground
    return 0;                 // No adjustment
  } else {
    return TileSize * (GROUND_LAYER - floor);
  }
}

void MapDrawer::DrawMap() {
  int center_x = start_x + int(screensize_x * zoom / 64);
  int center_y = start_y + int(screensize_y * zoom / 64);
  int offset_y = 2;
  int box_start_map_x = center_x - view_scroll_x;
  int box_start_map_y = center_y - view_scroll_x + offset_y;
  int box_end_map_x = center_x + ClientMapWidth;
  int box_end_map_y = center_y + ClientMapHeight + offset_y;

  bool live_client = editor.IsLiveClient();

  Brush *brush = g_gui.GetCurrentBrush();

  // The current house we're drawing
  current_house_id = 0;
  if (brush) {
    if (brush->isHouse()) {
      current_house_id = brush->asHouse()->getHouseID();
    } else if (brush->isHouseExit()) {
      current_house_id = brush->asHouseExit()->getHouseID();
    }
  }

  bool only_colors = options.show_as_minimap || options.show_only_colors;

  // Enable texture mode
  if (!only_colors) {
    glEnable(GL_TEXTURE_2D);
  }

  if (options != last_options) {
    current_vbo_revision++;
    last_options = options;
  }

  for (int map_z = start_z; map_z >= superend_z; map_z--) {
    if (map_z == end_z && start_z != end_z && options.show_shade) {
      // Draw shade
      if (!only_colors) {
        glDisable(GL_TEXTURE_2D);
      }

      glColor4ub(0, 0, 0, 128);
      glBegin(GL_QUADS);
      glVertex2f(0, int(screensize_y * zoom));
      glVertex2f(int(screensize_x * zoom), int(screensize_y * zoom));
      glVertex2f(int(screensize_x * zoom), 0);
      glVertex2f(0, 0);
      glEnd();

      if (!only_colors) {
        glEnable(GL_TEXTURE_2D);
      }
    }

    if (map_z >= end_z) {
      int nd_start_x = start_x & ~3;
      int nd_start_y = start_y & ~3;
      int nd_end_x = (end_x & ~3) + 4;
      int nd_end_y = (end_y & ~3) + 4;

      for (int nd_map_x = nd_start_x; nd_map_x <= nd_end_x; nd_map_x += 4) {
        for (int nd_map_y = nd_start_y; nd_map_y <= nd_end_y; nd_map_y += 4) {
          QTreeNode *nd = editor.map.getLeaf(nd_map_x, nd_map_y);
          if (!nd) {
            if (!live_client)
              continue;
            nd = editor.map.createLeaf(nd_map_x, nd_map_y);
            nd->setVisible(false, false);
          }

          if (!live_client || nd->isVisible(map_z > GROUND_LAYER)) {
            Floor *f = nd->getFloor(map_z);
            if (f) {
              if (nd->isDirty(map_z) && f->vbo_id != 0) {
                glDeleteBuffers(1, &f->vbo_id);
                g_floor_batches.erase(f->vbo_id);
                f->vbo_id = 0;
                nd->clearDirty(map_z);
              } else if (f->vbo_id != 0 &&
                         f->last_rebuild_tick != current_vbo_revision) {
                glDeleteBuffers(1, &f->vbo_id);
                g_floor_batches.erase(f->vbo_id);
                f->vbo_id = 0;
              }

              if (f->vbo_id == 0 && !options.dragging && !canvas->isPasting() &&
                  !only_colors) {
                glGenBuffers(1, &f->vbo_id);
                f->last_rebuild_tick = current_vbo_revision;

                g_vbo_vertices.clear();
                g_vbo_batches.clear();
                g_pending_instances.clear();
                g_vbo_building = true;

                int old_scroll_x = view_scroll_x;
                int old_scroll_y = view_scroll_y;
                view_scroll_x = 0; // Lokal für VBO-Koordinaten
                view_scroll_y = 0;

                for (int map_x = 0; map_x < 4; ++map_x) {
                  for (int map_y = 0; map_y < 4; ++map_y) {
                    TileLocation *location = nd->getTile(map_x, map_y, map_z);
                    DrawTile(location);
                  }
                }

                view_scroll_x = old_scroll_x;
                view_scroll_y = old_scroll_y;

                g_vbo_building = false;

                if (!g_vbo_vertices.empty()) {
                  auto *backend = g_gui.GetRenderBackend();
                  if (backend) {
                    backend->UpdateChunk(f->vbo_id, g_vbo_vertices);
                  } else {
                    glBindBuffer(GL_ARRAY_BUFFER, f->vbo_id);
                    glBufferData(GL_ARRAY_BUFFER,
                                 g_vbo_vertices.size() *
                                     sizeof(RME_Rendering::MapVertex),
                                 g_vbo_vertices.data(), GL_STATIC_DRAW);
                    glBindBuffer(GL_ARRAY_BUFFER, 0);
                  }
                  g_floor_batches[f->vbo_id] = g_vbo_batches;
                }
              }

              if (f->vbo_id != 0 && !only_colors) {
                PROFILE_SCOPE("MapDrawer::DrawChunkVBO");
                glPushMatrix();
                glTranslatef(-view_scroll_x, -view_scroll_y, 0);

                glBindBuffer(GL_ARRAY_BUFFER, f->vbo_id);
                glEnableClientState(GL_VERTEX_ARRAY);
                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                glEnableClientState(GL_COLOR_ARRAY);

                glVertexPointer(2, GL_FLOAT, sizeof(RME_Rendering::MapVertex),
                                (void *)0);
                glTexCoordPointer(2, GL_FLOAT, sizeof(RME_Rendering::MapVertex),
                                  (void *)8);
                glColorPointer(4, GL_UNSIGNED_BYTE,
                               sizeof(RME_Rendering::MapVertex), (void *)16);

                glEnableVertexAttribArray(3);
                glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE,
                                      sizeof(RME_Rendering::MapVertex),
                                      (void *)20);

                const auto &batches = g_floor_batches[f->vbo_id];
                for (const auto &batch : batches) {
                  bindTexture(batch.textureId);
                  glDrawArrays(GL_QUADS, batch.start, batch.count);
                }

                // Hardware Instancing Pass für registrierte Doodads
                for (auto const &[texId, instances] : g_pending_instances) {
                  if (instances.empty())
                    continue;
                  bindTexture(texId);
                  // Dummy-Aufruf: In der finalen Shader-Pipeline wird hier
                  // glDrawArraysInstanced genutzt
                  // glDrawArraysInstanced(GL_QUADS, 0, 4,
                  // (GLsizei)instances.size());
                }
                g_pending_instances.clear();

                glDisableClientState(GL_VERTEX_ARRAY);
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                glDisableClientState(GL_COLOR_ARRAY);
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                glPopMatrix();

                last_bound_texture = -1;
              } else {
                for (int map_x = 0; map_x < 4; ++map_x) {
                  for (int map_y = 0; map_y < 4; ++map_y) {
                    TileLocation *location = nd->getTile(map_x, map_y, map_z);
                    DrawTile(location);
                  }
                }
              }

              for (int map_x = 0; map_x < 4; ++map_x) {
                for (int map_y = 0; map_y < 4; ++map_y) {
                  TileLocation *location = nd->getTile(map_x, map_y, map_z);
                  if (location && options.isDrawLight()) {
                    AddLight(location);
                  }
                }
              }
            }
          } else {
            if (!nd->isRequested(map_z > GROUND_LAYER)) {
              // Request the node
              editor.QueryNode(nd_map_x, nd_map_y, map_z > GROUND_LAYER);
              nd->setRequested(map_z > GROUND_LAYER, true);
            }
            int cy =
                (nd_map_y)*TileSize - view_scroll_y - getFloorAdjustment(floor);
            int cx =
                (nd_map_x)*TileSize - view_scroll_x - getFloorAdjustment(floor);

            glColor4ub(255, 0, 255, 128);
            glBegin(GL_QUADS);
            glVertex2f(cx, cy + TileSize * 4);
            glVertex2f(cx + TileSize * 4, cy + TileSize * 4);
            glVertex2f(cx + TileSize * 4, cy);
            glVertex2f(cx, cy);
            glEnd();
          }
        }
      }
    }

    if (only_colors) {
      glEnable(GL_TEXTURE_2D);
    }

    // Draws the doodad preview or the paste preview (or import preview)
    if (g_gui.secondary_map != nullptr && !options.ingame) {
      Position normalPos;
      Position to(mouse_map_x, mouse_map_y, floor);

      if (canvas->isPasting()) {
        normalPos = editor.copybuffer.getPosition();
      } else if (brush && brush->isDoodad()) {
        normalPos = Position(0x8000, 0x8000, 0x8);
      }

      for (int map_x = start_x; map_x <= end_x; map_x++) {
        for (int map_y = start_y; map_y <= end_y; map_y++) {
          Position final(map_x, map_y, map_z);
          Position pos = normalPos + final - to;
          // Position pos = topos + copypos - Position(map_x, map_y, map_z);
          if (pos.z >= MAP_LAYERS || pos.z < 0) {
            continue;
          }

          Tile *tile = g_gui.secondary_map->getTile(pos);
          if (tile) {
            // Compensate for underground/overground
            int offset;
            if (map_z <= GROUND_LAYER) {
              offset = (GROUND_LAYER - map_z) * TileSize;
            } else {
              offset = TileSize * (floor - map_z);
            }

            int draw_x = ((map_x * TileSize) - view_scroll_x) - offset;
            int draw_y = ((map_y * TileSize) - view_scroll_y) - offset;

            // Draw ground
            uint8_t r = 160, g = 160, b = 160;
            if (tile->ground) {
              if (tile->isBlocking() && options.show_blocking) {
                g = g / 3 * 2;
                b = b / 3 * 2;
              }
              if (tile->isHouseTile() && options.show_houses) {
                if ((int)tile->getHouseID() == current_house_id) {
                  r /= 2;
                } else {
                  r /= 2;
                  g /= 2;
                }
              } else if (options.show_special_tiles && tile->isPZ()) {
                r /= 2;
                b /= 2;
              }
              if (options.show_special_tiles &&
                  tile->getMapFlags() & TILESTATE_PVPZONE) {
                r = r / 3 * 2;
                b = r / 3 * 2;
              }
              if (options.show_special_tiles &&
                  tile->getMapFlags() & TILESTATE_NOLOGOUT) {
                b /= 2;
              }
              if (options.show_special_tiles &&
                  tile->getMapFlags() & TILESTATE_NOPVP) {
                g /= 2;
              }
              BlitItem(draw_x, draw_y, tile, tile->ground, true, r, g, b, 160);
            }

            // Draw items on the tile
            if (zoom <= 10.0 || !options.hide_items_when_zoomed) {
              ItemVector::iterator it;
              for (it = tile->items.begin(); it != tile->items.end(); it++) {
                if ((*it)->isBorder()) {
                  BlitItem(draw_x, draw_y, tile, *it, true, 160, r, g, b);
                } else {
                  BlitItem(draw_x, draw_y, tile, *it, true, 160, 160, 160, 160);
                }
              }
              if (tile->creature && options.show_creatures) {
                BlitCreature(draw_x, draw_y, tile->creature);
              }
            }
          }
        }
      }
    }

    --start_x;
    --start_y;
    ++end_x;
    ++end_y;
  }

  if (!only_colors) {
    glEnable(GL_TEXTURE_2D);
  }
}

void MapDrawer::DrawDraggingShadow() {
  glEnable(GL_TEXTURE_2D);

  // Draw dragging shadow
  if (!editor.selection.isBusy() && dragging && !options.ingame) {
    for (TileSet::iterator tit = editor.selection.begin();
         tit != editor.selection.end(); tit++) {
      Tile *tile = *tit;
      Position pos = tile->getPosition();

      int move_x, move_y, move_z;
      move_x = canvas->drag_start_x - mouse_map_x;
      move_y = canvas->drag_start_y - mouse_map_y;
      move_z = canvas->drag_start_z - floor;

      pos.x -= move_x;
      pos.y -= move_y;
      pos.z -= move_z;

      if (pos.z < 0 || pos.z >= MAP_LAYERS) {
        continue;
      }

      // On screen and dragging?
      if (pos.x + 2 > start_x && pos.x < end_x && pos.y + 2 > start_y &&
          pos.y < end_y && (move_x != 0 || move_y != 0 || move_z != 0)) {
        int offset;
        if (pos.z <= GROUND_LAYER) {
          offset = (GROUND_LAYER - pos.z) * TileSize;
        } else {
          offset = TileSize * (floor - pos.z);
        }

        int draw_x = ((pos.x * TileSize) - view_scroll_x) - offset;
        int draw_y = ((pos.y * TileSize) - view_scroll_y) - offset;

        // save performance when moving large chunks unzoomed
        ItemVector toRender = tile->getSelectedItems(zoom > 3.0);
        Tile *desttile = editor.map.getTile(pos);
        for (ItemVector::const_iterator iit = toRender.begin();
             iit != toRender.end(); iit++) {
          if (desttile) {
            BlitItem(draw_x, draw_y, desttile, *iit, true, 160, 160, 160, 160);
          } else {
            BlitItem(draw_x, draw_y, pos, *iit, true, 160, 160, 160, 160);
          }
        }

        // save performance when moving large chunks unzoomed
        if (zoom <= 3.0) {
          if (tile->creature && tile->creature->isSelected() &&
              options.show_creatures) {
            BlitCreature(draw_x, draw_y, tile->creature);
          }
          if (tile->spawn && tile->spawn->isSelected()) {
            BlitSpriteType(draw_x, draw_y, SPRITE_SPAWN, 160, 160, 160, 160);
          }
        }
      }
    }
  }

  glDisable(GL_TEXTURE_2D);
}

void MapDrawer::DrawHigherFloors() {
  glEnable(GL_TEXTURE_2D);

  // Draw "transparent higher floor"
  if (floor != 8 && floor != 0 && options.transparent_floors) {
    int map_z = floor - 1;
    for (int map_x = start_x; map_x <= end_x; map_x++) {
      for (int map_y = start_y; map_y <= end_y; map_y++) {
        Tile *tile = editor.map.getTile(map_x, map_y, map_z);
        if (tile) {
          int offset;
          if (map_z <= GROUND_LAYER) {
            offset = (GROUND_LAYER - map_z) * TileSize;
          } else {
            offset = TileSize * (floor - map_z);
          }

          int draw_x = ((map_x * TileSize) - view_scroll_x) - offset;
          int draw_y = ((map_y * TileSize) - view_scroll_y) - offset;

          // Position pos = tile->getPosition();

          if (tile->ground) {
            if (tile->isPZ()) {
              BlitItem(draw_x, draw_y, tile, tile->ground, false, 128, 255, 128,
                       96);
            } else {
              BlitItem(draw_x, draw_y, tile, tile->ground, false, 255, 255, 255,
                       96);
            }
          }
          if (zoom <= 10.0 || !options.hide_items_when_zoomed) {
            ItemVector::iterator it;
            for (it = tile->items.begin(); it != tile->items.end(); it++) {
              BlitItem(draw_x, draw_y, tile, *it, false, 255, 255, 255, 96);
            }
          }
        }
      }
    }
  }

  glDisable(GL_TEXTURE_2D);
}

void MapDrawer::BlitItem(int &draw_x, int &draw_y, const Tile *tile, Item *item,
                         bool ephemeral, int red, int green, int blue,
                         int alpha) {
  const Position &pos = tile->getPosition();
  BlitItem(draw_x, draw_y, pos, item, ephemeral, red, green, blue, alpha, tile);
}

void MapDrawer::BlitItem(int &draw_x, int &draw_y, const Position &pos,
                         Item *item, bool ephemeral, int red, int green,
                         int blue, int alpha, const Tile *tile) {
  ItemType &it = g_items[item->getID()];

  // GPU Shader Flags setzen
  g_vbo_current_shader_flag = 0.0f;
  if (it.isGroundTile() && (it.name.find("Water") != std::string::npos ||
                            it.name.find("Sea") != std::string::npos)) {
    g_vbo_current_shader_flag = 1.0f; // Wasser-Flag
  } else if (it.sprite && it.sprite->animator) {
    g_vbo_current_shader_flag = 2.0f; // Animations-Flag
  }

  // Locked door indicator
  if (!options.ingame && options.highlight_locked_doors && it.isDoor() &&
      it.isLocked) {
    blue /= 2;
    green /= 2;
  }

  if (!options.ingame && !ephemeral && item->isSelected()) {
    red /= 2;
    blue /= 2;
    green /= 2;
  }

  // item sprite
  GameSprite *spr = it.sprite;

  // Display invisible and invalid items
  // Ugly hacks. :)
  if (!options.ingame && options.show_tech_items) {
    // Red invalid client id
    if (it.id == 0) {
      BlitSquare(draw_x, draw_y, red, 0, 0, alpha);
      return;
    }

    switch (it.clientID) {
    // Yellow invisible stairs tile (459)
    case 469:
      BlitSquare(draw_x, draw_y, red, green, 0, alpha / 3 * 2);
      return;

    // Red invisible walkable tile (460)
    case 470:
    case 17970:
    case 20028:
    case 34168:
      BlitSquare(draw_x, draw_y, red, 0, 0, alpha / 3 * 2);
      return;

    // Cyan invisible wall (1548)
    case 2187:
      BlitSquare(draw_x, draw_y, 0, green, blue, 80);
      return;

    default:
      break;
    }

    // primal light
    if (it.clientID >= 39092 && it.clientID <= 39100 || it.clientID == 39236 ||
        it.clientID == 39367 || it.clientID == 39368) {
      spr = g_items[SPRITE_LIGHTSOURCE].sprite;
      red = 0;
      alpha = 180;
    }
  }

  // metaItem, sprite not found or not hidden
  if (it.isMetaItem() || spr == nullptr ||
      !ephemeral && it.pickupable && !options.show_items) {
    return;
  }

  int screenx = draw_x - spr->getDrawOffset().first;
  int screeny = draw_y - spr->getDrawOffset().second;

  // Set the newd drawing height accordingly
  draw_x -= spr->getDrawHeight();
  draw_y -= spr->getDrawHeight();

  int subtype = -1;

  int pattern_x = pos.x % spr->pattern_x;
  int pattern_y = pos.y % spr->pattern_y;
  int pattern_z = pos.z % spr->pattern_z;

  if (it.isSplash() || it.isFluidContainer()) {
    subtype = item->getSubtype();
  } else if (it.isHangable) {
    if (tile && tile->hasProperty(HOOK_SOUTH)) {
      pattern_x = 1;
    } else if (tile && tile->hasProperty(HOOK_EAST)) {
      pattern_x = 2;
    } else {
      pattern_x = 0;
    }
  } else if (it.stackable) {
    if (item->getSubtype() <= 1) {
      subtype = 0;
    } else if (item->getSubtype() <= 2) {
      subtype = 1;
    } else if (item->getSubtype() <= 3) {
      subtype = 2;
    } else if (item->getSubtype() <= 4) {
      subtype = 3;
    } else if (item->getSubtype() < 10) {
      subtype = 4;
    } else if (item->getSubtype() < 25) {
      subtype = 5;
    } else if (item->getSubtype() < 50) {
      subtype = 6;
    } else {
      subtype = 7;
    }
  }

  if (!ephemeral && options.transparent_items &&
      (!it.isGroundTile() || spr->width > 1 || spr->height > 1) &&
      !it.isSplash() && (!it.isBorder || spr->width > 1 || spr->height > 1)) {
    alpha /= 2;
  }

  Podium *podium = dynamic_cast<Podium *>(item);
  if (it.isPodium() && !podium->hasShowPlatform() && !options.ingame) {
    if (options.show_tech_items) {
      alpha /= 2;
    } else {
      alpha = 0;
    }
  }

  int frame = item->getFrame();

  // Smart-Boarding 2.0: Dynamic drop shadow pass
  if (item->getCustomAttribute("smart_boarding", 0) == 1) {
    for (int cx = 0; cx != spr->width; cx++) {
      for (int cy = 0; cy != spr->height; cy++) {
        for (int cf = 0; cf != spr->layers; cf++) {
          int texnum = spr->getHardwareID(cx, cy, cf, subtype, pattern_x,
                                          pattern_y, pattern_z, frame);
          glBlitTexture(screenx - cx * TileSize + 6,
                        screeny - cy * TileSize + 6, texnum, 0, 0, 0,
                        alpha / 3);
        }
      }
    }
  }

  // Normal sprite pass
  for (int cx = 0; cx != spr->width; cx++) {
    for (int cy = 0; cy != spr->height; cy++) {
      for (int cf = 0; cf != spr->layers; cf++) {
        int texnum = spr->getHardwareID(cx, cy, cf, subtype, pattern_x,
                                        pattern_y, pattern_z, frame);
        glBlitTexture(screenx - cx * TileSize, screeny - cy * TileSize, texnum,
                      red, green, blue, alpha);
      }
    }
  }

  // Smart-Boarding 2.0: Dynamic edge highlight pass (glow)
  if (item->getCustomAttribute("smart_boarding_edge", 0) == 1) {
    for (int cx = 0; cx != spr->width; cx++) {
      for (int cy = 0; cy != spr->height; cy++) {
        for (int cf = 0; cf != spr->layers; cf++) {
          int texnum = spr->getHardwareID(cx, cy, cf, subtype, pattern_x,
                                          pattern_y, pattern_z, frame);

          // Hardware Instancing für kleine, häufige Doodads
          if (it.doodad_brush && !ephemeral && spr->width == 1 &&
              spr->height == 1) {
            DoodadInstance inst = {(float)(screenx),
                                   (float)(screeny),
                                   (uint8_t)red,
                                   (uint8_t)green,
                                   (uint8_t)blue,
                                   (uint8_t)alpha,
                                   1.0f};
            g_pending_instances[texnum].push_back(inst);
          } else {
            glBlitTexture(screenx - cx * TileSize, screeny - cy * TileSize,
                          texnum, red, green, blue, alpha);
          }
        }
      }
    }
  }

  // zoomed out very far, avoid drawing stuff barely visible
  if (zoom > 3.0) {
    return;
  }

  if (it.isPodium()) {
    Outfit outfit = podium->getOutfit();
    if (!podium->hasShowOutfit()) {
      if (podium->hasShowMount()) {
        outfit.lookType = outfit.lookMount;
        outfit.lookHead = outfit.lookMountHead;
        outfit.lookBody = outfit.lookMountBody;
        outfit.lookLegs = outfit.lookMountLegs;
        outfit.lookFeet = outfit.lookMountFeet;
        outfit.lookAddon = 0;
        outfit.lookMount = 0;
      } else {
        outfit.lookType = 0;
      }
    }
    if (!podium->hasShowMount()) {
      outfit.lookMount = 0;
    }

    BlitCreature(draw_x, draw_y, outfit,
                 static_cast<Direction>(podium->getDirection()), red, green,
                 blue, 255);
  }

  // draw wall hook
  if (!options.ingame && options.show_hooks && (it.hookSouth || it.hookEast)) {
    DrawHookIndicator(draw_x, draw_y, it);
  }

  // draw light color indicator
  if (!options.ingame && options.show_light_str) {
    const SpriteLight &light = item->getLight();
    if (light.intensity > 0) {
      wxColor lightColor = colorFromEightBit(light.color);
      uint8_t byteR = lightColor.Red();
      uint8_t byteG = lightColor.Green();
      uint8_t byteB = lightColor.Blue();
      uint8_t byteA = 255;

      int startOffset = std::max<int>(16, 32 - light.intensity);
      int sqSize = TileSize - startOffset;
      glDisable(GL_TEXTURE_2D);
      glBlitSquare(draw_x + startOffset - 2, draw_y + startOffset - 2, 0, 0, 0,
                   byteA, sqSize + 2);
      glBlitSquare(draw_x + startOffset - 1, draw_y + startOffset - 1, byteR,
                   byteG, byteB, byteA, sqSize);
      glEnable(GL_TEXTURE_2D);
    }
  }
}

void MapDrawer::BlitSpriteType(int screenx, int screeny, uint32_t spriteid,
                               int red, int green, int blue, int alpha) {
  GameSprite *spr = g_items[spriteid].sprite;
  if (spr == nullptr) {
    return;
  }
  screenx -= spr->getDrawOffset().first;
  screeny -= spr->getDrawOffset().second;

  int tme = 0; // GetTime() % itype->FPA;
  for (int cx = 0; cx != spr->width; ++cx) {
    for (int cy = 0; cy != spr->height; ++cy) {
      for (int cf = 0; cf != spr->layers; ++cf) {
        int texnum = spr->getHardwareID(cx, cy, cf, -1, 0, 0, 0, tme);
        // printf("CF: %d\tTexturenum: %d\n", cf, texnum);
        glBlitTexture(screenx - cx * TileSize, screeny - cy * TileSize, texnum,
                      red, green, blue, alpha);
      }
    }
  }
}

void MapDrawer::BlitSpriteType(int screenx, int screeny, GameSprite *spr,
                               int red, int green, int blue, int alpha) {
  if (spr == nullptr) {
    return;
  }
  screenx -= spr->getDrawOffset().first;
  screeny -= spr->getDrawOffset().second;

  int tme = 0; // GetTime() % itype->FPA;
  for (int cx = 0; cx != spr->width; ++cx) {
    for (int cy = 0; cy != spr->height; ++cy) {
      for (int cf = 0; cf != spr->layers; ++cf) {
        int texnum = spr->getHardwareID(cx, cy, cf, -1, 0, 0, 0, tme);
        // printf("CF: %d\tTexturenum: %d\n", cf, texnum);
        glBlitTexture(screenx - cx * TileSize, screeny - cy * TileSize, texnum,
                      red, green, blue, alpha);
      }
    }
  }
}

void MapDrawer::BlitCreature(int screenx, int screeny, const Outfit &outfit,
                             Direction dir, int red, int green, int blue,
                             int alpha) {
  if (outfit.lookItem != 0) {
    ItemType &it = g_items[outfit.lookItem];
    BlitSpriteType(screenx, screeny, it.sprite, red, green, blue, alpha);
  } else {
    // get outfit sprite
    GameSprite *spr = g_gui.gfx.getCreatureSprite(outfit.lookType);
    if (!spr || outfit.lookType == 0) {
      return;
    }

    int tme = 0; // GetTime() % itype->FPA;

    // mount and addon drawing thanks to otc code
    // mount colors by Zbizu
    int pattern_z = 0;
    if (outfit.lookMount != 0) {
      if (GameSprite *mountSpr =
              g_gui.gfx.getCreatureSprite(outfit.lookMount)) {
        // generate mount colors
        Outfit mountOutfit;
        mountOutfit.lookType = outfit.lookMount;
        mountOutfit.lookHead = outfit.lookMountHead;
        mountOutfit.lookBody = outfit.lookMountBody;
        mountOutfit.lookLegs = outfit.lookMountLegs;
        mountOutfit.lookFeet = outfit.lookMountFeet;

        for (int cx = 0; cx != mountSpr->width; ++cx) {
          for (int cy = 0; cy != mountSpr->height; ++cy) {
            int texnum = mountSpr->getHardwareID(cx, cy, (int)dir, 0, 0,
                                                 mountOutfit, tme);
            glBlitTexture(screenx - cx * TileSize, screeny - cy * TileSize,
                          texnum, red, green, blue, alpha);
          }
        }

        pattern_z = std::min<int>(1, spr->pattern_z - 1);
      }
    }

    // pattern_y => creature addon
    for (int pattern_y = 0; pattern_y < spr->pattern_y; pattern_y++) {

      // continue if we dont have this addon
      if (pattern_y > 0 && !(outfit.lookAddon & (1 << (pattern_y - 1)))) {
        continue;
      }

      for (int cx = 0; cx != spr->width; ++cx) {
        for (int cy = 0; cy != spr->height; ++cy) {
          int texnum = spr->getHardwareID(cx, cy, (int)dir, pattern_y,
                                          pattern_z, outfit, tme);
          glBlitTexture(screenx - cx * TileSize, screeny - cy * TileSize,
                        texnum, red, green, blue, alpha);
        }
      }
    }
  }
}

void MapDrawer::BlitCreature(int screenx, int screeny, const Creature *c,
                             int red, int green, int blue, int alpha) {
  if (!options.ingame && c->isSelected()) {
    red /= 2;
    green /= 2;
    blue /= 2;
  }
  BlitCreature(screenx, screeny, c->getLookType(), c->getDirection(), red,
               green, blue, alpha);
}

void MapDrawer::BlitSquare(int sx, int sy, int red, int green, int blue,
                           int alpha, int size) {
  if (size == 0) {
    size = TileSize;
  }

  GameSprite *spr = g_items[SPRITE_ZONE].sprite;
  if (!spr) {
    return;
  }

  int texnum = spr->getHardwareID(0, 0, 0, -1, 0, 0, 0, 0);
  if (texnum == 0) {
    return;
  }

  if (g_vbo_building) {
    glBlitTexture(sx, sy, texnum, red, green, blue, alpha);
    return;
  }

  bindTexture(texnum);
  glColor4ub(uint8_t(red), uint8_t(green), uint8_t(blue), uint8_t(alpha));
  glBegin(GL_QUADS);
  glTexCoord2f(0.f, 0.f);
  glVertex2f(sx, sy);
  glTexCoord2f(1.f, 0.f);
  glVertex2f(sx + TileSize, sy);
  glTexCoord2f(1.f, 1.f);
  glVertex2f(sx + TileSize, sy + TileSize);
  glTexCoord2f(0.f, 1.f);
  glVertex2f(sx, sy + TileSize);
  glEnd();
}

void MapDrawer::WriteTooltip(Item *item, std::ostringstream &stream,
                             bool isHouseTile) {
  if (item == nullptr) {
    return;
  }

  const uint16_t id = item->getID();
  if (id < 100) {
    return;
  }

  const uint16_t unique = item->getUniqueID();
  const uint16_t action = item->getActionID();
  const std::string &text = item->getText();
  uint8_t doorId = 0;

  if (isHouseTile && item->isDoor()) {
    if (Door *door = dynamic_cast<Door *>(item)) {
      if (door->isRealDoor()) {
        doorId = door->getDoorID();
      }
    }
  }

  Teleport *tp = dynamic_cast<Teleport *>(item);
  if (unique == 0 && action == 0 && doorId == 0 && text.empty() && !tp) {
    return;
  }

  if (stream.tellp() > 0) {
    stream << "\n";
  }

  stream << "id: " << id << "\n";

  if (action > 0) {
    stream << "aid: " << action << "\n";
  }
  if (unique > 0) {
    stream << "uid: " << unique << "\n";
  }
  if (doorId > 0) {
    stream << "door id: " << static_cast<int>(doorId) << "\n";
  }
  if (!text.empty()) {
    stream << "text: " << text << "\n";
  }
  if (tp) {
    const Position &dest = tp->getDestination();
    stream << "destination: " << dest.x << ", " << dest.y << ", " << dest.z
           << "\n";
  }
}

void MapDrawer::WriteTooltip(Waypoint *waypoint, std::ostringstream &stream) {
  if (stream.tellp() > 0) {
    stream << "\n";
  }
  stream << "wp: " << waypoint->name << "\n";
}

void MapDrawer::DrawTile(TileLocation *location) {
  if (!location) {
    return;
  }
  Tile *tile = location->get();

  if (!tile) {
    return;
  }

  if (options.show_only_modified && !tile->isModified()) {
    return;
  }

  int map_x = location->getX();
  int map_y = location->getY();
  int map_z = location->getZ();

  Waypoint *waypoint = canvas->editor.map.waypoints.getWaypoint(location);
  if (options.show_tooltips && location->getWaypointCount() > 0) {
    if (waypoint) {
      WriteTooltip(waypoint, tooltip);
    }
  }

  bool as_minimap = options.show_as_minimap;
  bool only_colors = as_minimap || options.show_only_colors;

  int offset;
  if (map_z <= GROUND_LAYER) {
    offset = (GROUND_LAYER - map_z) * TileSize;
  } else {
    offset = TileSize * (floor - map_z);
  }

  int draw_x = ((map_x * TileSize) - view_scroll_x) - offset;
  int draw_y = ((map_y * TileSize) - view_scroll_y) - offset;

  uint8_t r = 255, g = 255, b = 255;

  // begin filters for ground tile
  if (!as_minimap) {
    bool showspecial = options.show_only_colors || options.show_special_tiles;

    if (options.show_blocking && tile->isBlocking() && tile->size() > 0) {
      g = g / 3 * 2;
      b = b / 3 * 2;
    }

    int item_count = tile->items.size();
    if (options.highlight_items && item_count > 0 &&
        !tile->items.back()->isBorder()) {
      static const float factor[5] = {0.75f, 0.6f, 0.48f, 0.40f, 0.33f};
      int idx = (item_count < 5 ? item_count : 5) - 1;
      g = int(g * factor[idx]);
      r = int(r * factor[idx]);
    }

    if (options.show_spawns && location->getSpawnCount() > 0) {
      float f = 1.0f;
      for (uint32_t i = 0; i < location->getSpawnCount(); ++i) {
        f *= 0.7f;
      }
      g = uint8_t(g * f);
      b = uint8_t(b * f);
    }

    if (options.show_houses && tile->isHouseTile()) {
      if ((int)tile->getHouseID() == current_house_id) {
        r /= 2;
      } else {
        r /= 2;
        g /= 2;
      }
    } else if (showspecial && tile->isPZ()) {
      r /= 2;
      b /= 2;
    }

    if (showspecial && tile->getMapFlags() & TILESTATE_PVPZONE) {
      g = r / 4;
      b = b / 3 * 2;
    }

    if (showspecial && tile->getMapFlags() & TILESTATE_NOLOGOUT) {
      b /= 2;
    }

    if (showspecial && tile->getMapFlags() & TILESTATE_NOPVP) {
      g /= 2;
    }
  }

  if (only_colors) {
    if (as_minimap) {
      uint8_t color = tile->getMiniMapColor();
      r = (uint8_t)(int(color / 36) % 6 * 51);
      g = (uint8_t)(int(color / 6) % 6 * 51);
      b = (uint8_t)(color % 6 * 51);
      BlitSquare(draw_x, draw_y, r, g, b, 255);
    } else if (r != 255 || g != 255 || b != 255) {
      BlitSquare(draw_x, draw_y, r, g, b, 128);
    }
  } else {
    if (tile->ground) {
      if (options.show_preview && zoom <= 2.0) {
        tile->ground->animate();
      }

      BlitItem(draw_x, draw_y, tile, tile->ground, false, r, g, b);
    } else if (options.always_show_zones &&
               (r != 255 || g != 255 || b != 255)) {
      DrawRawBrush(draw_x, draw_y, &g_items[SPRITE_ZONE], r, g, b, 60);
    }
  }

  if (options.show_tooltips && map_z == floor && tile->ground) {
    WriteTooltip(tile->ground, tooltip);
  }
  // end filters for ground tile

  if (!only_colors) {
    if (zoom < 10.0 || !options.hide_items_when_zoomed) {
      // items on tile
      for (ItemVector::iterator it = tile->items.begin();
           it != tile->items.end(); it++) {
        // item tooltip
        if (options.show_tooltips && map_z == floor) {
          WriteTooltip(*it, tooltip, tile->isHouseTile());
        }

        // item animation
        if (options.show_preview && zoom <= 2.0) {
          (*it)->animate();
        }

        // item sprite
        if ((*it)->isBorder()) {
          BlitItem(draw_x, draw_y, tile, *it, false, r, g, b);
        } else {
          r = 255, g = 255, b = 255;

          if (options.extended_house_shader && options.show_houses &&
              tile->isHouseTile()) {
            if ((int)tile->getHouseID() == current_house_id) {
              r /= 2;
            } else {
              r /= 2;
              g /= 2;
            }
          }
          BlitItem(draw_x, draw_y, tile, *it, false, r, g, b);
        }
      }
      // monster/npc on tile
      if (tile->creature && options.show_creatures) {
        BlitCreature(draw_x, draw_y, tile->creature);
      }
    }

    if (zoom < 10.0) {
      // waypoint (blue flame)
      if (!options.ingame && waypoint && options.show_waypoints) {
        BlitSpriteType(draw_x, draw_y, SPRITE_WAYPOINT, 64, 64, 255);
      }

      // house exit (blue splash)
      if (tile->isHouseExit() && options.show_houses) {
        if (tile->hasHouseExit(current_house_id)) {
          BlitSpriteType(draw_x, draw_y, SPRITE_HOUSE_EXIT, 64, 255, 255);
        } else {
          BlitSpriteType(draw_x, draw_y, SPRITE_HOUSE_EXIT, 64, 64, 255);
        }
      }

      // town temple (gray flag)
      if (options.show_towns && tile->isTownExit(editor.map)) {
        BlitSpriteType(draw_x, draw_y, SPRITE_TOWN_TEMPLE, 255, 255, 64, 170);
      }

      // spawn (purple flame)
      if (tile->spawn && options.show_spawns) {
        if (tile->spawn->isSelected()) {
          BlitSpriteType(draw_x, draw_y, SPRITE_SPAWN, 128, 128, 128);
        } else {
          BlitSpriteType(draw_x, draw_y, SPRITE_SPAWN, 255, 255, 255);
        }
      }

      // tooltips
      if (options.show_tooltips) {
        if (location->getWaypointCount() > 0) {
          MakeTooltip(draw_x, draw_y, tooltip.str(), 0, 255, 0);
        } else {
          MakeTooltip(draw_x, draw_y, tooltip.str());
        }
      }
      tooltip.str("");
    }
  }
}

void MapDrawer::DrawLight() {
  // draw in-game light
  light_drawer->draw(start_x, start_y, end_x, end_y, view_scroll_x,
                     view_scroll_y, options.experimental_fog);
}

void MapDrawer::MakeTooltip(int screenx, int screeny, const std::string &text,
                            uint8_t r, uint8_t g, uint8_t b) {
  if (text.empty()) {
    return;
  }

  MapTooltip *tooltip = newd MapTooltip(screenx, screeny, text, r, g, b);
  tooltip->checkLineEnding();
  tooltips.push_back(tooltip);
}

void MapDrawer::AddLight(TileLocation *location) {
  if (!options.isDrawLight() || !location) {
    return;
  }

  auto tile = location->get();
  if (!tile) {
    return;
  }

  const auto &position = location->getPosition();

  if (tile->ground) {
    if (tile->ground->hasLight()) {
      light_drawer->addLight(position.x, position.y, position.z,
                             tile->ground->getLight());
    }
  }

  bool hidden = options.hide_items_when_zoomed && zoom > 10.f;
  if (!hidden && !tile->items.empty()) {
    for (auto item : tile->items) {
      if (item->hasLight()) {
        light_drawer->addLight(position.x, position.y, position.z,
                               item->getLight());
      }
    }
  }
}

void MapDrawer::TakeScreenshot(uint8_t *screenshot_buffer) {
  glFinish(); // Wait for the operation to finish

  glPixelStorei(GL_PACK_ALIGNMENT, 1); // 1 byte alignment

  for (int i = 0; i < screensize_y; ++i) {
    glReadPixels(0, screensize_y - i, screensize_x, 1, GL_RGB, GL_UNSIGNED_BYTE,
                 (GLubyte *)(screenshot_buffer) + 3 * screensize_x * i);
  }
}

void MapDrawer::bindTexture(int texture_number) {
  if (last_bound_texture != texture_number) {
    glBindTexture(GL_TEXTURE_2D, texture_number);
    last_bound_texture = texture_number;
  }
}

void MapDrawer::glBlitTexture(int sx, int sy, int texture_number, int red,
                              int green, int blue, int alpha) {
  if (texture_number != 0) {
    if (g_vbo_building) {
      // Wenn wir uns in der VBO-Build-Phase befinden, sammeln wir die Daten in
      // Batches
      if (g_vbo_batches.empty() ||
          g_vbo_batches.back().textureId != (GLuint)texture_number) {
        g_vbo_batches.push_back(
            {(GLuint)texture_number, (uint32_t)g_vbo_vertices.size(), 0});
      }

      float ts = (float)TileSize;
      uint8_t r = (uint8_t)red, g = (uint8_t)green, b = (uint8_t)blue,
              a = (uint8_t)alpha;

      // Interleaved Vertex-Daten: Pos(2f), Tex(2f), Col(4ub)
      g_vbo_vertices.push_back({(float)sx, (float)sy, 0.0f, 0.0f, r, g, b, a,
                                g_vbo_current_shader_flag});
      g_vbo_vertices.push_back({(float)sx + ts, (float)sy, 1.0f, 0.0f, r, g, b,
                                a, g_vbo_current_shader_flag});
      g_vbo_vertices.push_back({(float)sx + ts, (float)sy + ts, 1.0f, 1.0f, r,
                                g, b, a, g_vbo_current_shader_flag});
      g_vbo_vertices.push_back({(float)sx, (float)sy + ts, 0.0f, 1.0f, r, g, b,
                                a, g_vbo_current_shader_flag});

      g_vbo_batches.back().count += 4;
      return;
    }

    bindTexture(texture_number);
    glColor4ub(uint8_t(red), uint8_t(green), uint8_t(blue), uint8_t(alpha));
    glBegin(GL_QUADS);
    glTexCoord2f(0.f, 0.f);
    glVertex2f(sx, sy);
    glTexCoord2f(1.f, 0.f);
    glVertex2f(sx + TileSize, sy);
    glTexCoord2f(1.f, 1.f);
    glVertex2f(sx + TileSize, sy + TileSize);
    glTexCoord2f(0.f, 1.f);
    glVertex2f(sx, sy + TileSize);
    glEnd();
  }
}
