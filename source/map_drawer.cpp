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
#include "application.h"
#include "lua/lua_script_manager.h"
#include <cmath>
#include <imgui.h>
#include <map>
#include <unordered_map>
#include <nanovg.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

inline int getFloorAdjustment(int floor) {
  if (floor > GROUND_LAYER) {
    return 0;
  } else {
    return TileSize * (GROUND_LAYER - floor);
  }
}

// Returns a unique color for each house based on its ID.
// Uses a palette of 12 well-distinguishable colors so adjacent houses
// can be told apart visually.
struct HouseColor { uint8_t r, g, b; };

static HouseColor getHouseColor(uint32_t houseId) {
  static const HouseColor palette[12] = {
    { 30,  80, 200},  // Blue
    {200,  50,  50},  // Red
    { 50, 180,  80},  // Green
    {150,  50, 180},  // Purple
    {220, 140,  30},  // Orange
    { 30, 180, 200},  // Cyan
    {200,  80, 140},  // Rose
    {140, 200,  30},  // Yellow-Green
    {170,  30,  80},  // Deep Red
    { 30, 150, 140},  // Teal
    {200, 170,  50},  // Gold
    { 80,  50, 200},  // Indigo
  };
  return palette[houseId % 12];
}

#include "copybuffer.h"
#include "editor.h"
#include "graphics.h"
#include "gui.h"
#include "live_socket.h"
#include "map_display.h"
#include "map_drawer.h"
#include "settings.h"
#include "sprites.h"

#include "carpet_brush.h"
#include "creature_brush.h"
#include "doodad_brush.h"
#include "ground_brush.h"
#include "house_brush.h"
#include "house_exit_brush.h"
#include "light_drawer.h"
#include "raw_brush.h"
#include "renderer.h"
#include "spawn_brush.h"
#include "table_brush.h"
#include "town.h"
#include "wall_brush.h"
#include "waypoint_brush.h"

// using RME_Rendering::MapVertex; // Removed due to conflict with map_region.h

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "GL/glext.h"
#include <GL/gl.h>
#include <windows.h>
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#define GL_STATIC_DRAW  0x88E4
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_STREAM_DRAW  0x88E0
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
typedef void(APIENTRY *PFNGLBUFFERSUBDATAPROC)(GLenum target, ptrdiff_t offset,
                                               ptrdiff_t size, const void *data);
typedef void(APIENTRY *PFNGLUSEPROGRAMPROC)(GLuint program);
typedef void(APIENTRY *PFNGLBINDVERTEXARRAYPROC)(GLuint array);
typedef void(APIENTRY *PFNGLACTIVETEXTUREPROC)(GLenum texture);
typedef void(APIENTRY *PFNGLDISABLEVERTEXATTRIBARRAYPROC)(GLuint index);
// Phase 1: VAO management
typedef void(APIENTRY *PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint *arrays);
typedef void(APIENTRY *PFNGLDELETEVERTEXARRAYSPROC)(GLsizei n, const GLuint *arrays);

#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif

static PFNGLGENBUFFERSPROC glGenBuffers = nullptr;
static PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
static PFNGLBUFFERDATAPROC glBufferData = nullptr;
static PFNGLBUFFERSUBDATAPROC glBufferSubData = nullptr;
static PFNGLDELETEBUFFERSPROC glDeleteBuffers = nullptr;
static PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;
static PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = nullptr;
static PFNGLUSEPROGRAMPROC glUseProgram = nullptr;
static PFNGLBINDVERTEXARRAYPROC glBindVertexArray = nullptr;
static PFNGLACTIVETEXTUREPROC glActiveTexture = nullptr;
static PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray = nullptr;
static PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = nullptr;
static PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = nullptr;
#endif

#include "shader_program.h"

// ── Embedded GLSL source (no file-IO at runtime) ─────────────────────────────
static const char* k_MapVertSrc = R"GLSL(
#version 120
attribute vec2  aPos;
attribute vec2  aTexCoord;
attribute vec4  aColor;
attribute float aShaderData;
uniform   float uTime;
uniform   int   uAmbientEffects;
uniform   int   uFloor;
varying   vec2  vTexCoord;
varying   vec4  vColor;
varying   vec2  vWorldPos;
varying   float vShaderData;

void main() {
    vColor      = aColor;
    vShaderData = aShaderData;
    vWorldPos   = aPos;
    vTexCoord   = aTexCoord;

    gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * vec4(aPos, 0.0, 1.0);
}
)GLSL";

static const char* k_MapFragSrc = R"GLSL(
#version 120
uniform sampler2D uTexture;
uniform int   uUpscaling;
uniform float uTime;
uniform int   uFloor;

// Experimental Biome & Post-Processing Shaders
uniform int   uExpColorGrading;
uniform int   uExpVignette;
uniform float uExpVignetteStrength;

varying vec2  vTexCoord;
varying vec4  vColor;
varying vec2  vWorldPos;
varying float vShaderData;

void main() {
    vec2 uv = vTexCoord;
    vec4 raw = texture2D(uTexture, uv);
    if (raw.a < 0.01) discard;

    vec4 texel = raw;

    // ── 1. Graphic Upgrader: Rich Color Vibrance without Brightness Washout ──
    if (uUpscaling == 1) {
        vec3 col = texel.rgb;
        float luma = dot(col, vec3(0.299, 0.587, 0.114));
        // Pure chroma vibrance boost without lifting luminance
        col = mix(vec3(luma), col, 1.25);
        texel.rgb = clamp(col, 0.0, 1.0);
    }

    // ── 2. Cinematic Biome Color Grading & Moods ──
    if (uExpColorGrading == 0) {
        // 0: Vibrant Fantasy RPG (Oberwelt - Standard & Natürlich)
        texel.rgb *= vec3(1.04, 1.03, 0.97);
        float g = dot(texel.rgb, vec3(0.299, 0.587, 0.114));
        texel.rgb = mix(vec3(g), texel.rgb, 1.15);
    } else if (uExpColorGrading == 1) {
        // 1: Dark & Dangerous (Drachen, Untote, Blight, Dungeons)
        texel.rgb = pow(texel.rgb, vec3(1.10));
        float g = dot(texel.rgb, vec3(0.299, 0.587, 0.114));
        vec3 desat = mix(vec3(g), texel.rgb, 0.82);
        float warm = max(0.0, texel.r - max(texel.g, texel.b));
        texel.rgb = desat * vec3(0.92, 0.88, 1.04) + vec3(warm * 0.30, warm * 0.10, 0.0);
    } else if (uExpColorGrading == 2) {
        // 2: Gloomy Crypt & Cave (Kühler Höhlen-Look)
        float g = dot(texel.rgb, vec3(0.299, 0.587, 0.114));
        texel.rgb = mix(vec3(g), texel.rgb * vec3(0.88, 0.92, 1.08), 0.75);
    } else if (uExpColorGrading == 3) {
        // 3: Golden Sunset & Twilight (Warme Abenddämmerung)
        texel.rgb *= vec3(1.12, 0.98, 0.82);
    } else if (uExpColorGrading == 4) {
        // 4: Frozen Wastes & Frost (Kühles Eis-Blau)
        texel.rgb *= vec3(0.88, 1.05, 1.18);
    } else if (uExpColorGrading == 5) {
        // 5: Neutral / Classic Vanilla (Ungefiltert)
    }

    // ── 3. Cinematic Vignette ──
    if (uExpVignette == 1) {
        vec2 normPos = gl_FragCoord.xy / vec2(1920.0, 1080.0) - vec2(0.5);
        float dist = length(normPos);
        float vig = smoothstep(0.35, 0.80, dist);
        texel.rgb = mix(texel.rgb, texel.rgb * 0.55, vig * uExpVignetteStrength);
    }

    gl_FragColor = texel * vColor;
}
)GLSL";


// ── Shader singleton + time accumulator + FPS monitor ──────────────────────
static RME_Rendering::ShaderProgram g_map_shader;
static float g_shader_time = 0.0f;
static uint32_t g_shader_last_ms = 0;
static float g_current_fps = 120.0f;

// Build column-major orthographic matrix (no GLM dependency)
static void makeOrtho(float* m16, float l, float r, float b, float t) {
    // Zero
    for (int i = 0; i < 16; ++i) m16[i] = 0.0f;
    m16[0]  =  2.0f / (r - l);
    m16[5]  =  2.0f / (t - b);
    m16[10] = -1.0f;
    m16[12] = -(r + l) / (r - l);
    m16[13] = -(t + b) / (t - b);
    m16[15] =  1.0f;
}

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
  show_preview = true;
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
  show_preview = true;
  show_hooks = false;
  hide_items_when_zoomed = false;
}

bool DrawingOptions::isDrawLight() const noexcept { return show_lights; }

void MapDrawer::DrawSelectionBox() {
  if (options.ingame || !canvas) {
    return;
  }

  if (!m_nvg) {
    return;
  }

  nvgBeginFrame(m_nvg, screensize_x, screensize_y, 1.0f);
  float lx = static_cast<float>(canvas->last_click_abs_x - view_scroll_x) /
             static_cast<float>(zoom);
  float ly = static_cast<float>(canvas->last_click_abs_y - view_scroll_y) /
             static_cast<float>(zoom);
  nvgBeginPath(m_nvg);
  nvgRect(m_nvg, lx, ly, canvas->cursor_x - lx, canvas->cursor_y - ly);
  nvgStrokeColor(m_nvg, nvgRGBA(218, 165, 32, 200));
  nvgStrokeWidth(m_nvg, 1.0f / zoom);
  nvgStroke(m_nvg);
  nvgEndFrame(m_nvg);
}

void MapDrawer::DrawLiveCursors() {
  if (options.ingame || !editor.IsLive()) {
    return;
  }

  LiveSocket &live = editor.GetLive();
  for (LiveCursor &cursor : live.getCursorList()) {
    if (cursor.pos.z <= GROUND_LAYER && floor > GROUND_LAYER) {
      continue;
    }
    if (cursor.pos.z > GROUND_LAYER && floor <= 8) {
      continue;
    }

    int offset = (cursor.pos.z <= GROUND_LAYER)
                     ? (GROUND_LAYER - cursor.pos.z) * TileSize
                     : TileSize * (floor - cursor.pos.z);
    float draw_x = ((cursor.pos.x * TileSize) - view_scroll_x) - offset;
    float draw_y = ((cursor.pos.y * TileSize) - view_scroll_y) - offset;

    glColor(cursor.color);
    glBegin(GL_QUADS);
    glVertex2f(draw_x, draw_y);
    glVertex2f(draw_x + TileSize, draw_y);
    glVertex2f(draw_x + TileSize, draw_y + TileSize);
    glVertex2f(draw_x, draw_y + TileSize);
    glEnd();
  }
}

