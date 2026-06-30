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

#include "core_forward.h"
#include <sstream>
#include <time.h>
#include <wx/wfstream.h>

#include "application.h"
#include "browse_tile_window.h"
#include "brush.h"
#include "editor.h"
#include "graphics.h"
#include "gui.h"
#include "live_peer.h"
#include "live_server.h"
#include "live_socket.h"
#include "map.h"
#include "map_display.h"
#include "map_drawer.h"
#include "old_properties_window.h"
#include "palette_window.h"
#include "properties_window.h"
#include "sprites.h"
#include "svg_icons.h"
#include "tile.h"
#include "tileset_window.h"


#include "performance_logger.h"
#include <imgui.h>
#include <imgui_impl_opengl3.h>

#include "carpet_brush.h"
#include "creature_brush.h"
#include "doodad_brush.h"
#include "ground_brush.h"
#include "house_brush.h"
#include "house_exit_brush.h"
#include "lua/lua_script.h"
#include "lua/lua_script_manager.h"
#include "raw_brush.h"
#include "spawn_brush.h"
#include "table_brush.h"
#include "wall_brush.h"
#include "waypoint_brush.h"

BEGIN_EVENT_TABLE(MapCanvas, wxGLCanvas)
EVT_KEY_DOWN(MapCanvas::OnKeyDown)
EVT_KEY_DOWN(MapCanvas::OnKeyUp)

// Mouse events
EVT_MOTION(MapCanvas::OnMouseMove)
EVT_LEFT_UP(MapCanvas::OnMouseLeftRelease)
EVT_LEFT_DOWN(MapCanvas::OnMouseLeftClick)
EVT_LEFT_DCLICK(MapCanvas::OnMouseLeftDoubleClick)
EVT_MIDDLE_DOWN(MapCanvas::OnMouseCenterClick)
EVT_MIDDLE_UP(MapCanvas::OnMouseCenterRelease)
EVT_RIGHT_DOWN(MapCanvas::OnMouseRightClick)
EVT_RIGHT_UP(MapCanvas::OnMouseRightRelease)
EVT_MOUSEWHEEL(MapCanvas::OnWheel)
EVT_ENTER_WINDOW(MapCanvas::OnGainMouse)
EVT_LEAVE_WINDOW(MapCanvas::OnLoseMouse)

// Drawing events
EVT_PAINT(MapCanvas::OnPaint)
EVT_ERASE_BACKGROUND(MapCanvas::OnEraseBackground)

// Menu events
EVT_MENU(MAP_POPUP_MENU_CUT, MapCanvas::OnCut)
EVT_MENU(MAP_POPUP_MENU_COPY, MapCanvas::OnCopy)
EVT_MENU(MAP_POPUP_MENU_COPY_POSITION, MapCanvas::OnCopyPosition)
EVT_MENU(MAP_POPUP_MENU_PASTE, MapCanvas::OnPaste)
EVT_MENU(MAP_POPUP_MENU_DELETE, MapCanvas::OnDelete)
//----
EVT_MENU(MAP_POPUP_MENU_COPY_SERVER_ID, MapCanvas::OnCopyServerId)
EVT_MENU(MAP_POPUP_MENU_COPY_CLIENT_ID, MapCanvas::OnCopyClientId)
EVT_MENU(MAP_POPUP_MENU_COPY_NAME, MapCanvas::OnCopyName)
// ----
EVT_MENU(MAP_POPUP_MENU_ROTATE, MapCanvas::OnRotateItem)
EVT_MENU(MAP_POPUP_MENU_GOTO, MapCanvas::OnGotoDestination)
EVT_MENU(MAP_POPUP_MENU_SWITCH_DOOR, MapCanvas::OnSwitchDoor)
// ----
EVT_MENU(MAP_POPUP_MENU_SELECT_RAW_BRUSH, MapCanvas::OnSelectRAWBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_GROUND_BRUSH, MapCanvas::OnSelectGroundBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_DOODAD_BRUSH, MapCanvas::OnSelectDoodadBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_COLLECTION_BRUSH,
         MapCanvas::OnSelectCollectionBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_DOOR_BRUSH, MapCanvas::OnSelectDoorBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_WALL_BRUSH, MapCanvas::OnSelectWallBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_CARPET_BRUSH, MapCanvas::OnSelectCarpetBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_TABLE_BRUSH, MapCanvas::OnSelectTableBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_CREATURE_BRUSH, MapCanvas::OnSelectCreatureBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_SPAWN_BRUSH, MapCanvas::OnSelectSpawnBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_HOUSE_BRUSH, MapCanvas::OnSelectHouseBrush)
