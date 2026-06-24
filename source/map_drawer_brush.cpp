#include "main.h"
#include "carpet_brush.h"
#include "creature_brush.h"
#include "doodad_brush.h"
#include "editor.h"
#include "gui.h"
#include "house_brush.h"
#include "house_exit_brush.h"
#include "map_display.h"
#include "map_drawer.h"
#include "raw_brush.h"
#include "settings.h"
#include "spawn_brush.h"
#include "table_brush.h"
#include "wall_brush.h"
#include "waypoint_brush.h"

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

void MapDrawer::DrawBrush() {
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
    return;
  }

  Brush *brush = g_gui.GetCurrentBrush();

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

  if (dragging_draw) {
    ASSERT(brush->canDrag());

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
    } else {
      if (brush->isRaw())
        glEnable(GL_TEXTURE_2D);

      if (g_gui.GetBrushShape() == BRUSHSHAPE_SQUARE || brush->isSpawn()) {
        if (brush->isRaw() || brush->isOptionalBorder()) {
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

          RAWBrush *raw_brush = brush->isRaw() ? brush->asRaw() : nullptr;

          for (int y = start_y; y <= end_y; y++) {
            int cy = y * TileSize - view_scroll_y - getFloorAdjustment(floor);
            for (int x = start_x; x <= end_x; x++) {
              int cx = x * TileSize - view_scroll_x - getFloorAdjustment(floor);
              if (brush->isOptionalBorder()) {
                glColorCheck(brush, Position(x, y, floor));
              } else {
                DrawRawBrush(cx, cy, raw_brush->getItemType(), 160, 160, 160,
                             160);
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

          glColor(brushColor);
          glBegin(GL_QUADS);
          glVertex2f(last_click_start_sx, last_click_start_sy);
          glVertex2f(last_click_end_sx, last_click_start_sy);
          glVertex2f(last_click_end_sx, last_click_end_sy);
          glVertex2f(last_click_start_sx, last_click_end_sy);
          glEnd();
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

        RAWBrush *raw_brush = brush->isRaw() ? brush->asRaw() : nullptr;

        for (int y = start_y - 1; y <= end_y + 1; y++) {
          int cy = y * TileSize - view_scroll_y - getFloorAdjustment(floor);
          float dy = center_y - y;
          for (int x = start_x - 1; x <= end_x + 1; x++) {
            int cx = x * TileSize - view_scroll_x - getFloorAdjustment(floor);
            float dx = center_x - x;
            float distance = sqrt(dx * dx + dy * dy);
            if (distance < radii) {
              if (brush->isRaw()) {
                DrawRawBrush(cx, cy, raw_brush->getItemType(), 160, 160, 160,
                             160);
              } else {
                glColor(brushColor);
                glBegin(GL_QUADS);
                glVertex2f(cx, cy + TileSize);
                glVertex2f(cx + TileSize, cy + TileSize);
                glVertex2f(cx + TileSize, cy);
                glVertex2f(cx, cy);
                glEnd();
              }
            }
          }
        }
      }
      if (brush->isRaw())
        glDisable(GL_TEXTURE_2D);
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
    } else if (brush->isDoor()) {
      int cx =
          mouse_map_x * TileSize - view_scroll_x - getFloorAdjustment(floor);
      int cy =
          mouse_map_y * TileSize - view_scroll_y - getFloorAdjustment(floor);
      glColorCheck(brush, Position(mouse_map_x, mouse_map_y, floor));
      glBegin(GL_QUADS);
      glVertex2f(cx, cy + TileSize);
      glVertex2f(cx + TileSize, cy + TileSize);
      glVertex2f(cx + TileSize, cy);
      glVertex2f(cx, cy);
      glEnd();
    } else if (brush->isCreature()) {
      glEnable(GL_TEXTURE_2D);
      int cy =
          mouse_map_y * TileSize - view_scroll_y - getFloorAdjustment(floor);
      int cx =
          mouse_map_x * TileSize - view_scroll_x - getFloorAdjustment(floor);
      CreatureBrush *creature_brush = brush->asCreature();
      if (creature_brush->canDraw(&editor.map,
                                  Position(mouse_map_x, mouse_map_y, floor))) {
        BlitCreature(cx, cy, creature_brush->getType()->outfit, (Direction)2,
                     255, 255, 255, 160);
      } else {
        BlitCreature(cx, cy, creature_brush->getType()->outfit, (Direction)2,
                     255, 64, 64, 160);
      }
      glDisable(GL_TEXTURE_2D);
    } else if (!brush->isDoodad()) {
      RAWBrush *raw_brush = brush->isRaw() ? brush->asRaw() : nullptr;
      if (brush->isRaw())
        glEnable(GL_TEXTURE_2D);

      for (int y = -g_gui.GetBrushSize() - 1; y <= g_gui.GetBrushSize() + 1;
           y++) {
        int cy = (mouse_map_y + y) * TileSize - view_scroll_y -
                 getFloorAdjustment(floor);
        for (int x = -g_gui.GetBrushSize() - 1; x <= g_gui.GetBrushSize() + 1;
             x++) {
          int cx = (mouse_map_x + x) * TileSize - view_scroll_x -
                   getFloorAdjustment(floor);
          bool inside = false;
          if (g_gui.GetBrushShape() == BRUSHSHAPE_SQUARE) {
            inside = (x >= -g_gui.GetBrushSize() && x <= g_gui.GetBrushSize() &&
                      y >= -g_gui.GetBrushSize() && y <= g_gui.GetBrushSize());
          } else if (g_gui.GetBrushShape() == BRUSHSHAPE_CIRCLE) {
            inside = (sqrt(double(x * x) + double(y * y)) <
                      g_gui.GetBrushSize() + 0.005);
          }
          if (inside) {
            if (brush->isRaw()) {
              DrawRawBrush(cx, cy, raw_brush->getItemType(), 160, 160, 160,
                           160);
            } else {
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
            }
          }
        }
      }
      if (brush->isRaw())
        glDisable(GL_TEXTURE_2D);
    }
  }
}

void MapDrawer::DrawRawBrush(int screenx, int screeny, ItemType *itemType,
                             uint8_t r, uint8_t g, uint8_t b, uint8_t alpha) {
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
  BlitSpriteType(screenx, screeny, spr, r, g, b, alpha);
}

void MapDrawer::getColor(Brush *brush, const Position &position, uint8_t &r,
                         uint8_t &g, uint8_t &b) {
  if (brush->canDraw(&editor.map, position)) {
    if (brush->isWaypoint()) {
      r = 0x00;
      g = 0xff;
      b = 0x00;
    } else {
      r = 0x00;
      g = 0x00;
      b = 0xff;
    }
  } else {
    r = 0xff;
    g = 0x00;
    b = 0x00;
  }
}