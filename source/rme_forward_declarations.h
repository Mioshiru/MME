#ifndef RME_FORWARD_DECLARATIONS_H
#define RME_FORWARD_DECLARATIONS_H

// Kern-Logik
class MapEditor;
typedef MapEditor Editor;
class CopyBuffer;
class LiveClient;
class LiveServer;
class LiveSocket;
class ActionQueue;
class Selection;
class DirtyList;
class Map;
class BaseMap;
class Tile;
class Item;
class Creature;
class Spawn;
class Brush;
class Waypoint;
class House;

// Anzeige & Viewport
class MapCanvas;
class MapWindow;
class MapDrawer;

// Neue NanoVG UI-Struktur
namespace RME::UI {
    class UIElement;
    class Toolbar;
    class Sidebar;
    class StyleManager;
    
    namespace SVG {
        struct Icon;
    }
}

#endif // RME_FORWARD_DECLARATIONS_H
