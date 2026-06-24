# Mios Map Editor - Project Status & Roadmap

## Project Origins & Inspiration
* **Origin Repository:** [OTAcademy/RME](https://github.com/OTAcademy/RME) (Our baseline project)
* **Inspiration Repository:** [karolak6612/remeres-map-editor-redux](https://github.com/karolak6612/remeres-map-editor-redux) (Inspiration for this custom fork)

## ?? In Arbeit / Nchste Aufgaben (Next to Do)
* 🟡 **Multiplayer-Stabilität:** Langzeittest der VBO-Synchronisation bei 4 gleichzeitigen Usern unter Last.
* 🟡 **Advanced Shop Gen:** Integration eines Dialogs zur Auswahl von Ladentypen (Waffen, Magie, Nahrung).

## ?? Anmerkungen fr die Entwicklung (Notes for AI)
* ?? <span style="color: lightblue;">Info:</span> Implementierung aller vier Features "am Stck" geplant. (Warte auf Freigabe des Implementation Plans).

## 🎉 Erledigt (Completed)

### Mio's Map Editor v4.5 Fork Enhancements
*   **Branding & Executable:** Renamed compiled target output to `MME.exe` and updated Welcome Screen to display "Mio's Map Editor" and "v4.5 (a Fork by Mioshiro)".
*   **Vulkan Shader Fix:** Created Vulkan-compatible GLSL vertex/fragment shaders and compiled them successfully to `.spv` formats (`map_vertex.glsl.spv`, `map_fragment.glsl.spv`), resolving the SPIR-V loader startup errors.
*   **Global Dark Style Recursion:** Recursively styled preferences and all common dialogs (white text on dark backgrounds) for optimal readability.
*   **Startup Popup Suppression:** Disabled the startup hardware profiler dialog box ("Cme Information").
*   **Horizontal Centered Toolbar:** Re-oriented the "Tools & Brush" toolbar horizontally with centered elements, restricting its docking direction to Top-only to prevent docking to left/right side edges.
*   **Aligned Welcome Checkbox:** Aligned the "Show this dialog on startup" checkbox flush with the start of the action buttons.
*   **Prefab Icon Restoration:** Updated the toolbar button with the correct custom prefab creator icon (`prefab.png`).
*   **OpenGL Extension Loader Evaluation:** Assessed necessity and concluded that direct manual loading of the required VBO pointers is sufficient and cleaner for now.

### Layout & Design
*   **Settings Window & Layout Overhaul:** Fixed color conversion bugs in style_manager.h and preferences.cpp (scaling float color components by 255) so Settings tab text and checkbox labels render in readable white. Complete layout overhaul to match the retail design: left sidebar houses Creature Palette by default, bottom panel contains detached Collections Palette/Tileset, right sidebar contains vertically stacked In-game Preview (top) and Minimap (bottom), and top horizontal toolbar contains Intensity and Ambient sliders and a Toggle Lighting check button.
*   **Landingpage (Welcome Dialog):** Switched to Modern Fantasy RPG Design (Black/Blue with golden borders).
*   **Community Edition Branding:** App renamed to "Mio's Map Editor - Community Edition".
*   **Refined Dark Mode:** Centralized style system (StyleManager) with golden accents (#B48C32) and deep blue backgrounds.
*   **SVG-Toolbar:** Scalable vector toolbar via NanoVG. Icons for selection, brush, eraser, floor change, and waypoints finalized.
*   **Combined Tool Module:** "Tools" and "Brush Size" merged into a single docked panel for better space management.
*   **Savegame Management:** Recently opened maps (Recent Files) can be deleted directly from the game save list.
*   **Standard Theme:** Cursor updated to gold, background set to dark blue (settings.cpp).
*   **Preferences Redesign & DPI Auto-Sizing:**
    *   Replaced the directory pickers with a clean single-dropdown version selector, dynamic **Scan status** text, and an **Open** button.
    *   **Dynamic Scan Logic**: Selecting a version scans data/<version> for Tibia.dat and Tibia.spr. If present, it shows **Scan: Ready to use** in green and hides the download hyperlink. If missing, it specifies the missing file (e.g. **Scan: Tibia.dat missing**) in red, showing both the **Open** button and the download hyperlink.
    *   **Open Button**: Creates the folder if missing, opens it in Windows Explorer, and opens the downloads page in the browser.
    *   Changed **Copy Position Format** selection from a large radio box group to a wxChoice dropdown.
    *   Removed hardcoded window size bounds, allowing the dialog to automatically fit its content dynamically (DPI-aware).
*   **Selection-Based Prefab Creator Brush:** Highlight tiles on the map and save/register them as a virtual brush, with optional base64-safe .prefab file export. Uses the custom prefab.png icon.
*   **Integrated Tileset Search:** Housed a search box directly under the tileset dropdown to filter brushes in real-time. Typing inside the search box safely intercepts input events, preventing global menu hotkeys from triggering inadvertently.
*   **Movable Toolbars:** Unlocked toolbar docking constraints so brush and size toolbars can be placed next to standard/edit bars.
*   **About Window:** Updated dialog text to list "Mioshiro" as Fork-Editor with fork-specific improvements.
*   **Erweiterte Modul-Fenster:** "Tools" und "Brush Size" zu einem sauberen, kombinierten Modul zusammengefasst.
*   **Layout-Sicherheit (MinSize):** Mindestgrößen für alle Module (Palette, Tools/Brush, Search Results) definiert, um Kollabieren (Ineinanderquetschen) im Grid-System strikt zu verhindern.
*   **Visuelles Docking-Feedback:** Transparente, hellblaue Einrast-Vorschauen (`wxAUI_MGR_TRANSPARENT_HINT` & `wxAUI_MGR_HINT_FADE`) beim Bewegen von Fenstern aktiviert.

### Performance & Rendering
*   **Canvas Lag Optimization (Texture & Leaf Caching):** Fixed rendering lag in zoomed-out high perspectives by caching GL texture binds (preventing redundant glBindTexture state changes) and caching quadtree leaf lookups in BaseMap.
*   **Canvas Dragging Rendering Throttle:** Mitigated dropped-frame stutter during high-zoom brush dragging (like drawing continuous island borders) by throttling wxGLCanvas::Refresh events to 60 FPS in the mouse polling loop. This prevents immediate mode OpenGL draw calls from blocking the application message queue.
*   **60 FPS Performance Target:** Committed to a strict "60 FPS Minimum" standard, treating the editor's rendering pipeline like a game engine that leverages full CPU/GPU resources under heavy asset/animation/autoborder loads.
*   **Redux Atlas Architecture:** Texture Atlasing (2048x2048) for single-call chunk rendering.
*   **Vulkan Core Backend:** Initialization pipeline (Skelett) vorbereitet (noch umgangen zugunsten von Legacy OpenGL).
*   **RT Global Illumination:** Raytracing-based Compute-Shader Pass including Emissive-LUT for dynamic light reflections.
*   **ECS Architecture:** EnTT integration to manage tens of thousands of NPCs with autonomous wandering AI.
*   **MVP-Matrix & Modern Camera:** Removal of glOrtho and glMatrixMode. Camera controlled via glm::mat4.
*   **Pure GPU Lighting:** Deferred system with Gaussian blur for soft shadow edges.
*   **VBO Ring Buffering:** Triple-Buffering Architektur im Code als Backend vorbereitet.
*   **OpenGL VBO Triple-Buffering Crash Fix:**
    *   Generated the VBO pool buffers using glGenBuffers in SetupGL() and allocated buffer memory via glBufferData, fixing the rendering crash on canvas creation/loading.
    *   Deleted buffers in destructor to prevent leaks.
*   **OpenGL Context Initialization Fix:**
    *   Explicitly called SetCurrent() to activate the OpenGL context in MapCanvas before instantiating MapDrawer. This ensures NanoVG initialization (which depends on an active context) succeeds without crashing during map creation or loading.
    *   Added shader/style compiler invocation inside SetupGL() if m_shader_program is uninitialized, resolving missing shader rendering errors ("white canvas") on start.
*   **VBO Sprite-Batching (Final):** Vollständige Umstellung auf VBOs inklusive Shader-Flags.
*   **Vulkan High-DPI & Triple-Buffering:** Finalisierte Pipeline für modernste Hardware-Unterstützung.
*   **Async Sprite Streamer:** Worker-Thread für ruckelfreies Laden implementiert.
*   **NanoVG HUD:** FPS counter and coordinate overlays migrated to NanoVG.
*   **Code Decoupling:** Tooltip logic and HUD rendering moved to OverlayManager.
*   **Async Sprite Loading (Skelett):** Logik in Sprite-Klassen vorbereitet (Wartet auf Worker-Thread `AsyncLoader`).
*   **MVP-Shader Core:** map_vertex.glsl uses only the MVP camera matrix.
*   **Batch-Dirty Marking:** System to suppress recursive tree updates during mass changes.
*   **i18n Localization:** Tooltip contents moved to i18n.h for easy translation.
*   **Undo/Redo Snapshot System:** Binary region snapshots for efficient undo operations.
*   **C++20 Modernization:** Integrated std::format, std::span, and std::ranges in the core renderer.
*   **Vertex Shader Animation:** GPU-calculated water and lava animations.
*   **DPI Migration:** UI elements in viewport scale losslessly.
*   **SVG Icon Migration:** Vector icons defined for main toolbar (svg_icons.h).
*   **std::format Transition:** Modernized logging and sprite metadata processing.
*   **C++20 Refactoring:** std::span and std::ranges for tile iteration and layer filtering.
*   **Brush Concepts:** C++20 IsBrush Concept for strict type safety.
*   **RPG ProgressBar:** Modern progress bar design for loading operations.
*   **Frustum Culling:** Chunk-level visibility check.
*   **Dirty-Flag Logic:** Recursive invalidation of map areas in the HexTree.
*   **Batch Rendering:** Grouping drawing commands to reduce draw calls.
*   **FPS Unlock:** Hard-refresh rate von 16ms auf 2ms gesenkt (erlaubt 160-500 FPS bei Interaktion).
*   **Zoom Optimization:** Reduced resource usage when zooming.
*   **Shader & Style Hot-Reload:** Automatic reloading of GLSL files and ui_style.xml at runtime.
*   **NanoVG UI Migration:** Tooltips, SelectionBox, and BrushIndicators migrated.
*   **OpenGL 4.x Core Transition:** Post-processing uses VAO/VBO instead of glBegin.
*   **VAO Infrastructure:** Static VAO for full-screen effects.
*   **Post-Processing Pipeline:** FBO system for shader effects (CRT, FXAA).

### Automation & Tooling
*   **Windows Compiler:** windows_compiler.bat created for automated release builds with vcpkg integration.
*   **Standalone Release:** Automatically packs resources (data, brushes, etc.) and uses portable config via editor.cfg.
*   **Error & Crash Logging:**
    *   Enabled wxHandleFatalExceptions(true).
    *   Overrode OnFatalException, OnUnhandledException, and OnExceptionInMainLoop to write structured exception crashes (like access violations) and C++ exceptions to error.log with timestamps.

## Status & Solved Blockers ??
*   **Architecture:** Redux-Parity im UI und als Architektur-Skelett erreicht. Tiefer Renderer & Worker-Threads noch ausstehend (~60%).
*   **Vulkan:** Initializing pipeline and buffer management is stable.
*   **Performance:** Hohe FPS bei Interaktion erreicht (Cap auf 2ms reduziert). Caches implemented for textures/quadtree leaf lookups to maintain smooth frame rates during zoom/pan.
*   **UI:** Modern Corporate look fully integrated.
*   **Solved Blocker (MSVC Compiler):** Outlined structural errors in lua_api_algo.cpp regarding local structs inside lambdas (BSPNode) have been resolved by moving BSPNode to the outer scope namespace.
*   **Solved Blocker (OpenGL Canvas Crash):** Resolved the triple-buffer crash on canvas load by initializing m_vbo_pool buffers in SetupGL().
*   **Solved Blocker (New Map Crash / White Screen):** Resolved crashes and white canvas bugs during new map creation by binding the correct OpenGL context before creating the drawer, and compiling shaders immediately upon SetupGL initialization.

## Backlog / Future Ideas ??
*   **Asynchronous Loading:** Completely move map loading to background threads (non-blocking UI).

## Roadmap 2026 ??

### Phase 3.5: UI Modernization & Asset Fix (Aktuell)
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Modul-Flexibilität:** Alle Fenster (Palette, Preview, etc.) können nun über das "X" geschlossen werden.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Minimap-Redundanz:** Separates Minimap-Fenster entfernt zugunsten des In-Canvas HUDs.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Asset-Pipeline Fix:** Korrektur der Tileset-Kategorien und Laderoutine für Assets (RME-Parity).
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Standard-Layout:** GUI startet nun automatisch im klassischen RME-Layout (Creatures links, Collections unten).
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Vibe-Coding Parity:** Alle Kern-Monolithen (`map`, `gui`, `lua`, `items`) erfolgreich in < 500-Zeilen Module zerlegt.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Vibe-Coding Baseline:** Projekt-Struktur und `atlantica.code-workspace` für LLM-Kontextfenster optimiert (Module < 500 Zeilen).
*   ?? <span style="color: lightgreen;">Erledigt:</span> **UI Design Alignment:** Farbschema und Anordnung an das Referenz-Design angepasst.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Code-Modularisierung (Core):** `gui.cpp`, `editor.cpp` und nun auch `map.cpp` in spezialisierte Teil-Komponenten zerlegt.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Toolbar-Finalisierung:** Kombination von Buttons (Modi) und Brushes (Werkzeuge) in einer klassischen Desktop-Symbolleiste mit Hover-Effekten.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Grafik-Modularisierung:** `graphics.cpp` in Loading, Texture-Management und Atlas-Generation unterteilt.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Lua-API Modularisierung:** `lua_api.cpp` in logische Module (`map`, `tile`, `item`, `creature/player`) aufgeteilt.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Data-Modularisierung:** `items.cpp`, `materials.cpp` und `iomap_otbm.cpp` in spezialisierte Teil-Komponenten zerlegt.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **In-Canvas Minimap Toggle:** Minimap kann nun über das View-Menü im Canvas ein- und ausgeblendet werden.
*   🟡 **In Arbeit:** **Performance-Tuning:** Optimierung der VBO-Build-Zeiten in den neuen Drawer-Modulen.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Editor-Refactoring:** Finalisierung der `editor_actions.cpp` für komplexe Brush-Operationen.
*   🟢 <span style="color: lightgreen;">Erledigt:</span> **Compiler Size Check:** Limit-Prüfung (< 500 Zeilen) in `windows_compiler.bat` integriert.
*   🟢 <span style="color: lightgreen;">Erledigt:</span> **Display-Modularisierung (Schritt 1):** Die Menü-Logik aus `map_display.cpp` wurde in `map_display_menu.cpp` ausgelagert.
*   🟢 <span style="color: lightgreen;">Erledigt:</span> **Display-Modularisierung (Schritt 2):** Auslagerung von `OnPaint` und `TakeScreenshot` aus `map_display.cpp` in `map_display_paint.cpp`.
*   🟢 <span style="color: lightgreen;">Erledigt:</span> **Drawer-Modularisierung:** Brush-Rendering-Code von `map_drawer.cpp` in `map_drawer_brush.cpp` ausgelagert und doppelte UI-Definitionen bereinigt.
*   🟢 <span style="color: lightgreen;">Erledigt:</span> **GUI-Modularisierung:** `gui.cpp` durch Auslagerung von Hotkeys (`gui_hotkeys.cpp`) und Perspektiven (`gui_perspective.cpp`) verkleinert.
*   🟢 <span style="color: lightgreen;">Erledigt:</span> **Menü-Modularisierung:** `main_menubar.cpp` verkleinert durch Trennung von Such- (`main_menubar_search.cpp`) und Map-Aktionen (`main_menubar_map.cpp`).
*   🟡 **In Arbeit:** **Iterative Modularisierung:** Letzte Überprüfungen des 500-Zeilen-Limits auf verbleibende Dateien.

### Phase 1: Stability & Modern Foundation (Completed)
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Layer-Solo System:** Hotkey 'L' to isolate the active layer.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Vulkan Triple-Buffering:** Minimizing input lag at high framerates. (VBO-Synchronisation bei 4 gleichzeitigen Usern unter Last)
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Rename Refactor:** Transformed entire core to MapEditor.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Preferences Redesign:** Dropdown versions and scan status.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Crash Handler:** Crash reports logged to error.log.

### Phase 2: Intelligent Editing Tools (Q2 2026)
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Prefab System (V2):** Clipboard import & Vulkan VBO sync for smooth insertion, and Prefab Creator Brush.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **SVG Palette Icons:** Razor-sharp icons at all zoom levels (including prefab icon loaded dynamically).

### Phase 3: Performance & Workflow (Q3 2026)
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Event-Driven FPS Overlay:** Corrected thresholds for event-based rendering to prevent false performance alarms.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Autoborder Optimization:** Changed underlying tile-collection logic to O(1) std::set structures, caching texture binds and quadtree leaves to fix mass-placement rendering lag.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Smart-Boarding 2.0:** Context-sensitive shadow and edge highlights for walls and tables.
*   🟡 <span style="color: orange;">In Arbeit:</span> **Async Sprite Streamer:** Hintergrund-Laden (Worker-Thread) muss noch verknüpft werden.
*   🟡 <span style="color: orange;">In Arbeit:</span> **Modernes VBO Sprite-Batching:** Ersetzen von Immediate-Mode OpenGL in `map_drawer.cpp`.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Random Town Name Generator:** Würfel-Button im Town-Editor integriert.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Palette Loading Fix:** Fehler beim Laden mehrerer Tilesets behoben.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Shop Generator (V1):** Lua-Script zur automatischen Erzeugung von RPG-Läden unter extensions/shop_gen.lua.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Build Fix (Shaders & Kern):** Shader-Kompilierung in Batch korrigiert und Syntaxfehler im C++ Kern behoben.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **UI-Unified-Design:** RPG-Farbschema auf alle Hauptdialoge angewendet.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Multiplayer-Sync Optimization:** Client-Batching und Server-Caching für lag-freies Mapping.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Shop Gen Expansion:** Neue Kategorien (Rüstung, Juwelier, Bibliothek) hinzugefügt.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **RPG Toolset Scripts:** Nature Scatter, Room Clutter und Lighting Wizard implementiert.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **UI-Polishing (Map Properties):** Map Properties Dialog im RPG-Look verfeinert (StaticBoxes, Gold-Header).
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Multiplayer-Labels:** Spieler-Namen werden nun über dem Cursor via ImGui (Frontend) gerendert.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Multiplayer Menu:** "Multiplayer"-Hauptmenü mit Host/Join Logik verknüpft.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Legacy Script Cleanup:** Veraltete und nutzlose Scripte aus dem Menü entfernt.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **Undo Snapshots:** High-performance undo operations on >100k changes.
*   ?? <span style="color: lightgreen;">Erledigt:</span> **OTClient Export:** Export world map renders directly for OTClient.

### Phase 4: Collaboration (Q4 2026)
*   **Live-Multiplayer Mapping:** Stabilize live servers for mapping teams.
*   **Web Preview:** Interactive map preview in the web browser.
*   **Corporate UI Redesign:** Full migration to procedural NanoVG-GUI (Black/White theme) without bitmap assets.
*   **AI Asset Integration:** Automatic download/generation of sprites and icons via AI API interfaces.