EVT_MENU(MAP_POPUP_MENU_MOVE_TO_TILESET, MapCanvas::OnSelectMoveTo)
// ----
EVT_MENU(MAP_POPUP_MENU_PROPERTIES, MapCanvas::OnProperties)
// ----
EVT_MENU(MAP_POPUP_MENU_BROWSE_TILE, MapCanvas::OnBrowseTile)
EVT_MENU_RANGE(MAP_POPUP_MENU_SCRIPT_FIRST, MAP_POPUP_MENU_SCRIPT_LAST,
               MapCanvas::OnScriptMenu)
END_EVENT_TABLE()

#define MAPCANVAS_EVENT_STUB(name, type) \
  void MapCanvas::name(type &event) { (void)event; }

MAPCANVAS_EVENT_STUB(OnKeyDown, wxKeyEvent)
MAPCANVAS_EVENT_STUB(OnKeyUp, wxKeyEvent)

void MapCanvas::OnMouseLeftClick(wxMouseEvent& event) {
	cursor_x = event.GetX();
	cursor_y = event.GetY();

	int mouse_map_x, mouse_map_y;
	ScreenToMap(cursor_x, cursor_y, &mouse_map_x, &mouse_map_y);

	LogErrorToFile(wxString::Format("OnMouseLeftClick: screen=(%d, %d), map=(%d, %d), floor=%d, drawing=%d, current_brush=%s",
		cursor_x, cursor_y, mouse_map_x, mouse_map_y, floor, drawing,
		g_gui.GetCurrentBrush() ? g_gui.GetCurrentBrush()->getName().c_str() : "nullptr").ToStdString());

	if (event.RightIsDown()) {
		g_gui.SelectBrush(nullptr);
		return;
	}

	last_click_x = int(cursor_x * zoom);
	last_click_y = int(cursor_y * zoom);

	int start_x = 0, start_y = 0;
	if (auto* map_window = static_cast<MapWindow *>(GetParent())) {
		map_window->GetViewStart(&start_x, &start_y);
	}
	last_click_abs_x = last_click_x + start_x;
	last_click_abs_y = last_click_y + start_y;

	last_click_map_x = mouse_map_x;
	last_click_map_y = mouse_map_y;
	last_click_map_z = floor;

	if (drawing) {
		dragging_draw = true;
		rectangle_mode = event.ShiftDown();
		if (!rectangle_mode) {
			PositionVector tilestodraw;
			PositionVector tilestoborder;
			getTilesToDraw(mouse_map_x, mouse_map_y, floor, &tilestodraw, &tilestoborder, false);
			editor.draw(tilestodraw, tilestoborder, event.AltDown());
		}
	} else {
		Tile* tile = editor.map.getTile(mouse_map_x, mouse_map_y, floor);
		if (tile) {
			if (editor.selection.getTiles().count(tile) == 0 && !event.ShiftDown() && !event.ControlDown()) {
				editor.selection.clear();
				editor.selection.start(Selection::INTERNAL);
				Item* top_item = tile->getTopItem();
				if (top_item) {
					editor.selection.add(tile, top_item);
				} else if (tile->ground) {
					editor.selection.add(tile, tile->ground);
				}
				editor.selection.finish(Selection::INTERNAL);
			} else if (event.ShiftDown() || event.ControlDown()) {
				editor.selection.start(Selection::INTERNAL);
				Item* top_item = tile->getTopItem();
				if (top_item) {
					editor.selection.add(tile, top_item);
				} else if (tile->ground) {
					editor.selection.add(tile, tile->ground);
				}
				editor.selection.finish(Selection::INTERNAL);
			}
			dragging_selection = true;
			drag_start_map_x = mouse_map_x;
			drag_start_map_y = mouse_map_y;
		} else {
			editor.selection.clear();
		}
	}
	CallAfter([this]() { Refresh(); });
}

