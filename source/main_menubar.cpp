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

#include "main_menubar.h"
#include "application.h"
#include "preferences.h"
#include "about_window.h"
#include "dat_debug_view.h"
#include "result_window.h"
#include "extension_window.h"
#include "find_item_window.h"
#include "settings.h"

#include "gui.h"

#include <wx/chartype.h>

#include "editor.h"
#include "materials.h"
#include "live_client.h"
#include "live_server.h"
#include "lua/lua_script_manager.h"
#include "lua/lua_scripts_window.h"
#include "otc_export.h"

BEGIN_EVENT_TABLE(MainMenuBar, wxEvtHandler)
END_EVENT_TABLE()

MainMenuBar::MainMenuBar(MainFrame* frame) :
	frame(frame) {
	using namespace MenuBar;
	checking_programmaticly = false;

#define MAKE_ACTION(id, kind, handler) actions[#id] = new MenuBar::Action(#id, id, kind, wxCommandEventFunction(&MainMenuBar::handler))
#define MAKE_SET_ACTION(id, kind, setting_, handler)                                                  \
	actions[#id] = new MenuBar::Action(#id, id, kind, wxCommandEventFunction(&MainMenuBar::handler)); \
	actions[#id].setting = setting_

	MAKE_ACTION(NEW, wxITEM_NORMAL, OnNew);
	MAKE_ACTION(OPEN, wxITEM_NORMAL, OnOpen);
	MAKE_ACTION(SAVE, wxITEM_NORMAL, OnSave);
	MAKE_ACTION(SAVE_AS, wxITEM_NORMAL, OnSaveAs);
	MAKE_ACTION(GENERATE_MAP, wxITEM_NORMAL, OnGenerateMap);
	MAKE_ACTION(CLOSE, wxITEM_NORMAL, OnClose);

	MAKE_ACTION(IMPORT_MAP, wxITEM_NORMAL, OnImportMap);
	MAKE_ACTION(IMPORT_MONSTERS, wxITEM_NORMAL, OnImportMonsterData);
	MAKE_ACTION(IMPORT_MINIMAP, wxITEM_NORMAL, OnImportMinimap);
	MAKE_ACTION(EXPORT_MINIMAP, wxITEM_NORMAL, OnExportMinimap);
	MAKE_ACTION(EXPORT_TILESETS, wxITEM_NORMAL, OnExportTilesets);
	MAKE_ACTION(EXPORT_OTCLIENT, wxITEM_NORMAL, OnExportOTClient);

	MAKE_ACTION(RELOAD_DATA, wxITEM_NORMAL, OnReloadDataFiles);
	// MAKE_ACTION(RECENT_FILES, wxITEM_NORMAL, OnRecent);
	MAKE_ACTION(PREFERENCES, wxITEM_NORMAL, OnPreferences); // Label wird via XML/Update auf "Settings" gesetzt
	MAKE_ACTION(EXIT, wxITEM_NORMAL, OnQuit);

	MAKE_ACTION(UNDO, wxITEM_NORMAL, OnUndo);
	MAKE_ACTION(REDO, wxITEM_NORMAL, OnRedo);

	MAKE_ACTION(FIND_ITEM, wxITEM_NORMAL, OnSearchForItem);
	MAKE_ACTION(REPLACE_ITEMS, wxITEM_NORMAL, OnReplaceItems);
	MAKE_ACTION(SEARCH_ON_MAP_EVERYTHING, wxITEM_NORMAL, OnSearchForStuffOnMap);
	MAKE_ACTION(SEARCH_ON_MAP_UNIQUE, wxITEM_NORMAL, OnSearchForUniqueOnMap);
	MAKE_ACTION(SEARCH_ON_MAP_ACTION, wxITEM_NORMAL, OnSearchForActionOnMap);
	MAKE_ACTION(SEARCH_ON_MAP_CONTAINER, wxITEM_NORMAL, OnSearchForContainerOnMap);
	MAKE_ACTION(SEARCH_ON_MAP_WRITEABLE, wxITEM_NORMAL, OnSearchForWriteableOnMap);
	MAKE_ACTION(SEARCH_ON_SELECTION_EVERYTHING, wxITEM_NORMAL, OnSearchForStuffOnSelection);
	MAKE_ACTION(SEARCH_ON_SELECTION_UNIQUE, wxITEM_NORMAL, OnSearchForUniqueOnSelection);
	MAKE_ACTION(SEARCH_ON_SELECTION_ACTION, wxITEM_NORMAL, OnSearchForActionOnSelection);
	MAKE_ACTION(SEARCH_ON_SELECTION_CONTAINER, wxITEM_NORMAL, OnSearchForContainerOnSelection);
	MAKE_ACTION(SEARCH_ON_SELECTION_WRITEABLE, wxITEM_NORMAL, OnSearchForWriteableOnSelection);
	MAKE_ACTION(SEARCH_ON_SELECTION_ITEM, wxITEM_NORMAL, OnSearchForItemOnSelection);
	MAKE_ACTION(REPLACE_ON_SELECTION_ITEMS, wxITEM_NORMAL, OnReplaceItemsOnSelection);
	MAKE_ACTION(REMOVE_ON_SELECTION_ITEM, wxITEM_NORMAL, OnRemoveItemOnSelection);
	MAKE_ACTION(SELECT_MODE_COMPENSATE, wxITEM_RADIO, OnSelectionTypeChange);
	MAKE_ACTION(SELECT_MODE_LOWER, wxITEM_RADIO, OnSelectionTypeChange);
	MAKE_ACTION(SELECT_MODE_CURRENT, wxITEM_RADIO, OnSelectionTypeChange);
	MAKE_ACTION(SELECT_MODE_VISIBLE, wxITEM_RADIO, OnSelectionTypeChange);

	MAKE_ACTION(AUTOMAGIC, wxITEM_CHECK, OnToggleAutomagic);
	MAKE_ACTION(BORDERIZE_SELECTION, wxITEM_NORMAL, OnBorderizeSelection);
	MAKE_ACTION(BORDERIZE_MAP, wxITEM_NORMAL, OnBorderizeMap);
	MAKE_ACTION(RANDOMIZE_SELECTION, wxITEM_NORMAL, OnRandomizeSelection);
	MAKE_ACTION(RANDOMIZE_MAP, wxITEM_NORMAL, OnRandomizeMap);
	MAKE_ACTION(GOTO_PREVIOUS_POSITION, wxITEM_NORMAL, OnGotoPreviousPosition);
	MAKE_ACTION(GOTO_POSITION, wxITEM_NORMAL, OnGotoPosition);
	MAKE_ACTION(JUMP_TO_BRUSH, wxITEM_NORMAL, OnJumpToBrush);
	MAKE_ACTION(JUMP_TO_ITEM_BRUSH, wxITEM_NORMAL, OnJumpToItemBrush);

	MAKE_ACTION(CUT, wxITEM_NORMAL, OnCut);
	MAKE_ACTION(COPY, wxITEM_NORMAL, OnCopy);
	MAKE_ACTION(PASTE, wxITEM_NORMAL, OnPaste);

	MAKE_ACTION(EDIT_TOWNS, wxITEM_NORMAL, OnMapEditTowns);
	MAKE_ACTION(EDIT_ITEMS, wxITEM_NORMAL, OnMapEditItems);
	MAKE_ACTION(EDIT_MONSTERS, wxITEM_NORMAL, OnMapEditMonsters);

	MAKE_ACTION(CLEAR_INVALID_HOUSES, wxITEM_NORMAL, OnClearHouseTiles);
	MAKE_ACTION(CLEAR_MODIFIED_STATE, wxITEM_NORMAL, OnClearModifiedState);
	MAKE_ACTION(MAP_REMOVE_ITEMS, wxITEM_NORMAL, OnMapRemoveItems);
	MAKE_ACTION(MAP_REMOVE_CORPSES, wxITEM_NORMAL, OnMapRemoveCorpses);
	MAKE_ACTION(MAP_REMOVE_UNREACHABLE_TILES, wxITEM_NORMAL, OnMapRemoveUnreachable);
	MAKE_ACTION(MAP_CLEANUP, wxITEM_NORMAL, OnMapCleanup);
	MAKE_ACTION(MAP_CLEAN_HOUSE_ITEMS, wxITEM_NORMAL, OnMapCleanHouseItems);
	MAKE_ACTION(MAP_PROPERTIES, wxITEM_NORMAL, OnMapProperties);
	MAKE_ACTION(MAP_STATISTICS, wxITEM_NORMAL, OnMapStatistics);

	MAKE_ACTION(VIEW_TOOLBARS_BRUSHES, wxITEM_CHECK, OnToolbars);
	MAKE_ACTION(VIEW_TOOLBARS_POSITION, wxITEM_CHECK, OnToolbars);
	MAKE_ACTION(VIEW_TOOLBARS_SIZES, wxITEM_CHECK, OnToolbars);
	MAKE_ACTION(NEW_VIEW, wxITEM_NORMAL, OnNewView);
	MAKE_ACTION(TOGGLE_FULLSCREEN, wxITEM_NORMAL, OnToggleFullscreen);

	MAKE_ACTION(ZOOM_IN, wxITEM_NORMAL, OnZoomIn);
	MAKE_ACTION(ZOOM_OUT, wxITEM_NORMAL, OnZoomOut);
	MAKE_ACTION(ZOOM_NORMAL, wxITEM_NORMAL, OnZoomNormal);

	MAKE_ACTION(SHOW_SHADE, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_ALL_FLOORS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(GHOST_ITEMS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(GHOST_HIGHER_FLOORS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(HIGHLIGHT_ITEMS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(HIGHLIGHT_LOCKED_DOORS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_EXTRA, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_INGAME_BOX, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_LIGHTS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_LIGHT_STR, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_TECHNICAL_ITEMS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_WAYPOINTS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_GRID, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_CREATURES, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_SPAWNS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_SPECIAL, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_AS_MINIMAP, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_ONLY_COLORS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_ONLY_MODIFIED, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_HOUSES, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_PATHING, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_TOOLTIPS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_PREVIEW, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_WALL_HOOKS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_TOWNS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(ALWAYS_SHOW_ZONES, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(EXT_HOUSE_SHADER, wxITEM_CHECK, OnChangeViewSettings);

	MAKE_ACTION(EXPERIMENTAL_FOG, wxITEM_CHECK, OnChangeViewSettings); // experimental

	MAKE_ACTION(NEW_PALETTE, wxITEM_NORMAL, OnNewPalette); // This is not used in the current UI design
	MAKE_ACTION(TAKE_SCREENSHOT, wxITEM_NORMAL, OnTakeScreenshot);

	MAKE_ACTION(LIVE_START, wxITEM_NORMAL, OnStartLive); // Intern verknüpft mit "Host"
	MAKE_ACTION(LIVE_JOIN, wxITEM_NORMAL, OnJoinLive);   // Intern verknüpft mit "Join"
	MAKE_ACTION(LIVE_CLOSE, wxITEM_NORMAL, OnCloseLive); // Intern verknüpft mit "Disconnect"

	MAKE_ACTION(SELECT_TERRAIN, wxITEM_NORMAL, OnSelectTerrainPalette);
	MAKE_ACTION(SELECT_DOODAD, wxITEM_NORMAL, OnSelectDoodadPalette);
	MAKE_ACTION(SELECT_ITEM, wxITEM_NORMAL, OnSelectItemPalette);
	MAKE_ACTION(SELECT_COLLECTION, wxITEM_NORMAL, OnSelectCollectionPalette);
	MAKE_ACTION(SELECT_CREATURE, wxITEM_NORMAL, OnSelectCreaturePalette);
	MAKE_ACTION(SELECT_HOUSE, wxITEM_NORMAL, OnSelectHousePalette);
	MAKE_ACTION(SELECT_WAYPOINT, wxITEM_NORMAL, OnSelectWaypointPalette);
	MAKE_ACTION(SELECT_RAW, wxITEM_NORMAL, OnSelectRawPalette);

	MAKE_ACTION(FLOOR_0, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_1, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_2, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_3, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_4, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_5, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_6, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_7, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_8, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_9, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_10, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_11, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_12, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_13, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_14, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_15, wxITEM_RADIO, OnChangeFloor);

	MAKE_ACTION(DEBUG_VIEW_DAT, wxITEM_NORMAL, OnDebugViewDat);
	MAKE_ACTION(EXTENSIONS, wxITEM_NORMAL, OnListExtensions);
	MAKE_ACTION(GOTO_WEBSITE, wxITEM_NORMAL, OnGotoWebsite);
	MAKE_ACTION(ABOUT, wxITEM_NORMAL, OnAbout);

	// Scripts menu actions
	MAKE_ACTION(SCRIPTS_OPEN_FOLDER, wxITEM_NORMAL, OnScriptsOpenFolder);
	MAKE_ACTION(SCRIPTS_RELOAD, wxITEM_NORMAL, OnScriptsReload);
	MAKE_ACTION(SCRIPTS_MANAGER, wxITEM_NORMAL, OnScriptsManager);

	// A deleter, this way the frame does not need
	// to bother deleting us.
	class CustomMenuBar : public wxMenuBar {
	public:
		CustomMenuBar(MainMenuBar* mb) :
			mb(mb) { }
		~CustomMenuBar() {
			delete mb;
		}

	private:
		MainMenuBar* mb;
	};

	menubar = newd CustomMenuBar(this);
	frame->SetMenuBar(menubar);

	// Tie all events to this handler!

	for (std::map<std::string, MenuBar::Action*>::iterator ai = actions.begin(); ai != actions.end(); ++ai) {
		frame->Connect(MAIN_FRAME_MENU + ai->second->id, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)(wxEventFunction)(ai->second->handler), nullptr, this);
	}
	for (size_t i = 0; i < 10; ++i) {
		frame->Connect(recentFiles.GetBaseId() + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainMenuBar::OnOpenRecent), nullptr, this);
	}

	// Connect script execution events (dynamic range)
	for (int i = SCRIPTS_FIRST; i <= SCRIPTS_LAST; ++i) {
		frame->Connect(MAIN_FRAME_MENU + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainMenuBar::OnScriptExecute), nullptr, this);
	}

	// Connect show overlay toggle events (dynamic range)
	for (int i = SHOW_CUSTOM_FIRST; i <= SHOW_CUSTOM_LAST; ++i) {
		frame->Connect(MAIN_FRAME_MENU + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainMenuBar::OnShowOverlayToggle), nullptr, this);
	}
}

MainMenuBar::~MainMenuBar() {
	// Don't need to delete menubar, it's owned by the frame

	for (std::map<std::string, MenuBar::Action*>::iterator ai = actions.begin(); ai != actions.end(); ++ai) {
		delete ai->second;
	}
}

void MainMenuBar::EnableItem(MenuBar::ActionID id, bool enable) {
	std::map<MenuBar::ActionID, std::list<wxMenuItem*>>::iterator fi = items.find(id);
	if (fi == items.end()) {
		return;
	}

	std::list<wxMenuItem*>& li = fi->second;

	for (std::list<wxMenuItem*>::iterator i = li.begin(); i != li.end(); ++i) {
		(*i)->Enable(enable);
	}
}

void MainMenuBar::CheckItem(MenuBar::ActionID id, bool enable) {
	std::map<MenuBar::ActionID, std::list<wxMenuItem*>>::iterator fi = items.find(id);
	if (fi == items.end()) {
		return;
	}

	std::list<wxMenuItem*>& li = fi->second;

	checking_programmaticly = true;
	for (std::list<wxMenuItem*>::iterator i = li.begin(); i != li.end(); ++i) {
		(*i)->Check(enable);
	}
	checking_programmaticly = false;
}

bool MainMenuBar::IsItemChecked(MenuBar::ActionID id) const {
	std::map<MenuBar::ActionID, std::list<wxMenuItem*>>::const_iterator fi = items.find(id);
	if (fi == items.end()) {
		return false;
	}

	const std::list<wxMenuItem*>& li = fi->second;

	for (std::list<wxMenuItem*>::const_iterator i = li.begin(); i != li.end(); ++i) {
		if ((*i)->IsChecked()) {
			return true;
		}
	}

	return false;
}

void MainMenuBar::Update() {
	using namespace MenuBar;
	// This updates all buttons and sets them to proper enabled/disabled state

	bool enable = !g_gui.IsWelcomeDialogShown();
	menubar->Enable(enable);

	if (showMenu) {
		const auto& shows = g_luaScripts.getMapOverlayShows();
		const size_t maxCount = static_cast<size_t>(MenuBar::SHOW_CUSTOM_LAST - MenuBar::SHOW_CUSTOM_FIRST);
		const size_t desiredCount = std::min(shows.size(), maxCount);
		bool needsReload = showMenuCount != desiredCount;

		if (!needsReload) {
			for (size_t i = 0; i < desiredCount; ++i) {
				wxMenuItem* item = showMenu->FindItem(MAIN_FRAME_MENU + MenuBar::SHOW_CUSTOM_FIRST + static_cast<int>(i));
				if (!item) {
					needsReload = true;
					break;
				}
				wxString label = wxString::FromUTF8(shows[i].label);
				if (item->GetItemLabelText() != label) {
					needsReload = true;
					break;
				}
			}
		}

		if (needsReload) {
			LoadShowMenu();
		} else {
			for (size_t i = 0; i < desiredCount; ++i) {
				wxMenuItem* item = showMenu->FindItem(MAIN_FRAME_MENU + MenuBar::SHOW_CUSTOM_FIRST + static_cast<int>(i));
				if (item) {
					item->Check(g_luaScripts.isMapOverlayEnabled(shows[i].overlayId));
				}
			}
		}
	}

	if (!enable) {
		return;
	}

	Editor* editor = g_gui.GetCurrentEditor();
	if (editor) {
		EnableItem(UNDO, editor->actionQueue->canUndo());
		EnableItem(REDO, editor->actionQueue->canRedo());
		EnableItem(PASTE, editor->copybuffer.canPaste());
	} else {
		EnableItem(UNDO, false);
		EnableItem(REDO, false);
		EnableItem(PASTE, false);
	}

	bool loaded = g_gui.IsVersionLoaded();
	bool has_map = editor != nullptr;
	bool has_selection = editor && editor->hasSelection();
	bool is_live = editor && editor->IsLive();
	bool is_host = has_map && !editor->IsLiveClient();
	bool is_local = has_map && !is_live;

	EnableItem(CLOSE, is_local);
	EnableItem(SAVE, is_host);
	EnableItem(SAVE_AS, is_host);
	EnableItem(GENERATE_MAP, false);

	EnableItem(IMPORT_MAP, is_local);
	EnableItem(IMPORT_MONSTERS, is_local);
	EnableItem(IMPORT_MINIMAP, false);
	EnableItem(EXPORT_MINIMAP, is_local);
	EnableItem(EXPORT_TILESETS, loaded);
	EnableItem(EXPORT_OTCLIENT, is_local);

	EnableItem(FIND_ITEM, is_host);
	EnableItem(REPLACE_ITEMS, is_local);
	EnableItem(SEARCH_ON_MAP_EVERYTHING, is_host);
	EnableItem(SEARCH_ON_MAP_UNIQUE, is_host);
	EnableItem(SEARCH_ON_MAP_ACTION, is_host);
	EnableItem(SEARCH_ON_MAP_CONTAINER, is_host);
	EnableItem(SEARCH_ON_MAP_WRITEABLE, is_host);
	EnableItem(SEARCH_ON_SELECTION_EVERYTHING, has_selection && is_host);
	EnableItem(SEARCH_ON_SELECTION_UNIQUE, has_selection && is_host);
	EnableItem(SEARCH_ON_SELECTION_ACTION, has_selection && is_host);
	EnableItem(SEARCH_ON_SELECTION_CONTAINER, has_selection && is_host);
	EnableItem(SEARCH_ON_SELECTION_WRITEABLE, has_selection && is_host);
	EnableItem(SEARCH_ON_SELECTION_ITEM, has_selection && is_host);
	EnableItem(REPLACE_ON_SELECTION_ITEMS, has_selection && is_host);
	EnableItem(REMOVE_ON_SELECTION_ITEM, has_selection && is_host);

	EnableItem(CUT, has_map);
	EnableItem(COPY, has_map);

	EnableItem(BORDERIZE_SELECTION, has_map && has_selection);
	EnableItem(BORDERIZE_MAP, is_local);
	EnableItem(RANDOMIZE_SELECTION, has_map && has_selection);
	EnableItem(RANDOMIZE_MAP, is_local);

	EnableItem(GOTO_PREVIOUS_POSITION, has_map);
	EnableItem(GOTO_POSITION, has_map);
	EnableItem(JUMP_TO_BRUSH, loaded);
	EnableItem(JUMP_TO_ITEM_BRUSH, loaded);

	EnableItem(MAP_REMOVE_ITEMS, is_host);
	EnableItem(MAP_REMOVE_CORPSES, is_local);
	EnableItem(MAP_REMOVE_UNREACHABLE_TILES, is_local);
	EnableItem(CLEAR_INVALID_HOUSES, is_local);
	EnableItem(CLEAR_MODIFIED_STATE, is_local);

	EnableItem(EDIT_TOWNS, is_local);
	EnableItem(EDIT_ITEMS, false);
	EnableItem(EDIT_MONSTERS, false);

	EnableItem(MAP_CLEANUP, is_local);
	EnableItem(MAP_PROPERTIES, is_local);
	EnableItem(MAP_STATISTICS, is_local);

	EnableItem(NEW_VIEW, has_map);
	EnableItem(ZOOM_IN, has_map);
	EnableItem(ZOOM_OUT, has_map);
	EnableItem(ZOOM_NORMAL, has_map);

	if (has_map) {
		CheckItem(SHOW_SPAWNS, g_settings.getBoolean(Config::SHOW_SPAWNS));
	}

	// EnableItem(WIN_MINIMAP, loaded); // Minimap is now canvas-based, visibility controlled by config
	EnableItem(NEW_PALETTE, loaded);
	EnableItem(SELECT_TERRAIN, loaded);
	EnableItem(SELECT_DOODAD, loaded);
	EnableItem(SELECT_ITEM, loaded);
	EnableItem(SELECT_COLLECTION, loaded);
	EnableItem(SELECT_HOUSE, loaded);
	EnableItem(SELECT_CREATURE, loaded);
	EnableItem(SELECT_WAYPOINT, loaded);
	EnableItem(SELECT_RAW, loaded);

	EnableItem(LIVE_START, is_local);
	EnableItem(LIVE_JOIN, loaded);
	EnableItem(LIVE_CLOSE, is_live);

	EnableItem(DEBUG_VIEW_DAT, loaded);

	UpdateFloorMenu();
}

void MainMenuBar::LoadValues() {
	using namespace MenuBar;

	CheckItem(VIEW_TOOLBARS_BRUSHES, g_settings.getBoolean(Config::SHOW_TOOLBAR_BRUSHES));
	CheckItem(VIEW_TOOLBARS_POSITION, g_settings.getBoolean(Config::SHOW_TOOLBAR_POSITION));
	CheckItem(VIEW_TOOLBARS_SIZES, g_settings.getBoolean(Config::SHOW_TOOLBAR_SIZES));

	CheckItem(SELECT_MODE_COMPENSATE, g_settings.getBoolean(Config::COMPENSATED_SELECT));

	if (IsItemChecked(MenuBar::SELECT_MODE_CURRENT)) {
		g_settings.setInteger(Config::SELECTION_TYPE, SELECT_CURRENT_FLOOR);
	} else if (IsItemChecked(MenuBar::SELECT_MODE_LOWER)) {
		g_settings.setInteger(Config::SELECTION_TYPE, SELECT_ALL_FLOORS);
	} else if (IsItemChecked(MenuBar::SELECT_MODE_VISIBLE)) {
		g_settings.setInteger(Config::SELECTION_TYPE, SELECT_VISIBLE_FLOORS);
	}

	switch (g_settings.getInteger(Config::SELECTION_TYPE)) {
		case SELECT_CURRENT_FLOOR:
			CheckItem(SELECT_MODE_CURRENT, true);
			break;
		case SELECT_ALL_FLOORS:
			CheckItem(SELECT_MODE_LOWER, true);
			break;
		default:
		case SELECT_VISIBLE_FLOORS:
			CheckItem(SELECT_MODE_VISIBLE, true);
			break;
	}

	CheckItem(AUTOMAGIC, g_settings.getBoolean(Config::USE_AUTOMAGIC));

	CheckItem(SHOW_SHADE, g_settings.getBoolean(Config::SHOW_SHADE));
	CheckItem(SHOW_INGAME_BOX, g_settings.getBoolean(Config::SHOW_INGAME_BOX));
	CheckItem(SHOW_LIGHTS, g_settings.getBoolean(Config::SHOW_LIGHTS));
	CheckItem(SHOW_LIGHT_STR, g_settings.getBoolean(Config::SHOW_LIGHT_STR));
	CheckItem(SHOW_TECHNICAL_ITEMS, g_settings.getBoolean(Config::SHOW_TECHNICAL_ITEMS));
	CheckItem(SHOW_WAYPOINTS, g_settings.getBoolean(Config::SHOW_WAYPOINTS));
	CheckItem(SHOW_ALL_FLOORS, g_settings.getBoolean(Config::SHOW_ALL_FLOORS));
	CheckItem(GHOST_ITEMS, g_settings.getBoolean(Config::TRANSPARENT_ITEMS));
	CheckItem(GHOST_HIGHER_FLOORS, g_settings.getBoolean(Config::TRANSPARENT_FLOORS));
	CheckItem(SHOW_EXTRA, !g_settings.getBoolean(Config::SHOW_EXTRA));
	CheckItem(SHOW_GRID, g_settings.getBoolean(Config::SHOW_GRID));
	CheckItem(HIGHLIGHT_ITEMS, g_settings.getBoolean(Config::HIGHLIGHT_ITEMS));
	CheckItem(HIGHLIGHT_LOCKED_DOORS, g_settings.getBoolean(Config::HIGHLIGHT_LOCKED_DOORS));
	CheckItem(SHOW_CREATURES, g_settings.getBoolean(Config::SHOW_CREATURES));
	CheckItem(SHOW_SPAWNS, g_settings.getBoolean(Config::SHOW_SPAWNS));
	CheckItem(SHOW_SPECIAL, g_settings.getBoolean(Config::SHOW_SPECIAL_TILES));
	CheckItem(SHOW_AS_MINIMAP, g_settings.getBoolean(Config::SHOW_AS_MINIMAP));
	CheckItem(SHOW_ONLY_COLORS, g_settings.getBoolean(Config::SHOW_ONLY_TILEFLAGS));
	CheckItem(SHOW_ONLY_MODIFIED, g_settings.getBoolean(Config::SHOW_ONLY_MODIFIED_TILES));
	CheckItem(SHOW_HOUSES, g_settings.getBoolean(Config::SHOW_HOUSES));
	CheckItem(SHOW_PATHING, g_settings.getBoolean(Config::SHOW_BLOCKING));
	CheckItem(SHOW_TOOLTIPS, g_settings.getBoolean(Config::SHOW_TOOLTIPS));
	CheckItem(SHOW_PREVIEW, g_settings.getBoolean(Config::SHOW_PREVIEW));
	CheckItem(SHOW_WALL_HOOKS, g_settings.getBoolean(Config::SHOW_WALL_HOOKS));
	CheckItem(SHOW_TOWNS, g_settings.getBoolean(Config::SHOW_TOWNS));
	CheckItem(ALWAYS_SHOW_ZONES, g_settings.getBoolean(Config::ALWAYS_SHOW_ZONES));
	CheckItem(EXT_HOUSE_SHADER, g_settings.getBoolean(Config::EXT_HOUSE_SHADER));

	CheckItem(MenuBar::SHOW_MINIMAP_HUD, g_settings.getBoolean(Config::MINIMAP_VISIBLE)); // Check the new minimap HUD item
	CheckItem(SHOW_MINIMAP_HUD, g_settings.getBoolean(Config::MINIMAP_VISIBLE));
}

void MainMenuBar::LoadRecentFiles() {
	recentFiles.Load(g_settings.getConfigObject());
}

void MainMenuBar::SaveRecentFiles() {
	recentFiles.Save(g_settings.getConfigObject());
}

void MainMenuBar::AddRecentFile(const FileName file) {
	recentFiles.AddFileToHistory(file.GetFullPath());
}

void MainMenuBar::RemoveRecentFile(size_t index) {
    if (index < recentFiles.GetCount()) {
        recentFiles.RemoveFileFromHistory(index);
        SaveRecentFiles();
    }
}

std::vector<wxString> MainMenuBar::GetRecentFiles() {
	std::vector<wxString> files(recentFiles.GetCount());
	for (size_t i = 0; i < recentFiles.GetCount(); ++i) {
		files[i] = recentFiles.GetHistoryFile(i);
	}
	return files;
}

void MainMenuBar::UpdateFloorMenu() {
	// this will have to be changed if you want to have more floors
	// see MAKE_ACTION(FLOOR_0, wxITEM_RADIO, OnChangeFloor);
	if (MAP_MAX_LAYER < 16) {
		if (g_gui.IsEditorOpen()) {
			for (int i = 0; i < MAP_LAYERS; ++i) {
				CheckItem(MenuBar::ActionID(MenuBar::FLOOR_0 + i), false);
			}
			CheckItem(MenuBar::ActionID(MenuBar::FLOOR_0 + g_gui.GetCurrentFloor()), true);
		}
	}
}

bool MainMenuBar::Load(const FileName& path, wxArrayString& warnings, wxString& error) {
	// Open the XML file
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(path.GetFullPath().mb_str());
	if (!result) {
		error = "Could not open " + path.GetFullName() + " (file not found or syntax error)";
		return false;
	}

	pugi::xml_node node = doc.child("menubar");
	if (!node) {
		error = path.GetFullName() + ": Invalid rootheader.";
		return false;
	}

	// Clear the menu
	while (menubar->GetMenuCount() > 0) {
		menubar->Remove(0);
	}

	// Load succeded
	for (pugi::xml_node menuNode = node.first_child(); menuNode; menuNode = menuNode.next_sibling()) {
		// For each child node, load it
		wxObject* i = LoadItem(menuNode, nullptr, warnings, error);
		wxMenu* m = dynamic_cast<wxMenu*>(i);
		if (m) {
			menubar->Append(m, m->GetTitle());
#ifdef __APPLE__
			m->SetTitle(m->GetTitle());
#else
			m->SetTitle("");
#endif
		} else if (i) {
			delete i;
			warnings.push_back(path.GetFullName() + ": Only menus can be subitems of main menu");
		}
	}

#ifdef __LINUX__
	const int count = 44;
	wxAcceleratorEntry entries[count];
	// Edit
	entries[0].Set(wxACCEL_CTRL, (int)'Z', MAIN_FRAME_MENU + MenuBar::UNDO);
	entries[1].Set(wxACCEL_CTRL | wxACCEL_SHIFT, (int)'Z', MAIN_FRAME_MENU + MenuBar::REDO);
	entries[2].Set(wxACCEL_CTRL, (int)'F', MAIN_FRAME_MENU + MenuBar::FIND_ITEM);
	entries[3].Set(wxACCEL_CTRL | wxACCEL_SHIFT, (int)'F', MAIN_FRAME_MENU + MenuBar::REPLACE_ITEMS);
	entries[4].Set(wxACCEL_NORMAL, (int)'A', MAIN_FRAME_MENU + MenuBar::AUTOMAGIC);
	entries[5].Set(wxACCEL_CTRL, (int)'B', MAIN_FRAME_MENU + MenuBar::BORDERIZE_SELECTION);
	entries[6].Set(wxACCEL_NORMAL, (int)'P', MAIN_FRAME_MENU + MenuBar::GOTO_PREVIOUS_POSITION);
	entries[7].Set(wxACCEL_CTRL, (int)'G', MAIN_FRAME_MENU + MenuBar::GOTO_POSITION);
	entries[8].Set(wxACCEL_NORMAL, (int)'J', MAIN_FRAME_MENU + MenuBar::JUMP_TO_BRUSH);
	entries[9].Set(wxACCEL_CTRL, (int)'X', MAIN_FRAME_MENU + MenuBar::CUT);
	entries[10].Set(wxACCEL_CTRL, (int)'C', MAIN_FRAME_MENU + MenuBar::COPY);
	entries[11].Set(wxACCEL_CTRL, (int)'V', MAIN_FRAME_MENU + MenuBar::PASTE);
	// View
	entries[12].Set(wxACCEL_CTRL, (int)'=', MAIN_FRAME_MENU + MenuBar::ZOOM_IN);
	entries[13].Set(wxACCEL_CTRL, (int)'-', MAIN_FRAME_MENU + MenuBar::ZOOM_OUT);
	entries[14].Set(wxACCEL_CTRL, (int)'0', MAIN_FRAME_MENU + MenuBar::ZOOM_NORMAL);
	entries[15].Set(wxACCEL_NORMAL, (int)'Q', MAIN_FRAME_MENU + MenuBar::SHOW_SHADE);
	entries[16].Set(wxACCEL_CTRL, (int)'W', MAIN_FRAME_MENU + MenuBar::SHOW_ALL_FLOORS);
	entries[17].Set(wxACCEL_NORMAL, (int)'Q', MAIN_FRAME_MENU + MenuBar::GHOST_ITEMS);
	entries[18].Set(wxACCEL_CTRL, (int)'L', MAIN_FRAME_MENU + MenuBar::GHOST_HIGHER_FLOORS);
	entries[19].Set(wxACCEL_SHIFT, (int)'I', MAIN_FRAME_MENU + MenuBar::SHOW_INGAME_BOX);
	entries[20].Set(wxACCEL_SHIFT, (int)'L', MAIN_FRAME_MENU + MenuBar::SHOW_LIGHTS);
	entries[21].Set(wxACCEL_SHIFT, (int)'G', MAIN_FRAME_MENU + MenuBar::SHOW_GRID);
	entries[22].Set(wxACCEL_NORMAL, (int)'V', MAIN_FRAME_MENU + MenuBar::HIGHLIGHT_ITEMS);
	entries[23].Set(wxACCEL_NORMAL, (int)'X', MAIN_FRAME_MENU + MenuBar::HIGHLIGHT_LOCKED_DOORS);
	entries[24].Set(wxACCEL_NORMAL, (int)'F', MAIN_FRAME_MENU + MenuBar::SHOW_CREATURES);
	entries[25].Set(wxACCEL_NORMAL, (int)'S', MAIN_FRAME_MENU + MenuBar::SHOW_SPAWNS);
	entries[26].Set(wxACCEL_NORMAL, (int)'E', MAIN_FRAME_MENU + MenuBar::SHOW_SPECIAL);
	entries[27].Set(wxACCEL_SHIFT, (int)'E', MAIN_FRAME_MENU + MenuBar::SHOW_AS_MINIMAP);
	entries[28].Set(wxACCEL_CTRL, (int)'E', MAIN_FRAME_MENU + MenuBar::SHOW_ONLY_COLORS);
	entries[29].Set(wxACCEL_CTRL, (int)'M', MAIN_FRAME_MENU + MenuBar::SHOW_ONLY_MODIFIED);
	entries[30].Set(wxACCEL_CTRL, (int)'H', MAIN_FRAME_MENU + MenuBar::SHOW_HOUSES);
	entries[31].Set(wxACCEL_NORMAL, (int)'O', MAIN_FRAME_MENU + MenuBar::SHOW_PATHING);
	entries[32].Set(wxACCEL_NORMAL, (int)'Y', MAIN_FRAME_MENU + MenuBar::SHOW_TOOLTIPS);
	entries[33].Set(wxACCEL_NORMAL, (int)'L', MAIN_FRAME_MENU + MenuBar::SHOW_PREVIEW);
	entries[34].Set(wxACCEL_NORMAL, (int)'K', MAIN_FRAME_MENU + MenuBar::SHOW_WALL_HOOKS);

	// Window
	entries[35].Set(wxACCEL_NORMAL, (int)'M', MAIN_FRAME_MENU + MenuBar::WIN_MINIMAP);
	entries[36].Set(wxACCEL_NORMAL, (int)'T', MAIN_FRAME_MENU + MenuBar::SELECT_TERRAIN);
	entries[37].Set(wxACCEL_NORMAL, (int)'D', MAIN_FRAME_MENU + MenuBar::SELECT_DOODAD);
	entries[38].Set(wxACCEL_NORMAL, (int)'I', MAIN_FRAME_MENU + MenuBar::SELECT_ITEM);
	entries[39].Set(wxACCEL_NORMAL, (int)'N', MAIN_FRAME_MENU + MenuBar::SELECT_COLLECTION);
	entries[40].Set(wxACCEL_NORMAL, (int)'H', MAIN_FRAME_MENU + MenuBar::SELECT_HOUSE);
	entries[41].Set(wxACCEL_NORMAL, (int)'C', MAIN_FRAME_MENU + MenuBar::SELECT_CREATURE);
	entries[42].Set(wxACCEL_NORMAL, (int)'W', MAIN_FRAME_MENU + MenuBar::SELECT_WAYPOINT);
	entries[43].Set(wxACCEL_NORMAL, (int)'R', MAIN_FRAME_MENU + MenuBar::SELECT_RAW);

	wxAcceleratorTable accelerator(count, entries);
	frame->SetAcceleratorTable(accelerator);
#endif

	/*
	// Create accelerator table
	accelerator_table = newd wxAcceleratorTable(accelerators.size(), &accelerators[0]);

	// Tell all clients of the renewed accelerators
	RenewClients();
	*/

	recentFiles.AddFilesToMenu();
	Update();
	LoadValues();
	return true;
}

wxObject* MainMenuBar::LoadItem(pugi::xml_node node, wxMenu* parent, wxArrayString& warnings, wxString& error) {
	pugi::xml_attribute attribute;

	const std::string& nodeName = as_lower_str(node.name());
	if (nodeName == "menu") {
		if (!(attribute = node.attribute("name"))) {
			return nullptr;
		}

		std::string name = attribute.as_string();
		std::string menuName = name;
		std::replace(name.begin(), name.end(), '$', '&');

		wxMenu* menu = newd wxMenu;
		if ((attribute = node.attribute("special"))) {
			std::string special = attribute.as_string();
			if (special == "RECENT_FILES") {
				recentFiles.UseMenu(menu);
			} else if (special == "SCRIPTS") {
				// Store reference to scripts menu for dynamic population
				scriptsMenu = menu;
				// Add static items
				for (pugi::xml_node menuNode = node.first_child(); menuNode; menuNode = menuNode.next_sibling()) {
					LoadItem(menuNode, menu, warnings, error);
				}
				// Load script entries
				LoadScriptsMenu();
			}
		} else {
			for (pugi::xml_node menuNode = node.first_child(); menuNode; menuNode = menuNode.next_sibling()) {
				// Load an add each item in order
				LoadItem(menuNode, menu, warnings, error);
			}
		}

		if (menuName == "Show") {
			showMenu = menu;
			showMenuCount = 0;
			showMenuHasSeparator = false;
			LoadShowMenu();
		}

		// If we have a parent, add ourselves.
		// If not, we just return the item and the parent function
		// is responsible for adding us to wherever
		if (parent) {
			parent->AppendSubMenu(menu, wxstr(name));
		} else {
			menu->SetTitle((name));
		}
		return menu;
	} else if (nodeName == "item") {
		// We must have a parent when loading items
		if (!parent) {
			return nullptr;
		} else if (!(attribute = node.attribute("name"))) {
			return nullptr;
		}

		std::string name = attribute.as_string();
		std::replace(name.begin(), name.end(), '$', '&');
		if (!(attribute = node.attribute("action"))) {
			return nullptr;
		}

		const std::string& action = attribute.as_string();
		std::string hotkey = node.attribute("hotkey").as_string();
		if (!hotkey.empty()) {
			hotkey = '\t' + hotkey;
		}

		const std::string& help = node.attribute("help").as_string();
		name += hotkey;

		auto it = actions.find(action);
		if (it == actions.end()) {
			warnings.push_back("Invalid action type '" + wxstr(action) + "'.");
			return nullptr;
		}

		const MenuBar::Action& act = *it->second;
		wxAcceleratorEntry* entry = wxAcceleratorEntry::Create(wxstr(hotkey));
		if (entry) {
			delete entry; // accelerators.push_back(entry);
		} else {
			warnings.push_back("Invalid hotkey.");
		}

		wxMenuItem* tmp = parent->Append(
			MAIN_FRAME_MENU + act.id, // ID
			wxstr(name), // Title of button
			wxstr(help), // Help text
			act.kind // Kind of item
		);
		items[MenuBar::ActionID(act.id)].push_back(tmp);
		return tmp;
	} else if (nodeName == "separator") {
		// We must have a parent when loading items
		if (!parent) {
			return nullptr;
		}
		return parent->AppendSeparator();
	}
	return nullptr;
}

void MainMenuBar::OnNew(wxCommandEvent& WXUNUSED(event)) {
	g_gui.NewMap();
}

void MainMenuBar::OnGenerateMap(wxCommandEvent& WXUNUSED(event)) {
	/*
	if(!DoQuerySave()) return;

	std::ostringstream os;
	os << "Untitled-" << untitled_counter << ".otbm";
	++untitled_counter;

	editor.generateMap(wxstr(os.str()));

	g_gui.SetStatusText("Generated newd map");

	g_gui.UpdateTitle();
	g_gui.RefreshPalettes();
	g_gui.UpdateMinimap();
	g_gui.FitViewToMap();
	UpdateMenubar();
	Refresh();
	*/
}

void MainMenuBar::OnOpenRecent(wxCommandEvent& event) {
	FileName fn(recentFiles.GetHistoryFile(event.GetId() - recentFiles.GetBaseId()));
	frame->LoadMap(fn);
}

void MainMenuBar::OnOpen(wxCommandEvent& WXUNUSED(event)) {
	g_gui.OpenMap();
}

void MainMenuBar::OnClose(wxCommandEvent& WXUNUSED(event)) {
	frame->DoQuerySave(true); // It closes the editor too
}

void MainMenuBar::OnSave(wxCommandEvent& WXUNUSED(event)) {
	g_gui.SaveMap();
}

void MainMenuBar::OnSaveAs(wxCommandEvent& WXUNUSED(event)) {
	g_gui.SaveMapAs();
}

void MainMenuBar::OnPreferences(wxCommandEvent& WXUNUSED(event)) {
	PreferencesWindow dialog(frame);
	dialog.ShowModal();
	dialog.Destroy();
}

void MainMenuBar::OnQuit(wxCommandEvent& WXUNUSED(event)) {
	/*
	while(g_gui.IsEditorOpen())
		if(!frame->DoQuerySave(true))
			return;
			*/
	//((Application*)wxTheApp)->Unload();
	g_gui.root->Close();
}

void MainMenuBar::OnImportMap(wxCommandEvent& WXUNUSED(event)) {
	ASSERT(g_gui.GetCurrentEditor());
	wxDialog* importmap = newd ImportMapWindow(frame, *g_gui.GetCurrentEditor());
	importmap->ShowModal();
}

void MainMenuBar::OnImportMonsterData(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog dlg(g_gui.root, "Import monster/npc file", "", "", "*.xml", wxFD_OPEN | wxFD_MULTIPLE | wxFD_FILE_MUST_EXIST);
	if (dlg.ShowModal() == wxID_OK) {
		wxArrayString paths;
		dlg.GetPaths(paths);
		for (uint32_t i = 0; i < paths.GetCount(); ++i) {
			wxString error;
			wxArrayString warnings;
			bool ok = g_creatures.importXMLFromOT(FileName(paths[i]), error, warnings);
			if (!ok) {
				g_gui.PopupDialog("Error", error, wxOK);
			}
		}
	}
}

void MainMenuBar::OnImportMinimap(wxCommandEvent& WXUNUSED(event)) {
	ASSERT(g_gui.IsEditorOpen());
	// wxDialog* importmap = newd ImportMapWindow();
	// importmap->ShowModal();
}

void MainMenuBar::OnExportOTClient(wxCommandEvent& WXUNUSED(event)) {
	if (g_gui.IsEditorOpen()) {
		wxFileDialog dlg(frame, "Export OTClient Map Package", "", wxString::FromUTF8(g_gui.GetCurrentMap().getName()), "OTBM Map (*.otbm)|*.otbm", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		if (dlg.ShowModal() == wxID_OK) {
			RME::Core::OTCExport::ExportMap(&g_gui.GetCurrentMap(), nstr(dlg.GetPath()));
			g_gui.SetStatusText("OTClient Map Package exported successfully!");
		}
	}
}

void MainMenuBar::OnExportTilesets(wxCommandEvent& WXUNUSED(event)) {
	if (g_gui.GetCurrentEditor()) {
		ExportTilesetsWindow dlg(frame, *g_gui.GetCurrentEditor());
		dlg.ShowModal();
		dlg.Destroy();
	}
}

void MainMenuBar::OnDebugViewDat(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg(frame, wxID_ANY, "Debug .dat file", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
	new DatDebugView(&dlg);
	dlg.ShowModal();
}

void MainMenuBar::OnReloadDataFiles(wxCommandEvent& WXUNUSED(event)) {
	wxString error;
	wxArrayString warnings;
	g_gui.LoadVersion(g_gui.GetCurrentVersionID(), error, warnings, true);
	g_gui.PopupDialog("Error", error, wxOK);
	g_gui.ListDialog("Warnings", warnings);
}

void MainMenuBar::OnListExtensions(wxCommandEvent& WXUNUSED(event)) {
	ExtensionsDialog exts(frame);
	exts.ShowModal();
}

void MainMenuBar::OnGotoWebsite(wxCommandEvent& WXUNUSED(event)) {
	::wxLaunchDefaultBrowser(__SITE_URL__, wxBROWSER_NEW_WINDOW);
}

void MainMenuBar::OnAbout(wxCommandEvent& WXUNUSED(event)) {
	AboutWindow about(frame);
	about.ShowModal();
}

void MainMenuBar::OnUndo(wxCommandEvent& WXUNUSED(event)) {
	g_gui.DoUndo();
}

void MainMenuBar::OnRedo(wxCommandEvent& WXUNUSED(event)) {
	g_gui.DoRedo();
}

void MainMenuBar::OnSelectionTypeChange(wxCommandEvent& WXUNUSED(event)) {
	g_settings.setInteger(Config::COMPENSATED_SELECT, IsItemChecked(MenuBar::SELECT_MODE_COMPENSATE));

	if (IsItemChecked(MenuBar::SELECT_MODE_CURRENT)) {
		g_settings.setInteger(Config::SELECTION_TYPE, SELECT_CURRENT_FLOOR);
	} else if (IsItemChecked(MenuBar::SELECT_MODE_LOWER)) {
		g_settings.setInteger(Config::SELECTION_TYPE, SELECT_ALL_FLOORS);
	} else if (IsItemChecked(MenuBar::SELECT_MODE_VISIBLE)) {
		g_settings.setInteger(Config::SELECTION_TYPE, SELECT_VISIBLE_FLOORS);
	}
}

void MainMenuBar::OnCopy(wxCommandEvent& WXUNUSED(event)) {
	g_gui.DoCopy();
}

void MainMenuBar::OnCut(wxCommandEvent& WXUNUSED(event)) {
	g_gui.DoCut();
}

void MainMenuBar::OnPaste(wxCommandEvent& WXUNUSED(event)) {
	g_gui.PreparePaste();
}

void MainMenuBar::OnToggleAutomagic(wxCommandEvent& WXUNUSED(event)) {
	g_settings.setInteger(Config::USE_AUTOMAGIC, IsItemChecked(MenuBar::AUTOMAGIC));
	g_settings.setInteger(Config::BORDER_IS_GROUND, IsItemChecked(MenuBar::AUTOMAGIC));
	if (g_settings.getInteger(Config::USE_AUTOMAGIC)) {
		g_gui.SetStatusText("Automagic enabled.");
	} else {
		g_gui.SetStatusText("Automagic disabled.");
	}
}

void MainMenuBar::OnBorderizeSelection(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	g_gui.GetCurrentEditor()->borderizeSelection();
	g_gui.RefreshView();
}

void MainMenuBar::OnBorderizeMap(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	int ret = g_gui.PopupDialog("Borderize Map", "Are you sure you want to borderize the entire map (this action cannot be undone)?", wxYES | wxNO);
	if (ret == wxID_YES) {
		g_gui.GetCurrentEditor()->borderizeMap(true);
	}

	g_gui.RefreshView();
}

void MainMenuBar::OnRandomizeSelection(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	g_gui.GetCurrentEditor()->randomizeSelection();
	g_gui.RefreshView();
}

void MainMenuBar::OnRandomizeMap(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	int ret = g_gui.PopupDialog("Randomize Map", "Are you sure you want to randomize the entire map (this action cannot be undone)?", wxYES | wxNO);
	if (ret == wxID_YES) {
		g_gui.GetCurrentEditor()->randomizeMap(true);
	}

	g_gui.RefreshView();
}

void MainMenuBar::OnJumpToBrush(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsVersionLoaded()) {
		return;
	}

	// Create the jump to dialog
	FindDialog* dlg = newd FindBrushDialog(frame);

	// Display dialog to user
	dlg->ShowModal();

	// Retrieve result, if null user canceled
	const Brush* brush = dlg->getResult();
	if (brush) {
		g_gui.SelectBrush(brush, TILESET_UNKNOWN);
	}
	delete dlg;
}

void MainMenuBar::OnJumpToItemBrush(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsVersionLoaded()) {
		return;
	}

	// Create the jump to dialog
	FindItemDialog dialog(frame, "Jump to Item");
	dialog.setSearchMode((FindItemDialog::SearchMode)g_settings.getInteger(Config::JUMP_TO_ITEM_MODE));
	if (dialog.ShowModal() == wxID_OK) {
		// Retrieve result, if null user canceled
		const Brush* brush = dialog.getResult();
		if (brush) {
			g_gui.SelectBrush(brush, TILESET_RAW);
		}
		g_settings.setInteger(Config::JUMP_TO_ITEM_MODE, (int)dialog.getSearchMode());
	}
	dialog.Destroy();
}

void MainMenuBar::OnGotoPreviousPosition(wxCommandEvent& WXUNUSED(event)) {
	MapTab* mapTab = g_gui.GetCurrentMapTab();
	if (mapTab) {
		mapTab->GoToPreviousCenterPosition();
	}
}

void MainMenuBar::OnGotoPosition(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	// Display dialog, it also controls the actual jump
	GotoPositionDialog dlg(frame, *g_gui.GetCurrentEditor());
	dlg.ShowModal();
}

void MainMenuBar::OnToolbars(wxCommandEvent& event) {
	using namespace MenuBar;

	ActionID id = static_cast<ActionID>(event.GetId() - (wxID_HIGHEST + 1));
	switch (id) {
		case VIEW_TOOLBARS_BRUSHES:
			g_gui.ShowToolbar(TOOLBAR_BRUSHES, event.IsChecked());
			g_settings.setInteger(Config::SHOW_TOOLBAR_BRUSHES, event.IsChecked());
			break;
		case VIEW_TOOLBARS_POSITION:
			g_gui.ShowToolbar(TOOLBAR_POSITION, event.IsChecked());
			g_settings.setInteger(Config::SHOW_TOOLBAR_POSITION, event.IsChecked());
			break;
		case VIEW_TOOLBARS_SIZES:
			g_gui.ShowToolbar(TOOLBAR_SIZES, event.IsChecked());
			g_settings.setInteger(Config::SHOW_TOOLBAR_SIZES, event.IsChecked());
			break;
		default:
			break;
	}
}

void MainMenuBar::OnNewView(wxCommandEvent& WXUNUSED(event)) {
	g_gui.NewMapView();
}

void MainMenuBar::OnToggleFullscreen(wxCommandEvent& WXUNUSED(event)) {
	if (frame->IsFullScreen()) {
		frame->ShowFullScreen(false);
	} else {
		frame->ShowFullScreen(true, wxFULLSCREEN_NOBORDER | wxFULLSCREEN_NOCAPTION);
	}
}

void MainMenuBar::OnTakeScreenshot(wxCommandEvent& WXUNUSED(event)) {
	wxString path = wxstr(g_settings.getString(Config::SCREENSHOT_DIRECTORY));
	if (path.size() > 0 && (path.Last() == '/' || path.Last() == '\\')) {
		path = path + "/";
	}

	g_gui.GetCurrentMapTab()->GetView()->GetCanvas()->TakeScreenshot(
		path, wxstr(g_settings.getString(Config::SCREENSHOT_FORMAT))
	);
}

void MainMenuBar::OnZoomIn(wxCommandEvent& event) {
	double zoom = g_gui.GetCurrentZoom();
	g_gui.SetCurrentZoom(zoom - 0.1);
}

void MainMenuBar::OnZoomOut(wxCommandEvent& event) {
	double zoom = g_gui.GetCurrentZoom();
	g_gui.SetCurrentZoom(zoom + 0.1);
}

void MainMenuBar::OnZoomNormal(wxCommandEvent& event) {
	g_gui.SetCurrentZoom(1.0);
}

void MainMenuBar::OnChangeViewSettings(wxCommandEvent& event) {
	g_settings.setInteger(Config::SHOW_ALL_FLOORS, IsItemChecked(MenuBar::SHOW_ALL_FLOORS));
	if (IsItemChecked(MenuBar::SHOW_ALL_FLOORS)) {
		EnableItem(MenuBar::SELECT_MODE_VISIBLE, true);
		EnableItem(MenuBar::SELECT_MODE_LOWER, true);
	} else {
		EnableItem(MenuBar::SELECT_MODE_VISIBLE, false);
		EnableItem(MenuBar::SELECT_MODE_LOWER, false);
		CheckItem(MenuBar::SELECT_MODE_CURRENT, true);
		g_settings.setInteger(Config::SELECTION_TYPE, SELECT_CURRENT_FLOOR);
	}
	g_settings.setInteger(Config::TRANSPARENT_FLOORS, IsItemChecked(MenuBar::GHOST_HIGHER_FLOORS));
	g_settings.setInteger(Config::TRANSPARENT_ITEMS, IsItemChecked(MenuBar::GHOST_ITEMS));
	g_settings.setInteger(Config::SHOW_INGAME_BOX, IsItemChecked(MenuBar::SHOW_INGAME_BOX));
	g_settings.setInteger(Config::SHOW_LIGHTS, IsItemChecked(MenuBar::SHOW_LIGHTS));
	g_settings.setInteger(Config::SHOW_LIGHT_STR, IsItemChecked(MenuBar::SHOW_LIGHT_STR));
	g_settings.setInteger(Config::SHOW_TECHNICAL_ITEMS, IsItemChecked(MenuBar::SHOW_TECHNICAL_ITEMS));
	g_settings.setInteger(Config::SHOW_WAYPOINTS, IsItemChecked(MenuBar::SHOW_WAYPOINTS));
	g_settings.setInteger(Config::SHOW_GRID, IsItemChecked(MenuBar::SHOW_GRID));
	g_settings.setInteger(Config::SHOW_EXTRA, !IsItemChecked(MenuBar::SHOW_EXTRA));

	g_settings.setInteger(Config::SHOW_SHADE, IsItemChecked(MenuBar::SHOW_SHADE));
	g_settings.setInteger(Config::SHOW_SPECIAL_TILES, IsItemChecked(MenuBar::SHOW_SPECIAL));
	g_settings.setInteger(Config::SHOW_AS_MINIMAP, IsItemChecked(MenuBar::SHOW_AS_MINIMAP));
	g_settings.setInteger(Config::SHOW_ONLY_TILEFLAGS, IsItemChecked(MenuBar::SHOW_ONLY_COLORS));
	g_settings.setInteger(Config::SHOW_ONLY_MODIFIED_TILES, IsItemChecked(MenuBar::SHOW_ONLY_MODIFIED));
	g_settings.setInteger(Config::SHOW_CREATURES, IsItemChecked(MenuBar::SHOW_CREATURES));
	g_settings.setInteger(Config::SHOW_SPAWNS, IsItemChecked(MenuBar::SHOW_SPAWNS));
	g_settings.setInteger(Config::SHOW_HOUSES, IsItemChecked(MenuBar::SHOW_HOUSES));
	g_settings.setInteger(Config::HIGHLIGHT_ITEMS, IsItemChecked(MenuBar::HIGHLIGHT_ITEMS));
	g_settings.setInteger(Config::HIGHLIGHT_LOCKED_DOORS, IsItemChecked(MenuBar::HIGHLIGHT_LOCKED_DOORS));
	g_settings.setInteger(Config::SHOW_BLOCKING, IsItemChecked(MenuBar::SHOW_PATHING));
	g_settings.setInteger(Config::SHOW_TOOLTIPS, IsItemChecked(MenuBar::SHOW_TOOLTIPS));
	g_settings.setInteger(Config::SHOW_PREVIEW, IsItemChecked(MenuBar::SHOW_PREVIEW));
	g_settings.setInteger(Config::SHOW_WALL_HOOKS, IsItemChecked(MenuBar::SHOW_WALL_HOOKS));
	g_settings.setInteger(Config::SHOW_TOWNS, IsItemChecked(MenuBar::SHOW_TOWNS));
	g_settings.setInteger(Config::ALWAYS_SHOW_ZONES, IsItemChecked(MenuBar::ALWAYS_SHOW_ZONES));
	g_settings.setInteger(Config::EXT_HOUSE_SHADER, IsItemChecked(MenuBar::EXT_HOUSE_SHADER));

	g_settings.setInteger(Config::EXPERIMENTAL_FOG, IsItemChecked(MenuBar::EXPERIMENTAL_FOG));

	g_gui.RefreshView();
	// g_gui.UpdateMinimap(true); // No longer needed, minimap is canvas-based and refreshes with view
}

void MainMenuBar::OnChangeFloor(wxCommandEvent& event) {
	// Workaround to stop events from looping
	if (checking_programmaticly) {
		return;
	}

	// this will have to be changed if you want to have more floors
	// see MAKE_ACTION(FLOOR_0, wxITEM_RADIO, OnChangeFloor);
	if (MAP_MAX_LAYER < 16) {
		for (int i = 0; i < MAP_LAYERS; ++i) {
			if (IsItemChecked(MenuBar::ActionID(MenuBar::FLOOR_0 + i))) {
				g_gui.ChangeFloor(i);
			}
		}
	}
}

void MainMenuBar::OnNewPalette(wxCommandEvent& event) {
	g_gui.NewPalette();
}

void MainMenuBar::OnSelectTerrainPalette(wxCommandEvent& WXUNUSED(event)) {
	g_gui.SelectPalettePage(TILESET_TERRAIN);
}

void MainMenuBar::OnSelectDoodadPalette(wxCommandEvent& WXUNUSED(event)) {
	g_gui.SelectPalettePage(TILESET_DOODAD);
}

void MainMenuBar::OnSelectItemPalette(wxCommandEvent& WXUNUSED(event)) {
	g_gui.SelectPalettePage(TILESET_ITEM);
}

void MainMenuBar::OnSelectCollectionPalette(wxCommandEvent& WXUNUSED(event)) {
	g_gui.SelectPalettePage(TILESET_COLLECTION);
}

void MainMenuBar::OnSelectHousePalette(wxCommandEvent& WXUNUSED(event)) {
	g_gui.SelectPalettePage(TILESET_HOUSE);
}

void MainMenuBar::OnSelectCreaturePalette(wxCommandEvent& WXUNUSED(event)) {
	g_gui.SelectPalettePage(TILESET_CREATURE);
}

void MainMenuBar::OnSelectWaypointPalette(wxCommandEvent& WXUNUSED(event)) {
	g_gui.SelectPalettePage(TILESET_WAYPOINT);
}

void MainMenuBar::OnSelectRawPalette(wxCommandEvent& WXUNUSED(event)) {
	g_gui.SelectPalettePage(TILESET_RAW);
}

void MainMenuBar::OnStartLive(wxCommandEvent& event) {
	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor) {
		g_gui.PopupDialog("Error", "You need to have a map open to start a live mapping session.", wxOK);
		return;
	}
	if (editor->IsLive()) {
		g_gui.PopupDialog("Error", "You can not start two live servers on the same map (or a server using a remote map).", wxOK);
		return;
	}

	wxDialog* live_host_dlg = newd wxDialog(frame, wxID_ANY, "Host Live Server", wxDefaultPosition, wxDefaultSize);

	wxSizer* top_sizer = newd wxBoxSizer(wxVERTICAL);
	wxFlexGridSizer* gsizer = newd wxFlexGridSizer(2, 10, 10);
	gsizer->AddGrowableCol(0, 2);
	gsizer->AddGrowableCol(1, 3);

	// Data fields
	wxTextCtrl* hostname;
	wxSpinCtrl* port;
	wxCheckBox* allow_copy;

	gsizer->Add(newd wxStaticText(live_host_dlg, wxID_ANY, "Server Name:"));
	gsizer->Add(hostname = newd wxTextCtrl(live_host_dlg, wxID_ANY, "RME Live Server"), 0, wxEXPAND);

	gsizer->Add(newd wxStaticText(live_host_dlg, wxID_ANY, "Port:"));
	gsizer->Add(port = newd wxSpinCtrl(live_host_dlg, wxID_ANY, "31313", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 65535, 31313), 0, wxEXPAND);
	top_sizer->Add(gsizer, 0, wxALL, 20);

	top_sizer->Add(allow_copy = newd wxCheckBox(live_host_dlg, wxID_ANY, "Allow copy & paste between maps."), 0, wxRIGHT | wxLEFT, 20);
	allow_copy->SetToolTip("Allows remote clients to copy & paste from the hosted map to local maps.");

	wxSizer* ok_sizer = newd wxBoxSizer(wxHORIZONTAL);
	ok_sizer->Add(newd wxButton(live_host_dlg, wxID_OK, "OK"), 1, wxCENTER);
	ok_sizer->Add(newd wxButton(live_host_dlg, wxID_CANCEL, "Cancel"), wxCENTER, 1);
	top_sizer->Add(ok_sizer, 0, wxCENTER | wxALL, 20);

	live_host_dlg->SetSizerAndFit(top_sizer);

	while (true) {
		int ret = live_host_dlg->ShowModal();
		if (ret == wxID_OK) {
			LiveServer* liveServer = editor->StartLiveServer();
			liveServer->setName(hostname->GetValue());
			liveServer->setPort(port->GetValue());

			const wxString& error = liveServer->getLastError();
			if (!error.empty()) {
				g_gui.PopupDialog(live_host_dlg, "Error", error, wxOK);
				editor->CloseLiveServer();
				continue;
			}

			if (!liveServer->bind()) {
				g_gui.PopupDialog("Socket Error", "Could not bind socket! Try another port?", wxOK);
				editor->CloseLiveServer();
			} else {
				liveServer->createLogWindow(g_gui.tabbook);
			}
			break;
		} else {
			break;
		}
	}
	live_host_dlg->Destroy();
	Update();
}

void MainMenuBar::OnJoinLive(wxCommandEvent& event) {
	wxDialog* live_join_dlg = newd wxDialog(frame, wxID_ANY, "Join Live Server", wxDefaultPosition, wxDefaultSize);

	wxSizer* top_sizer = newd wxBoxSizer(wxVERTICAL);
	wxFlexGridSizer* gsizer = newd wxFlexGridSizer(2, 10, 10);
	gsizer->AddGrowableCol(0, 2);
	gsizer->AddGrowableCol(1, 3);

	// Data fields
	wxTextCtrl* name;
	wxTextCtrl* ip;
	wxSpinCtrl* port;

	gsizer->Add(newd wxStaticText(live_join_dlg, wxID_ANY, "Name:"));
	gsizer->Add(name = newd wxTextCtrl(live_join_dlg, wxID_ANY, ""), 0, wxEXPAND);

	gsizer->Add(newd wxStaticText(live_join_dlg, wxID_ANY, "IP:"));
	gsizer->Add(ip = newd wxTextCtrl(live_join_dlg, wxID_ANY, "localhost"), 0, wxEXPAND);

	gsizer->Add(newd wxStaticText(live_join_dlg, wxID_ANY, "Port:"));
	gsizer->Add(port = newd wxSpinCtrl(live_join_dlg, wxID_ANY, "31313", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 65535, 31313), 0, wxEXPAND);
	top_sizer->Add(gsizer, 0, wxALL, 20);

	wxSizer* ok_sizer = newd wxBoxSizer(wxHORIZONTAL);
	ok_sizer->Add(newd wxButton(live_join_dlg, wxID_OK, "OK"), 1, wxRIGHT);
	ok_sizer->Add(newd wxButton(live_join_dlg, wxID_CANCEL, "Cancel"), 1, wxRIGHT);
	top_sizer->Add(ok_sizer, 0, wxCENTER | wxALL, 20);

	live_join_dlg->SetSizerAndFit(top_sizer);

	while (true) {
		int ret = live_join_dlg->ShowModal();
		if (ret == wxID_OK) {
			LiveClient* liveClient = newd LiveClient();

			wxString tmp = name->GetValue();
			if (tmp.empty()) {
				tmp = "User";
			}
			liveClient->setName(tmp);

			const wxString& error = liveClient->getLastError();
			if (!error.empty()) {
				g_gui.PopupDialog(live_join_dlg, "Error", error, wxOK);
				delete liveClient;
				continue;
			}

			const wxString& address = ip->GetValue();
			int32_t portNumber = port->GetValue();

			liveClient->createLogWindow(g_gui.tabbook);
			if (!liveClient->connect(nstr(address), portNumber)) {
				g_gui.PopupDialog("Connection Error", liveClient->getLastError(), wxOK);
				delete liveClient;
			}

			break;
		} else {
			break;
		}
	}
	live_join_dlg->Destroy();
	Update();
}

void MainMenuBar::OnCloseLive(wxCommandEvent& event) {
	Editor* editor = g_gui.GetCurrentEditor();
	if (editor && editor->IsLive()) {
		g_gui.CloseLiveEditors(&editor->GetLive());
	}

	Update();
}

// ============================================================================
// Scripts Menu Handlers

void MainMenuBar::OnScriptsOpenFolder(wxCommandEvent& WXUNUSED(event)) {
	g_luaScripts.openScriptsFolder();
}

void MainMenuBar::OnScriptsReload(wxCommandEvent& WXUNUSED(event)) {
	g_luaScripts.reloadScripts();
	RefreshScriptsMenu();
	g_gui.SetStatusText("Scripts reloaded");

	// Also refresh the Script Manager window if open
	LuaScriptsWindow* scriptsWindow = LuaScriptsWindow::Get();
	if (scriptsWindow) {
		scriptsWindow->RefreshScriptList();
	}
}

void MainMenuBar::OnScriptsManager(wxCommandEvent& WXUNUSED(event)) {
	// Toggle visibility of Script Manager panel
	wxAuiPaneInfo& pane = g_gui.aui_manager->GetPane("ScriptManager");
	if (pane.IsOk()) {
		pane.Show(!pane.IsShown());
		g_gui.aui_manager->Update();
	}
}

void MainMenuBar::OnScriptExecute(wxCommandEvent& event) {
	using namespace MenuBar;

	int scriptIndex = event.GetId() - MAIN_FRAME_MENU - SCRIPTS_FIRST;
	const auto& scripts = g_luaScripts.getScripts();

	if (scriptIndex >= 0 && scriptIndex < (int)scripts.size()) {
		LuaScript* script = scripts[scriptIndex].get();
		if (!g_luaScripts.executeScript(script)) {
			wxMessageBox(
				wxString("Script error:\n") + g_luaScripts.getLastError(),
				"Lua Script Error",
				wxOK | wxICON_ERROR
			);
		}
	}
}

void MainMenuBar::OnShowOverlayToggle(wxCommandEvent& event) {
	using namespace MenuBar;

	int showIndex = event.GetId() - MAIN_FRAME_MENU - SHOW_CUSTOM_FIRST;
	const auto& shows = g_luaScripts.getMapOverlayShows();
	if (showIndex < 0 || showIndex >= static_cast<int>(shows.size())) {
		return;
	}

	const auto& showItem = shows[showIndex];
	g_luaScripts.setMapOverlayShowEnabled(showItem.overlayId, event.IsChecked());
	g_gui.RefreshView();
}

void MainMenuBar::LoadScriptsMenu() {
	using namespace MenuBar;

	if (!scriptsMenu) {
		return;
	}

	// But let's be safe and look for the second separator
	int separatorCount = 0;
	size_t clearFrom = 0;
	bool found = false;

	for (size_t i = 0; i < scriptsMenu->GetMenuItemCount(); ++i) {
		wxMenuItem* item = scriptsMenu->FindItemByPosition(i);
		if (item && item->IsSeparator()) {
			separatorCount++;
			if (separatorCount == 2) {
				clearFrom = i + 1;
				found = true;
				break;
			}
		}
	}

	// If we found the second separator, delete everything after it
	if (!found) {
		clearFrom = 5;
	}

	while (scriptsMenu->GetMenuItemCount() > clearFrom) {
		scriptsMenu->Delete(scriptsMenu->FindItemByPosition(clearFrom));
	}

	// Add scripts
	const auto& scripts = g_luaScripts.getScripts();
	if (!scripts.empty()) {
		// scriptsMenu->AppendSeparator(); // Separator is already there ideally

		int scriptIndex = 0;
		for (const auto& script : scripts) {
			if (scriptIndex >= (SCRIPTS_LAST - SCRIPTS_FIRST)) {
				break; // Maximum scripts reached
			}

			if (!script->isEnabled()) {
				scriptIndex++;
				continue;
			}

			wxString label = wxString::FromUTF8(script->getDisplayName());
			wxString shortcut = wxString::FromUTF8(script->getShortcut());

			if (!shortcut.IsEmpty()) {
				label += "\t" + shortcut;
			}

			wxMenuItem* item = scriptsMenu->Append(
				MAIN_FRAME_MENU + SCRIPTS_FIRST + scriptIndex,
				label,
				wxString::FromUTF8(script->getDescription())
			);
			scriptIndex++;
		}
	}
}

void MainMenuBar::RefreshScriptsMenu() {
	LoadScriptsMenu();
}

void MainMenuBar::LoadShowMenu() {
	using namespace MenuBar;

	if (!showMenu) {
		return;
	}

	size_t total = showMenu->GetMenuItemCount();
	if (showMenuCount > 0) {
		for (size_t i = 0; i < showMenuCount && total > 0; ++i) {
			showMenu->Delete(showMenu->FindItemByPosition(total - 1));
			--total;
		}
	}
	if (showMenuHasSeparator && total > 0) {
		showMenu->Delete(showMenu->FindItemByPosition(total - 1));
		--total;
	}

	showMenuCount = 0;
	showMenuHasSeparator = false;

	const auto& shows = g_luaScripts.getMapOverlayShows();
	if (shows.empty()) {
		return;
	}

	showMenu->AppendSeparator();
	showMenuHasSeparator = true;

	const size_t maxCount = static_cast<size_t>(SHOW_CUSTOM_LAST - SHOW_CUSTOM_FIRST);
	size_t count = 0;
	for (const auto& show : shows) {
		if (count >= maxCount) {
			break;
		}

		wxMenuItem* item = showMenu->Append(
			MAIN_FRAME_MENU + SHOW_CUSTOM_FIRST + static_cast<int>(count),
			wxString::FromUTF8(show.label),
			wxString(),
			wxITEM_CHECK
		);
		if (item) {
			item->Check(show.enabled);
		}
		++count;
	}

	showMenuCount = count;
}

void MainMenuBar::RefreshShowMenu() {
	LoadShowMenu();
}

void MainMenuBar::OnExportMinimap(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	wxFileDialog dialog(frame, "Export Minimap", wxEmptyString, wxEmptyString,
		"PNG files (*.png)|*.png", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	if (dialog.ShowModal() == wxID_OK) {
		g_gui.GetCurrentEditor()->exportMiniMap(dialog.GetPath(), GROUND_LAYER, true);
	}
}
