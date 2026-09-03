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
#include <queue>
#include <sstream>
#include <time.h>
#include <unordered_set>
#include <wx/filefn.h>
#include <wx/stdpaths.h>
#include <wx/wfstream.h>
#include <wx/app.h>
#include <wx/toplevel.h>

#include "application.h"
#include "browse_tile_window.h"
#include "brush.h"
#include "editor.h"
#include "copybuffer.h"
#include "graphics.h"
#include "gui.h"
#include "live_client.h"
#include "live_peer.h"
#include "live_server.h"
#include "live_socket.h"
#include "map.h"
#include "map_display.h"
#include "map_window.h"
#include "map_drawer.h"
#include "materials.h"
#include "old_properties_window.h"
#include "palette_window.h"
#include "procedural_generator_window.h"
#include "properties_window.h"
#include "sprites.h"
#include "svg_icons.h"
#include "tile.h"
#include "tileset_window.h"
#include "main_menubar.h"

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
EVT_KEY_UP(MapCanvas::OnKeyUp)
EVT_CHAR(MapCanvas::OnChar)

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
EVT_MENU(MAP_POPUP_MENU_CHANGE, MapCanvas::OnChangeConnected)
EVT_MENU(MAP_POPUP_MENU_ROTATE, MapCanvas::OnRotateItem)
EVT_MENU(MAP_POPUP_MENU_GOTO, MapCanvas::OnGotoDestination)
EVT_MENU(MAP_POPUP_MENU_SWITCH_DOOR, MapCanvas::OnSwitchDoor)
EVT_MENU(MAP_POPUP_MENU_QUICK_PING, MapCanvas::OnQuickPing)
EVT_MENU(MAP_POPUP_MENU_ADD_ANNOTATION, MapCanvas::OnAddAnnotation)
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
EVT_MENU(MAP_POPUP_MENU_ADD_FAVORITE, MapCanvas::OnAddFavorite)
EVT_MENU(MAP_POPUP_MENU_MOVE_TO_TILESET, MapCanvas::OnSelectMoveTo)
// ----
EVT_MENU(MAP_POPUP_MENU_PROPERTIES, MapCanvas::OnProperties)
EVT_MENU(MAP_POPUP_MENU_CREATE_TOWN, MapCanvas::OnCreateTown)
EVT_MENU(MAP_POPUP_MENU_EDIT_TOWN, MapCanvas::OnEditTown)
// ----
EVT_MENU(MAP_POPUP_MENU_BROWSE_TILE, MapCanvas::OnBrowseTile)
EVT_MENU_RANGE(MAP_POPUP_MENU_SCRIPT_FIRST, MAP_POPUP_MENU_SCRIPT_LAST,
               MapCanvas::OnScriptMenu)
END_EVENT_TABLE()

namespace {
void SyncImGuiMouseState(const wxMouseEvent& event) {
  if (!ImGui::GetCurrentContext()) {
    return;
  }

  ImGuiIO& io = ImGui::GetIO();
  io.MousePos = ImVec2((float)event.GetX(), (float)event.GetY());
  io.MouseDown[0] = event.LeftIsDown();
  io.MouseDown[1] = event.RightIsDown();
  io.MouseDown[2] = event.MiddleIsDown();
}

wxString ResolveBorderIconPath() {
  wxArrayString candidates;
  wxString exe_dir = wxPathOnly(wxStandardPaths::Get().GetExecutablePath());
  wxString cwd = wxGetCwd();

  candidates.Add("icons/auto_border.png");
  candidates.Add("../icons/auto_border.png");
  candidates.Add("../../icons/auto_border.png");
  candidates.Add("Map Editor/icons/auto_border.png");
  candidates.Add("../Map Editor/icons/auto_border.png");

  candidates.Add(exe_dir + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "auto_border.png");
  candidates.Add(cwd + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "auto_border.png");

  candidates.Add("brushes/optional_border_small.png");
  candidates.Add("../brushes/optional_border_small.png");
  candidates.Add("../../brushes/optional_border_small.png");
  candidates.Add("Map Editor/brushes/optional_border_small.png");
  candidates.Add("../Map Editor/brushes/optional_border_small.png");

  candidates.Add(exe_dir + wxFILE_SEP_PATH + "brushes" + wxFILE_SEP_PATH + "optional_border_small.png");
  candidates.Add(exe_dir + wxFILE_SEP_PATH + ".." + wxFILE_SEP_PATH + "brushes" + wxFILE_SEP_PATH + "optional_border_small.png");
  candidates.Add(exe_dir + wxFILE_SEP_PATH + ".." + wxFILE_SEP_PATH + ".." + wxFILE_SEP_PATH + "brushes" + wxFILE_SEP_PATH + "optional_border_small.png");

  candidates.Add(cwd + wxFILE_SEP_PATH + "brushes" + wxFILE_SEP_PATH + "optional_border_small.png");
  candidates.Add(cwd + wxFILE_SEP_PATH + ".." + wxFILE_SEP_PATH + "brushes" + wxFILE_SEP_PATH + "optional_border_small.png");
  candidates.Add(cwd + wxFILE_SEP_PATH + ".." + wxFILE_SEP_PATH + ".." + wxFILE_SEP_PATH + "brushes" + wxFILE_SEP_PATH + "optional_border_small.png");

  for (const wxString& candidate : candidates) {
    if (wxFileExists(candidate)) {
      return candidate;
    }
  }

  return wxString();
}

wxString ResolveBucketIconPath() {
  wxArrayString candidates;
  wxString exe_dir = wxPathOnly(wxStandardPaths::Get().GetExecutablePath());
  wxString cwd = wxGetCwd();

  candidates.Add("icons/bucket.png");
  candidates.Add("../icons/bucket.png");
  candidates.Add("../../icons/bucket.png");
  candidates.Add("Map Editor/icons/bucket.png");
  candidates.Add("../Map Editor/icons/bucket.png");

  candidates.Add(exe_dir + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "bucket.png");
  candidates.Add(exe_dir + wxFILE_SEP_PATH + ".." + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "bucket.png");
  candidates.Add(exe_dir + wxFILE_SEP_PATH + ".." + wxFILE_SEP_PATH + ".." + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "bucket.png");

  candidates.Add(cwd + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "bucket.png");
  candidates.Add(cwd + wxFILE_SEP_PATH + ".." + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "bucket.png");
  candidates.Add(cwd + wxFILE_SEP_PATH + ".." + wxFILE_SEP_PATH + ".." + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "bucket.png");

  for (const wxString& candidate : candidates) {
    if (wxFileExists(candidate)) {
      return candidate;
    }
  }

  return wxString();
}
} // namespace

void MapCanvas::ShowHUDNotification(const std::string& text, uint32_t color) {
  hud_notification_text = text;
  hud_notification_time_ms = wxGetLocalTimeMillis().GetValue();
  hud_notification_color = (color != 0) ? color : 0xFFFBBF24; // Default gold
}