void MapDrawer::DrawBrush() {
  static uint32_t last_log_time = 0;
  uint32_t current_time = wxGetLocalTimeMillis().GetValue();
  bool log_this = (current_time - last_log_time > 1000);

  if (g_gui.prefab_creator_brush) {
    const auto &selected = g_gui.prefab_creator_brush->getSelectedTiles();
    if (!selected.empty()) {
      glDisable(GL_TEXTURE_2D);
      glColor4ub(0, 128, 255, 100);
      glBegin(GL_QUADS);
      for (const auto &pos : selected) {
        if (pos.z == floor) {
          int cx = pos.x * TileSize - view_scroll_x - getFloorAdjustment(floor);
          int cy = pos.y * TileSize - view_scroll_y - getFloorAdjustment(floor);
          glVertex2f(cx, cy);
          glVertex2f(cx + TileSize, cy);
          glVertex2f(cx + TileSize, cy + TileSize);
          glVertex2f(cx, cy + TileSize);
        }
      }
      glEnd();
      glEnable(GL_TEXTURE_2D);
    }
  }

  if (!g_gui.IsDrawingMode() || !g_gui.GetCurrentBrush() || options.ingame) {
    if (!options.ingame && mouse_map_x != -1 && mouse_map_y != -1) {
      int offset = getFloorAdjustment(floor);
      float draw_x = ((mouse_map_x * TileSize) - view_scroll_x) - offset;
      float draw_y = ((mouse_map_y * TileSize) - view_scroll_y) - offset;
      glDisable(GL_TEXTURE_2D);
      // Subtle gold fill
      glColor4ub(255, 215, 0, 40);
      glBegin(GL_QUADS);
      glVertex2f(draw_x, draw_y);
      glVertex2f(draw_x + TileSize, draw_y);
      glVertex2f(draw_x + TileSize, draw_y + TileSize);
      glVertex2f(draw_x, draw_y + TileSize);
      glEnd();

      // Outer Corporate Gold Border (#FFD700)
      glLineWidth(2.0f);
      glColor4ub(255, 215, 0, 230);
      glBegin(GL_LINE_LOOP);
      glVertex2f(draw_x, draw_y);
      glVertex2f(draw_x + TileSize, draw_y);
      glVertex2f(draw_x + TileSize, draw_y + TileSize);
      glVertex2f(draw_x, draw_y + TileSize);
      glEnd();

      // Inner Warm Accent Gold Line (#D4AF37)
      glLineWidth(1.0f);
      glColor4ub(212, 175, 55, 170);
      glBegin(GL_LINE_LOOP);
      glVertex2f(draw_x + 1.0f, draw_y + 1.0f);
      glVertex2f(draw_x + TileSize - 1.0f, draw_y + 1.0f);
      glVertex2f(draw_x + TileSize - 1.0f, draw_y + TileSize - 1.0f);
      glVertex2f(draw_x + 1.0f, draw_y + TileSize - 1.0f);
      glEnd();
      glEnable(GL_TEXTURE_2D);
    }
    // [PERF] Removed: Disk I/O in render hot-path
    // if (log_this) {
    //   LogErrorToFile(...);
    //   last_log_time = current_time;
    // }
    return;
  }

  Brush *brush = g_gui.GetCurrentBrush();
  if (brush && brush->isCreature()) {
    if (mouse_map_x != -1 && mouse_map_y != -1) {
      glEnable(GL_TEXTURE_2D);
      int cy = mouse_map_y * TileSize - view_scroll_y - getFloorAdjustment(floor);
      int cx = mouse_map_x * TileSize - view_scroll_x - getFloorAdjustment(floor);
      CreatureBrush *creature_brush = brush->asCreature();
      if (creature_brush->canDraw(&editor.map, Position(mouse_map_x, mouse_map_y, floor))) {
        BlitCreature(cx, cy, creature_brush->getType()->outfit, (Direction)2, 255, 255, 255, 160);
      } else {
        BlitCreature(cx, cy, creature_brush->getType()->outfit, (Direction)2, 255, 64, 64, 160);
      }
    }
    return;
  }

  int lookid = brush->getLookID();
  bool has_preview = (lookid > 0 && g_items.typeExists(lookid));

  // [PERF] Removed: Disk I/O in render hot-path
  // if (log_this) {
  //   LogErrorToFile(...);
  //   last_log_time = current_time;
  // }

  BrushColor brushColor = COLOR_BLANK;
  if (brush->isTerrain() || brush->isTable() || brush->isCarpet()) {
    brushColor = COLOR_BRUSH;
  } else if (brush->isHouse()) {
    brushColor = COLOR_HOUSE_BRUSH;
  } else if (brush->isFlag()) {
    brushColor = COLOR_FLAG_BRUSH;
  } else if (brush->isSpawn()) {
    brushColor = COLOR_SPAWN_BRUSH;
  } else if (brush->isEraser()) {
    brushColor = COLOR_ERASER;
  }

  if (dragging_draw && canvas->rectangle_mode) {

    if (brush->isWall()) {
      int last_click_start_map_x =
          std::min(canvas->last_click_map_x, mouse_map_x);
      int last_click_start_map_y =
          std::min(canvas->last_click_map_y, mouse_map_y);
      int last_click_end_map_x =
          std::max(canvas->last_click_map_x, mouse_map_x) + 1;
      int last_click_end_map_y =
          std::max(canvas->last_click_map_y, mouse_map_y) + 1;

      int last_click_start_sx = last_click_start_map_x * TileSize -
                                view_scroll_x - getFloorAdjustment(floor);
      int last_click_start_sy = last_click_start_map_y * TileSize -
                                view_scroll_y - getFloorAdjustment(floor);
      int last_click_end_sx = last_click_end_map_x * TileSize - view_scroll_x -
                              getFloorAdjustment(floor);
      int last_click_end_sy = last_click_end_map_y * TileSize - view_scroll_y -
                              getFloorAdjustment(floor);

      int delta_x = last_click_end_sx - last_click_start_sx;
      int delta_y = last_click_end_sy - last_click_start_sy;

      glDisable(GL_TEXTURE_2D);
      glColor(brushColor);
      glBegin(GL_QUADS);
      {
        glVertex2f(last_click_start_sx, last_click_start_sy + TileSize);
        glVertex2f(last_click_end_sx, last_click_start_sy + TileSize);
        glVertex2f(last_click_end_sx, last_click_start_sy);
        glVertex2f(last_click_start_sx, last_click_start_sy);
      }

      if (delta_y > TileSize) {
        glVertex2f(last_click_start_sx, last_click_end_sy - TileSize);
        glVertex2f(last_click_start_sx + TileSize,
                   last_click_end_sy - TileSize);
        glVertex2f(last_click_start_sx + TileSize,
                   last_click_start_sy + TileSize);
        glVertex2f(last_click_start_sx, last_click_start_sy + TileSize);
      }

      if (delta_x > TileSize && delta_y > TileSize) {
        glVertex2f(last_click_end_sx - TileSize,
                   last_click_start_sy + TileSize);
        glVertex2f(last_click_end_sx, last_click_start_sy + TileSize);
        glVertex2f(last_click_end_sx, last_click_end_sy - TileSize);
        glVertex2f(last_click_end_sx - TileSize, last_click_end_sy - TileSize);
      }

      if (delta_y > TileSize) {
        glVertex2f(last_click_start_sx, last_click_end_sy - TileSize);
        glVertex2f(last_click_end_sx, last_click_end_sy - TileSize);
        glVertex2f(last_click_end_sx, last_click_end_sy);
        glVertex2f(last_click_start_sx, last_click_end_sy);
      }
      glEnd();
      glEnable(GL_TEXTURE_2D);
    } else {
      if (g_gui.GetBrushShape() == BRUSHSHAPE_SQUARE || brush->isSpawn()) {
        if (has_preview || brush->isOptionalBorder()) {
          int start_x, end_x, start_y, end_y;

          if (mouse_map_x < canvas->last_click_map_x) {
            start_x = mouse_map_x;
            end_x = canvas->last_click_map_x;
          } else {
            start_x = canvas->last_click_map_x;
            end_x = mouse_map_x;
          }
          if (mouse_map_y < canvas->last_click_map_y) {
            start_y = mouse_map_y;
            end_y = canvas->last_click_map_y;
          } else {
            start_y = canvas->last_click_map_y;
            end_y = mouse_map_y;
          }

          for (int y = start_y; y <= end_y; y++) {
            int cy = y * TileSize - view_scroll_y - getFloorAdjustment(floor);
            for (int x = start_x; x <= end_x; x++) {
              int cx = x * TileSize - view_scroll_x - getFloorAdjustment(floor);
              if (brush->isOptionalBorder()) {
                glDisable(GL_TEXTURE_2D);
                glColorCheck(brush, Position(x, y, floor));
                glBegin(GL_QUADS);
                glVertex2f(cx, cy + TileSize);
                glVertex2f(cx + TileSize, cy + TileSize);
                glVertex2f(cx + TileSize, cy);
                glVertex2f(cx, cy);
                glEnd();
                glEnable(GL_TEXTURE_2D);
              } else {
                glEnable(GL_TEXTURE_2D);
                DrawRawBrush(cx, cy, &g_items[lookid], 160, 160, 160, 160);
              }
            }
          }
        } else {
          int last_click_start_map_x =
              std::min(canvas->last_click_map_x, mouse_map_x);
          int last_click_start_map_y =
              std::min(canvas->last_click_map_y, mouse_map_y);
          int last_click_end_map_x =
              std::max(canvas->last_click_map_x, mouse_map_x) + 1;
          int last_click_end_map_y =
              std::max(canvas->last_click_map_y, mouse_map_y) + 1;

          int last_click_start_sx = last_click_start_map_x * TileSize -
                                    view_scroll_x - getFloorAdjustment(floor);
          int last_click_start_sy = last_click_start_map_y * TileSize -
                                    view_scroll_y - getFloorAdjustment(floor);
          int last_click_end_sx = last_click_end_map_x * TileSize -
                                  view_scroll_x - getFloorAdjustment(floor);
          int last_click_end_sy = last_click_end_map_y * TileSize -
                                  view_scroll_y - getFloorAdjustment(floor);

          glDisable(GL_TEXTURE_2D);
          glColor(brushColor);
          glBegin(GL_QUADS);
          glVertex2f(last_click_start_sx, last_click_start_sy);
          glVertex2f(last_click_end_sx, last_click_start_sy);
          glVertex2f(last_click_end_sx, last_click_end_sy);
          glVertex2f(last_click_start_sx, last_click_end_sy);
          glEnd();
          glEnable(GL_TEXTURE_2D);
        }
      } else if (g_gui.GetBrushShape() == BRUSHSHAPE_CIRCLE) {
        int start_x, end_x, start_y, end_y;
        int width =
            std::max(std::abs(std::max(mouse_map_y, canvas->last_click_map_y) -
                              std::min(mouse_map_y, canvas->last_click_map_y)),
                     std::abs(std::max(mouse_map_x, canvas->last_click_map_x) -
                              std::min(mouse_map_x, canvas->last_click_map_x)));

        if (mouse_map_x < canvas->last_click_map_x) {
          start_x = canvas->last_click_map_x - width;
          end_x = canvas->last_click_map_x;
        } else {
          start_x = canvas->last_click_map_x;
          end_x = canvas->last_click_map_x + width;
        }

        if (mouse_map_y < canvas->last_click_map_y) {
          start_y = canvas->last_click_map_y - width;
          end_y = canvas->last_click_map_y;
        } else {
          start_y = canvas->last_click_map_y;
          end_y = canvas->last_click_map_y + width;
        }

        int center_x = start_x + (end_x - start_x) / 2;
        int center_y = start_y + (end_y - start_y) / 2;
        float radii = width / 2.0f + 0.005f;

        for (int y = start_y - 1; y <= end_y + 1; y++) {
          int cy = y * TileSize - view_scroll_y - getFloorAdjustment(floor);
          float dy = center_y - y;
          for (int x = start_x - 1; x <= end_x + 1; x++) {
            int cx = x * TileSize - view_scroll_x - getFloorAdjustment(floor);
            float dx = center_x - x;
            // [PERF] Squared distance comparison instead of sqrt()
            float dist_sq = dx * dx + dy * dy;
            float radii_sq = radii * radii;
            if (dist_sq < radii_sq) {
              if (has_preview) {
                glEnable(GL_TEXTURE_2D);
                DrawRawBrush(cx, cy, &g_items[lookid], 160, 160, 160, 160);
              } else {
                glDisable(GL_TEXTURE_2D);
                glColor(brushColor);
                glBegin(GL_QUADS);
                glVertex2f(cx, cy + TileSize);
                glVertex2f(cx + TileSize, cy + TileSize);
                glVertex2f(cx + TileSize, cy);
                glVertex2f(cx, cy);
                glEnd();
                glEnable(GL_TEXTURE_2D);
              }
            }
          }
        }
      }
    }
  } else {
    if (brush->isWall()) {
      int start_map_x = mouse_map_x - g_gui.GetBrushSize();
      int start_map_y = mouse_map_y - g_gui.GetBrushSize();
      int end_map_x = mouse_map_x + g_gui.GetBrushSize() + 1;
      int end_map_y = mouse_map_y + g_gui.GetBrushSize() + 1;
      int start_sx =
          start_map_x * TileSize - view_scroll_x - getFloorAdjustment(floor);
      int start_sy =
          start_map_y * TileSize - view_scroll_y - getFloorAdjustment(floor);
      int end_sx =
          end_map_x * TileSize - view_scroll_x - getFloorAdjustment(floor);
      int end_sy =
          end_map_y * TileSize - view_scroll_y - getFloorAdjustment(floor);
      int delta_x = end_sx - start_sx, delta_y = end_sy - start_sy;

      glDisable(GL_TEXTURE_2D);
      glColor(brushColor);
      glBegin(GL_QUADS);
      glVertex2f(start_sx, start_sy + TileSize);
      glVertex2f(end_sx, start_sy + TileSize);
      glVertex2f(end_sx, start_sy);
      glVertex2f(start_sx, start_sy);
      if (delta_y > TileSize) {
        glVertex2f(start_sx, end_sy - TileSize);
        glVertex2f(start_sx + TileSize, end_sy - TileSize);
        glVertex2f(start_sx + TileSize, start_sy + TileSize);
        glVertex2f(start_sx, start_sy + TileSize);
      }
      if (delta_x > TileSize && delta_y > TileSize) {
        glVertex2f(end_sx - TileSize, start_sy + TileSize);
        glVertex2f(end_sx, start_sy + TileSize);
        glVertex2f(end_sx, end_sy - TileSize);
        glVertex2f(end_sx - TileSize, end_sy - TileSize);
      }
      if (delta_y > TileSize) {
        glVertex2f(start_sx, end_sy - TileSize);
        glVertex2f(end_sx, end_sy - TileSize);
        glVertex2f(end_sx, end_sy);
        glVertex2f(start_sx, end_sy);
      }
      glEnd();
      glEnable(GL_TEXTURE_2D);
    } else if (brush->isDoor()) {
      int cx =
          mouse_map_x * TileSize - view_scroll_x - getFloorAdjustment(floor);
      int cy =
          mouse_map_y * TileSize - view_scroll_y - getFloorAdjustment(floor);
      glDisable(GL_TEXTURE_2D);
      glColorCheck(brush, Position(mouse_map_x, mouse_map_y, floor));
      glBegin(GL_QUADS);
      glVertex2f(cx, cy + TileSize);
      glVertex2f(cx + TileSize, cy + TileSize);
      glVertex2f(cx + TileSize, cy);
      glVertex2f(cx, cy);
      glEnd();
      glEnable(GL_TEXTURE_2D);
    } else if (!brush->isDoodad() && !brush->isCreature()) {
      for (int y = -g_gui.GetBrushSize() - 1; y <= g_gui.GetBrushSize() + 1;
           y++) {
        int cy = (mouse_map_y + y) * TileSize - view_scroll_y -
                 getFloorAdjustment(floor);
        for (int x = -g_gui.GetBrushSize() - 1; x <= g_gui.GetBrushSize() + 1;
             x++) {
          int cx = (mouse_map_x + x) * TileSize - view_scroll_x -
                   getFloorAdjustment(floor);
          bool inside = false;
          bool is_wall = brush && brush->isWall();
          int bsize = g_gui.GetBrushSize();
          if (g_gui.GetBrushShape() == BRUSHSHAPE_SQUARE) {
            bool in_box = (x >= -bsize && x <= bsize && y >= -bsize && y <= bsize);
            if (in_box) {
              if (is_wall) {
                inside = (bsize == 0) || (std::abs(x) == bsize || std::abs(y) == bsize);
              } else {
                inside = true;
              }
            }
          } else if (g_gui.GetBrushShape() == BRUSHSHAPE_CIRCLE) {
            double threshold = bsize + 0.005;
            bool in_circle = (double(x * x) + double(y * y) < threshold * threshold);
            if (in_circle) {
              if (is_wall) {
                if (bsize == 0) {
                  inside = true;
                } else {
                  static const int dx4[] = {-1, 1, 0, 0};
                  static const int dy4[] = {0, 0, -1, 1};
                  for (int i = 0; i < 4; ++i) {
                    int nx = x + dx4[i];
                    int ny = y + dy4[i];
                    if (double(nx * nx) + double(ny * ny) >= threshold * threshold) {
                      inside = true;
                      break;
                    }
                  }
                }
              } else {
                inside = true;
              }
            }
          }
          if (inside) {
            if (has_preview) {
              glEnable(GL_TEXTURE_2D);
              DrawRawBrush(cx, cy, &g_items[lookid], 160, 160, 160, 160);
            } else {
              glDisable(GL_TEXTURE_2D);
              if (brush->isHouseExit() || brush->isOptionalBorder()) {
                glColorCheck(brush,
                             Position(mouse_map_x + x, mouse_map_y + y, floor));
              } else {
                glColor(brushColor);
              }
              glBegin(GL_QUADS);
              glVertex2f(cx, cy + TileSize);
              glVertex2f(cx + TileSize, cy + TileSize);
              glVertex2f(cx + TileSize, cy);
              glVertex2f(cx, cy);
              glEnd();
              glEnable(GL_TEXTURE_2D);
            }
          }
        }
      }
    }
  }
}

