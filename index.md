# MME (Mios Map Editor) - Funktionsliste & QA-Regression-Testplan

Dieses Dokument dient als zentrale **Funktions- und Test-Checkliste** des MME Projekts. Es verknüpft die Quellcode-Architektur direkt mit testbaren Schritten, um nach jeder Änderung eine systematische Qualitätskontrolle (Regression Testing) durchzuführen.

---

## Legende für das Testmanagement
- `[ ]` Noch nicht getestet
- `[x]` Erfolgreich getestet / OK
- `[!]` Fehler / Bug gefunden (Details in Issue / Chat notieren)

---

## 1. Paletten-System (`Palette Management`)
Verwaltet Seitenleisten, Pinselwähler, Item-/Monster-Bäume, Haus- und Wegpunkt-Verwaltung.

| Quellcode-Dateien | Zuständigkeit |
| :--- | :--- |
| [`source/palette_window.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/palette_window.cpp) | Haupt-Containerfenster für angedockte/schwebende Paletten |
| [`source/palette_common.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/palette_common.cpp) | Basis-Steuerelemente, Pinselbuttons, Listen |
| [`source/palette_brushlist.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/palette_brushlist.cpp) | Terrain-, Doodad-, Wall- und RAW-Listen |
| [`source/palette_creature.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/palette_creature.cpp) | Monster- & NPC-Auswahl |
| [`source/palette_house.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/palette_house.cpp) | Häuser-, Town- und Exit-Verwaltung |
| [`source/palette_waypoints.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/palette_waypoints.cpp) | Wegpunkt- und Pfad-Verwaltung |

### Test-Checkliste Palette
- [ ] **Erste Palette (Hauptpalette)**: Öffnet sich beim Start am rechten Rand im Dark-Theme ohne Flackern.
- [x] **Zweite Palette (`Windows -> New Palette`)**: Öffnet sich als schwebendes Fenster (`Float`), stürzt nicht ab, kann frei bewegt und angedockt werden.
- [ ] **Minimap-Modul**: Minimap lässt sich in der Palette ein-/ausklappen und per Rechtsklick-Kontextmenü aktivieren/deaktivieren.
- [ ] **Quest Checklist Modul**: Lässt sich per Rechtsklick-Kontextmenü einblenden und Einträge abhaken/bearbeiten.
- [ ] **Paletten-Kategorien**: Umschalten zwischen Terrain, Doodads, Items, Creatures, Houses, Waypoints, RAW und Prefabs funktioniert sofort.
- [ ] **Live-Suche**: Suchfeld filtert Tilesets/Items in Echtzeit; `ESC` leert die Suche und setzt den Fokus zurück.
- [ ] **Favoriten**: Items können zu den Favoriten hinzugefügt werden und die Favoriten-Box aktualisiert sich nahtlos.

---

## 2. Canvas & Rendering-Engine (`Map Viewport`)
Zeichnen der Tiles, Beleuchtung, Layer, Overlays, ImGui Radial-Menü und Minimap.

| Quellcode-Dateien | Zuständigkeit |
| :--- | :--- |
| [`source/map_display.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/map_display.cpp) | MapCanvas, Maus- und Tastatur-Events, Viewport-Scrolling |
| [`source/map_display_paint.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/map_display_paint.cpp) | Paint-Pipeline, ImGui Overlays, Shift+Q Radial Wheel, Pinsel-Vorschau |
| [`source/map_display_menu.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/map_display_menu.cpp) | Rechtsklick-Kontextmenüs auf der Map |
| [`source/map_drawer.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/map_drawer.cpp) | OpenGL/BGFX Rendering, Layer-Blending, Gitterlinien, Lichteffekte |
| [`source/drawer_overlay.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/drawer_overlay.cpp) | HUD-Overlays, Minimap-Kasten, Hover-Tooltips |
| [`source/map_tab.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/map_tab.cpp) | Tabs für mehrere geöffnete Karten |