void MapCanvas::OnMouseLeftRelease(wxMouseEvent& event) {
	if (drawing && dragging_draw && rectangle_mode) {
		int cursor_x = event.GetX();
		int cursor_y = event.GetY();
		int mouse_map_x, mouse_map_y;
		ScreenToMap(cursor_x, cursor_y, &mouse_map_x, &mouse_map_y);

		if (last_click_map_x != -1 && last_click_map_y != -1) {
			int start_x = std::min(last_click_map_x, mouse_map_x);
			int end_x = std::max(last_click_map_x, mouse_map_x);
			int start_y = std::min(last_click_map_y, mouse_map_y);
			int end_y = std::max(last_click_map_y, mouse_map_y);

			bool is_wall = g_gui.GetCurrentBrush() && g_gui.GetCurrentBrush()->isWall();
			PositionVector tilestodraw;
			PositionVector tilestoborder;
			for (int y = start_y - 1; y <= end_y + 1; ++y) {
				for (int x = start_x - 1; x <= end_x + 1; ++x) {
					Position pos(x, y, floor);
					if (x >= start_x && x <= end_x && y >= start_y && y <= end_y) {
						if (!is_wall || (x == start_x || x == end_x || y == start_y || y == end_y)) {
							tilestodraw.push_back(pos);
						}
					}
					tilestoborder.push_back(pos);
				}
			}
			editor.draw(tilestodraw, tilestoborder, event.AltDown());
		}
	}
	
	if (!drawing && dragging_selection) {
		int cursor_x = event.GetX();
		int cursor_y = event.GetY();
		int mouse_map_x, mouse_map_y;
		ScreenToMap(cursor_x, cursor_y, &mouse_map_x, &mouse_map_y);
		
		int dx = mouse_map_x - drag_start_map_x;
		int dy = mouse_map_y - drag_start_map_y;
		if (dx != 0 || dy != 0) {
			editor.moveSelection(Position(dx, dy, 0));
		}
		dragging_selection = false;
	}

	dragging_draw = false;
	rectangle_mode = false;
	last_click_map_x = -1;
	last_click_map_y = -1;
	last_click_map_z = -1;
	CallAfter([this]() { Refresh(); });
}

void MapCanvas::OnMouseRightClick(wxMouseEvent& event) {
	cursor_x = event.GetX();
	cursor_y = event.GetY();

	int mouse_map_x, mouse_map_y;
	ScreenToMap(cursor_x, cursor_y, &mouse_map_x, &mouse_map_y);

	LogErrorToFile(wxString::Format("OnMouseRightClick: screen=(%d, %d), map=(%d, %d), floor=%d, drawing=%d, current_brush=%s",
		cursor_x, cursor_y, mouse_map_x, mouse_map_y, floor, drawing,
		g_gui.GetCurrentBrush() ? g_gui.GetCurrentBrush()->getName().c_str() : "nullptr").ToStdString());

	last_click_x = int(cursor_x * zoom);
	last_click_y = int(cursor_y * zoom);

	int start_x = 0, start_y = 0;
	if (auto* map_window = static_cast<MapWindow *>(GetParent())) {
		map_window->GetViewStart(&start_x, &start_y);
	}
	last_click_abs_x = last_click_x + start_x;
	last_click_abs_y = last_click_y + start_y;

	last_click_map_x = mouse_map_x;
	last_click_map_y = mouse_map_y;
	last_click_map_z = floor;

	if (drawing) {
		if (event.LeftIsDown()) {
			g_gui.SelectBrush(nullptr);
		} else if (event.ShiftDown()) {
			dragging_draw = true;
			rectangle_mode = true;
		} else if (g_gui.GetCurrentBrush() != nullptr) {
			g_gui.SelectBrush(nullptr);
		} else {
			if (editor.selection.size() == 0) {
				Tile* tile = editor.map.getTile(mouse_map_x, mouse_map_y, floor);
				if (tile) {
					editor.selection.start(Selection::INTERNAL);
					Item* top_item = tile->getTopItem();
					if (top_item) {
						editor.selection.add(tile, top_item);
					} else if (tile->ground) {
						editor.selection.add(tile, tile->ground);
					}
					editor.selection.finish(Selection::INTERNAL);
				}
			}
			popup_menu->Update();
			PopupMenu(popup_menu);
		}
	} else {
		if (editor.selection.size() == 0) {
			Tile* tile = editor.map.getTile(mouse_map_x, mouse_map_y, floor);
			if (tile) {
				editor.selection.start(Selection::INTERNAL);
				Item* top_item = tile->getTopItem();
				if (top_item) {
					editor.selection.add(tile, top_item);
				} else if (tile->ground) {
					editor.selection.add(tile, tile->ground);
				}
				editor.selection.finish(Selection::INTERNAL);
			}
		}
		popup_menu->Update();
		PopupMenu(popup_menu);
	}
	CallAfter([this]() { Refresh(); });
}

