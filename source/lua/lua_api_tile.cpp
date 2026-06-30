#include "main.h"
#include "creature.h"
#include "item.h"
#include "lua/lua_api.h"
#include "spawn.h"
#include "tile.h"


namespace LuaAPI {
void registerTileAPI(sol::state &lua);

void registerTile(sol::state &lua) {
  registerTileAPI(lua);
}

void registerTileAPI(sol::state &lua) {
  lua.new_usertype<Tile>("Tile",
      sol::no_constructor,
      "getPosition", [](Tile& tile) { return tile.getPosition(); },
      "getX", &Tile::getX,
      "getY", &Tile::getY,
      "getZ", &Tile::getZ,
      "isHouseTile", &Tile::isHouseTile,
      "isEmpty", &Tile::empty,
      "isSelected", &Tile::isSelected,
      "getFlags", &Tile::getMapFlags,
      "setMapFlags", &Tile::setMapFlags,
      "setHouseId", &Tile::setHouseID,
      "addItem", &Tile::addItem);
}
} // namespace LuaAPI