### Test-Checkliste Canvas
- [ ] **Kartenansicht & Zoom**: Zoomen mit Mausrad (vor/zurück) und Panning mit mittlerer Maustaste/Leertaste flüssig.
- [ ] **Stockwerk-Navigation**: `+` / `-` bzw. Bild-Auf / Bild-Ab wechselt die Ebenen (Floor 0 bis 15) sauber.
- [ ] **ImGui Radial Tool Wheel (`Shift + Q`)**: Öffnet sich am Mauszeiger, Pinsel- und Werkzeugauswahl funktioniert per Klick.
- [ ] **Pinsel-Vorschau**: Zeigt die Pinselform (Quadrat/Kreis) mit exakter Größe (1-7) transparent auf der Karte an.
- [ ] **Rechtsklick-Menü**: Properties, Cut, Copy, Delete, Browse Field, Create Prefab öffnen die korrekten Aktionen.
- [ ] **Multi-Tab**: Mehrere Maps können in Tabs geöffnet und unabhängig voneinander editiert werden.

---

## 3. Menüleiste (`Main Menu Bar`)
Alle Befehle in Datei, Bearbeiten, Ansicht, Karte, Suchen, Tools, TFS-Tools, Live, Window, Hilfe.

| Quellcode-Dateien | Zuständigkeit |
| :--- | :--- |
| [`source/main_menubar.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/main_menubar.cpp) | Menü-Definitionen, Event-Routing |
| [`source/main_menubar_map.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/main_menubar_map.cpp) | Map-Menü (Eigenschaften, Bereinigen, Statistik, Floor) |
| [`source/main_menubar_search.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/main_menubar_search.cpp) | Suchen-Menü (Find Item, Action-ID, Unique-ID) |

### Test-Checkliste Menüs
- [ ] **File**: New Map, Open Map, Save Map (`Strg+S`), Save As, Export OTBM/OTC, Recent Files Liste.
- [ ] **Edit**: Undo (`Strg+Z`), Redo (`Strg+Y`), Select All, Selection Mode, Preferences (`P`).
- [ ] **View**: Toggle Spawns, Houses, Pathing, Special Tiles, Light Shading, FPS Display, Chat.
- [ ] **Map**: Map Properties (Name, Client-Version, Dimensionen), Clean Map, Statistics (Item-/Spawn-Zählung).
- [ ] **Search**: Find Item (`Strg+F`), Find Unique ID, Find Action ID, Jump to Coordinate (`Strg+J`).

---

## 4. Werkzeugleiste & Pinsel (`Toolbar & Brushes`)
Modi, Pinsel-Größen, Pinsel-Formen und Zeichenwerkzeuge.

| Quellcode-Dateien | Zuständigkeit |
| :--- | :--- |
| [`source/main_toolbar.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/main_toolbar.cpp) | Obere Werkzeugleiste, Icons, Tooltips |
| [`source/brush.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/brush.cpp) | Pinsel-Basishierarchie |
| [`source/ground_brush.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/ground_brush.cpp) | Ground-Terrain, Auto-Border, Eimer (Bucket Fill) |
| [`source/wall_brush.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/wall_brush.cpp) | Wände, Ecken, Türen, Fenster |
| [`source/eraser_brush.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/eraser_brush.cpp) | Radiergummi-Pinsel |

### Test-Checkliste Werkzeuge & Pinsel
- [ ] **Pencil (Stift)**: Normales Platzieren von Terrain und Items.
- [ ] **Bucket (Fülleimer)**: Füllt geschlossene Bereiche mit dem gewählten Terrain aus.
- [ ] **Eraser (Radierer)**: Entfernt Items/Terrain entsprechend der eingestellten Filter.
- [ ] **Auto-Bordering**: Erzeugt korrekte Übergangskanten zwischen verschiedenen Bodentypen.
- [ ] **Türen- & Wand-Tool**: Setzt normale, Locked-, Magic-, Quest- und Window-Türen korrekt in Wände.
- [ ] **Zonen-Pinsel**: PZ (Protection Zone), Non-PVP, No-Logout und PVP-Zones lassen sich zeichnen und optisch hervorheben.

---

## 5. Dialoge, Tools & Assistenten (`Pop-Up Windows & Tools`)
Spezialfenster für Einstellungen, Diagnose, NPC/Quest-Generatoren und Prefabs.