void MapCanvas::OnMouseRightRelease(wxMouseEvent& event) {
	if (drawing && dragging_draw && rectangle_mode) {
		int cursor_x = event.GetX();
		int cursor_y = event.GetY();
		int mouse_map_x, mouse_map_y;
		ScreenToMap(cursor_x, cursor_y, &mouse_map_x, &mouse_map_y);

		if (last_click_map_x != -1 && last_click_map_y != -1) {
			int start_x = std::min(last_click_map_x, mouse_map_x);
			int end_x = std::max(last_click_map_x, mouse_map_x);
			int start_y = std::min(last_click_map_y, mouse_map_y);
			int end_y = std::max(last_click_map_y, mouse_map_y);

			PositionVector tilestodraw;
			PositionVector tilestoborder;
			for (int y = start_y - 1; y <= end_y + 1; ++y) {
				for (int x = start_x - 1; x <= end_x + 1; ++x) {
					Position pos(x, y, floor);
					if (x >= start_x && x <= end_x && y >= start_y && y <= end_y) {
						tilestodraw.push_back(pos);
					}
					tilestoborder.push_back(pos);
				}
			}
			editor.undraw(tilestodraw, tilestoborder, event.AltDown());
		}
	}
	dragging_draw = false;
	rectangle_mode = false;
	last_click_map_x = -1;
	last_click_map_y = -1;
	last_click_map_z = -1;
	CallAfter([this]() { Refresh(); });
}

void MapCanvas::OnMouseMove(wxMouseEvent& event) {
	cursor_x = event.GetX();
	cursor_y = event.GetY();

	if (screendragging && event.MiddleIsDown()) {
		int dx = cursor_x - drag_start_x;
		int dy = cursor_y - drag_start_y;
		
		if (dx != 0 || dy != 0) {
			MapWindow* map_win = static_cast<MapWindow*>(GetParent());
			int scroll_x, scroll_y;
			map_win->GetViewStart(&scroll_x, &scroll_y);
			map_win->Scroll(scroll_x - dx, scroll_y - dy);
			
			// Update start pos so we can drag continuously
			drag_start_x = cursor_x;
			drag_start_y = cursor_y;
		}
	}

	int mouse_map_x, mouse_map_y;
	ScreenToMap(cursor_x, cursor_y, &mouse_map_x, &mouse_map_y);
	
	if (drawing && (mouse_map_x != last_cursor_map_x || mouse_map_y != last_cursor_map_y || floor != last_cursor_map_z)) {
		LogErrorToFile(wxString::Format("OnMouseMove (drawing): screen=(%d, %d), map=(%d, %d), floor=%d, dragging_draw=%d",
			cursor_x, cursor_y, mouse_map_x, mouse_map_y, floor, dragging_draw).ToStdString());
	}

	if (mouse_map_x != last_cursor_map_x || mouse_map_y != last_cursor_map_y || floor != last_cursor_map_z) {
		int prev_x = last_cursor_map_x;
		int prev_y = last_cursor_map_y;
		last_cursor_map_x = mouse_map_x;
		last_cursor_map_y = mouse_map_y;
		last_cursor_map_z = floor;
		UpdatePositionStatus(mouse_map_x, mouse_map_y);

		if (drawing && dragging_draw && !rectangle_mode) {
			PositionVector tilestodraw;
			PositionVector tilestoborder;

			int x0 = (prev_x != -1) ? prev_x : mouse_map_x;
			int y0 = (prev_y != -1) ? prev_y : mouse_map_y;
			int x1 = mouse_map_x;
			int y1 = mouse_map_y;

			int dx = std::abs(x1 - x0);
			int dy = std::abs(y1 - y0);
			int sx = (x0 < x1) ? 1 : -1;
			int sy = (y0 < y1) ? 1 : -1;
			int err = dx - dy;

			while (true) {
				getTilesToDraw(x0, y0, floor, &tilestodraw, &tilestoborder, false);
				if (x0 == x1 && y0 == y1) break;
				int e2 = 2 * err;
				if (e2 > -dy) { err -= dy; x0 += sx; }
				if (e2 < dx) { err += dx; y0 += sy; }
			}
			
			if (event.LeftIsDown()) {
				editor.draw(tilestodraw, tilestoborder, event.AltDown());
			} else if (event.RightIsDown()) {
				editor.undraw(tilestodraw, tilestoborder, event.AltDown());
			}
		}
	}
	Refresh();
}

MAPCANVAS_EVENT_STUB(OnMouseLeftDoubleClick, wxMouseEvent)

void MapCanvas::OnMouseCenterClick(wxMouseEvent& event) {
	if (!screendragging) {
		screendragging = true;
		drag_start_x = event.GetX();
		drag_start_y = event.GetY();
		SetCursor(wxCursor(wxCURSOR_HAND));
	}
}