void MapCanvas::OnKeyDown(wxKeyEvent& event) {
  if (!GetParent()) {
    event.Skip();
    return;
  }

	if (ImGui::GetCurrentContext() && (ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantTextInput)) {
		ImGuiIO& io = ImGui::GetIO();
		int key = event.GetKeyCode();
		if (key == WXK_BACK) { io.AddKeyEvent(ImGuiKey_Backspace, true); io.AddKeyEvent(ImGuiKey_Backspace, false); }
		else if (key == WXK_RETURN || key == WXK_NUMPAD_ENTER) { io.AddKeyEvent(ImGuiKey_Enter, true); io.AddKeyEvent(ImGuiKey_Enter, false); }
		else if (key == WXK_LEFT) { io.AddKeyEvent(ImGuiKey_LeftArrow, true); io.AddKeyEvent(ImGuiKey_LeftArrow, false); }
		else if (key == WXK_RIGHT) { io.AddKeyEvent(ImGuiKey_RightArrow, true); io.AddKeyEvent(ImGuiKey_RightArrow, false); }
		else if (key == WXK_HOME) { io.AddKeyEvent(ImGuiKey_Home, true); io.AddKeyEvent(ImGuiKey_Home, false); }
		else if (key == WXK_END) { io.AddKeyEvent(ImGuiKey_End, true); io.AddKeyEvent(ImGuiKey_End, false); }
		else if (key == WXK_DELETE) { io.AddKeyEvent(ImGuiKey_Delete, true); io.AddKeyEvent(ImGuiKey_Delete, false); }
		else if (key == WXK_ESCAPE) { ImGui::SetWindowFocus(nullptr); }
		event.Skip();
		Refresh();
		return;
	}

	if (event.GetKeyCode() == WXK_TAB) {
		if (g_gui.IsFillBrushMode()) {
			g_gui.SetFillBrushMode(false);
			g_gui.SetSelectionMode();
			g_gui.SetStatusText("Mode: Selection");
		} else if (g_gui.IsSelectionMode()) {
			g_gui.SetDrawingMode();
			g_gui.SetStatusText("Mode: Drawing");
		} else {
			g_gui.SetFillBrushMode(true);
			g_gui.SetStatusText("Mode: Fill Brush");
		}
		Refresh();
		return;
	}

  if (canvas_context_menu_open && event.GetKeyCode() == WXK_ESCAPE) {
    canvas_context_menu_open = false;
    Refresh();
    return;
  }

  if (g_gui.IsInChangeMode() && event.GetKeyCode() == WXK_ESCAPE) {
    g_gui.SetPendingChangeMode(false);
    g_gui.SetStatusText("Change mode cancelled.");
    Refresh();
    return;
  }

  if (tool_wheel_open && event.GetKeyCode() == WXK_ESCAPE) {
    tool_wheel_open = false;
    Refresh();
    return;
  }

  if (isPasting() && event.GetKeyCode() == WXK_ESCAPE) {
    EndPasting();
    Refresh();
    return;
  }

  if (event.ShiftDown() && (event.GetKeyCode() == 'Q' || event.GetKeyCode() == 'q')) {
    tool_wheel_open = !tool_wheel_open;
    if (tool_wheel_open) {
      tool_wheel_sub_menu = 0;
      int mouse_map_x, mouse_map_y;
      ScreenToMap(cursor_x, cursor_y, &mouse_map_x, &mouse_map_y);
      tool_wheel_tile_x = mouse_map_x;
      tool_wheel_tile_y = mouse_map_y;
      tool_wheel_tile_z = floor;
    }
    Refresh();
    return;
  }

  if (event.ControlDown()) {
    if (event.GetKeyCode() == 'C' || event.GetKeyCode() == 'c') {
      editor.copybuffer.copy(editor, floor);
      return;
    }
    if (event.GetKeyCode() == 'V' || event.GetKeyCode() == 'v') {
      if (editor.copybuffer.canPaste()) {
        g_gui.PreparePaste();
      }
      return;
    }
    if (event.GetKeyCode() == 'X' || event.GetKeyCode() == 'x') {
      editor.copybuffer.cut(editor, floor);
      Refresh();
      return;
    }
    if (event.GetKeyCode() == 'Z' || event.GetKeyCode() == 'z') {
      if (event.ShiftDown()) {
        if (editor.actionQueue && editor.actionQueue->canRedo()) {
          editor.actionQueue->redo();
        }
      } else {
        if (editor.actionQueue && editor.actionQueue->canUndo()) {
          editor.actionQueue->undo();
        }
      }
      Refresh();
      return;
    }
    if (event.GetKeyCode() == 'F' || event.GetKeyCode() == 'f' || event.GetKeyCode() == 'K' || event.GetKeyCode() == 'k' || event.GetKeyCode() == 'P' || event.GetKeyCode() == 'p') {
      wxCommandEvent cmd_evt;
      if (g_gui.root && g_gui.root->menu_bar) {
        g_gui.root->menu_bar->OnCommandPalette(cmd_evt);
      }
      return;
    }
    if (event.GetKeyCode() == 'Y' || event.GetKeyCode() == 'y') {
      if (editor.actionQueue && editor.actionQueue->canRedo()) {
        editor.actionQueue->redo();
      }
      Refresh();
      return;
    }
  }

  if (event.GetKeyCode() == WXK_DELETE || event.GetKeyCode() == WXK_NUMPAD_DELETE) {
    editor.destroySelection();
    Refresh();
    return;
  }

  if (!event.ControlDown() && (event.GetKeyCode() == 'R' || event.GetKeyCode() == 'r' || event.GetKeyCode() == 'Z' || event.GetKeyCode() == 'z')) {
    if (isPasting()) {
      editor.copybuffer.rotate90(true);
      if (g_gui.secondary_map) {
        g_gui.secondary_map->clear();
        for (MapIterator it = editor.copybuffer.getBufferMap().begin(); it != editor.copybuffer.getBufferMap().end(); ++it) {
          Tile* t = (*it)->get();
          if (t) g_gui.secondary_map->setTile(t->getPosition(), t->deepCopy(*g_gui.secondary_map));
        }
      }
      ShowHUDNotification("Rotated Prefab / Stamp 90°", 0xFF38BDF8);
      Refresh();
      return;
    }

    Brush* brush = g_gui.GetCurrentBrush();
    if (brush) {
      if (brush->isRaw()) {
        RAWBrush* raw_brush = static_cast<RAWBrush*>(brush);
        const ItemType& itemtype = g_items[raw_brush->getItemID()];
        if (itemtype.rotateTo != 0) {
          const ItemType& rotated_type = g_items[itemtype.rotateTo];
          if (rotated_type.raw_brush) {
            g_gui.SelectBrush(rotated_type.raw_brush);
          } else {
            raw_brush->setItemID(itemtype.rotateTo);
          }
          Refresh();
          return;
        }
      } else if (brush->isDoodad()) {
        DoodadBrush* doodad = static_cast<DoodadBrush*>(brush);
        if (doodad->getMaxVariation() > 1) {
          int current_var = g_gui.GetBrushVariation();
          int next_var = (current_var + 1) % doodad->getMaxVariation();
          g_gui.SetBrushVariation(next_var);
          Refresh();
          return;
        }
      }
    }

    if (editor.selection.size() > 0) {
      wxCommandEvent dummy;
      OnRotateItem(dummy);
      return;
    } else {
      int mouse_map_x, mouse_map_y;
      ScreenToMap(cursor_x, cursor_y, &mouse_map_x, &mouse_map_y);
      last_click_map_x = mouse_map_x;
      last_click_map_y = mouse_map_y;
      last_click_map_z = floor;
      wxCommandEvent dummy;
      OnRotateItem(dummy);
      return;
    }
  }

  // B Key: Toggle Auto-Bordering with on-screen HUD notification
  if (!event.ControlDown() && !event.AltDown() && !event.ShiftDown() && (event.GetKeyCode() == 'B' || event.GetKeyCode() == 'b')) {
    bool current = (g_settings.getInteger(Config::USE_AUTOMAGIC) != 0);
    bool next_state = !current;
    g_settings.setInteger(Config::USE_AUTOMAGIC, next_state ? 1 : 0);
    g_settings.setInteger(Config::BORDER_IS_GROUND, next_state ? 1 : 0);
    if (next_state) {
      ShowHUDNotification("Auto-Border: ON (Active)", 0xFF10B981); // Emerald Green
      g_gui.SetStatusText("Auto-Border: ON (Automatic bordering enabled)");
    } else {
      ShowHUDNotification("Auto-Border: OFF (Manual)", 0xFFF59E0B); // Amber Gold
      g_gui.SetStatusText("Auto-Border: OFF (Manual bordering mode)");
    }
    Refresh();
    return;
  }

  // Keys 1..7 (Top row and Numpad): Brush Size Quick Keys
  if (!event.ControlDown() && !event.AltDown() && !event.ShiftDown()) {
    int key = event.GetKeyCode();
    if (key == '1' || key == WXK_NUMPAD1) {
      g_gui.SetBrushSize(0);
      Refresh();
      return;
    } else if (key == '2' || key == WXK_NUMPAD2) {
      g_gui.SetBrushSize(1);
      Refresh();
      return;
    } else if (key == '3' || key == WXK_NUMPAD3) {
      g_gui.SetBrushSize(2);
      Refresh();
      return;
    } else if (key == '4' || key == WXK_NUMPAD4) {
      g_gui.SetBrushSize(4);
      Refresh();
      return;
    } else if (key == '5' || key == WXK_NUMPAD5) {
      g_gui.SetBrushSize(6);
      Refresh();
      return;
    } else if (key == '6' || key == WXK_NUMPAD6) {
      g_gui.SetBrushSize(8);
      Refresh();
      return;
    } else if (key == '7' || key == WXK_NUMPAD7) {
      g_gui.SetBrushSize(11);
      Refresh();
      return;
    }
  }

  // Brush Size +/- Quick Keys
  if (!event.ControlDown() && !event.AltDown()) {
    int key = event.GetKeyCode();
    if (key == WXK_ADD || key == WXK_NUMPAD_ADD || key == '+' || key == '=') {
      g_gui.IncreaseBrushSize();
      Refresh();
      return;
    } else if (key == WXK_SUBTRACT || key == WXK_NUMPAD_SUBTRACT || key == '-') {
      g_gui.DecreaseBrushSize();
      Refresh();
      return;
    }
  }

  MapWindow* map_window = static_cast<MapWindow*>(GetParent());
  constexpr int pan_step = 96;

	switch (event.GetKeyCode()) {
		case WXK_TAB: {
			bool is_selection = g_gui.IsSelectionMode();
			bool is_eraser = (g_gui.GetCurrentBrush() == g_gui.eraser);
			bool is_bucket = g_gui.IsFillBrushMode();

			if (is_selection) {
				// Selection -> Pencil
				g_gui.SetFillBrushMode(false);
				g_gui.SetDrawingMode();
				if (g_gui.GetCurrentBrush() == g_gui.eraser) {
					g_gui.SelectBrush(nullptr);
				}
			} else if (is_eraser) {
				// Eraser -> Selection
				g_gui.SetFillBrushMode(false);
				g_gui.SetSelectionMode();
			} else if (is_bucket) {
				// Bucket -> Eraser
				g_gui.SetFillBrushMode(false);
				g_gui.SetDrawingMode();
				g_gui.SelectBrush(g_gui.eraser);
			} else {
				// Pencil -> Bucket
				g_gui.SetDrawingMode();
				g_gui.SetFillBrushMode(true);
				if (g_gui.GetCurrentBrush() == g_gui.eraser) {
					g_gui.SelectBrush(nullptr);
				}
			}
			Refresh();
			return;
		}
		case 'A':
		case 'a':
      map_window->ScrollRelative(-pan_step, 0);
      Refresh();
      return;
    case 'D':
    case 'd':
      map_window->ScrollRelative(pan_step, 0);
      Refresh();
      return;
    case 'W':
    case 'w':
      map_window->ScrollRelative(0, -pan_step);
      Refresh();
      return;
    case 'S':
    case 's':
      map_window->ScrollRelative(0, pan_step);
      Refresh();
      return;
    case WXK_PAGEUP:
      g_gui.ChangeFloor(std::max(0, floor - 1));
      Refresh();
      return;
    case WXK_PAGEDOWN:
      g_gui.ChangeFloor(std::min(MAP_MAX_LAYER, floor + 1));
      Refresh();
      return;
    default:
      event.Skip();
      return;
  }
}

void MapCanvas::OnKeyUp(wxKeyEvent& event) {
  event.Skip();
}

void MapCanvas::OnChar(wxKeyEvent& event) {
  if (ImGui::GetCurrentContext() && (ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantTextInput)) {
    ImGuiIO& io = ImGui::GetIO();
    wxChar uc = event.GetUnicodeKey();
    if (uc >= 32 && uc < 0x10FFFF) {
      io.AddInputCharacter((unsigned int)uc);
    }
    Refresh();
    return;
  }
  event.Skip();
}