void MapDrawer::DrawIngameBox() {
  int center_x = start_x + int(screensize_x * zoom / 64);
  int center_y = start_y + int(screensize_y * zoom / 64);
  int box_start_x = center_x * TileSize - view_scroll_x;
  int box_start_y = (center_y + 2) * TileSize - view_scroll_y;
  int box_w = ClientMapWidth * TileSize;
  int box_h = ClientMapHeight * TileSize;

  // Extended outer client box (+10 tiles on each side for widescreen / high-res preview)
  int ext_margin = 10 * TileSize;
  int ext_x = box_start_x - ext_margin;
  int ext_y = box_start_y - ext_margin;
  int ext_w = box_w + 2 * ext_margin;
  int ext_h = box_h + 2 * ext_margin;

  glDisable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // --- 1. Weather Simulation Overlay (strictly inside client box, throttled if FPS < 70) ---
  int weather = g_settings.getInteger(Config::WEATHER_EFFECT);
  if (weather > 0 && g_current_fps >= 70.0f) {
    // Weather 1: Clouds & Shadows
    if (weather == 1) {
      // Soft drifting cloud masses
      for (int c = 0; c < 5; ++c) {
        float cx = ext_x + fmod((c * 230.0f) + g_shader_time * 25.0f * (1.0f + c * 0.15f), (float)(ext_w + 200.0f)) - 100.0f;
        float cy = ext_y + fmod((c * 170.0f) + g_shader_time * 12.0f, (float)(ext_h + 100.0f)) - 50.0f;
        float cw = 160.0f + (c % 3) * 60.0f;
        float ch = 100.0f + (c % 2) * 50.0f;
        glColor4ub(10, 20, 35, 45 + (c % 3) * 15);
        glBegin(GL_QUADS);
        glVertex2f(cx, cy);
        glVertex2f(cx + cw, cy);
        glVertex2f(cx + cw, cy + ch);
        glVertex2f(cx, cy + ch);
        glEnd();
      }
      // Sunlight Godray beams
      for (int g = 0; g < 3; ++g) {
        float gx = ext_x + fmod((g * 280.0f) + g_shader_time * 18.0f, (float)(ext_w + 150.0f)) - 50.0f;
        glColor4ub(255, 245, 190, 22);
        glBegin(GL_QUADS);
        glVertex2f(gx, (float)ext_y);
        glVertex2f(gx + 55.0f, (float)ext_y);
        glVertex2f(gx + 120.0f, (float)(ext_y + ext_h));
        glVertex2f(gx + 65.0f, (float)(ext_y + ext_h));
        glEnd();
      }
    }
    // Weather 2: Rain & Storm
    else if (weather == 2) {
      // Storm atmospheric tint
      glColor4ub(15, 25, 45, 55);
      glBegin(GL_QUADS);
      glVertex2f((float)ext_x, (float)ext_y);
      glVertex2f((float)(ext_x + ext_w), (float)ext_y);
      glVertex2f((float)(ext_x + ext_w), (float)(ext_y + ext_h));
      glVertex2f((float)ext_x, (float)(ext_y + ext_h));
      glEnd();

      // Slanted falling raindrops
      glLineWidth(1.5f);
      glBegin(GL_LINES);
      for (int i = 0; i < 280; ++i) {
        float speed = 520.0f + (i % 8) * 35.0f;
        float rx = ext_x + fmod((i * 137.5f) + g_shader_time * 110.0f, (float)ext_w);
        float ry = ext_y + fmod((i * 283.1f) + g_shader_time * speed, (float)ext_h);
        float len = 14.0f + (i % 5) * 3.0f;
        uint8_t a = static_cast<uint8_t>(130 + (i % 80));
        glColor4ub(190, 220, 255, a);
        glVertex2f(rx, ry);
        glVertex2f(rx - 5.0f, ry + len);
      }
      glEnd();
      glLineWidth(1.0f);

      // Rain splash ripples
      for (int i = 0; i < 24; ++i) {
        float cycle = fmod(g_shader_time * 3.2f + i * 0.41f, 1.0f);
        float sx = ext_x + fmod(i * 191.3f, (float)ext_w);
        float sy = ext_y + fmod(i * 317.7f, (float)ext_h);
        float r = cycle * 7.0f;
        uint8_t a = static_cast<uint8_t>((1.0f - cycle) * 110.0f);
        if (a > 5) {
          glColor4ub(200, 230, 255, a);
          glBegin(GL_LINE_LOOP);
          for (int seg = 0; seg < 8; ++seg) {
            float angle = seg * (6.2831853f / 8.0f);
            glVertex2f(sx + cos(angle) * r, sy + sin(angle) * (r * 0.45f));
          }
          glEnd();
        }
      }
    }
    // Weather 3: Snow & Blizzard
    else if (weather == 3) {
      // Winter cold atmospheric tint
      glColor4ub(210, 230, 255, 25);
      glBegin(GL_QUADS);
      glVertex2f((float)ext_x, (float)ext_y);
      glVertex2f((float)(ext_x + ext_w), (float)ext_y);
      glVertex2f((float)(ext_x + ext_w), (float)(ext_y + ext_h));
      glVertex2f((float)ext_x, (float)(ext_y + ext_h));
      glEnd();

      // Fluttering snowflakes
      glBegin(GL_QUADS);
      for (int i = 0; i < 320; ++i) {
        float speed = 40.0f + (i % 7) * 14.0f;
        float sway = sin(g_shader_time * 2.2f + i * 1.3f) * (12.0f + (i % 5) * 3.0f);
        float sx = ext_x + fmod(i * 83.7f + sway + g_shader_time * 18.0f, (float)ext_w);
        float sy = ext_y + fmod(i * 157.3f + g_shader_time * speed, (float)ext_h);
        float sz = 1.2f + (i % 4) * 0.8f;
        uint8_t a = static_cast<uint8_t>(140 + (i % 95));
        glColor4ub(245, 250, 255, a);
        glVertex2f(sx - sz, sy - sz);
        glVertex2f(sx + sz, sy - sz);
        glVertex2f(sx + sz, sy + sz);
        glVertex2f(sx - sz, sy + sz);
      }
      glEnd();
    }
    // Weather 4: Desert Heat / Haze
    else if (weather == 4) {
      // Golden desert atmospheric tint
      glColor4ub(255, 175, 40, 40);
      glBegin(GL_QUADS);
      glVertex2f((float)ext_x, (float)ext_y);
      glVertex2f((float)(ext_x + ext_w), (float)ext_y);
      glVertex2f((float)(ext_x + ext_w), (float)(ext_y + ext_h));
      glVertex2f((float)ext_x, (float)(ext_y + ext_h));
      glEnd();

      // Shimmering upward heat waves
      for (int b = 0; b < 10; ++b) {
        float by = ext_y + fmod(b * (ext_h / 10.0f) - g_shader_time * 25.0f, (float)ext_h);
        if (by < ext_y) by += ext_h;
        float wave = sin(g_shader_time * 2.8f + b * 1.4f) * 8.0f;
        uint8_t a = static_cast<uint8_t>(18 + sin(g_shader_time * 1.8f + b) * 10);
        glColor4ub(255, 220, 130, a);
        glBegin(GL_QUADS);
        glVertex2f((float)ext_x, by - 8.0f + wave);
        glVertex2f((float)(ext_x + ext_w), by - 8.0f - wave);
        glVertex2f((float)(ext_x + ext_w), by + 8.0f - wave);
        glVertex2f((float)ext_x, by + 8.0f + wave);
        glEnd();
      }

      // Drifting sand motes
      glBegin(GL_QUADS);
      for (int i = 0; i < 60; ++i) {
        float sx = ext_x + fmod(i * 113.0f + g_shader_time * 45.0f, (float)ext_w);
        float sy = ext_y + fmod(i * 73.0f + sin(g_shader_time * 2.0f + i) * 6.0f, (float)ext_h);
        float sz = 1.0f + (i % 3) * 0.5f;
        glColor4ub(240, 200, 120, static_cast<uint8_t>(120 + (i % 60)));
        glVertex2f(sx - sz, sy - sz);
        glVertex2f(sx + sz, sy - sz);
        glVertex2f(sx + sz, sy + sz);
        glVertex2f(sx - sz, sy + sz);
      }
      glEnd();
    }
    // Weather 5: Dense Fog / Mist
    else if (weather == 5) {
      // Base fog layer
      glColor4ub(220, 230, 240, 65);
      glBegin(GL_QUADS);
      glVertex2f((float)ext_x, (float)ext_y);
      glVertex2f((float)(ext_x + ext_w), (float)ext_y);
      glVertex2f((float)(ext_x + ext_w), (float)(ext_y + ext_h));
      glVertex2f((float)ext_x, (float)(ext_y + ext_h));
      glEnd();

      // Rolling mist waves
      for (int m = 0; m < 6; ++m) {
        float mx = ext_x + fmod((m * 210.0f) + g_shader_time * 16.0f * (1.0f + m * 0.1f), (float)(ext_w + 300.0f)) - 150.0f;
        float my = ext_y + (m * (ext_h / 6.0f));
        float mw = 220.0f + (m % 3) * 70.0f;
        float mh = (ext_h / 5.0f);
        glColor4ub(230, 240, 250, 40 + (m % 3) * 15);
        glBegin(GL_QUADS);
        glVertex2f(mx, my);
        glVertex2f(mx + mw, my);
        glVertex2f(mx + mw, my + mh);
        glVertex2f(mx, my + mh);
        glEnd();
      }
    }
  }

  // --- 2. Inner Standard Client Box (Solid Red Line) ---
  drawRect(box_start_x, box_start_y, box_w, box_h, *wxRED);

  // --- 3. Outer Extended Client Box (+10 Tiles: Red Dashed Line) ---
  glColor4ub(255, 50, 50, 230);
  const int dash_len = 8;
  const int gap_len = 6;
  // Top edge
  for (int x = ext_x; x < ext_x + ext_w; x += dash_len + gap_len) {
    int x2 = std::min(x + dash_len, ext_x + ext_w);
    glBegin(GL_LINES);
    glVertex2i(x, ext_y);
    glVertex2i(x2, ext_y);
    glEnd();
  }
  // Bottom edge
  for (int x = ext_x; x < ext_x + ext_w; x += dash_len + gap_len) {
    int x2 = std::min(x + dash_len, ext_x + ext_w);
    glBegin(GL_LINES);
    glVertex2i(x, ext_y + ext_h);
    glVertex2i(x2, ext_y + ext_h);
    glEnd();
  }
  // Left edge
  for (int y = ext_y; y < ext_y + ext_h; y += dash_len + gap_len) {
    int y2 = std::min(y + dash_len, ext_y + ext_h);
    glBegin(GL_LINES);
    glVertex2i(ext_x, y);
    glVertex2i(ext_x, y2);
    glEnd();
  }
  // Right edge
  for (int y = ext_y; y < ext_y + ext_h; y += dash_len + gap_len) {
    int y2 = std::min(y + dash_len, ext_y + ext_h);
    glBegin(GL_LINES);
    glVertex2i(ext_x + ext_w, y);
    glVertex2i(ext_x + ext_w, y2);
    glEnd();
  }

  glEnable(GL_TEXTURE_2D);
}

