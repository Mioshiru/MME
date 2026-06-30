#include "main.h"
#include "editor.h"
#include "gui.h"
#include "lua/lua_api.h"
#include "map.h"


namespace LuaAPI {
void registerMapAPI(sol::state &lua);

void registerMap(sol::state &lua) {
  registerMapAPI(lua);
}

void registerMapAPI(sol::state &lua) {
  auto map_api = lua["map"].get_or_create<sol::table>();

  // Map Grundfunktionen
  map_api["getSize"] = []() -> std::pair<int, int> {
    Map &m = g_gui.GetCurrentMap();
    return {m.getWidth(), m.getHeight()};
  };

  map_api["getTile"] = [](int x, int y, int z) -> Tile * {
    return g_gui.GetCurrentMap().getTile(x, y, z);
  };

  map_api["setDescription"] = [](const std::string &desc) {
    g_gui.GetCurrentMap().setMapDescription(desc);
    g_gui.GetCurrentMap().doChange();
  };

  map_api["getDescription"] = []() -> std::string {
    return g_gui.GetCurrentMap().getMapDescription();
  };

  map_api["getName"] = []() -> std::string {
    return g_gui.GetCurrentMap().getName();
  };

  map_api["clearDirtyFlags"] = []() {
    g_gui.GetCurrentMap().clearDirtyFlags();
  };

  // Snapshot Support für Prefabs/Undo
  map_api["createRegionSnapshot"] = [](int x, int y, int w,
                                       int h) -> std::vector<uint8_t> {
    return g_gui.GetCurrentMap().createRegionSnapshot(x, y, w, h);
  };

  // Metadaten-Hooks
  map_api["getSpawnCount"] = []() -> uint32_t {
    return g_gui.GetCurrentMap().getTileCount();
  };
}
} // namespace LuaAPI