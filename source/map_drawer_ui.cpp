#include "main.h"
#include "brush.h"
#include "creature_brush.h"
#include "editor.h"
#include "gui.h"
#include "live_client.h"
#include "live_peer.h"
#include "live_server.h"
#include "live_socket.h"
#include "map_display.h"
#include "map_drawer.h"
#include "raw_brush.h"
#include "settings.h"
#include "style_manager.h"
#include <GL/glut.h>
#include <imgui.h>
#include <nanovg.h>


static int getFloorAdj(int floor) {
  return (floor > GROUND_LAYER ? 0 : (GROUND_LAYER - floor));
}

void MapDrawer::DrawSelectionBox() {
  if (options.ingame || !canvas)
    return;
  nvgBeginFrame(m_nvg, screensize_x, screensize_y, 1.0f);
  float lx = (float)(canvas->last_click_abs_x - view_scroll_x) / (float)zoom;
  float ly = (float)(canvas->last_click_abs_y - view_scroll_y) / (float)zoom;
  nvgBeginPath(m_nvg);
  nvgRect(m_nvg, lx, ly, canvas->cursor_x - lx, canvas->cursor_y - ly);
  nvgStrokeColor(m_nvg,
                 nvgRGBA(218, 165, 32, 200)); // Goldener Selektionsrahmen
  nvgStrokeWidth(m_nvg, 1.0f / zoom);
  nvgStroke(m_nvg);
  nvgEndFrame(m_nvg);
}

void MapDrawer::DrawLiveCursors() {
  if (options.ingame || !editor.IsLive())
    return;

  LiveSocket &live = editor.GetLive();
  for (LiveCursor &cursor : live.getCursorList()) {
    if (cursor.pos.z <= GROUND_LAYER && floor > GROUND_LAYER)
      continue;
    if (cursor.pos.z > GROUND_LAYER && floor <= 8)
      continue;

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

    // Player name label
    if (ImGui::GetCurrentContext()) {
      ImDrawList *draw_list = ImGui::GetForegroundDrawList();
      ImU32 col = IM_COL32(cursor.color.Red(), cursor.color.Green(),
                           cursor.color.Blue(), 255);
      ImU32 shadow_col = IM_COL32(0, 0, 0, 255);

      std::string label = "Mapper #" + std::to_string(cursor.id);
      if (editor.IsLiveServer()) {
        LiveServer *server = editor.GetLiveServer();
        auto it = server->clients.find(cursor.id);
        if (it != server->clients.end()) {
          label = it->second->getName().ToStdString();
        } else if (cursor.id == 0) {
          label = server->getName().ToStdString(); // Host
        }
      } else if (editor.IsLiveClient()) {
        if (cursor.id == 0)
          label = "Host";
      }

      ImVec2 pos(draw_x, draw_y - 15.0f);
      draw_list->AddText(ImVec2(pos.x + 1.0f, pos.y + 1.0f), shadow_col,
                         label.c_str());
      draw_list->AddText(pos, col, label.c_str());
    }
  }
}

void MapDrawer::DrawGrid() {
  glDisable(GL_TEXTURE_2D);
  for (int y = start_y; y < end_y; ++y) {
    glColor4ub(0, 0, 0, 128);
    glBegin(GL_LINES);
    glVertex2f(start_x * TileSize - view_scroll_x,
               y * TileSize - view_scroll_y);
    glVertex2f(end_x * TileSize - view_scroll_x, y * TileSize - view_scroll_y);
    glEnd();
  }
  for (int x = start_x; x < end_x; ++x) {
    glBegin(GL_LINES);
    glVertex2f(x * TileSize - view_scroll_x,
               start_y * TileSize - view_scroll_y);
    glVertex2f(x * TileSize - view_scroll_x, end_y * TileSize - view_scroll_y);
    glEnd();
  }
  glEnable(GL_TEXTURE_2D);
}

void MapDrawer::DrawIngameBox() {
  int center_x = start_x + int(screensize_x * zoom / 64);
  int center_y = start_y + int(screensize_y * zoom / 64);
  int box_start_x = center_x * TileSize - view_scroll_x;
  int box_start_y = (center_y + 2) * TileSize - view_scroll_y;

  static wxColor side_color = wxColor(10, 15, 25);
  glDisable(GL_TEXTURE_2D);
  // Zeichne abgedunkelte Ränder außerhalb des Ingame-Sichtfelds
  drawFilledRect(0, 0, box_start_x, screensize_y * zoom, side_color);
  drawRect(box_start_x, box_start_y, ClientMapWidth * TileSize,
           ClientMapHeight * TileSize, *wxRED);
  glEnable(GL_TEXTURE_2D);
}

void MapDrawer::DrawBrushIndicator(int x, int y, Brush *brush, uint8_t r,
                                   uint8_t g, uint8_t b) {
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glColor4ub(r, g, b, 80); // Leicht schimmernd (Alpha 80)
  
  glBegin(GL_QUADS);
  glVertex2f(x, y);
  glVertex2f(x + TileSize, y);
  glVertex2f(x + TileSize, y + TileSize);
  glVertex2f(x, y + TileSize);
  glEnd();

  // Outline
  glColor4ub(r, g, b, 180);
  glBegin(GL_LINE_LOOP);
  glVertex2f(x, y);
  glVertex2f(x + TileSize, y);
  glVertex2f(x + TileSize, y + TileSize);
  glVertex2f(x, y + TileSize);
  glEnd();
  
  glEnable(GL_TEXTURE_2D);
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
  if (brush->canDraw(&editor.map, pos))
    glColor(COLOR_VALID);
  else
    glColor(COLOR_INVALID);
}

void MapDrawer::drawRect(int x, int y, int w, int h, const wxColor &color,
                         int width) {
  glLineWidth(width);
  glColor4ub(color.Red(), color.Green(), color.Blue(), color.Alpha());
  glBegin(GL_LINE_STRIP);
  glVertex2f(x, y);
  glVertex2f(x + w, y);
  glVertex2f(x + w, y + h);
  glVertex2f(x, y + h);
  glVertex2f(x, y);
  glEnd();
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
  if (size == 0)
    size = TileSize;
  glColor4ub(uint8_t(red), uint8_t(green), uint8_t(blue), uint8_t(alpha));
  glBegin(GL_QUADS);
  glVertex2f(sx, sy);
  glVertex2f(sx + size, sy);
  glVertex2f(sx + size, sy + size);
  glVertex2f(sx, sy + size);
  glEnd();
}

void MapDrawer::DrawTooltips() {
  if (!m_nvg)
    return;
  nvgBeginFrame(m_nvg, screensize_x, screensize_y, 1.0f);
  for (auto *tooltip : tooltips) {
    // Tooltip Rendering...
  }
  nvgEndFrame(m_nvg);
}