| Quellcode-Dateien | Zuständigkeit |
| :--- | :--- |
| [`source/preferences.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/preferences.cpp) | Einstellungen (Allgemein, Grafik, Interface, Hotkeys, Paletten) |
| [`source/properties_window.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/properties_window.cpp) | Item-/Spawn-/Creature-Eigenschaften (Action ID, Unique ID, Text) |
| [`source/map_diagnostic_window.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/map_diagnostic_window.cpp) | Diagnose-Scanner für ungültige Tiles, Duplikate & Geisteritems |
| [`source/map_diff_window.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/map_diff_window.cpp) | Vergleich zweier Maps mit farblicher Hervorhebung |
| [`source/procedural_generator_window.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/procedural_generator_window.cpp) | Prozeduraler Generator für Höhlen, Inseln und Ruinen |
| [`source/creature_wiki_dialog.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/creature_wiki_dialog.cpp) | Tibia Bestiary & Monster-Wiki Dialog mit Suche und Sortierung |
| [`source/tfs_npc_editor.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/tfs_npc_editor.cpp) | TFS XML/Lua NPC-Editor |
| [`source/tfs_quest_generator.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/tfs_quest_generator.cpp) | TFS Quest Script & Chest Generator |
| [`source/prefab_manager.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/prefab_manager.cpp) | Prefab-Bibliothek zum Speichern und Einfügen vorgefertigter Bauten |

### Test-Checkliste Dialoge
- [ ] **Einstellungen (`P`)**: Änderungen von UI-Scale, Theme, Hotkeys und Dateipfaden werden korrekt gespeichert.
- [ ] **Properties (`Rechtsklick -> Properties`)**: Modifizieren von Action ID, Unique ID, Texten und Container-Inhalten funktioniert und wird in Undo/Redo erfasst.
- [ ] **Map Diagnostic**: Scan läuft durch, findet Fehler und bietet "Jump to" / "Fix all" an.
- [ ] **Map Diff**: Öffnet zwei Maps und stellt Unterschiede visuell dar.
- [ ] **Procedural Generator**: Generiert Höhlen/Dungeons mit konfigurierbaren Parametern auf dem Canvas.
- [ ] **Creature Wiki**: Öffnet sich schnell, Suchfunktion filtert Monster, Bestiary-Stats stimmen.

---

## 6. Backend, File I/O & Multiplayer (`Core Engine`)
Laden und Speichern von Dateien, Versionskompatibilität und Live-Multiplayer.

| Quellcode-Dateien | Zuständigkeit |
| :--- | :--- |
| [`source/iomap_otbm.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/iomap_otbm.cpp) | OTBM Binary Reader / Writer |
| [`source/graphics.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/graphics.cpp) | `Tibia.dat` / `Tibia.spr` Spritelader und Cache |
| [`source/action.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/action.cpp) | Undo- und Redo-Stack |
| [`source/live_server.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/live_server.cpp) | Host-Server für Live-Multiplayer-Mapping |
| [`source/live_client.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/live_client.cpp) | Client für Live-Multiplayer-Session |
| [`source/lua/lua_script_manager.cpp`](file:///c:/Users/weber/Dokumente/Projekt/In%20Arbeit/Map%20Editor/source/lua/lua_script_manager.cpp) | Lua-Skripting-Engine und API-Bindings |

### Test-Checkliste Backend & Stabilität
- [ ] **OTBM Speichern & Laden**: Gespeicherte Maps lassen sich ohne Datenverlust oder falsche Sprites wieder öffnen.
- [ ] **Undo / Redo (`Strg+Z / Strg+Y`)**: 20+ Aktionen rückgängig machen und wiederholen ohne Crash oder inkonsistente Tiles.
- [ ] **Live Multiplayer**: Host starten (`Live -> Start`), zweiter Client verbindet sich (`Live -> Join`), Änderungen werden in Echtzeit synchronisiert.
- [ ] **Crash-Resilienz / Logging**: Unerwartete Fehler schreiben einen Stacktrace in `error.log`, ohne den Benutzer ohne Meldung zu beenden.