void MapCanvas::OnMouseCenterRelease(wxMouseEvent& event) {
	if (screendragging) {
		screendragging = false;
		SetCursor(wxNullCursor);
	}
}
void MapCanvas::OnWheel(wxMouseEvent& event) {
  if (event.GetWheelRotation() > 0) {
    SetZoom(zoom * 0.9);
  } else if (event.GetWheelRotation() < 0) {
    SetZoom(zoom * 1.1);
  }
}
MAPCANVAS_EVENT_STUB(OnGainMouse, wxMouseEvent)
MAPCANVAS_EVENT_STUB(OnLoseMouse, wxMouseEvent)

void MapCanvas::ChangeFloor(int new_floor) {
  floor = new_floor;
  Refresh();
}

void MapCanvas::EnterSelectionMode() {
  drawing = false;
  Refresh();
}

void MapCanvas::EnterDrawingMode() {
  drawing = true;
  Refresh();
}

bool MapCanvas::isPasting() const { return false; }

#undef MAPCANVAS_EVENT_STUB

bool MapCanvas::processed[MapCanvas::BLOCK_SIZE * MapCanvas::BLOCK_SIZE] = {
    0};                           // Correctly size the static array
int MapCanvas::countMaxFills = 0; // Definition for static member
MapCanvas::MapCanvas(wxWindow *parent, MapEditor &editor_ref, int *attriblist)
    : wxGLCanvas(parent, wxID_ANY, attriblist, wxDefaultPosition, wxDefaultSize,
                 wxWANTS_CHARS),
      editor(editor_ref), floor(GROUND_LAYER), zoom(1.0), cursor_x(-1),
      cursor_y(-1), dragging(false), boundbox_selection(false), dragging_selection(false), drag_start_map_x(-1), drag_start_map_y(-1),
      screendragging(false), drawing(false), dragging_draw(false),
      replace_dragging(false), rectangle_mode(false),

      screenshot_buffer(nullptr),

      drag_start_x(-1), drag_start_y(-1), drag_start_z(-1),

      last_cursor_map_x(-1), last_cursor_map_y(-1), last_cursor_map_z(-1),

      last_click_map_x(-1), last_click_map_y(-1), last_click_map_z(-1),
      last_click_abs_x(-1), last_click_abs_y(-1), last_click_x(-1),
      last_click_y(-1),

      last_mmb_click_x(-1), last_mmb_click_y(-1) {
  popup_menu = newd MapPopupMenu(editor);
  animation_timer = newd AnimationTimer(this);
  // NOTE: Do NOT start the animation timer here. The timer fires every 4ms
  // and calls Refresh() -> OnPaint(). If it fires before drawer is constructed
  // (line below), drawer is still nullptr -> crash. Timer is started after
  // full initialization at the bottom of this constructor.

  // SetCurrent requires a realized (shown) canvas. Guard against calling it
  // on an unrealized canvas which can crash on some wxWidgets/Windows builds.
  if (IsShownOnScreen()) {
    SetCurrent(*g_gui.GetGLContext(this));
  }
  drawer = std::make_unique<MapDrawer>(this); // Use unique_ptr
  keyCode = WXK_NONE;

  // Initialisierung der Toolbar mit gespeicherten Werten aus der Config
  int tx = g_settings.getInteger(Config::UI_TOOLBAR_X);
  int ty = g_settings.getInteger(Config::UI_TOOLBAR_Y);
  if (tx <= 0)
    tx = 10;
  if (ty <= 0)
    ty = 10;
  // ui_toolbar wird jetzt in der Member-Initialisierungsliste initialisiert
  ui_toolbar =
      std::make_unique<RME::UI::UIToolbar>((float)tx, (float)ty, 45, 300);
  ui_toolbar->addButton("Selection", RME::UI::SVG::ICON_SELECT,
                        [this]() { g_gui.SetSelectionMode(); });
  ui_toolbar->addButton("Pencil", RME::UI::SVG::ICON_PENCIL,
                        [this]() { g_gui.SetDrawingMode(); });
  ui_toolbar->addButton("Eraser", RME::UI::SVG::ICON_ERASER,
                        [this]() { g_gui.SelectBrush(g_gui.eraser); });
  ui_toolbar->addButton("Waypoints", RME::UI::SVG::ICON_WAYPOINT, [this]() {
    g_gui.SelectPalettePage(TILESET_WAYPOINT);
  });

  // Start animation timer AFTER drawer is fully initialized so that the
  // first Notify() -> Refresh() -> OnPaint() never sees a null drawer.
  animation_timer->Start();
}