void MapCanvas::OnMouseLeftClick(wxMouseEvent& event) {
	cursor_x = event.GetX();
	cursor_y = event.GetY();
	SyncImGuiMouseState(event);
  if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) {
    Refresh();
    return;
  }

  if (canvas_context_menu_open) {
    canvas_context_menu_open = false;
    Refresh();
  }

  if (tool_wheel_open) {
    int hovered = GetHoveredRadialSlice();
    if (hovered >= 0) {
      if (tool_wheel_sub_menu == 0) { // Main Radial Wheel
        switch (hovered) {
          case 0: // Selection
            g_gui.SetFillBrushMode(false);
            g_gui.SetSelectionMode();
            break;
          case 1: // Pencil
            g_gui.SetFillBrushMode(false);
            g_gui.SetDrawingMode();
            break;
          case 2: // Bucket
            g_gui.SetDrawingMode();
            g_gui.SetFillBrushMode(true);
            break;
          case 3: // Zones Sub-Menu
            tool_wheel_sub_menu = 1;
            Refresh();
            return;
          case 4: // Doors Sub-Menu
            tool_wheel_sub_menu = 2;
            Refresh();
            return;
          case 5: // Windows Sub-Menu
            tool_wheel_sub_menu = 3;
            Refresh();
            return;
          case 6: // Eraser
            g_gui.SetFillBrushMode(false);
            g_gui.SelectBrush(g_gui.eraser);
            break;
          case 7: // Prefab Creator
            g_gui.SetFillBrushMode(false);
            g_gui.SelectBrush(g_gui.prefab_creator_brush);
            break;
        }
      } else if (tool_wheel_sub_menu == 1) { // Zones Sub-Menu
        switch (hovered) {
          case 0: // Protection Zone
            g_gui.SetFillBrushMode(false);
            g_gui.SelectBrush(g_gui.pz_brush);
            break;
          case 1: // No Logout Zone
            g_gui.SetFillBrushMode(false);
            g_gui.SelectBrush(g_gui.nolog_brush);
            break;
          case 2: // No PvP Zone
            g_gui.SetFillBrushMode(false);
            g_gui.SelectBrush(g_gui.rook_brush);
            break;
          case 3: // PvP Zone
            g_gui.SetFillBrushMode(false);
            g_gui.SelectBrush(g_gui.pvp_brush);
            break;
          case 4: // Back
            tool_wheel_sub_menu = 0;
            Refresh();
            return;
        }
      } else if (tool_wheel_sub_menu == 2) { // Doors Sub-Menu
        switch (hovered) {
          case 0: // Normal Door
            g_gui.SetFillBrushMode(false);
            g_gui.SelectBrush(g_gui.normal_door_brush);
            break;
          case 1: // Locked Door
            g_gui.SetFillBrushMode(false);
            g_gui.SelectBrush(g_gui.locked_door_brush);
            break;
          case 2: // Magic Door
            g_gui.SetFillBrushMode(false);
            g_gui.SelectBrush(g_gui.magic_door_brush);
            break;
          case 3: // Quest Door
            g_gui.SetFillBrushMode(false);
            g_gui.SelectBrush(g_gui.quest_door_brush);
            break;
          case 4: // Back
            tool_wheel_sub_menu = 0;
            Refresh();
            return;
        }
      } else if (tool_wheel_sub_menu == 3) { // Windows Sub-Menu
        switch (hovered) {
          case 0: // Hatch Window
            g_gui.SetFillBrushMode(false);
            g_gui.SelectBrush(g_gui.hatch_door_brush);
            break;
          case 1: // Window
            g_gui.SetFillBrushMode(false);
            g_gui.SelectBrush(g_gui.window_door_brush);
            break;
          case 2: // Back
            tool_wheel_sub_menu = 0;
            Refresh();
            return;
        }
      }
    }
    tool_wheel_open = false;
    tool_wheel_sub_menu = 0;
    Refresh();
    return;
  }

  if (isPasting()) {
    int mouse_map_x, mouse_map_y;
    ScreenToMap(cursor_x, cursor_y, &mouse_map_x, &mouse_map_y);
    editor.copybuffer.paste(editor, Position(mouse_map_x, mouse_map_y, floor));
    EndPasting();
    Refresh();
    return;
  }

  if (ui_toolbar && ui_toolbar->isVisible() && ui_toolbar->isPointInside((float)cursor_x, (float)cursor_y)) {
    ui_toolbar->onMouseDown((float)cursor_x, (float)cursor_y, 0);
    Refresh();
    return;
  }

	int mouse_map_x, mouse_map_y;
	ScreenToMap(cursor_x, cursor_y, &mouse_map_x, &mouse_map_y);

	// [PERF] Removed: Disk I/O in mouse hot-path kills frame rate
	// LogErrorToFile(wxString::Format("OnMouseLeftClick: screen=(%d, %d), map=(%d, %d), floor=%d, drawing=%d, current_brush=%s",
	// 	cursor_x, cursor_y, mouse_map_x, mouse_map_y, floor, drawing,
	// 	g_gui.GetCurrentBrush() ? g_gui.GetCurrentBrush()->getName().c_str() : "nullptr").ToStdString());

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
		Brush* current_brush = g_gui.GetCurrentBrush();
		if (event.AltDown()) {
			if (current_brush && current_brush->isDoodad()) {
				DoodadBrush* doodad = static_cast<DoodadBrush*>(current_brush);
				if (doodad->getMaxVariation() > 1) {
					int current_var = g_gui.GetBrushVariation();
					int next_var = (current_var + 1) % doodad->getMaxVariation();
					g_gui.SetBrushVariation(next_var);
				} else {
					g_gui.FillDoodadPreviewBuffer();
				}
				dragging_draw = false;
				CallAfter([this]() { Refresh(); });
				return;
			} else {
				// Eyedropper / Pipette tool: Alt + Click on tile picks top item or ground brush
				Tile* tile = editor.map.getTile(mouse_map_x, mouse_map_y, floor);
				if (tile) {
					Brush* picked_brush = nullptr;
					Item* top_item = tile->getTopItem();
					if (top_item) {
						picked_brush = top_item->getBrush();
						if (!picked_brush) picked_brush = top_item->getRAWBrush();
					}
					if (!picked_brush && tile->ground) {
						picked_brush = tile->ground->getGroundBrush();
						if (!picked_brush) picked_brush = tile->ground->getBrush();
						if (!picked_brush) picked_brush = tile->ground->getRAWBrush();
					}
					if (picked_brush) {
						g_gui.SelectBrush(picked_brush);
						ShowHUDNotification("Eyedropper Picked: " + picked_brush->getName(), 0xFF38BDF8);
						g_gui.SetStatusText("Eyedropper picked '" + wxString(picked_brush->getName()) + "'");
						dragging_draw = false;
						CallAfter([this]() { Refresh(); });
						return;
					}
				}
			}
		}

    // Live automagic bordering during mouse drag: do not defer borders
    // editor.setDeferBorders(true);
    editor.setDeferNetworkSync(true);
    dragging_draw = !g_gui.IsFillBrushMode();
    rectangle_mode = event.ShiftDown() && !g_gui.IsFillBrushMode();
    if (g_gui.IsFillBrushMode()) {
      PositionVector tilestodraw;
      PositionVector tilestoborder;
      getTilesToDraw(mouse_map_x, mouse_map_y, floor, &tilestodraw, &tilestoborder, true);
      if (event.ControlDown()) {
        editor.undraw(tilestodraw, tilestoborder, false);
      } else {
        editor.draw(tilestodraw, tilestoborder, false);
        if (current_brush && current_brush->isWall()) {
          for (const Position& pos : tilestodraw) {
            for (int dy = -1; dy <= 1; ++dy) {
              for (int dx = -1; dx <= 1; ++dx) {
                Tile* t = editor.map.getTile(pos.x + dx, pos.y + dy, pos.z);
                if (t) {
                  WallBrush::doWalls(&editor.map, t);
                }
              }
            }
          }
        }
      }
    } else if (!rectangle_mode) {
			PositionVector tilestodraw;
			PositionVector tilestoborder;
			getTilesToDraw(mouse_map_x, mouse_map_y, floor, &tilestodraw, &tilestoborder, false);
			if (event.ControlDown()) {
				editor.undraw(tilestodraw, tilestoborder, false);
			} else {
				editor.draw(tilestodraw, tilestoborder, false);
			}
		}
	} else {
		Tile* tile = editor.map.getTile(mouse_map_x, mouse_map_y, floor);
		bool clicked_selected = (tile != nullptr && editor.selection.getTiles().count(tile) > 0);

		if (clicked_selected) {
			dragging_selection = true;
			drag_start_map_x = mouse_map_x;
			drag_start_map_y = mouse_map_y;
			drag_start_map_z = floor;
		} else {
			rubber_band_mode = true;
			rubber_start_x = cursor_x;
			rubber_start_y = cursor_y;
			rubber_end_x = cursor_x;
			rubber_end_y = cursor_y;

			if (!event.ShiftDown() && !event.ControlDown()) {
				editor.selection.clear();
				markDirty();
			}
		}
	}
	CallAfter([this]() { Refresh(); });
}

