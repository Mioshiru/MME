# MME (Mios Map Editor) - Project Source Map & Architecture Directory

This document provides a comprehensive overview and Table of Contents of the codebase, categorizing source files by their functional domain within the application.

---

## 1. Palette System (`Palette`)
The Palette system handles side panels, brush selectors, item/creature trees, house management, and waypoint controls.

| File Name | Description |
| :--- | :--- |
| [`source/palette_window.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/palette_window.cpp) | Main container window for docking all palette notebooks and tabs. |
| [`source/palette_common.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/palette_common.cpp) | Base components, shared UI helpers, and controls used across all palettes. |
| [`source/palette_brushlist.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/palette_brushlist.cpp) | Brush selection lists for terrain, walls, doodads, and RAW item palettes. |
| [`source/palette_creature.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/palette_creature.cpp) | Creature & NPC palette selection tree and preview renderers. |
| [`source/palette_house.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/palette_house.cpp) | House, house exit, and town management panel. |
| [`source/palette_waypoints.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/palette_waypoints.cpp) | Waypoint management palette interface for pathing & AI markers. |

---

## 2. Canvas & Rendering Engine (`Canvas`)
The Canvas domain manages map rendering, viewports, mouse/keyboard input on the map, overlays, ImGui radial wheel, and minimap displays.

| File Name | Description |
| :--- | :--- |
| [`source/map_display.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/map_display.cpp) | Core interactive map canvas (`MapCanvas`), mouse/keyboard event loop, and viewport scrolling. |
| [`source/map_display_paint.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/map_display_paint.cpp) | Canvas paint pass, ImGui overlay rendering, Shift+Q Radial Tool Wheel, and brush previews. |
| [`source/map_display_menu.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/map_display_menu.cpp) | Canvas right-click context menus and popup actions. |
| [`source/map_drawer.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/map_drawer.cpp) | OpenGL map rendering pipeline, tile layer drawing, grid lines, and lighting routines. |
| [`source/drawer_overlay.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/drawer_overlay.cpp) | Canvas HUD overlays, minimap render boxes, and hover tooltips. |
| [`source/map_window.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/map_window.cpp) | MDI sub-window wrapper containing a single map display canvas. |
| [`source/map_tab.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/map_tab.cpp) | Tab bar page wrapper for multi-document map editor tabs. |

---

## 3. Main Menu Bar (`Menüleiste`)
Controls top-level application menus, menu shortcuts, search commands, and view toggles.

| File Name | Description |
| :--- | :--- |
| [`source/main_menubar.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/main_menubar.cpp) | Construction and event handlers for File, Edit, View, Window, and Help menus. |
| [`source/main_menubar_map.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/main_menubar_map.cpp) | Map menu commands (Map Properties, Clean, Statistics, Floor Navigation). |
| [`source/main_menubar_search.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/main_menubar_search.cpp) | Search menu actions (Find Item, Find Unique ID, Find Action ID). |
| [`source/gui_menu.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/gui.cpp) | GUI state synchronizer for updating checked/enabled states of menu items. |

---

## 4. Main Tool Bar & UI Controls (`Toolleiste`)
Manages primary toolbars, tool selection icons, high-DPI art providers, and custom UI controls.

| File Name | Description |
| :--- | :--- |
| [`source/main_toolbar.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/main_toolbar.cpp) | Main application toolbar construction, tool buttons, and icon scaling. |
| [`source/artprovider.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/artprovider.cpp) | High-DPI candidate icon loader and fallback artwork provider for tools/zones. |
| [`source/dcbutton.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/dcbutton.cpp) | Custom device-context button control for specialized toolbar buttons. |
| [`source/gui_ids.h`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/gui_ids.h) | Central menu and toolbar action identifier declarations. |

---

## 5. Pop-Up Windows & Dialogs (`Pop-Up Fenster`)
Includes preferences, modal property editors, diagnostic tools, and wizard dialogs.