MapCanvas::~MapCanvas() {
  delete popup_menu;
  delete animation_timer;
  // drawer is now unique_ptr, no manual delete needed
  free(screenshot_buffer); // unique_ptr verwaltet dies jetzt
  if (minimap_tex_id != 0) {
    glDeleteTextures(1, &minimap_tex_id);
  }
}

const float *
MapCanvas::GetProjectionMatrix() const {   // unique_ptr verwaltet dies jetzt
  return drawer->GetCameraProjectionPtr(); // Access through unique_ptr
}

RME::UI::UIToolbar *MapCanvas::GetUIToolbar() { return ui_toolbar.get(); }

// Virtual implementation (base assumes parent is MapWindow)
void MapCanvas::SetZoom(double value) {
  if (value < 0.5) {
    value = 0.5;
  }

  if (value > 5.0) {
    value = 5.0;
  }

  if (zoom != value) {
    int center_x, center_y;
    GetScreenCenter(&center_x, &center_y);

    zoom = value;

    // Unsafe cast if parent isn't MapWindow, but this is the base
    // implementation
    if (GetParent()) {
      static_cast<MapWindow *>(GetParent())
          ->SetScreenCenterPosition(Position(center_x, center_y, floor));
    }

    UpdatePositionStatus();
    UpdateZoomStatus();
    Refresh();
  }
}

void MapCanvas::GetViewBox(int *view_scroll_x, int *view_scroll_y,
                           int *screensize_x, int *screensize_y) const {
  static_cast<MapWindow *>(GetParent())
      ->GetViewSize(screensize_x,
                    screensize_y); // unique_ptr verwaltet dies jetzt
  static_cast<MapWindow *>(GetParent())
      ->GetViewStart(view_scroll_x, view_scroll_y);
}

void MapCanvas::ScreenToMap(int screen_x, int screen_y, int *map_x,
                            int *map_y) {
  int start_x, start_y;

  // Base implementation calls MapWindow parent
  static_cast<MapWindow *>(GetParent())->GetViewStart(&start_x, &start_y);

  screen_x *= GetContentScaleFactor();
  screen_y *= GetContentScaleFactor();

  if (screen_x < 0) {
    *map_x = (start_x + screen_x) / TileSize;
  } else {
    *map_x = int(start_x + (screen_x * zoom)) / TileSize;
  }

  if (screen_y < 0) {
    *map_y = (start_y + screen_y) / TileSize;
  } else {
    *map_y = int(start_y + (screen_y * zoom)) / TileSize;
  }

  if (floor <= GROUND_LAYER) {
    *map_x += GROUND_LAYER - floor;
    *map_y += GROUND_LAYER - floor;
  } /* else {
           *map_x += MAP_MAX_LAYER - floor;
           *map_y += MAP_MAX_LAYER - floor;
   }*/
}

void MapCanvas::GetScreenCenter(int *map_x, int *map_y) {
  int width, height;
  static_cast<MapWindow *>(GetParent())->GetViewSize(&width, &height);
  return ScreenToMap(width / 2, height / 2, map_x, map_y);
}

Position MapCanvas::GetCursorPosition() const {
  return Position(last_cursor_map_x, last_cursor_map_y, floor);
}

void MapCanvas::UpdatePositionStatus(int x, int y) {
  if (x == -1) {
    x = cursor_x;
  }
  if (y == -1) {
    y = cursor_y;
  }

  int map_x, map_y;
  ScreenToMap(x, y, &map_x, &map_y);

  wxString ss;
  ss << "x: " << map_x << " y:" << map_y << " z:" << floor;
  g_gui.root->SetStatusText(ss, 2);

  ss = "";
  Tile *tile = editor.map.getTile(map_x, map_y, floor);
  if (tile) {
    if (tile->spawn && g_settings.getInteger(Config::SHOW_SPAWNS)) {
      ss << "Spawn radius: " << tile->spawn->getSize();
    } else if (tile->creature &&
               g_settings.getInteger(Config::SHOW_CREATURES)) {
      ss << (tile->creature->isNpc() ? "NPC" : "Monster");
      ss << " \"" << wxstr(tile->creature->getName())
         << "\" spawntime: " << tile->creature->getSpawnTime();
    } else if (Item *item = tile->getTopItem()) {
      ss << "Item \"" << wxstr(item->getName()) << "\"";
      ss << " id:" << item->getID();
      ss << " cid:" << item->getClientID();
      if (item->getUniqueID()) {
        ss << " uid:" << item->getUniqueID(); // Korrigierter Zugriff
      }
      if (item->getActionID()) {
        ss << " aid:" << item->getActionID();
      }
      if (item->hasWeight()) {
        wxString s;
        s.Printf("%.2f", item->getWeight());
        ss << " weight: " << s;
      }
    } else {
      ss << "Nothing";
    }
  } else {
    ss << "Nothing";
  }

  if (editor.IsLive()) {
    editor.GetLive().updateCursor(Position(map_x, map_y, floor));
  }

  g_gui.root->SetStatusText(ss, 1);
}