void MapCanvas::OnMouseLeftRelease(wxMouseEvent& event) {
	SyncImGuiMouseState(event);
  if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) {
    return;
  }
  if (ui_toolbar && ui_toolbar->isVisible() && ui_toolbar->onMouseClick((float)event.GetX(), (float)event.GetY(), 0)) {
    Refresh();
    return;
  }

	if (rubber_band_mode) {
		rubber_band_mode = false;
		
		int dist_x = std::abs(rubber_start_x - rubber_end_x);
		int dist_y = std::abs(rubber_start_y - rubber_end_y);
		
		if (dist_x < 4 && dist_y < 4) {
			// Single click selection
			int mouse_map_x, mouse_map_y;
			ScreenToMap(rubber_start_x, rubber_start_y, &mouse_map_x, &mouse_map_y);
			Tile* tile = editor.map.getTile(mouse_map_x, mouse_map_y, floor);
				if (tile) {
					editor.selection.start(Selection::INTERNAL);
					bool is_selected = editor.selection.getTiles().count(tile) > 0;
					if (event.ControlDown() && is_selected) {
						editor.selection.remove(tile);
					} else if (!is_selected || event.ControlDown()) {
						if (tile->spawn && g_settings.getInteger(Config::SHOW_SPAWNS)) {
							editor.selection.add(tile, tile->spawn);
						} else if (tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)) {
							editor.selection.add(tile, tile->creature);
						} else {
							Item* top_item = tile->getTopItem();
							if (top_item) {
								editor.selection.add(tile, top_item);
							} else if (tile->ground) {
								editor.selection.add(tile, tile->ground);
							} else {
								editor.selection.add(tile);
							}
						}
					}
					editor.selection.finish(Selection::INTERNAL);
					if (editor.selection.size() > 0) {
						editor.copybuffer.copy(editor, floor);
					}
					markDirty();
				}
		} else {
			// Drag select
			int start_map_x, start_map_y;
			int end_map_x, end_map_y;
			ScreenToMap(rubber_start_x, rubber_start_y, &start_map_x, &start_map_y);
			ScreenToMap(rubber_end_x, rubber_end_y, &end_map_x, &end_map_y);
			
			int x1 = std::min(start_map_x, end_map_x);
			int x2 = std::max(start_map_x, end_map_x);
			int y1 = std::min(start_map_y, end_map_y);
			int y2 = std::max(start_map_y, end_map_y);

			int min_z = floor;
			int max_z = floor;

			int sel_type = g_settings.getInteger(Config::SELECTION_TYPE);
			if (sel_type == SELECT_ALL_FLOORS) {
				min_z = 0;
				max_z = MAP_LAYERS - 1;
			} else if (sel_type == SELECT_VISIBLE_FLOORS) {
				if (floor <= GROUND_LAYER) {
					min_z = floor;
					max_z = GROUND_LAYER;
				} else {
					min_z = 8;
					max_z = std::min(MAP_MAX_LAYER, floor + 2);
				}
			}

			editor.selection.start(Selection::INTERNAL);
			for (int z = min_z; z <= max_z; ++z) {
				for (int y = y1; y <= y2; ++y) {
					for (int x = x1; x <= x2; ++x) {
						Tile* tile = editor.map.getTile(x, y, z);
						if (!tile && z == floor) {
							tile = editor.map.getOrCreateTile(Position(x, y, z));
						}
						if (tile) {
							bool is_selected = editor.selection.getTiles().count(tile) > 0;

							if (event.ControlDown()) {
								if (is_selected) {
									editor.selection.remove(tile);
								} else {
									editor.selection.add(tile);
								}
							} else {
								if (!is_selected) {
									editor.selection.add(tile);
								}
							}
						}
					}
				}
			}
			editor.selection.finish(Selection::INTERNAL);
			if (editor.selection.size() > 0) {
				editor.copybuffer.copy(editor, floor);
			}
			markDirty();
		}
		dragging_draw = false;
		rectangle_mode = false;
		last_click_map_x = -1;
		last_click_map_y = -1;
		last_click_map_z = -1;
		CallAfter([this]() { Refresh(); });
		return;
	}

	if (drawing && dragging_draw && rectangle_mode) {
		int cursor_x = event.GetX();
		int cursor_y = event.GetY();
		int mouse_map_x, mouse_map_y;
		ScreenToMap(cursor_x, cursor_y, &mouse_map_x, &mouse_map_y);

		if (last_click_map_x != -1 && last_click_map_y != -1) {
			int brush_size = g_gui.GetBrushSize();
			int start_x = std::min(last_click_map_x, mouse_map_x);
			int end_x = std::max(last_click_map_x, mouse_map_x);
			int start_y = std::min(last_click_map_y, mouse_map_y);
			int end_y = std::max(last_click_map_y, mouse_map_y);

			bool is_wall = g_gui.GetCurrentBrush() && g_gui.GetCurrentBrush()->isWall();
			PositionVector tilestodraw;
			PositionVector tilestoborder;
			for (int y = start_y - brush_size - 1; y <= end_y + brush_size + 1; ++y) {
				for (int x = start_x - brush_size - 1; x <= end_x + brush_size + 1; ++x) {
					Position pos(x, y, floor);
					if (x >= start_x - brush_size && x <= end_x + brush_size && y >= start_y - brush_size && y <= end_y + brush_size) {
						if (!is_wall || (x == start_x - brush_size || x == end_x + brush_size || y == start_y - brush_size || y == end_y + brush_size)) {
							tilestodraw.push_back(pos);
						}
					}
					tilestoborder.push_back(pos);
				}
			}
			if (event.ControlDown()) {
				editor.undraw(tilestodraw, tilestoborder, false);
			} else {
				editor.draw(tilestodraw, tilestoborder, false);
			}
		}
	}
	
	if (!drawing && dragging_selection) {
		int cursor_x = event.GetX();
		int cursor_y = event.GetY();
		int mouse_map_x, mouse_map_y;
		ScreenToMap(cursor_x, cursor_y, &mouse_map_x, &mouse_map_y);
		
		int dx = mouse_map_x - drag_start_map_x;
		int dy = mouse_map_y - drag_start_map_y;
		int dz = floor - drag_start_map_z;
		if (dx != 0 || dy != 0 || dz != 0) {
			editor.moveSelection(Position(dx, dy, dz));
		} else {
			// Single click on already selected tile: clear other selections
			if (!event.ShiftDown() && !event.ControlDown()) {
				Tile* tile = editor.map.getTile(mouse_map_x, mouse_map_y, floor);
				if (tile) {
					editor.selection.start(Selection::INTERNAL);
					editor.selection.clear();
					if (tile->spawn && g_settings.getInteger(Config::SHOW_SPAWNS)) {
						editor.selection.add(tile, tile->spawn);
					} else if (tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)) {
						editor.selection.add(tile, tile->creature);
					} else {
						Item* top_item = tile->getTopItem();
						if (top_item) {
							editor.selection.add(tile, top_item);
						} else if (tile->ground) {
							editor.selection.add(tile, tile->ground);
						} else {
							editor.selection.add(tile);
						}
					}
					editor.selection.finish(Selection::INTERNAL);
					if (editor.selection.size() > 0) {
						editor.copybuffer.copy(editor, floor);
					}
				}
			}
		}
		dragging_selection = false;
		markDirty();
	}

	dragging_draw = false;
	rectangle_mode = false;
	last_click_map_x = -1;
	last_click_map_y = -1;
	last_click_map_z = -1;
	if (drawing) {
		editor.setDeferBorders(false);
		editor.setDeferNetworkSync(false);
	}
	CallAfter([this]() { Refresh(); });
}

void MapCanvas::OnMouseRightClick(wxMouseEvent& event) {
	cursor_x = event.GetX();
	cursor_y = event.GetY();
  SyncImGuiMouseState(event);
  if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) {
    return;
  }

  if (tool_wheel_open) {
    tool_wheel_open = false;
    Refresh();
    return;
  }

  if (isPasting()) {
    EndPasting();
    Refresh();
    return;
  }

  if (ui_toolbar && ui_toolbar->isVisible() && ui_toolbar->isPointInside((float)cursor_x, (float)cursor_y)) {
    return;
  }

	int mouse_map_x, mouse_map_y;
	ScreenToMap(cursor_x, cursor_y, &mouse_map_x, &mouse_map_y);

	// [PERF] Removed: Disk I/O in mouse hot-path kills frame rate
	// LogErrorToFile(wxString::Format("OnMouseRightClick: screen=(%d, %d), map=(%d, %d), floor=%d, drawing=%d, current_brush=%s",
	// 	cursor_x, cursor_y, mouse_map_x, mouse_map_y, floor, drawing,
	// 	g_gui.GetCurrentBrush() ? g_gui.GetCurrentBrush()->getName().c_str() : "nullptr").ToStdString());

	if (g_gui.IsInChangeMode()) {
		g_gui.SetPendingChangeMode(false);
		g_gui.SetStatusText("Change mode cancelled.");
		Refresh();
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

  if (!g_gui.IsSelectionMode() || g_gui.GetCurrentBrush() != nullptr) {
    g_gui.SetSelectionMode();
    g_gui.SelectBrush(nullptr);
    CallAfter([this]() { Refresh(); });
  }

	Tile* clicked_tile = editor.map.getTile(mouse_map_x, mouse_map_y, floor);
	if (clicked_tile) {
		if (editor.selection.size() <= 1 || clicked_tile != editor.selection.getSelectedTile()) {
			editor.selection.clear();
			editor.selection.start(Selection::INTERNAL);
			if (clicked_tile->spawn && g_settings.getInteger(Config::SHOW_SPAWNS)) {
				editor.selection.add(clicked_tile, clicked_tile->spawn);
			} else if (clicked_tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)) {
				editor.selection.add(clicked_tile, clicked_tile->creature);
			} else {
				Item* top_item = clicked_tile->getTopItem();
				if (top_item) {
					editor.selection.add(clicked_tile, top_item);
				} else if (clicked_tile->ground) {
					editor.selection.add(clicked_tile, clicked_tile->ground);
				}
			}
			editor.selection.finish(Selection::INTERNAL);
		}
	}

	canvas_context_menu_open = true;
	canvas_context_menu_just_opened = true;
	canvas_context_menu_x = cursor_x;
	canvas_context_menu_y = cursor_y;
	Refresh();
}

void MapCanvas::OnMouseRightRelease(wxMouseEvent& event) {
  SyncImGuiMouseState(event);
  if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) {
    return;
  }
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
	if (drawing) {
		editor.setDeferBorders(false);
		editor.setDeferNetworkSync(false);
	}
	CallAfter([this]() { Refresh(); });
}

void MapCanvas::OnMouseMove(wxMouseEvent& event) {
	cursor_x = event.GetX();
	cursor_y = event.GetY();
  SyncImGuiMouseState(event);
  if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) {
    return;
  }

  if (tool_wheel_open) {
    Refresh();
    return;
  }

	if (rubber_band_mode) {
		rubber_end_x = cursor_x;
		rubber_end_y = cursor_y;
		Refresh();
		return;
	}

	if (ui_toolbar && ui_toolbar->isVisible()) {
		ui_toolbar->onHover((float)cursor_x, (float)cursor_y);
	}

	if (screendragging && event.MiddleIsDown()) {
		int dx = cursor_x - drag_start_x;
		int dy = cursor_y - drag_start_y;
		
		if (dx != 0 || dy != 0) {
			MapWindow* map_win = static_cast<MapWindow*>(GetParent());
			int scroll_x, scroll_y;
			map_win->GetViewStart(&scroll_x, &scroll_y);
			map_win->Scroll(scroll_x - int(dx * zoom), scroll_y - int(dy * zoom));
			
			unsigned int current_time = wxGetLocalTimeMillis().GetValue();
			unsigned int dt = current_time - last_drag_time;
			if (dt > 0) {
				double inst_vx = (double)(-dx * zoom) / (double)dt;
				double inst_vy = (double)(-dy * zoom) / (double)dt;
				drag_velocity_x = drag_velocity_x * 0.6 + inst_vx * 0.4;
				drag_velocity_y = drag_velocity_y * 0.6 + inst_vy * 0.4;
			}
			last_drag_time = current_time;
			
			int client_w, client_h;
			GetClientSize(&client_w, &client_h);
			
			// [UI] Auto-scroll warping at edges: keep cursor inside canvas limits
			const int border_threshold = 40;
			if (cursor_x < border_threshold || cursor_x > client_w - border_threshold ||
				cursor_y < border_threshold || cursor_y > client_h - border_threshold) 
			{
				int target_x = client_w / 2;
				int target_y = client_h / 2;
				WarpPointer(target_x, target_y);
				drag_start_x = target_x;
				drag_start_y = target_y;
			} else {
				drag_start_x = cursor_x;
				drag_start_y = cursor_y;
			}
		}
		return;
	}

	int mouse_map_x, mouse_map_y;
	ScreenToMap(cursor_x, cursor_y, &mouse_map_x, &mouse_map_y);
	
	// [PERF] Removed: Disk I/O in mouse move hot-path kills frame rate
	// if (drawing && (mouse_map_x != last_cursor_map_x || mouse_map_y != last_cursor_map_y || floor != last_cursor_map_z)) {
	// 	LogErrorToFile(wxString::Format("OnMouseMove (drawing): screen=(%d, %d), map=(%d, %d), floor=%d, dragging_draw=%d",
	// 		cursor_x, cursor_y, mouse_map_x, mouse_map_y, floor, dragging_draw).ToStdString());
	// }

	bool tile_changed = (mouse_map_x != last_cursor_map_x || mouse_map_y != last_cursor_map_y || floor != last_cursor_map_z);
	if (tile_changed) {
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
				if (event.ControlDown()) {
					editor.undraw(tilestodraw, tilestoborder, false);
				} else {
					editor.draw(tilestodraw, tilestoborder, false);
				}
			} else if (event.RightIsDown()) {
				editor.undraw(tilestodraw, tilestoborder, event.ControlDown());
			}
		}
		Refresh();
	} else if (screendragging || rubber_band_mode) {
		Refresh();
	}
}