| File Name | Description |
| :--- | :--- |
| [`source/preferences.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/preferences.cpp) | Wide-format Settings/Preferences dialog with sub-tabs (General, Editing, Graphic, Interface, Hotkeys). |
| [`source/properties_window.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/properties_window.cpp) | Modern item, tile, creature, and spawn property editor window. |
| [`source/old_properties_window.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/old_properties_window.cpp) | Legacy item and container properties edit window. |
| [`source/container_properties_window.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/container_properties_window.cpp) | Specialized container contents and slot editor dialog. |
| [`source/browse_tile_window.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/browse_tile_window.cpp) | Tile contents browser window for stacked items. |
| [`source/map_diff_window.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/map_diff_window.cpp) | Visual map comparison and diff review window. |
| [`source/map_diagnostic_window.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/map_diagnostic_window.cpp) | Map error, duplicate item, and invalid tile diagnostic scanner dialog. |
| [`source/find_item_window.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/find_item_window.cpp) | Item search dialog with filtering by name, ID, or attribute. |
| [`source/replace_items_window.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/replace_items_window.cpp) | Batch item replacement dialog across maps or selections. |
| [`source/add_tileset_window.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/add_tileset_window.cpp) | Custom tileset creation and edit window. |
| [`source/tfs_npc_editor.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/tfs_npc_editor.cpp) | TFS NPC file editor and dialogue builder dialog. |
| [`source/tfs_quest_generator.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/tfs_quest_generator.cpp) | TFS Quest script generator wizard. |
| [`source/creature_wiki_dialog.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/creature_wiki_dialog.cpp) | Tibia Creature Wiki & Bestiary Knowledge Base dialog with live search, official difficulty filtering, clickable column sorting, and heart favorites. |
| [`source/creature_bestiary.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/creature_bestiary.cpp) | Official TibiaWiki creature stats database, HP/EXP ratio calculator, and CipSoft difficulty tier estimator. |
| [`source/welcome_dialog.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/welcome_dialog.cpp) | Startup welcome screen, recent map history, and quick actions. |
| [`source/about_window.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/about_window.cpp) | Application About dialog with credits and version information. |
| [`source/common_windows.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/common_windows.cpp) | Shared auxiliary modal windows (Jump To Position, Selection Options, Floor Jumper). |

---

## 6. Brushes & Editing Tools (`Brushes & Tools`)
Implements placement brushes, flood fill, magic wand selection, and automatic border logic.

| File Name | Description |
| :--- | :--- |
| [`source/brush.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/brush.cpp) | Core brush class hierarchy, shape calculation, and brush sizing. |
| [`source/ground_brush.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/ground_brush.cpp) | Ground terrain painting, autobordering, and flood fill (bucket) algorithms. |
| [`source/wall_brush.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/wall_brush.cpp) | Automatic wall cornering, door placement, and window alignment logic. |
| [`source/doodad_brush.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/doodad_brush.cpp) | Multi-tile doodad and composite object placement brush. |
| [`source/creature_brush.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/creature_brush.cpp) | Monster & NPC placement brush. |
| [`source/house_brush.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/house_brush.cpp) | House tile & exit assignment brush. |
| [`source/spawn_brush.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/spawn_brush.cpp) | Creature spawn area brush. |
| [`source/eraser_brush.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/eraser_brush.cpp) | Eraser tool logic for removing items/tiles. |

---

## 7. Backend Engine & Map Core (`Backend-Funktionen`)
Core data structures, file I/O (OTBM/OTMM), undo/redo history, item registry, and graphics manager.

| File Name | Description |
| :--- | :--- |
| [`source/editor.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/editor.cpp) | Main editor document state, action stack, selection manager, and copy buffer. |
| [`source/map.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/map.cpp) | High-level map representation, spatial indexing, house/town lists, and tile lookups. |
| [`source/basemap.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/basemap.cpp) | Low-level grid storage for map nodes and quad-tree regions. |
| [`source/tile.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/tile.cpp) | Tile data model (ground, items, creatures, spawns, zone flags). |
| [`source/item.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/item.cpp) | Item instance model, attributes (Action ID, Unique ID, text), and sub-items. |
| [`source/items.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/items.cpp) | Global item type database (`items.otb` / `items.xml` loader). |
| [`source/iomap_otbm.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/iomap_otbm.cpp) | Binary OTBM map format reader and writer. |
| [`source/graphics.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/graphics.cpp) | Sprite sheet loader (`Tibia.dat` / `Tibia.spr`), texture cache, and surface blitters. |
| [`source/action.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/action.cpp) | Undo/Redo action queue implementation. |

---

## 8. Multiplayer Live Collaboration (`Multiplayer Live`)
Real-time peer-to-peer and client-server map co-editing.

| File Name | Description |
| :--- | :--- |
| [`source/live_server.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/live_server.cpp) | Live collaboration host server manager. |
| [`source/live_client.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/live_client.cpp) | Live collaboration client sync manager. |
| [`source/live_socket.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/live_socket.cpp) | Low-level TCP socket handler for live editing packets. |
| [`source/live_peer.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/live_peer.cpp) | Connected user session & peer state tracking. |

---

## 9. Scripting & Extension Engine (`Lua Scripting`)
Lua API bindings and custom extension script execution environment.

| File Name | Description |
| :--- | :--- |
| [`source/lua/lua_script_manager.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/lua/lua_script_manager.cpp) | Main Lua environment initialization, plugin loader, and event bindings. |
| [`source/lua/lua_api_map.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/lua/lua_api_map.cpp) | Lua map manipulation API bindings. |
| [`source/lua/lua_api_brush.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/lua/lua_api_brush.cpp) | Lua brush execution and procedural generation bindings. |
