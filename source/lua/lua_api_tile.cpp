#include "main.h"
#include "creature.h"
#include "item.h"
#include "lua/lua_api.h"
#include "spawn.h"
#include "tile.h"


namespace LuaAPI {
void registerTileAPI(sol::state &lua) {
  lua.new_usertype<Tile>(
      "Tile", "getPosition", &Tile::getPosition, "getGround",
      [](Tile &t) -> Item * { return t.ground; }, "getItems",
      [](Tile &t) { return sol::as_table(t.items); }, "getItemCount",
      [](Tile &t) { return t.size(); }, "getTopItem",
      [](Tile &t) { return t.getTopItem(); }, "getCreature",
      [](Tile &t) -> Creature * { return t.creature; }, "getSpawn",
      [](Tile &t) -> Spawn * { return t.spawn; }, "getHouseId",
      [](Tile &t) { return t.getHouseID(); }, "isHouseTile",
      [](Tile &t) { return t.isHouseTile(); }, "isPZ",
      [](Tile &t) { return t.isPZ(); }, "getFlags",
      [](Tile &t) { return t.getMapFlags(); }, "isEmpty",
      [](Tile &t) { return t.empty(); }, "isSelected",
      [](Tile &t) { return t.isSelected(); },

      // Modifikatoren
      "setMapFlags", [](Tile &t, uint32_t flags) { t.setMapFlags(flags); },
      "setHouseId", [](Tile &t, uint32_t id) { t.setHouseID(id); },
      "addCreature",
      [](Tile &t, Creature *c) {
        if (t.creature)
          delete t.creature;
        t.creature = c;
        t.modify();
      },
      "addItem",
      [](Tile &t, Item *i) {
        t.addItem(i);
        t.modify();
      },
      "removeSpawn",
      [](Tile &t) {
        if (t.spawn) {
          delete t.spawn;
          t.spawn = nullptr;
          t.modify();
        }
      },
      "clear",
      [](Tile &t) {
        for (auto *it : t.items)
          delete it;
        t.items.clear();
        if (t.ground) {
          delete t.ground;
          t.ground = nullptr;
        }
        t.modify();
      });
}
} // namespace LuaAPI