void MapCanvas::OnMouseLeftDoubleClick(wxMouseEvent& event) {
  SyncImGuiMouseState(event);
  if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) {
    return;
  }
  
  int mouse_map_x, mouse_map_y;
  ScreenToMap(event.GetX(), event.GetY(), &mouse_map_x, &mouse_map_y);
  Tile* tile = editor.map.getTile(mouse_map_x, mouse_map_y, floor);
  if (tile) {
    bool has_spawn = (tile->spawn != nullptr && g_settings.getInteger(Config::SHOW_SPAWNS));
    bool has_creature = (tile->creature != nullptr && g_settings.getInteger(Config::SHOW_CREATURES));
    Item* top_item = tile->getTopItem();
    if (top_item == tile->ground) {
      top_item = nullptr;
    }
    
    if (has_spawn || has_creature || top_item) {
      editor.selection.start(Selection::INTERNAL);
      editor.selection.clear();
      if (has_spawn) {
        editor.selection.add(tile, tile->spawn);
      } else if (has_creature) {
        editor.selection.add(tile, tile->creature);
      } else if (top_item) {
        editor.selection.add(tile, top_item);
      }
      editor.selection.finish(Selection::INTERNAL);
      
      last_click_map_x = mouse_map_x;
      last_click_map_y = mouse_map_y;
      
      wxCommandEvent dummy;
      OnProperties(dummy);
    }
  }
}

void MapCanvas::OnMouseCenterClick(wxMouseEvent& event) {
  SyncImGuiMouseState(event);
  if (tool_wheel_open) {
    return;
  }
  if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) {
    return;
  }
	if (!screendragging) {
		screendragging = true;
		drag_start_x = event.GetX();
		drag_start_y = event.GetY();
		SetCursor(wxCursor(wxCURSOR_HAND));
		is_kinetic_scrolling = false;
		drag_velocity_x = 0.0;
		drag_velocity_y = 0.0;
		last_drag_time = wxGetLocalTimeMillis().GetValue();
	}
}

void MapCanvas::OnMouseCenterRelease(wxMouseEvent& event) {
  SyncImGuiMouseState(event);
  if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) {
    return;
  }
	if (screendragging) {
		screendragging = false;
		SetCursor(wxNullCursor);
		double speed = std::sqrt(drag_velocity_x * drag_velocity_x + drag_velocity_y * drag_velocity_y);
		if (speed > 0.05) {
			is_kinetic_scrolling = true;
			kinetic_velocity_x = drag_velocity_x;
			kinetic_velocity_y = drag_velocity_y;
			last_kinetic_time = wxGetLocalTimeMillis().GetValue();
		}
	}
}
void MapCanvas::OnWheel(wxMouseEvent& event) {
  if (tool_wheel_open) {
    return;
  }
  if (event.AltDown() || event.ControlDown() || event.ShiftDown()) {
    if (event.GetWheelRotation() > 0) {
      g_gui.IncreaseBrushSize();
    } else if (event.GetWheelRotation() < 0) {
      g_gui.DecreaseBrushSize();
    }
    Refresh();
    return;
  }
  if (ImGui::GetCurrentContext()) {
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)event.GetX(), (float)event.GetY());
    io.MouseWheel = (float)event.GetWheelRotation() / (float)event.GetWheelDelta();
  }

  if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) {
    return;
  }

  if (!is_smooth_zooming) {
    target_zoom = zoom;
  }

  // Discrete zoom steps from 0.5 (200% zoom-in) to 1.0 (100% normal) up to 10.0 (10% wide ultra-zoom)
  static const double kZoomSteps[] = {
    0.50, 0.55, 0.60, 0.65, 0.70, 0.75, 0.80, 0.85, 0.90, 0.95,
    1.00, 1.10, 1.25, 1.40, 1.60, 1.85, 2.15, 2.50, 3.00, 3.60,
    4.30, 5.10, 6.00, 7.00, 8.00, 9.00, 10.00
  };
  static const int kNumSteps = sizeof(kZoomSteps) / sizeof(kZoomSteps[0]);

  if (event.GetWheelRotation() > 0) {
    // Zoom IN (smaller zoom factor value)
    double next_target = kZoomSteps[0];
    for (int i = kNumSteps - 1; i >= 0; --i) {
      if (kZoomSteps[i] < target_zoom - 0.01) {
        next_target = kZoomSteps[i];
        break;
      }
    }
    target_zoom = next_target;
  } else if (event.GetWheelRotation() < 0) {
    // Zoom OUT (larger zoom factor value)
    double next_target = kZoomSteps[kNumSteps - 1];
    for (int i = 0; i < kNumSteps; ++i) {
      if (kZoomSteps[i] > target_zoom + 0.01) {
        next_target = kZoomSteps[i];
        break;
      }
    }
    target_zoom = next_target;
  }

  zoom_focus_x = event.GetX();
  zoom_focus_y = event.GetY();
  is_smooth_zooming = true;
}





void MapCanvas::OnGainMouse(wxMouseEvent& event) {
  SyncImGuiMouseState(event);
  SetFocus();
}

void MapCanvas::OnLoseMouse(wxMouseEvent& event) {
  SyncImGuiMouseState(event);
  if (ImGui::GetCurrentContext()) {
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[0] = false;
    io.MouseDown[1] = false;
    io.MouseDown[2] = false;
  }
  event.Skip();
}

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

void MapCanvas::StartPasting() {
  g_gui.StartPasting();
}

void MapCanvas::EndPasting() {
  g_gui.EndPasting();
}

bool MapCanvas::isPasting() const {
  return g_gui.IsPasting();
}

#undef MAPCANVAS_EVENT_STUB

bool MapCanvas::processed[MapCanvas::BLOCK_SIZE * MapCanvas::BLOCK_SIZE] = {
    0};                           // Correctly size the static array