void MapCanvas::UpdateZoomStatus() {
  // Map zoom range [5.0, 0.5] -> [1%, 100%]
  int percentage = (int)(100 - ((zoom - 0.5) / 4.5 * 99));
  if (percentage < 1)
    percentage = 1;
  if (percentage > 100)
    percentage = 100;
  wxString ss;
  ss << "zoom: " << percentage << "%";
  g_gui.root->SetStatusText(ss, 3);
}

void MapCanvas::getTilesToDraw(int mouse_map_x, int mouse_map_y, int floor,
                               PositionVector *tilestodraw,
                               PositionVector *tilestoborder,
                               bool fill /*= false*/) {
  if (fill) {
    Brush *brush = g_gui.GetCurrentBrush();
    if (!brush || !brush->isGround()) {
      return;
    }

    GroundBrush *newBrush = brush->asGround();
    Position position(mouse_map_x, mouse_map_y, floor);

    Tile *tile = editor.map.getTile(position);
    GroundBrush *oldBrush = nullptr;
    if (tile) {
      oldBrush = tile->getGroundBrush();
    }

    if (oldBrush && oldBrush->getID() == newBrush->getID()) {
      return;
    }

    if ((tile && tile->ground && !oldBrush) || (!tile && oldBrush)) {
      return;
    }

    if (tile && oldBrush) {
      GroundBrush *groundBrush = tile->getGroundBrush();
      if (!groundBrush || groundBrush->getID() != oldBrush->getID()) {
        return;
      }
    }

    std::fill(std::begin(processed), std::end(processed), false);
    MapCanvas::countMaxFills = 0;
    floodFill(&editor.map, position, BLOCK_SIZE / 2, BLOCK_SIZE / 2, oldBrush,
              tilestodraw);

  } else {
    for (int y = -g_gui.GetBrushSize() - 1; y <= g_gui.GetBrushSize() + 1;
         y++) {
      for (int x = -g_gui.GetBrushSize() - 1; x <= g_gui.GetBrushSize() + 1;
           x++) {
        if (g_gui.GetBrushShape() == BRUSHSHAPE_SQUARE) {
          if (x >= -g_gui.GetBrushSize() && x <= g_gui.GetBrushSize() &&
              y >= -g_gui.GetBrushSize() && y <= g_gui.GetBrushSize()) {
            if (tilestodraw) {
              tilestodraw->push_back(
                  Position(mouse_map_x + x, mouse_map_y + y, floor));
            }
          }
          if (std::abs(x) - g_gui.GetBrushSize() < 2 &&
              std::abs(y) - g_gui.GetBrushSize() < 2) {
            if (tilestoborder) {
              tilestoborder->push_back(
                  Position(mouse_map_x + x, mouse_map_y + y, floor));
            }
          }
        } else if (g_gui.GetBrushShape() == BRUSHSHAPE_CIRCLE) {
          double distance = sqrt(double(x * x) + double(y * y));
          if (distance < g_gui.GetBrushSize() + 0.005) {
            if (tilestodraw) {
              tilestodraw->push_back(
                  Position(mouse_map_x + x, mouse_map_y + y, floor));
            }
          }
          if (std::abs(distance - g_gui.GetBrushSize()) < 1.5) {
            if (tilestoborder) {
              tilestoborder->push_back(
                  Position(mouse_map_x + x, mouse_map_y + y, floor));
            }
          }
        }
      }
    }
  }
}