void MapDrawer::DrawGrid() {
  int grid_opacity = g_settings.getInteger(Config::GRID_OPACITY);
  if (grid_opacity < 10) {
    grid_opacity = 10;
  } else if (grid_opacity > 180) {
    grid_opacity = 180;
  }

  glDisable(GL_TEXTURE_2D);
  bool isDarkTheme = g_settings.getInteger(Config::UI_THEME) == 0;
  if (isDarkTheme) {
    glColor4ub(255, 255, 255, static_cast<uint8_t>(grid_opacity));
  } else {
    glColor4ub(0, 0, 0, static_cast<uint8_t>(grid_opacity));
  }
  glBegin(GL_LINES);
  for (int y = start_y; y < end_y; ++y) {
    glVertex2f(start_x * TileSize - view_scroll_x,
               y * TileSize - view_scroll_y);
    glVertex2f(end_x * TileSize - view_scroll_x, y * TileSize - view_scroll_y);
  }
  for (int x = start_x; x < end_x; ++x) {
    glVertex2f(x * TileSize - view_scroll_x,
               start_y * TileSize - view_scroll_y);
    glVertex2f(x * TileSize - view_scroll_x, end_y * TileSize - view_scroll_y);
  }
  glEnd();
  glEnable(GL_TEXTURE_2D);
}