int MapCanvas::countMaxFills = 0; // Definition for static member
MapCanvas::MapCanvas(wxWindow *parent, MapEditor &editor_ref, int *attriblist)
    : wxGLCanvas(parent, wxID_ANY, attriblist, wxDefaultPosition, wxDefaultSize,
                 wxWANTS_CHARS),
      editor(editor_ref), floor(GROUND_LAYER), zoom(1.0), cursor_x(-1),
      cursor_y(-1), dragging(false), boundbox_selection(false), dragging_selection(false), drag_start_map_x(-1), drag_start_map_y(-1), drag_start_map_z(-1),
      screendragging(false), drawing(false), dragging_draw(false),
      replace_dragging(false), rectangle_mode(false),
      rubber_band_mode(false), rubber_start_x(-1), rubber_start_y(-1), rubber_end_x(-1), rubber_end_y(-1),

      screenshot_buffer(nullptr),

      drag_start_x(-1), drag_start_y(-1), drag_start_z(-1),

      last_cursor_map_x(-1), last_cursor_map_y(-1), last_cursor_map_z(-1),

      last_click_map_x(-1), last_click_map_y(-1), last_click_map_z(-1),
      last_click_abs_x(-1), last_click_abs_y(-1), last_click_x(-1),
      last_click_y(-1),

        last_mmb_click_x(-1), last_mmb_click_y(-1) {
      tool_wheel_open = false;
      tool_wheel_sub_menu = 0;
      tool_wheel_x = 0.0f;
      tool_wheel_y = 0.0f;
      tool_wheel_tile_x = -1;
      tool_wheel_tile_y = -1;
      tool_wheel_tile_z = 7;
      minimap_zoom = 1.0f;
      memset(minimap_pixels, 0, sizeof(minimap_pixels));
  popup_menu = newd MapPopupMenu(editor);
  animation_timer = newd AnimationTimer(this);

  drawer = std::make_unique<MapDrawer>(this); // Use unique_ptr
  keyCode = WXK_NONE;

  // Initialisierung der Toolbar mit gespeicherten Werten aus der Config
  int tx = g_settings.getInteger(Config::UI_TOOLBAR_X);
  int ty = g_settings.getInteger(Config::UI_TOOLBAR_Y);
  const float ui_scale = std::max(1.0f, (float)GetContentScaleFactor());
  if (tx <= 0)
    tx = 10;
  if (ty <= 0)
    ty = 10;
  // ui_toolbar wird jetzt in der Member-Initialisierungsliste initialisiert
  ui_toolbar = std::make_unique<RME::UI::UIToolbar>((float)tx, (float)ty,
                                                    45.0f * ui_scale,
                                                    300.0f * ui_scale,
                                                    ui_scale);
  ui_toolbar->addButton("Selection", RME::UI::SVG::ICON_SELECT,
                        [this]() {
                          g_gui.SetFillBrushMode(false);
                          g_gui.SetSelectionMode();
                        });
  ui_toolbar->addButton("Pencil", RME::UI::SVG::ICON_PENCIL,
                        [this]() {
                          g_gui.SetFillBrushMode(false);
                          g_gui.SetDrawingMode();
                        });
  wxString bucket_icon_path = ResolveBucketIconPath();
  if (!bucket_icon_path.empty()) {
    ui_toolbar->addButtonImage("Bucket", bucket_icon_path.ToStdString(), [this]() {
      g_gui.SetDrawingMode();
      g_gui.SetFillBrushMode(!g_gui.IsFillBrushMode());
    });
  } else {
    ui_toolbar->addButton("Bucket", RME::UI::SVG::ICON_BUCKET, [this]() {
      g_gui.SetDrawingMode();
      g_gui.SetFillBrushMode(!g_gui.IsFillBrushMode());
    });
  }
  ui_toolbar->addButton("Eraser", RME::UI::SVG::ICON_ERASER,
                        [this]() {
                          g_gui.SetFillBrushMode(false);
                          g_gui.SelectBrush(g_gui.eraser);
                        });
  wxString border_icon_path = ResolveBorderIconPath();
  if (!border_icon_path.empty()) {
    ui_toolbar->addButtonImage("Autoborder", border_icon_path.ToStdString(), [this]() {
      g_gui.SetFillBrushMode(false);
      g_gui.SelectBrush(g_gui.optional_brush);
    });
  } else {
    ui_toolbar->addButton("Autoborder", RME::UI::SVG::ICON_SELECT, [this]() {
      g_gui.SetFillBrushMode(false);
      g_gui.SelectBrush(g_gui.optional_brush);
    });
  }
  ui_toolbar->addButton("Waypoints", RME::UI::SVG::ICON_WAYPOINT, [this]() {
    g_gui.SetFillBrushMode(false);
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
  if (imgui_context) {
    ImGui::SetCurrentContext(imgui_context);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext(imgui_context);
    imgui_context = nullptr;
  }
}

const float *
MapCanvas::GetProjectionMatrix() const {   // unique_ptr verwaltet dies jetzt
  return drawer->GetCameraProjectionPtr(); // Access through unique_ptr
}

RME::UI::UIToolbar *MapCanvas::GetUIToolbar() { return ui_toolbar.get(); }

// Virtual implementation (base assumes parent is MapWindow)
void MapCanvas::SetZoom(double value, int focus_x, int focus_y) {
  if (value < 0.5) {
    value = 0.5;
  }

  if (value > 10.0) {
    value = 10.0;
  }

  if (zoom != value) {
    if (GetParent()) {
      int scroll_x, scroll_y;
      static_cast<MapWindow *>(GetParent())->GetViewStart(&scroll_x, &scroll_y);

      int fx = focus_x;
      int fy = focus_y;
      if (fx == -1 || fy == -1) {
        int screen_w, screen_h;
        GetSize(&screen_w, &screen_h);
        fx = screen_w / 2;
        fy = screen_h / 2;
      }

      double old_zoom = zoom;
      zoom = value;

      int new_scroll_x = (int)(scroll_x + fx * (old_zoom - zoom));
      int new_scroll_y = (int)(scroll_y + fy * (old_zoom - zoom));

      static_cast<MapWindow *>(GetParent())->Scroll(new_scroll_x, new_scroll_y, false);
      ChangeFloor(floor);
    } else {
      zoom = value;
    }

    UpdatePositionStatus();
    UpdateZoomStatus();
    last_minimap_update_time = 0;
    Refresh();
  }
}

void MapCanvas::UpdateKineticScroll() {
  if (!is_kinetic_scrolling) return;

  unsigned int current_time = wxGetLocalTimeMillis().GetValue();
  unsigned int dt = current_time - last_kinetic_time;
  if (dt == 0) return;

  if (dt > 100) dt = 100;
  last_kinetic_time = current_time;

  MapWindow* map_win = static_cast<MapWindow*>(GetParent());
  if (map_win) {
    int scroll_x, scroll_y;
    map_win->GetViewStart(&scroll_x, &scroll_y);

    int dx = (int)std::round(kinetic_velocity_x * dt);
    int dy = (int)std::round(kinetic_velocity_y * dt);

    if (dx != 0 || dy != 0) {
      map_win->Scroll(scroll_x + dx, scroll_y + dy, false);
    }

    double friction = std::pow(0.92, (double)dt / 16.0);
    kinetic_velocity_x *= friction;
    kinetic_velocity_y *= friction;

    double speed = std::sqrt(kinetic_velocity_x * kinetic_velocity_x + kinetic_velocity_y * kinetic_velocity_y);
    if (speed < 0.05) {
      is_kinetic_scrolling = false;
    }
    Refresh();
  } else {
    is_kinetic_scrolling = false;
  }
}

void MapCanvas::UpdateSmoothZoom() {
  if (!is_smooth_zooming) return;

  double zoom_diff = target_zoom - zoom;
  if (std::abs(zoom_diff) < 0.005) {
    SetZoom(target_zoom, zoom_focus_x, zoom_focus_y);
    is_smooth_zooming = false;
    return;
  }

  double next_zoom = zoom + zoom_diff * 0.20;
  SetZoom(next_zoom, zoom_focus_x, zoom_focus_y);
}

bool MapCanvas::IsAnimating() const {
  MapWindow* parent_win = static_cast<MapWindow*>(GetParent());
  bool scrolling = is_kinetic_scrolling || is_smooth_zooming || (parent_win && parent_win->is_smooth_scrolling);
  if (scrolling) return true;
  if (hud_notification_time_ms != 0 && (wxGetLocalTimeMillis().GetValue() - hud_notification_time_ms < 3000)) return true;
  return false;
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

void MapCanvas::UpdatePositionStatus(int /*x*/, int /*y*/) {
  // [PERF] Use cached map coordinates directly instead of redundant ScreenToMap.
  // The x and y arguments from OnMouseMove were actually already map coordinates,
  // causing a bug where ScreenToMap was called on map coordinates!
  int map_x = last_cursor_map_x;
  int map_y = last_cursor_map_y;

  wxString ss;
  ss << "x: " << map_x << " y:" << map_y << " z:" << floor;
  
  static wxString last_ss_pos;
  if (last_ss_pos != ss) {
      g_gui.root->SetStatusText(ss, 2);
      last_ss_pos = ss;
  }

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
        ss << " uid:" << item->getUniqueID();
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

  static wxString last_ss_info;
  if (last_ss_info != ss) {
      g_gui.root->SetStatusText(ss, 1);
      last_ss_info = ss;
  }
}

void MapCanvas::UpdateZoomStatus() {
  int percentage = static_cast<int>(std::round(100.0 / zoom));
  if (percentage < 1)
    percentage = 1;
  wxString ss;
  ss << "zoom: " << percentage << "%";
  g_gui.root->SetStatusText(ss, 3);
}







void MapCanvas::getTilesToDraw(int mouse_map_x, int mouse_map_y, int floor,
                               PositionVector *tilestodraw,
                               PositionVector *tilestoborder,
                               bool fill /*= false*/) {
  const int map_width = editor.map.getWidth();
  const int map_height = editor.map.getHeight();

  if (fill) {
    Brush *brush = g_gui.GetCurrentBrush();
    bool is_eraser = brush && (brush == g_gui.eraser || brush->asEraser() != nullptr);
    if (!brush || (!brush->isGround() && !brush->isWall() && !is_eraser)) {
      return;
    }

    bool is_wall = brush->isWall();
    GroundBrush *newBrush = (is_wall || is_eraser) ? nullptr : brush->asGround();
    Position start(mouse_map_x, mouse_map_y, floor);
    if (start.x <= 0 || start.y <= 0 || start.x >= map_width || start.y >= map_height) {
      return;
    }

    Tile *start_tile = editor.map.getTile(start);
    GroundBrush *oldBrush = start_tile ? start_tile->getGroundBrush() : nullptr;

    if (!is_wall && !is_eraser && oldBrush && newBrush && oldBrush->getID() == newBrush->getID()) {
      return;
    }

    int min_x = 1;
    int min_y = 1;
    int max_x = map_width - 1;
    int max_y = map_height - 1;

    auto is_mountain_ground_tile = [&](Tile* t) -> bool {
      if (!t || !t->ground) return false;
      if (t->getGroundBrush()) {
        const std::string name = as_lower_str(t->getGroundBrush()->getName());
        if (name.find("mountain") != std::string::npos || name.find("cliff") != std::string::npos || name.find("rock") != std::string::npos) {
          return true;
        }
      }
      if (t->ground->typeExists()) {
        const std::string name = as_lower_str(t->ground->getName());
        if (name.find("mountain") != std::string::npos || name.find("cliff") != std::string::npos) {
          return true;
        }
      }
      return false;
    };

    const bool fill_empty = (start_tile == nullptr || start_tile->ground == nullptr);
    bool fill_mountain_top = false;
    if (fill_empty && floor < MAP_LAYERS - 1) {
      Tile* lower_start = editor.map.getTile(start.x, start.y, floor + 1);
      fill_mountain_top = is_mountain_ground_tile(lower_start);
    }

    if (fill_empty && !fill_mountain_top) {
      int screen_start_x, screen_start_y;
      int screen_end_x, screen_end_y;
      ScreenToMap(0, 0, &screen_start_x, &screen_start_y);
      ScreenToMap(GetSize().GetWidth(), GetSize().GetHeight(), &screen_end_x, &screen_end_y);

      min_x = std::max(1, screen_start_x - 20);
      min_y = std::max(1, screen_start_y - 20);
      max_x = std::min(map_width - 1, screen_end_x + 20);
      max_y = std::min(map_height - 1, screen_end_y + 20);
    }

    const size_t max_fill_tiles = 1000000;

    auto encode = [](int x, int y, int z) -> uint64_t {
      return (static_cast<uint64_t>(z) << 48) |
             (static_cast<uint64_t>(x & 0xFFFFFF) << 24) |
             static_cast<uint64_t>(y & 0xFFFFFF);
    };

    auto get_ground_id = [](Tile *tile) -> uint32_t {
      if (!tile) return 0;
      if (tile->getGroundBrush()) return tile->getGroundBrush()->getID();
      if (tile->ground) return tile->ground->getID();
      return 0;
    };

    const uint32_t source_ground_id = get_ground_id(start_tile);
    std::queue<Position> queue;
    std::unordered_set<uint64_t> visited;
    std::vector<Position> temp_tiles;

    auto is_visited = [&](int x, int y) -> bool {
      return visited.find(encode(x, y, floor)) != visited.end();
    };
    auto set_visited = [&](int x, int y) {
      visited.insert(encode(x, y, floor));
    };

    queue.push(start);
    set_visited(start.x, start.y);

    auto tile_has_wall = [&](Tile* t) -> bool {
      if (!t) return false;
      for (Item* item : t->items) {
        if (item && (item->isWall() || g_items[item->getID()].isWall)) return true;
      }
      return false;
    };

    while (!queue.empty() && temp_tiles.size() < max_fill_tiles) {
      const Position current = queue.front();
      queue.pop();

      Tile *tile = editor.map.getTile(current);
      bool tile_has_ground = (tile != nullptr && tile->ground != nullptr);

      if (is_wall && tile_has_wall(tile)) {
        continue;
      }

      if (fill_mountain_top) {
        if (tile_has_ground) {
          continue;
        }
        Tile* lower_tile = editor.map.getTile(current.x, current.y, current.z + 1);
        if (!is_mountain_ground_tile(lower_tile)) {
          continue;
        }
      } else if (fill_empty) {
        if (tile_has_ground) {
          continue;
        }
      } else if (!tile_has_ground || get_ground_id(tile) != source_ground_id) {
        continue;
      }

      temp_tiles.push_back(current);

      static const int dx[4] = {-1, 1, 0, 0};
      static const int dy[4] = {0, 0, -1, 1};
      for (int i = 0; i < 4; ++i) {
        int nx = current.x + dx[i];
        int ny = current.y + dy[i];
        if (nx >= min_x && nx <= max_x && ny >= min_y && ny <= max_y) {
          if (!is_visited(nx, ny)) {
            Tile* neighbor_tile = editor.map.getTile(nx, ny, current.z);
            if (is_wall && tile_has_wall(neighbor_tile)) {
              set_visited(nx, ny);
              continue;
            }
            set_visited(nx, ny);
            if (fill_mountain_top) {
              Tile* neighbor_lower = editor.map.getTile(nx, ny, current.z + 1);
              if (is_mountain_ground_tile(neighbor_lower)) {
                queue.push(Position(nx, ny, current.z));
              }
            } else {
              queue.push(Position(nx, ny, current.z));
            }
          }
        }
      }
    }

    if (!is_wall) {
      if (tilestodraw->empty()) {
        *tilestodraw = std::move(temp_tiles);
      } else {
        tilestodraw->insert(tilestodraw->end(), temp_tiles.begin(), temp_tiles.end());
      }
    } else {
      std::unordered_set<uint64_t> component_set;
      component_set.reserve(temp_tiles.size());
      for (const auto& pos : temp_tiles) {
        component_set.insert(encode(pos.x, pos.y, pos.z));
      }

      std::unordered_set<uint64_t> wall_coords;
      for (const auto& pos : temp_tiles) {
        Tile* t_north = editor.map.getTile(pos.x, pos.y - 1, pos.z);
        Tile* t_south = editor.map.getTile(pos.x, pos.y + 1, pos.z);
        Tile* t_west  = editor.map.getTile(pos.x - 1, pos.y, pos.z);
        Tile* t_east  = editor.map.getTile(pos.x + 1, pos.y, pos.z);

        bool no_north = (pos.y <= 0 || tile_has_wall(t_north) || component_set.find(encode(pos.x, pos.y - 1, pos.z)) == component_set.end());
        bool no_south = (pos.y >= map_height - 1 || tile_has_wall(t_south) || component_set.find(encode(pos.x, pos.y + 1, pos.z)) == component_set.end());
        bool no_west  = (pos.x <= 0 || tile_has_wall(t_west) || component_set.find(encode(pos.x - 1, pos.y, pos.z)) == component_set.end());
        bool no_east  = (pos.x >= map_width - 1 || tile_has_wall(t_east) || component_set.find(encode(pos.x + 1, pos.y, pos.z)) == component_set.end());

        // North wall on the outer tile (pos.x, pos.y - 1)
        if (no_north && pos.y > 0) {
          wall_coords.insert(encode(pos.x, pos.y - 1, pos.z));
        }
        // West wall on the outer tile (pos.x - 1, pos.y)
        if (no_west && pos.x > 0) {
          wall_coords.insert(encode(pos.x - 1, pos.y, pos.z));
        }
        // South wall on the outer tile (pos.x, pos.y + 1)
        if (no_south && pos.y + 1 < map_height) {
          wall_coords.insert(encode(pos.x, pos.y + 1, pos.z));
        }
        // East wall on the outer tile (pos.x + 1, pos.y)
        if (no_east && pos.x + 1 < map_width) {
          wall_coords.insert(encode(pos.x + 1, pos.y, pos.z));
        }
        // North-West outer corner on (pos.x - 1, pos.y - 1)
        if (no_north && no_west && pos.x > 0 && pos.y > 0) {
          wall_coords.insert(encode(pos.x - 1, pos.y - 1, pos.z));
        }
        // North-East outer corner on (pos.x + 1, pos.y - 1)
        if (no_north && no_east && pos.x + 1 < map_width && pos.y > 0) {
          wall_coords.insert(encode(pos.x + 1, pos.y - 1, pos.z));
        }
        // South-West outer corner on (pos.x - 1, pos.y + 1)
        if (no_south && no_west && pos.x > 0 && pos.y + 1 < map_height) {
          wall_coords.insert(encode(pos.x - 1, pos.y + 1, pos.z));
        }
        // South-East outer corner on (pos.x + 1, pos.y + 1)
        if (no_south && no_east && pos.x + 1 < map_width && pos.y + 1 < map_height) {
          wall_coords.insert(encode(pos.x + 1, pos.y + 1, pos.z));
        }
      }

      for (uint64_t code : wall_coords) {
        int x = static_cast<int>((code >> 24) & 0xFFFFFF);
        int y = static_cast<int>(code & 0xFFFFFF);
        int z = static_cast<int>(code >> 48);

        Tile* existing_tile = editor.map.getTile(x, y, z);
        if (!tile_has_wall(existing_tile)) {
          tilestodraw->push_back(Position(x, y, z));
        }
      }
    }

    if (tilestoborder && !tilestodraw->empty()) {
      std::unordered_set<uint64_t> border_positions;
      border_positions.reserve(tilestodraw->size() * 3);

      for (const Position &pos : *tilestodraw) {
        const Position neighbors[] = {
            Position(pos.x, pos.y, pos.z),
            Position(pos.x - 1, pos.y, pos.z),
            Position(pos.x + 1, pos.y, pos.z),
            Position(pos.x, pos.y - 1, pos.z),
            Position(pos.x, pos.y + 1, pos.z),
            Position(pos.x - 1, pos.y - 1, pos.z),
            Position(pos.x + 1, pos.y - 1, pos.z),
            Position(pos.x - 1, pos.y + 1, pos.z),
            Position(pos.x + 1, pos.y + 1, pos.z)};

        for (const Position &neighbor : neighbors) {
          if (neighbor.x <= 0 || neighbor.y <= 0 || neighbor.x >= map_width ||
              neighbor.y >= map_height) {
            continue;
          }

          const uint64_t key = encode(neighbor.x, neighbor.y, neighbor.z);
          if (border_positions.insert(key).second) {
            tilestoborder->push_back(neighbor);
          }
        }
      }
    }

  } else {
    Brush *brush = g_gui.GetCurrentBrush();
    bool is_wall = brush && brush->isWall();
    int brush_size = (brush && brush->isCreature()) ? 0 : g_gui.GetBrushSize();

    for (int y = -brush_size; y <= brush_size; y++) {
      for (int x = -brush_size; x <= brush_size; x++) {
        int tx = mouse_map_x + x;
        int ty = mouse_map_y + y;
        if (tx <= 0 || ty <= 0 || tx >= map_width || ty >= map_height) {
          continue;
        }

        if (g_gui.GetBrushShape() == BRUSHSHAPE_SQUARE) {
          bool is_border = (brush_size == 0) || (std::abs(x) == brush_size || std::abs(y) == brush_size);
          if (!is_wall || is_border) {
            if (tilestodraw) {
              tilestodraw->push_back(Position(tx, ty, floor));
            }
          }
        } else if (g_gui.GetBrushShape() == BRUSHSHAPE_CIRCLE) {
          double distance = sqrt(double(x * x) + double(y * y));
          if (distance < brush_size + 0.005) {
            bool is_border = (brush_size == 0);
            if (!is_border && is_wall) {
              static const int dx4[] = {-1, 1, 0, 0};
              static const int dy4[] = {0, 0, -1, 1};
              for (int i = 0; i < 4; ++i) {
                int nx = x + dx4[i];
                int ny = y + dy4[i];
                if (sqrt(double(nx * nx) + double(ny * ny)) >= brush_size + 0.005) {
                  is_border = true;
                  break;
                }
              }
            }
            if (!is_wall || is_border) {
              if (tilestodraw) {
                tilestodraw->push_back(Position(tx, ty, floor));
              }
            }
          }
        }
      }
    }

    if (tilestoborder && tilestodraw && !tilestodraw->empty()) {
      std::unordered_set<uint64_t> border_set;
      border_set.reserve(tilestodraw->size() * 9);
      for (const Position &pos : *tilestodraw) {
        for (int dy = -1; dy <= 1; ++dy) {
          for (int dx = -1; dx <= 1; ++dx) {
            int bx = pos.x + dx;
            int by = pos.y + dy;
            if (bx > 0 && by > 0 && bx < map_width && by < map_height) {
              uint64_t key = (static_cast<uint64_t>(pos.z) << 48) |
                             (static_cast<uint64_t>(bx & 0xFFFFFF) << 24) |
                             static_cast<uint64_t>(by & 0xFFFFFF);
              if (border_set.insert(key).second) {
                tilestoborder->push_back(Position(bx, by, pos.z));
              }
            }
          }
        }
      }
    }
  }

}

void MapCanvas::ExecuteMagicWandSelect(int mouse_map_x, int mouse_map_y, int floor, bool add_to_selection) {
  const int map_width = editor.map.getWidth();
  const int map_height = editor.map.getHeight();

  Position start(mouse_map_x, mouse_map_y, floor);
  if (start.x <= 0 || start.y <= 0 || start.x >= map_width || start.y >= map_height) {
    return;
  }

  int min_x = 1, min_y = 1, max_x = map_width - 1, max_y = map_height - 1;
  int screen_w = 0, screen_h = 0;
  GetClientSize(&screen_w, &screen_h);
  if (screen_w > 0 && screen_h > 0) {
    int vis_min_x = 0, vis_min_y = 0, vis_max_x = 0, vis_max_y = 0;
    ScreenToMap(0, 0, &vis_min_x, &vis_min_y);
    ScreenToMap(screen_w, screen_h, &vis_max_x, &vis_max_y);

    int v_left = std::min(vis_min_x, vis_max_x);
    int v_right = std::max(vis_min_x, vis_max_x);
    int v_top = std::min(vis_min_y, vis_max_y);
    int v_bottom = std::max(vis_min_y, vis_max_y);

    min_x = std::max(1, v_left - 5);
    min_y = std::max(1, v_top - 5);
    max_x = std::min(map_width - 1, v_right + 5);
    max_y = std::min(map_height - 1, v_bottom + 5);
  }

  min_x = std::min(min_x, start.x);
  max_x = std::max(max_x, start.x);
  min_y = std::min(min_y, start.y);
  max_y = std::max(max_y, start.y);

  const size_t range_w = static_cast<size_t>(max_x - min_x + 1);
  const size_t range_h = static_cast<size_t>(max_y - min_y + 1);
  const size_t max_fill_tiles = range_w * range_h;

  Tile *start_tile = editor.map.getTile(start);

  auto get_ground_id = [](Tile *tile) -> uint32_t {
    if (!tile) return 0;
    if (tile->getGroundBrush()) return tile->getGroundBrush()->getID();
    if (tile->ground) return tile->ground->getID();
    return 0;
  };

  auto get_top_item_id = [](Tile *tile) -> uint16_t {
    if (!tile) return 0;
    Item* top = tile->getTopItem();
    return top ? top->getID() : 0;
  };

  const bool fill_empty = (start_tile == nullptr || start_tile->ground == nullptr);
  const uint32_t source_ground_id = get_ground_id(start_tile);
  const uint16_t source_item_id = get_top_item_id(start_tile);

  std::queue<Position> queue;
  std::vector<uint8_t> visited(range_w * range_h, 0);
  std::vector<Tile*> flooded_tiles;

  auto is_visited = [&](int x, int y) -> bool {
    return visited[static_cast<size_t>(y - min_y) * range_w + static_cast<size_t>(x - min_x)] != 0;
  };
  auto set_visited = [&](int x, int y) {
    visited[static_cast<size_t>(y - min_y) * range_w + static_cast<size_t>(x - min_x)] = 1;
  };

  queue.push(start);
  set_visited(start.x, start.y);

  while (!queue.empty() && flooded_tiles.size() < max_fill_tiles) {
    const Position current = queue.front();
    queue.pop();

    Tile *tile = editor.map.getTile(current);
    bool tile_has_ground = (tile != nullptr && tile->ground != nullptr);

    if (fill_empty) {
      if (tile_has_ground) {
        continue;
      }
    } else {
      if (!tile_has_ground) {
        continue;
      }
      if (source_item_id > 0) {
        if (get_top_item_id(tile) != source_item_id) {
          continue;
        }
      } else {
        if (get_ground_id(tile) != source_ground_id) {
          continue;
        }
      }
    }

    if (tile) {
      flooded_tiles.push_back(tile);
    }

    static const int dx[4] = {-1, 1, 0, 0};
    static const int dy[4] = {0, 0, -1, 1};
    for (int i = 0; i < 4; ++i) {
      int nx = current.x + dx[i];
      int ny = current.y + dy[i];
      if (nx >= min_x && nx <= max_x && ny >= min_y && ny <= max_y) {
        if (!is_visited(nx, ny)) {
          set_visited(nx, ny);
          queue.push(Position(nx, ny, current.z));
        }
      }
    }
  }

  Brush* current_brush = g_gui.GetCurrentBrush();
  if (current_brush && current_brush->isWall() && !flooded_tiles.empty() && !fill_empty) {
    // If a WallBrush is active, outline the connected ground perimeter with walls
    std::unordered_set<uint64_t> component_set;
    component_set.reserve(flooded_tiles.size());
    auto encode_pos = [](int x, int y, int z) -> uint64_t {
      return (static_cast<uint64_t>(z) << 48) |
             (static_cast<uint64_t>(x & 0xFFFFFF) << 24) |
              static_cast<uint64_t>(y & 0xFFFFFF);
    };
    for (Tile* t : flooded_tiles) {
      if (t) {
        component_set.insert(encode_pos(t->getX(), t->getY(), t->getZ()));
      }
    }

    PositionVector tilestodraw;
    PositionVector tilestoborder;
    for (Tile* t : flooded_tiles) {
      if (!t) continue;
      int tx = t->getX();
      int ty = t->getY();
      int tz = t->getZ();
      bool is_boundary = false;
      Position neighbors[4] = {
        Position(tx - 1, ty, tz),
        Position(tx + 1, ty, tz),
        Position(tx, ty - 1, tz),
        Position(tx, ty + 1, tz)
      };
      for (int ni = 0; ni < 4; ++ni) {
        const Position& neighbor = neighbors[ni];
        if (neighbor.x <= 0 || neighbor.y <= 0 || neighbor.x >= map_width || neighbor.y >= map_height) {
          is_boundary = true;
          break;
        }
        if (component_set.find(encode_pos(neighbor.x, neighbor.y, neighbor.z)) == component_set.end()) {
          is_boundary = true;
          break;
        }
      }
      if (is_boundary) {
        tilestodraw.push_back(Position(tx, ty, tz));
      }
    }

    if (!tilestodraw.empty()) {
      editor.selection.clear();
      editor.draw(tilestodraw, tilestoborder, false);
      markDirty();
      Refresh();
      return;
    }
  }

  if (!add_to_selection) {
    editor.selection.clear();
  }

  editor.selection.start(Selection::INTERNAL);
  for (Tile* t : flooded_tiles) {
    if (t) {
      if (source_item_id > 0) {
        Item* top = t->getTopItem();
        if (top) editor.selection.add(t, top);
      } else if (t->ground) {
        editor.selection.add(t, t->ground);
      } else {
        editor.selection.add(t);
      }
    }
  }
  editor.selection.finish(Selection::INTERNAL);

  if (editor.selection.size() > 0) {
    editor.copybuffer.copy(editor, floor);
  }
  markDirty();
  Refresh();
}

void MapCanvas::ReplaceSelectionWithBrush(Brush* brush) {
  if (!brush || editor.selection.empty()) return;

  PositionVector tilestodraw;
  PositionVector tilestoborder;

  for (Tile* t : editor.selection.getTiles()) {
    if (t) {
      tilestodraw.push_back(t->getPosition());
    }
  }

  if (!tilestodraw.empty()) {
    if (!brush->isGround()) {
      editor.destroySelection();
    } else {
      editor.selection.clear();
    }

    g_gui.SelectBrush(brush);
    editor.draw(tilestodraw, tilestoborder, false);

    if (brush->isWall()) {
      for (const Position& pos : tilestodraw) {
        Tile* t = editor.map.getTile(pos);
        if (t) {
          WallBrush::doWalls(&editor.map, t);
        }
      }
    }

    markDirty();
    Refresh();
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

void AnimationTimer::Notify() {
    if (!map_canvas || !map_canvas->IsShownOnScreen()) {
        return;
    }

    if (map_canvas->GetParent()) {
        static_cast<MapWindow*>(map_canvas->GetParent())->UpdateSmoothScroll();
    }
    
    map_canvas->UpdateKineticScroll();
    map_canvas->UpdateSmoothZoom();
    
    bool is_dirty = map_canvas->isDirty();
    if (is_dirty) {
        map_canvas->clearDirty();
    }
    if (is_dirty || map_canvas->IsAnimating()) {
        map_canvas->Refresh(false);
    }
}

void AnimationTimer::Start() {
  if (!started) {
    started = true;
    wxTimer::Start(16);
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

  if (IsShownOnScreen()) {
    SetCurrent(*g_gui.GetGLContext(this));
  }

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

  int span_w = (int)(180.0f * minimap_zoom);
  int span_h = (int)(180.0f * minimap_zoom);
  int map_width = editor.map.getWidth();
  int map_height = editor.map.getHeight();

  int start_x;
  if (span_w >= map_width) {
    start_x = (map_width - span_w) / 2;
  } else {
    start_x = std::max(0, std::min(center_x - span_w / 2, map_width - span_w));
  }

  int start_y;
  if (span_h >= map_height) {
    start_y = (map_height - span_h) / 2;
  } else {
    start_y = std::max(0, std::min(center_y - span_h / 2, map_height - span_h));
  }

  minimap_start_x = start_x;
  minimap_start_y = start_y;
  minimap_span_w = std::max(1, span_w);
  minimap_span_h = std::max(1, span_h);

  static uint8_t tex_data[180 * 180 * 3];
  memset(tex_data, 0, sizeof(tex_data));

  if (g_gui.IsRenderingEnabled()) {
    for (int window_y = 0; window_y < 180; ++window_y) {
      for (int window_x = 0; window_x < 180; ++window_x) {
        int x = start_x + (int)(window_x * ((double)span_w / 180.0));
        int y = start_y + (int)(window_y * ((double)span_h / 180.0));
        if (x >= 0 && x < map_width && y >= 0 && y < map_height) {
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
  }

  // Copy into minimap_pixels so the palette-docked minimap can read it via wxImage
  memcpy(minimap_pixels, tex_data, sizeof(tex_data));

  glBindTexture(GL_TEXTURE_2D, minimap_tex_id);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 180, 180, GL_RGB, GL_UNSIGNED_BYTE,
                  tex_data);
}

void MapCanvas::OnIdle(wxIdleEvent& event) {
  if (!IsShownOnScreen()) {
    return;
  }

  static auto last_frame_time = std::chrono::steady_clock::now();
  auto now = std::chrono::steady_clock::now();
  double dt = std::chrono::duration<double>(now - last_frame_time).count();

  if (editor.IsLiveClient()) {
    editor.GetLiveClient()->checkInactivity();
    editor.GetLiveClient()->sendHeartbeat();
  }

  bool wants_continuous = true;

  if (wants_continuous) {
    if (isDirty()) {
      clearDirty();
    }
    Refresh(false);
    last_frame_time = std::chrono::steady_clock::now();
    event.RequestMore(true);
  }
}

void MapCanvas::OnQuickPing(wxCommandEvent &event) {
  int map_x = last_click_map_x;
  int map_y = last_click_map_y;
  Position pos(map_x, map_y, floor);

  PingFeedback feedback;
  feedback.pos = pos;
  feedback.start_time_ms = wxGetLocalTimeMillis().GetValue();
  active_pings.push_back(feedback);

  if (editor.IsLiveClient()) {
    editor.GetLiveClient()->sendPing(pos);
  } else if (editor.IsLiveServer()) {
    LivePing ping;
    ping.pos = pos;
    ping.senderId = 0;
    ping.senderName = editor.GetLiveServer()->getName();
    ping.color = *wxGREEN;
    ping.timestamp = wxGetLocalTimeMillis().GetValue();
    editor.GetLiveServer()->broadcastPing(ping);
  }
  g_gui.SetStatusText(wxString::Format("Quick Ping placed at (%d, %d, %d)", pos.x, pos.y, pos.z));
  Refresh();
}

void MapCanvas::OnAddAnnotation(wxCommandEvent &event) {
  int map_x = last_click_map_x;
  int map_y = last_click_map_y;
  Position pos(map_x, map_y, floor);

  CallAfter([this, pos]() {
    wxTextEntryDialog dialog(this, "Enter note text for this map position:", "Add Map Note");
    if (dialog.ShowModal() == wxID_OK) {
      wxString text = dialog.GetValue();
      if (!text.IsEmpty()) {
        static uint32_t nextId = 1;
        MapEditor::MapNote note;
        note.id = nextId++;
        note.pos = pos;
        note.text = text;
        note.author = editor.IsLive() ? editor.GetLiveServer()->getName() : wxString("Mapper");
        editor.map_notes.push_back(note);

        if (editor.IsLiveClient()) {
          editor.GetLiveClient()->sendAddAnnotation(pos, text);
        } else if (editor.IsLiveServer()) {
          MapAnnotation annotation;
          annotation.id = note.id;
          annotation.pos = pos;
          annotation.text = text;
          annotation.author = note.author;
          annotation.color = *wxGREEN;
          editor.GetLiveServer()->broadcastAnnotation(annotation, false);
        }
        g_gui.SetStatusText(wxString::Format("Added Note at (%d, %d, %d): %s", pos.x, pos.y, pos.z, text));
        Refresh();
      }
    }
  });
}