bool MapCanvas::floodFill(Map *map, const Position &center, int x, int y,
                          GroundBrush *brush, PositionVector *positions) {
  MapCanvas::countMaxFills++; // Access static member
  if (countMaxFills > (BLOCK_SIZE * 4 * 4)) {
    countMaxFills = 0;
    return true;
  }

  if (x <= 0 || y <= 0 || x >= BLOCK_SIZE || y >= BLOCK_SIZE) {
    return false;
  }

  processed[getFillIndex(x, y)] = true;

  int px = (center.x + x) - (BLOCK_SIZE / 2);
  int py = (center.y + y) - (BLOCK_SIZE / 2);
  if (px <= 0 || py <= 0 || px >= map->getWidth() || py >= map->getHeight()) {
    return false;
  }

  Tile *tile = map->getTile(px, py, center.z);
  if ((tile && tile->ground && !brush) || (!tile && brush)) {
    return false;
  }

  if (tile && brush) {
    GroundBrush *groundBrush = tile->getGroundBrush();
    if (!groundBrush || groundBrush->getID() != brush->getID()) {
      return false;
    }
  }

  positions->push_back(Position(px, py, center.z));

  bool deny = false;
  if (!processed[getFillIndex(x - 1, y)]) {
    deny = floodFill(map, center, x - 1, y, brush, positions);
  }

  if (!deny && !processed[getFillIndex(x, y - 1)]) {
    deny = floodFill(map, center, x, y - 1, brush, positions);
  }

  if (!deny && !processed[getFillIndex(x + 1, y)]) {
    deny = floodFill(map, center, x + 1, y, brush, positions);
  }

  if (!deny && !processed[getFillIndex(x, y + 1)]) {
    deny = floodFill(map, center, x, y + 1, brush, positions);
  }

  return deny;
}

// ============================================================================
// AnimationTimer

AnimationTimer::AnimationTimer(MapCanvas *canvas)
    : wxTimer(), map_canvas(canvas), started(false) {
        ////
      };

AnimationTimer::~AnimationTimer() {
  ////
};

void AnimationTimer::Notify() { map_canvas->Refresh(); };

void AnimationTimer::Start() {
  if (!started) {
    started = true;
    wxTimer::Start(4);
  }
};

void AnimationTimer::Stop() {
  if (started) {
    started = false;
    wxTimer::Stop();
  }
};

void MapCanvas::UpdateMinimapTexture() {
  if (!g_gui.IsEditorOpen()) {
    return;
  }

  uint32_t current_time = wxGetLocalTimeMillis().GetValue();
  if (minimap_tex_id != 0 && current_time - last_minimap_update_time < 200) {
    return;
  }
  last_minimap_update_time = current_time;

  if (minimap_tex_id == 0) {
    glGenTextures(1, &minimap_tex_id); // Generate texture ID only once
    glBindTexture(GL_TEXTURE_2D, minimap_tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    0x812F); // GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                    0x812F); // GL_CLAMP_TO_EDGE
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 180, 180, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, nullptr);
  }

  Editor &editor = *g_gui.GetCurrentEditor();
  int center_x, center_y;
  GetScreenCenter(&center_x, &center_y);

  int start_x = center_x - 90;
  int start_y = center_y - 90;

  if (start_x < 0) {
    start_x = 0;
  } else if (start_x + 180 > editor.map.getWidth()) {
    start_x = editor.map.getWidth() - 180;
  }
  if (start_y < 0) {
    start_y = 0;
  } else if (start_y + 180 > editor.map.getHeight()) {
    start_y = editor.map.getHeight() - 180;
  }

  start_x = std::max(start_x, 0);
  start_y = std::max(start_y, 0);
  int end_x = std::min(start_x + 180, editor.map.getWidth());
  int end_y = std::min(start_y + 180, editor.map.getHeight());

  minimap_start_x = start_x;
  minimap_start_y = start_y;

  static uint8_t tex_data[180 * 180 * 3];
  memset(tex_data, 0, sizeof(tex_data));

  if (g_gui.IsRenderingEnabled()) {
    for (int y = start_y; y < end_y; ++y) {
      int window_y = y - start_y;
      if (window_y < 0 || window_y >= 180)
        continue;
      for (int x = start_x; x < end_x; ++x) {
        int window_x = x - start_x;
        if (window_x < 0 || window_x >= 180)
          continue;

        Tile *tile = editor.map.getTile(x, y, floor);
        if (tile) {
          uint8_t color_idx = tile->getMiniMapColor();
          if (color_idx) {
            int idx = (window_y * 180 + window_x) * 3;
            tex_data[idx] = minimap_color[color_idx].red;
            tex_data[idx + 1] = minimap_color[color_idx].green;
            tex_data[idx + 2] = minimap_color[color_idx].blue;
          }
        }
      }
    }
  }

  glBindTexture(GL_TEXTURE_2D, minimap_tex_id);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 180, 180, GL_RGB, GL_UNSIGNED_BYTE,
                  tex_data);
}