void MapDrawer::DrawTooltips() {
  if (!m_nvg) {
    return;
  }

  nvgBeginFrame(m_nvg, screensize_x, screensize_y, 1.0f);
  for (auto *tooltip : tooltips) {
    if (!tooltip) {
      continue;
    }
    nvgBeginPath(m_nvg);
    nvgRect(m_nvg, tooltip->x, tooltip->y, 160.0f, 24.0f);
    nvgFillColor(m_nvg, nvgRGBA(tooltip->r, tooltip->g, tooltip->b, 200));
    nvgFill(m_nvg);
  }
  nvgEndFrame(m_nvg);
}

void MapDrawer::DrawRawBrush(int screenx, int screeny, ItemType *itemType,
                             uint8_t r, uint8_t g, uint8_t b, uint8_t alpha) {
  if (!itemType) {
    return;
  }

  GameSprite *spr = itemType->sprite;
  uint16_t cid = itemType->clientID;
  if (cid == 469) {
    b = 0;
    alpha = alpha / 3 * 2;
    spr = g_items[SPRITE_ZONE].sprite;
  } else if (cid == 470) {
    g = 0;
    b = 0;
    alpha = alpha / 3 * 2;
    spr = g_items[SPRITE_ZONE].sprite;
  } else if (cid == 2187) {
    r = 0;
    alpha = alpha / 3;
    spr = g_items[SPRITE_ZONE].sprite;
  }
  if ((cid >= 39092 && cid <= 39100) || cid == 39236 || cid == 39367 ||
      cid == 39368) {
    spr = g_items[SPRITE_LIGHTSOURCE].sprite;
    r = 0;
    alpha = alpha / 3 * 2;
  }
  
  if (spr) {
    BlitSpriteType(screenx, screeny, spr, r, g, b, alpha);
  } else {
    glDisable(GL_TEXTURE_2D);
    glBlitSquare(screenx, screeny, r, g, b, alpha, TileSize);
    glEnable(GL_TEXTURE_2D);
  }
}

void MapDrawer::DrawHookIndicator(int x, int y, const ItemType &type) {
  glDisable(GL_TEXTURE_2D);
  glColor4ub(0, 0, 255, 200);
  glBegin(GL_QUADS);
  if (type.hookSouth) {
    glVertex2f(x - 10, y + 10);
    glVertex2f(x, y + 10);
    glVertex2f(x + 10, y + 20);
    glVertex2f(x, y + 20);
  }
  glEnd();
  glEnable(GL_TEXTURE_2D);
}

void MapDrawer::glColor(wxColor color) {
  glColor4ub(color.Red(), color.Green(), color.Blue(), color.Alpha());
}

void MapDrawer::glColor(MapDrawer::BrushColor color) {
  switch (color) {
  case COLOR_BRUSH:
    glColor4ub(g_settings.getInteger(Config::CURSOR_RED),
               g_settings.getInteger(Config::CURSOR_GREEN),
               g_settings.getInteger(Config::CURSOR_BLUE),
               g_settings.getInteger(Config::CURSOR_ALPHA));
    break;
  case COLOR_SPAWN_BRUSH:
  case COLOR_ERASER:
    glColor4ub(166, 0, 0, 128);
    break;
  case COLOR_VALID:
    glColor4ub(0, 166, 0, 128);
    break;
  default:
    glColor4ub(255, 255, 255, 128);
    break;
  }
}

void MapDrawer::glColorCheck(Brush *brush, const Position &pos) {
  if (brush->canDraw(&editor.map, pos)) {
    glColor(COLOR_VALID);
  } else {
    glColor(COLOR_INVALID);
  }
}

void MapDrawer::drawRect(int x, int y, int w, int h, const wxColor &color,
                         int width) {
  glDisable(GL_TEXTURE_2D);
  glLineWidth(width);
  glColor4ub(color.Red(), color.Green(), color.Blue(), color.Alpha());
  glBegin(GL_LINE_STRIP);
  glVertex2f(x, y);
  glVertex2f(x + w, y);
  glVertex2f(x + w, y + h);
  glVertex2f(x, y + h);
  glVertex2f(x, y);
  glEnd();
  glEnable(GL_TEXTURE_2D);
}

void MapDrawer::drawFilledRect(int x, int y, int w, int h,
                               const wxColor &color) {
  glColor4ub(color.Red(), color.Green(), color.Blue(), color.Alpha());
  glBegin(GL_QUADS);
  glVertex2f(x, y);
  glVertex2f(x + w, y);
  glVertex2f(x + w, y + h);
  glVertex2f(x, y + h);
  glEnd();
}

void MapDrawer::glBlitSquare(int sx, int sy, int red, int green, int blue,
                             int alpha, int size) {
  if (size == 0) {
    size = TileSize;
  }
  glColor4ub(static_cast<uint8_t>(red), static_cast<uint8_t>(green),
             static_cast<uint8_t>(blue), static_cast<uint8_t>(alpha));
  glBegin(GL_QUADS);
  glVertex2f(sx, sy);
  glVertex2f(sx + size, sy);
  glVertex2f(sx + size, sy + size);
  glVertex2f(sx, sy + size);
  glEnd();
}

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

  if (g_settings.getBoolean(Config::EXP_PIXEL_SNAPPING) && zoom > 0.0f) {
    view_scroll_x = int(std::round(float(view_scroll_x) * zoom) / zoom);
    view_scroll_y = int(std::round(float(view_scroll_y) * zoom) / zoom);
  }

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
  static bool s_gl_procs_initialized = false;
  if (!s_gl_procs_initialized) {
    auto load_gl = [](const char* name) -> void* {
      void* p = (void*)wglGetProcAddress(name);
      if (!p || p == (void*)0x1 || p == (void*)0x2 || p == (void*)0x3 || p == (void*)-1) {
        static HMODULE hMod = GetModuleHandleA("opengl32.dll");
        if (hMod) p = (void*)GetProcAddress(hMod, name);
      }
      return p;
    };
    glGenBuffers        = (PFNGLGENBUFFERSPROC)      load_gl("glGenBuffers");
    glBindBuffer        = (PFNGLBINDBUFFERPROC)      load_gl("glBindBuffer");
    glBufferData        = (PFNGLBUFFERDATAPROC)      load_gl("glBufferData");
    glBufferSubData     = (PFNGLBUFFERSUBDATAPROC)   load_gl("glBufferSubData");
    glDeleteBuffers     = (PFNGLDELETEBUFFERSPROC)   load_gl("glDeleteBuffers");
    glEnableVertexAttribArray  = (PFNGLENABLEVERTEXATTRIBARRAYPROC) load_gl("glEnableVertexAttribArray");
    glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)load_gl("glDisableVertexAttribArray");
    glVertexAttribPointer      = (PFNGLVERTEXATTRIBPOINTERPROC)     load_gl("glVertexAttribPointer");
    glUseProgram        = (PFNGLUSEPROGRAMPROC)      load_gl("glUseProgram");
    glBindVertexArray   = (PFNGLBINDVERTEXARRAYPROC) load_gl("glBindVertexArray");
    glActiveTexture     = (PFNGLACTIVETEXTUREPROC)   load_gl("glActiveTexture");
    glGenVertexArrays   = (PFNGLGENVERTEXARRAYSPROC)    load_gl("glGenVertexArrays");
    glDeleteVertexArrays= (PFNGLDELETEVERTEXARRAYSPROC) load_gl("glDeleteVertexArrays");
    s_gl_procs_initialized = true;
  }
  // Phase 2: Build the map shader once (no-op if already built)
  if (!g_map_shader.isValid()) {
    g_map_shader.build(k_MapVertSrc, k_MapFragSrc);
  }

  if (glUseProgram) {
    glUseProgram(0);
  }
  if (glBindVertexArray) {
    glBindVertexArray(0);
  }
  if (glActiveTexture) {
    glActiveTexture(GL_TEXTURE0);
  }
  if (glBindBuffer) {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }
#endif

  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_DEPTH_TEST);

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

  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // ── Phase 2: update shader uniforms ────────────────────────────────────────
  if (g_map_shader.isValid()) {
    // Advance animation time & calculate moving-average FPS
    uint32_t now_ms = wxGetLocalTimeMillis().GetValue();
    if (g_shader_last_ms != 0) {
      float dt = (now_ms - g_shader_last_ms) / 1000.0f;
      g_shader_time = float(g_gui.gfx.getElapsedTime()) / 1000.0f;
    }
    g_shader_last_ms = now_ms;

    bool allow_upscaling = g_settings.getBoolean(Config::FAKE_HD_ASSETS);

    g_map_shader.use();
    g_map_shader.setFloat("uTime",   g_shader_time);
    g_map_shader.setInt("uTexture", 0); // texture unit 0
    g_map_shader.setInt("uUpscaling", allow_upscaling ? 1 : 0);
    g_map_shader.setInt("uFloor", 7); // default: Oberflaeche

    // Experimental Biome & Vignette uniforms
    g_map_shader.setInt("uExpColorGrading", g_settings.getInteger(Config::EXP_COLOR_GRADING));
    g_map_shader.setInt("uExpVignette", g_settings.getBoolean(Config::EXP_VIGNETTE) ? 1 : 0);
    g_map_shader.setFloat("uExpVignetteStrength", g_settings.getFloat(Config::EXP_VIGNETTE_STRENGTH));

    RME_Rendering::ShaderProgram::unuse(); // VBO draw path activates it per-chunk
  }
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

  // g_floor_batches is managed by VBO lifecycle, do not clear it here.
  g_pending_instances.clear();

  // Deactivate shader (back to fixed-function for UI/overlay paths)
  if (g_map_shader.isValid()) {
    RME_Rendering::ShaderProgram::unuse();
  }

  // Disable 2D mode
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

void MapDrawer::Draw() {
  last_bound_texture = -1;
  {
    DrawBackground();
  }
  {
    DrawMap();
  }
  if (options.isDrawLight()) {
    DrawLight();
  }


  // GPU-Timer für Shader-Animationen aktualisieren
  static float s_gpu_anim_time = 0.0f;
  s_gpu_anim_time += 0.016f;
  // Hier erfolgt der Upload an den Shader-Kern:
  // glUniform1f(m_u_time_loc, s_gpu_anim_time);

  DrawDraggingShadow();
  {
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
  int bg_choice = g_settings.getInteger(Config::BG_COLOR);
  switch (bg_choice) {
    case 1: // Parchment / Warm Beige
      glClearColor(0.85f, 0.78f, 0.67f, 1.0f);
      break;
    case 2: // Dark Slate
      glClearColor(0.08f, 0.10f, 0.14f, 1.0f);
      break;
    case 3: // Ocean Blue
      glClearColor(0.04f, 0.12f, 0.24f, 1.0f);
      break;
    case 4: // Forest Dark
      glClearColor(0.05f, 0.12f, 0.07f, 1.0f);
      break;
    case 5: // Classic Grey
      glClearColor(0.20f, 0.20f, 0.20f, 1.0f);
      break;
    case 6: // Pure White
      glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
      break;
    case 0: // Black (Default)
    default:
      glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      break;
  }

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
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

  int map_w = editor.map.getWidth();
  int map_h = editor.map.getHeight();
  int nd_start_x = std::max(0, (start_x & ~3));
  int nd_start_y = std::max(0, (start_y & ~3));
  int nd_end_x = std::min(map_w - 1, (end_x & ~3) + 4);
  int nd_end_y = std::min(map_h - 1, (end_y & ~3) + 4);

  std::vector<QTreeNode::VisibleNode> visible_nodes;
  if (live_client) {
    visible_nodes.reserve(((nd_end_x - nd_start_x) / 4 + 1) * ((nd_end_y - nd_start_y) / 4 + 1));
    for (int nd_map_x = nd_start_x; nd_map_x <= nd_end_x; nd_map_x += 4) {
      for (int nd_map_y = nd_start_y; nd_map_y <= nd_end_y; nd_map_y += 4) {
        QTreeNode *nd = editor.map.getLeaf(nd_map_x, nd_map_y);
        if (!nd) {
          nd = editor.map.createLeaf(nd_map_x, nd_map_y);
          nd->setVisible(false, false);
        }
        visible_nodes.push_back({nd, nd_map_x, nd_map_y});
      }
    }
  } else {
    editor.map.root.getVisibleLeaves(0, 0, -1, nd_start_x, nd_start_y, nd_end_x, nd_end_y, visible_nodes);
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
      bool translated = false;
      bool client_states_active = false;
      bool shader_active = false;

      // Check if shader pipeline is available for this floor
      bool can_use_shader = (g_map_shader.isValid() &&
                             glEnableVertexAttribArray && glVertexAttribPointer &&
                             glDisableVertexAttribArray && !only_colors);

      if (can_use_shader) {
        bool allow_ambient   = g_settings.getBoolean(Config::AMBIENT_EFFECTS);
        bool allow_upscaling = g_settings.getBoolean(Config::FAKE_HD_ASSETS);

        g_map_shader.use();
        g_map_shader.setFloat("uTime", g_shader_time);
        g_map_shader.setInt("uTexture", 0);
        g_map_shader.setInt("uUpscaling", allow_upscaling ? 1 : 0);
        g_map_shader.setInt("uAmbientEffects", allow_ambient ? 1 : 0);
        g_map_shader.setInt("uFloor", map_z);

        const GLsizei stride = sizeof(RME_Rendering::MapVertex);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
        shader_active = true;
      }

      for (const auto& vn : visible_nodes) {
        QTreeNode *nd = vn.node;
        int nd_map_x = vn.map_x;
        int nd_map_y = vn.map_y;

        if (!live_client || nd->isVisible(map_z > GROUND_LAYER)) {
          Floor *f = nd->getFloor(map_z);
          if (f) {
            bool needs_rebuild = false;

            if (nd->isDirty(map_z)) {
              // Hard reset: user edited tiles — VBO must be fully cleared
              if (f->vbo_id != 0) {
                glDeleteBuffers(1, &f->vbo_id);
                g_floor_batches.erase(f->vbo_id);
                f->vbo_id = 0;
                f->vbo_allocated_size = 0;
              }
              nd->clearDirty(map_z);
              needs_rebuild = true;
            } else if ((f->has_animations && zoom < 1.95f) || f->last_rebuild_tick != current_vbo_revision) {
              // Soft update: animation frame or global revision change
              needs_rebuild = true;
              if (f->vbo_id != 0) {
                g_floor_batches.erase(f->vbo_id);
              }
            }

            if (needs_rebuild && !options.dragging && !only_colors) {
              if (translated) {
                glPopMatrix();
                translated = false;
              }
              if (client_states_active) {
                glDisableClientState(GL_VERTEX_ARRAY);
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                glDisableClientState(GL_COLOR_ARRAY);
                client_states_active = false;
              }

              // Allocate new VBO only if we don't have one yet
              if (f->vbo_id == 0) {
                glGenBuffers(1, &f->vbo_id);
              }
              f->last_rebuild_tick = current_vbo_revision;

              g_vbo_vertices.clear();
              g_vbo_batches.clear();
              g_pending_instances.clear();
              g_vbo_building = true;
              f->has_animations = false;
              f->lights.clear();

              int old_scroll_x = view_scroll_x;
              int old_scroll_y = view_scroll_y;
              view_scroll_x = 0;
              view_scroll_y = 0;

              for (int map_x = 0; map_x < 4; ++map_x) {
                for (int map_y = 0; map_y < 4; ++map_y) {
                  TileLocation *location = nd->getTile(map_x, map_y, map_z);
                  DrawTile(location, f);
                }
              }

              view_scroll_x = old_scroll_x;
              view_scroll_y = old_scroll_y;

              g_vbo_building = false;

              glBindBuffer(GL_ARRAY_BUFFER, f->vbo_id);
              if (!g_vbo_vertices.empty()) {
                const ptrdiff_t needed = static_cast<ptrdiff_t>(
                    g_vbo_vertices.size() * sizeof(RME_Rendering::MapVertex));

                if (needed > f->vbo_allocated_size) {
                  glBufferData(GL_ARRAY_BUFFER, needed, nullptr, GL_STREAM_DRAW);
                  glBufferData(GL_ARRAY_BUFFER, needed, g_vbo_vertices.data(), GL_STREAM_DRAW);
                  f->vbo_allocated_size = needed;
                } else if (glBufferSubData) {
                  glBufferData(GL_ARRAY_BUFFER, f->vbo_allocated_size, nullptr, GL_STREAM_DRAW);
                  glBufferSubData(GL_ARRAY_BUFFER, 0, needed, g_vbo_vertices.data());
                } else {
                  glBufferData(GL_ARRAY_BUFFER, needed, g_vbo_vertices.data(), GL_STREAM_DRAW);
                  f->vbo_allocated_size = needed;
                }

                glBindBuffer(GL_ARRAY_BUFFER, 0);
                g_floor_batches[f->vbo_id] = g_vbo_batches;
                f->is_empty = false;
              } else {
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                f->is_empty = true;
              }
            }

            if (f->vbo_id != 0 && !f->is_empty && !only_colors) {
              if (!translated) {
                glPushMatrix();
                glTranslatef(-view_scroll_x, -view_scroll_y, 0);
                translated = true;
              }

              if (shader_active) {
                glBindBuffer(GL_ARRAY_BUFFER, f->vbo_id);
                const GLsizei stride = sizeof(RME_Rendering::MapVertex);
                glVertexAttribPointer(0, 2, GL_FLOAT,         GL_FALSE, stride, (void*)0);  // aPos
                glVertexAttribPointer(1, 2, GL_FLOAT,         GL_FALSE, stride, (void*)8);  // aTexCoord
                glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE,  stride, (void*)16); // aColor
                glVertexAttribPointer(3, 1, GL_FLOAT,         GL_FALSE, stride, (void*)20); // aShaderData

                const auto &batches = g_floor_batches[f->vbo_id];
                for (const auto &batch : batches) {
                  bindTexture(batch.textureId);
                  glDrawArrays(GL_QUADS, batch.start, batch.count);
                }
              } else {
                // ── Fallback: legacy fixed-function path ───────────────────
                glBindBuffer(GL_ARRAY_BUFFER, f->vbo_id);
                if (!client_states_active) {
                  glEnableClientState(GL_VERTEX_ARRAY);
                  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                  glEnableClientState(GL_COLOR_ARRAY);
                  client_states_active = true;
                }
                glVertexPointer  (2, GL_FLOAT,         sizeof(RME_Rendering::MapVertex), (void*)0);
                glTexCoordPointer(2, GL_FLOAT,         sizeof(RME_Rendering::MapVertex), (void*)8);
                glColorPointer   (4, GL_UNSIGNED_BYTE, sizeof(RME_Rendering::MapVertex), (void*)16);

                const auto &batches = g_floor_batches[f->vbo_id];
                for (const auto &batch : batches) {
                  bindTexture(batch.textureId);
                  glDrawArrays(GL_QUADS, batch.start, batch.count);
                }
                glBindBuffer(GL_ARRAY_BUFFER, 0);
              }

              g_pending_instances.clear();
              last_bound_texture = -1;
            } else if ((f->vbo_id == 0 && !f->is_empty) || only_colors) {
              if (translated) {
                glPopMatrix();
                translated = false;
              }
              if (client_states_active) {
                glDisableClientState(GL_VERTEX_ARRAY);
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                glDisableClientState(GL_COLOR_ARRAY);
                client_states_active = false;
              }

              for (int map_x = 0; map_x < 4; ++map_x) {
                for (int map_y = 0; map_y < 4; ++map_y) {
                  TileLocation *location = nd->getTile(map_x, map_y, map_z);
                  DrawTile(location);
                }
              }
            }

            if (options.isDrawLight() && map_z == floor) {
              if (f->vbo_id != 0) {
                for (const auto &l : f->lights) {
                  light_drawer->addLight(l.map_x, l.map_y, l.map_z, SpriteLight{l.intensity, l.color});
                }
              } else {
                for (int map_x = 0; map_x < 4; ++map_x) {
                  for (int map_y = 0; map_y < 4; ++map_y) {
                    TileLocation *location = nd->getTile(map_x, map_y, map_z);
                    if (location) {
                      AddLight(location);
                    }
                  }
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
          if (translated) {
            glPopMatrix();
            translated = false;
          }
          if (client_states_active) {
            glDisableClientState(GL_VERTEX_ARRAY);
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            glDisableClientState(GL_COLOR_ARRAY);
            client_states_active = false;
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

      if (shader_active) {
        glDisableVertexAttribArray(3);
        glDisableVertexAttribArray(2);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        RME_Rendering::ShaderProgram::unuse();
        shader_active = false;
      }

      if (translated) {
        glPopMatrix();
        translated = false;
      }
      if (client_states_active) {
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
        client_states_active = false;
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
                  r = uint8_t(r * 0.7f);
                  g = uint8_t(g * 0.7f);
                  b = uint8_t(std::min(255, int(b * 1.3f)));
                } else {
                  r = uint8_t(r * 0.75f);
                  g = uint8_t(g * 0.75f);
                  b = uint8_t(std::min(255, int(b * 1.2f)));
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

              if (options.show_houses && tile->isHouseTile()) {
                if ((int)tile->getHouseID() == current_house_id) {
                  // Selected house: bright blue overlay
                  BlitSquare(draw_x, draw_y, 0, 90, 220, 90);
                } else {
                  // Non-selected house: subtle dark blue overlay
                  BlitSquare(draw_x, draw_y, 0, 35, 120, 80);
                }
              }
            }

            // Draw items on the tile
            if (zoom <= 1.5 || !options.hide_items_when_zoomed) {
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
      move_x = canvas->drag_start_map_x - mouse_map_x;
      move_y = canvas->drag_start_map_y - mouse_map_y;
      move_z = canvas->drag_start_map_z - floor;

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
        ItemVector toRender = tile->getSelectedItems(zoom > 1.5);
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
        if (zoom <= 1.5 || !options.hide_items_when_zoomed) {
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
  if (floor != 8 && floor != 0 && options.transparent_floors && zoom <= 1.5f) {
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
          if (zoom <= 1.5 || !options.hide_items_when_zoomed) {
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
  std::string lowerName = it.name;
  std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

  if (it.isGroundTile() && (lowerName.find("water") != std::string::npos ||
                            lowerName.find("sea")   != std::string::npos ||
                            lowerName.find("ocean") != std::string::npos ||
                            lowerName.find("river") != std::string::npos ||
                            lowerName.find("lake")  != std::string::npos)) {
    g_vbo_current_shader_flag = 1.0f; // Wasser – Dynamic Caustic Shimmer
  } else if (it.isGroundTile() && (lowerName.find("lava") != std::string::npos ||
                                   lowerName.find("magma") != std::string::npos)) {
    g_vbo_current_shader_flag = 3.0f; // Lava / Magma – Heat Glow
  } else if (it.isGroundTile() && lowerName.find("sand") != std::string::npos &&
             lowerName.find("beach") == std::string::npos &&
             lowerName.find("shore") == std::string::npos) {
    g_vbo_current_shader_flag = 5.0f; // Wüstensand – Hitzeschleier & Dune Grain
  } else if (it.isGroundTile() && (lowerName.find("snow") != std::string::npos ||
                                   lowerName.find("ice")  != std::string::npos ||
                                   lowerName.find("frost") != std::string::npos)) {
    g_vbo_current_shader_flag = 6.0f; // Schnee / Eis – Frostschleier & Crystal Sparkle
  } else if (it.isGroundTile() && (lowerName.find("grass") != std::string::npos ||
                                   lowerName.find("dirt")  != std::string::npos ||
                                   lowerName.find("earth") != std::string::npos ||
                                   lowerName.find("mud")   != std::string::npos ||
                                   lowerName.find("jungle")!= std::string::npos ||
                                   lowerName.find("moss")  != std::string::npos ||
                                   item->getID() == 102 || item->getID() == 103 || item->getID() == 4526)) {
    g_vbo_current_shader_flag = 10.0f; // Grass & Earth – Organic Blades & Soil Variegation
  } else if (it.isGroundTile() && (lowerName.find("cobble") != std::string::npos ||
                                   lowerName.find("stone")  != std::string::npos ||
                                   lowerName.find("pavement")!= std::string::npos ||
                                   lowerName.find("tile")   != std::string::npos ||
                                   lowerName.find("brick")  != std::string::npos ||
                                   lowerName.find("floor")  != std::string::npos ||
                                   lowerName.find("flag")   != std::string::npos ||
                                   lowerName.find("gravel") != std::string::npos ||
                                   lowerName.find("rock")   != std::string::npos)) {
    g_vbo_current_shader_flag = 11.0f; // Cobblestone & Masonry – Beveled Crevice AO & Mineral Grit
  } else if (it.isGroundTile()) {
    g_vbo_current_shader_flag = 12.0f; // Generic Ground Layer Detail
  } else if (!it.isGroundTile() && !it.isBorder && !it.isWall) {
    // Explicit blacklist: Shelves, furniture, lights, structures must NEVER sway
    bool isBlacklisted = (
      lowerName.find("shelf")    != std::string::npos ||
      lowerName.find("rack")     != std::string::npos ||
      lowerName.find("book")     != std::string::npos ||
      lowerName.find("chest")    != std::string::npos ||
      lowerName.find("table")    != std::string::npos ||
      lowerName.find("chair")    != std::string::npos ||
      lowerName.find("bench")    != std::string::npos ||
      lowerName.find("bed")      != std::string::npos ||
      lowerName.find("box")      != std::string::npos ||
      lowerName.find("crate")    != std::string::npos ||
      lowerName.find("barrel")   != std::string::npos ||
      lowerName.find("drawer")   != std::string::npos ||
      lowerName.find("cupboard") != std::string::npos ||
      lowerName.find("cabinet")  != std::string::npos ||
      lowerName.find("stump")    != std::string::npos ||
      lowerName.find("trunk")    != std::string::npos ||
      lowerName.find("log")      != std::string::npos ||
      lowerName.find("lamp")     != std::string::npos ||
      lowerName.find("lantern")  != std::string::npos ||
      lowerName.find("torch")    != std::string::npos ||
      lowerName.find("candle")   != std::string::npos ||
      lowerName.find("sign")     != std::string::npos ||
      lowerName.find("statue")   != std::string::npos ||
      lowerName.find("carpet")   != std::string::npos ||
      lowerName.find("tapestry") != std::string::npos ||
      lowerName.find("dead")     != std::string::npos ||
      lowerName.find("dry")      != std::string::npos ||
      lowerName.find("fallen")   != std::string::npos ||
      lowerName.find("cut")      != std::string::npos ||
      lowerName.find("stone")    != std::string::npos ||
      lowerName.find("rock")     != std::string::npos ||
      lowerName.find("fence")    != std::string::npos
    );

    if (!isBlacklisted && (
        // Foliage: einfaches Name-Matching – alle Bäume, Pflanzen etc.
        lowerName.find("tree")      != std::string::npos ||
        lowerName.find("grass")     != std::string::npos ||
        lowerName.find("wheat")     != std::string::npos ||
        lowerName.find("flower")    != std::string::npos ||
        lowerName.find("flowers")   != std::string::npos ||
        lowerName.find("fern")      != std::string::npos ||
        lowerName.find("spice")     != std::string::npos ||
        lowerName.find("plant")     != std::string::npos ||
        lowerName.find("plants")    != std::string::npos ||
        lowerName.find("tentacle")  != std::string::npos ||
        lowerName.find("sprout")    != std::string::npos ||
        // Bekannte Baumarten per Name
        lowerName.find("willow")    != std::string::npos ||
        lowerName.find("pine")      != std::string::npos ||
        lowerName.find("poplar")    != std::string::npos ||
        lowerName.find("birch")     != std::string::npos ||
        lowerName.find("oak")       != std::string::npos ||
        lowerName.find("palm")      != std::string::npos ||
        lowerName.find("bamboo")    != std::string::npos ||
        lowerName.find("bush")      != std::string::npos ||
        lowerName.find("shrub")     != std::string::npos ||
        lowerName.find("reed")      != std::string::npos ||
        lowerName.find("vine")      != std::string::npos ||
        lowerName.find("mushroom")  != std::string::npos ||
        lowerName.find("crop")      != std::string::npos ||
        // ID-Whitelist bekannter Tibia-Bäume (willow, pine, poplar, birch)
        item->getID() == 2700 || item->getID() == 2701 || item->getID() == 2702 ||
        item->getID() == 2703 || item->getID() == 2704 || item->getID() == 2705 ||
        item->getID() == 2706 || item->getID() == 2707 || item->getID() == 2708 ||
        item->getID() == 2709 || item->getID() == 2710 || item->getID() == 2711 ||
        item->getID() == 2712 || item->getID() == 2713 || item->getID() == 2714 ||
        item->getID() == 2715 || item->getID() == 2716 || item->getID() == 2717 ||
        item->getID() == 2718 || item->getID() == 2719 || item->getID() == 2720 ||
        item->getID() == 8313 || item->getID() == 8314 || item->getID() == 8315 ||
        item->getID() == 8316 || item->getID() == 8317 || item->getID() == 20178)) {
      g_vbo_current_shader_flag = 4.0f; // Foliage – Wind Sway
    } else if (it.sprite && it.sprite->animator) {
      g_vbo_current_shader_flag = 2.0f; // Generisches Animations-Flag
    }
  } else if (it.sprite && it.sprite->animator) {
    g_vbo_current_shader_flag = 2.0f; // Generisches Animations-Flag
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
  uint16_t cur_id = item->getID();
  if (zoom < 1.95f) {
    if (cur_id >= 598 && cur_id <= 601) {
      long time_val = g_gui.gfx.getElapsedTime();
      uint16_t anim_id = 598 + ((time_val / 280) % 4);
      if (g_items[anim_id].sprite) spr = g_items[anim_id].sprite;
    } else if (cur_id >= 4608 && cur_id <= 4615) {
      long time_val = g_gui.gfx.getElapsedTime();
      uint16_t anim_id = 4608 + ((time_val / 250) % 8);
      if (g_items[anim_id].sprite) spr = g_items[anim_id].sprite;
    } else if (cur_id >= 4616 && cur_id <= 4625) {
      long time_val = g_gui.gfx.getElapsedTime();
      uint16_t anim_id = 4616 + ((time_val / 250) % 10);
      if (g_items[anim_id].sprite) spr = g_items[anim_id].sprite;
    }
  }

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
  if (zoom < 1.95f) {
    if (spr->animator) {
      frame = spr->animator->getFrame();
    } else if (spr->frames > 1) {
      long time_val = g_gui.gfx.getElapsedTime();
      frame = (time_val / 200) % spr->frames;
    }
  } else {
    frame = 0;
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

  // zoomed out very far, avoid drawing stuff barely visible
  if (zoom > 1.5) {
    return;
  }

  if (it.isPodium()) {
    Outfit outfit = podium->getOutfit();
    if (!podium->hasShowPlatform() && !options.ingame) {
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

  if (!options.ingame && !ephemeral && item->isSelected()) {
    glDisable(GL_TEXTURE_2D);
    glLineWidth(1.5f);
    glColor4ub(255, 0, 0, 255);
    glBegin(GL_LINE_LOOP);
    int bx1 = screenx - (spr->width - 1) * TileSize;
    int by1 = screeny - (spr->height - 1) * TileSize;
    int bx2 = screenx + TileSize;
    int by2 = screeny + TileSize;
    glVertex2i(bx1, by1);
    glVertex2i(bx2, by1);
    glVertex2i(bx2, by2);
    glVertex2i(bx1, by2);
    glEnd();
    glEnable(GL_TEXTURE_2D);
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
  if (!options.ingame && c->isSelected()) {
    glDisable(GL_TEXTURE_2D);
    glLineWidth(1.5f);
    glColor4ub(255, 0, 0, 255);
    glBegin(GL_LINE_LOOP);
    glVertex2i(screenx, screeny);
    glVertex2i(screenx + TileSize, screeny);
    glVertex2i(screenx + TileSize, screeny + TileSize);
    glVertex2i(screenx, screeny + TileSize);
    glEnd();
    glEnable(GL_TEXTURE_2D);
  }
}

void MapDrawer::BlitSquare(int sx, int sy, int red, int green, int blue,
                           int alpha, int size) {
  if (size == 0) {
    size = TileSize;
  }

  GameSprite *spr = g_items[SPRITE_ZONE].sprite;
  if (!spr) {
    glDisable(GL_TEXTURE_2D);
    glBlitSquare(sx, sy, red, green, blue, alpha, size);
    glEnable(GL_TEXTURE_2D);
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
  Container *container = dynamic_cast<Container *>(item);
  size_t itemCount = container ? container->getItemCount() : 0;

  if (unique == 0 && action == 0 && doorId == 0 && text.empty() && !tp && itemCount == 0) {
    return;
  }

  if (stream.tellp() > 0) {
    stream << "\n";
  }

  stream << "id: " << id;
  if (!item->getName().empty()) {
    stream << " (" << item->getName() << ")";
  }
  stream << "\n";

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
  if (container && itemCount > 0) {
    stream << "contains: " << itemCount << " item(s)\n";
    for (size_t i = 0; i < itemCount && i < 10; ++i) {
      if (Item *sub = container->getItem(i)) {
        stream << " - " << sub->getName();
        if (sub->isStackable() && sub->getSubtype() > 1) {
          stream << " (" << static_cast<int>(sub->getSubtype()) << "x)";
        }
        if (sub->getActionID() > 0) {
          stream << " [aid:" << sub->getActionID() << "]";
        }
        if (sub->getUniqueID() > 0) {
          stream << " [uid:" << sub->getUniqueID() << "]";
        }
        stream << "\n";
      }
    }
    if (itemCount > 10) {
      stream << " ... +" << (itemCount - 10) << " more\n";
    }
  }
}

void MapDrawer::WriteTooltip(Waypoint *waypoint, std::ostringstream &stream) {
  if (stream.tellp() > 0) {
    stream << "\n";
  }
  stream << "wp: " << waypoint->name << "\n";
}

void MapDrawer::DrawTile(TileLocation *location, Floor *f) {
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
    bool showspecial = options.show_only_colors || options.show_special_tiles || options.always_show_zones;

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

    if (options.show_houses && tile->isHouseTile()) {
      if ((int)tile->getHouseID() == current_house_id) {
        r = uint8_t(r * 0.7f);
        g = uint8_t(g * 0.7f);
        b = uint8_t(std::min(255, int(b * 1.3f)));
      } else {
        r = uint8_t(r * 0.75f);
        g = uint8_t(g * 0.75f);
        b = uint8_t(std::min(255, int(b * 1.2f)));
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
      tile->ground->animate();
      if (tile->ground->getID() != 0) {
        ItemType& type = g_items[tile->ground->getID()];
        uint16_t gid = tile->ground->getID();
        bool is_anim = (gid >= 598 && gid <= 601) || (gid >= 4608 && gid <= 4625) ||
                       (gid >= 4644 && gid <= 4678) || (gid >= 4808 && gid <= 4819) ||
                       (type.sprite && (type.sprite->animator || type.sprite->frames > 1));
        if (is_anim && f) {
          f->has_animations = true;
        }
      }

      if (f && tile->ground->hasLight()) {
        SpriteLight sl = tile->ground->getLight();
        f->lights.push_back({map_x, map_y, map_z, sl.intensity, sl.color});
      }

      BlitItem(draw_x, draw_y, tile, tile->ground, false, r, g, b);
    }

    if (options.show_houses && tile->isHouseTile()) {
      if ((int)tile->getHouseID() == current_house_id) {
        // Selected house: bright blue overlay
        BlitSquare(draw_x, draw_y, 0, 90, 220, 90);
      } else {
        // Non-selected house: subtle dark blue overlay
        BlitSquare(draw_x, draw_y, 0, 35, 120, 80);
      }
    }
    
    if (options.always_show_zones && (r != 255 || g != 255 || b != 255)) {
      DrawRawBrush(draw_x, draw_y, &g_items[SPRITE_ZONE], r, g, b, 60);
    }
  }

  if (options.show_tooltips && map_z == floor && tile->ground) {
    WriteTooltip(tile->ground, tooltip);
  }
  // end filters for ground tile

  if (!only_colors) {
    // items on tile
    for (ItemVector::iterator it = tile->items.begin();
         it != tile->items.end(); it++) {
      // item tooltip
      if (options.show_tooltips && map_z == floor) {
        WriteTooltip(*it, tooltip, tile->isHouseTile());
      }

      // item animation
      (*it)->animate();
      if ((*it)->getID() != 0) {
        ItemType& type = g_items[(*it)->getID()];
        uint16_t iid = (*it)->getID();
        bool is_anim = (iid >= 598 && iid <= 601) || (iid >= 4608 && iid <= 4625) ||
                       (iid >= 4644 && iid <= 4678) || (iid >= 4808 && iid <= 4819) ||
                       (type.sprite && (type.sprite->animator || type.sprite->frames > 1)) ||
                       (*it)->isBorder();
        if (is_anim && f) f->has_animations = true;
      }

      if (f && (*it)->hasLight()) {
        SpriteLight sl = (*it)->getLight();
        f->lights.push_back({map_x, map_y, map_z, sl.intensity, sl.color});
      }

      // item sprite
      if ((*it)->isBorder()) {
        BlitItem(draw_x, draw_y, tile, *it, false, r, g, b);
      } else {
        BlitItem(draw_x, draw_y, tile, *it, false, 255, 255, 255);
      }
    }

    // monster/npc on tile
    if (tile->creature && options.show_creatures) {
      BlitCreature(draw_x, draw_y, tile->creature);
    }

    if (zoom <= 1.5) {
      // waypoint (blue flame)
      if (!options.ingame && waypoint && options.show_waypoints) {
        BlitSpriteType(draw_x, draw_y, SPRITE_WAYPOINT, 64, 64, 255);
      }

      // house exit (blue splash)
      if (tile->isHouseExit() && options.show_houses) {
        bool is_current = tile->hasHouseExit(current_house_id);
        int r = 64, g = is_current ? 255 : 64, b = 255;
        BlitSpriteType(draw_x, draw_y, SPRITE_HOUSE_EXIT, r, g, b, 255);
      }

      // town temple (gray flag)
      if (options.show_towns && tile->isTownExit(editor.map)) {
        BlitSpriteType(draw_x, draw_y, SPRITE_TOWN_TEMPLE, 255, 255, 64, 170);
      }

      // spawn area translucent overlay (above ground, borders, and items)
      if (options.show_spawns && location->getSpawnCount() > 0) {
        int alpha = std::min(140, 50 * (int)location->getSpawnCount());
        BlitSquare(draw_x, draw_y, 180, 50, 200, alpha);
      }

      // spawn (purple flame)
      if (tile->spawn && options.show_spawns) {
        if (tile->spawn->isSelected()) {
          BlitSpriteType(draw_x, draw_y, SPRITE_SPAWN, 128, 128, 128);
          glDisable(GL_TEXTURE_2D);
          glLineWidth(1.5f);
          glColor4ub(255, 0, 0, 255);
          glBegin(GL_LINE_LOOP);
          glVertex2i(draw_x, draw_y);
          glVertex2i(draw_x + TileSize, draw_y);
          glVertex2i(draw_x + TileSize, draw_y + TileSize);
          glVertex2i(draw_x, draw_y + TileSize);
          glEnd();
          glEnable(GL_TEXTURE_2D);
        } else {
          BlitSpriteType(draw_x, draw_y, SPRITE_SPAWN, 255, 255, 255);
        }
      }

      // Corporate Design Gold Framing for selected tiles & items
      if (!options.ingame && (tile->isSelected() || (tile->ground && tile->ground->isSelected()))) {
        glDisable(GL_TEXTURE_2D);
        // Semi-transparent golden background highlight
        glColor4ub(255, 215, 0, 45);
        glBegin(GL_QUADS);
        glVertex2f(draw_x, draw_y);
        glVertex2f(draw_x + TileSize, draw_y);
        glVertex2f(draw_x + TileSize, draw_y + TileSize);
        glVertex2f(draw_x, draw_y + TileSize);
        glEnd();

        // Outer Corporate Gold Border (#FFD700)
        glLineWidth(2.0f);
        glColor4ub(255, 215, 0, 230);
        glBegin(GL_LINE_LOOP);
        glVertex2f(draw_x, draw_y);
        glVertex2f(draw_x + TileSize, draw_y);
        glVertex2f(draw_x + TileSize, draw_y + TileSize);
        glVertex2f(draw_x, draw_y + TileSize);
        glEnd();

        // Inner Warm Accent Gold Line (#D4AF37)
        glLineWidth(1.0f);
        glColor4ub(212, 175, 55, 170);
        glBegin(GL_LINE_LOOP);
        glVertex2f(draw_x + 1.0f, draw_y + 1.0f);
        glVertex2f(draw_x + TileSize - 1.0f, draw_y + 1.0f);
        glVertex2f(draw_x + TileSize - 1.0f, draw_y + TileSize - 1.0f);
        glVertex2f(draw_x + 1.0f, draw_y + TileSize - 1.0f);
        glEnd();
        glEnable(GL_TEXTURE_2D);
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
  // draw in-game light with soft shadows, raycasting and dynamic flickering
  light_drawer->draw(start_x, start_y, end_x, end_y, view_scroll_x,
                     view_scroll_y, &editor.map, floor, g_shader_time);
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

  bool hidden = options.hide_items_when_zoomed && zoom > 1.5f;
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
