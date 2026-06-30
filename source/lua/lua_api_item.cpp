#include "main.h"
#include "lua/lua_api.h"
#include "item.h"

namespace LuaAPI {
    void registerItemAPI(sol::state& lua);

    void registerItem(sol::state& lua) {
        registerItemAPI(lua);
    }

    void registerItemAPI(sol::state& lua) {
        lua.new_usertype<Item>("Item",
            sol::no_constructor,
            "getId", &Item::getID,
            "getClientId", &Item::getClientID,
            "getName", &Item::getName,
            "getFullName", &Item::getFullName,
            "getWeight", [](Item& item) { return item.getWeight(); },
            "getCount", &Item::getCount,
            "setCount", &Item::setSubtype,
            "getUniqueId", &Item::getUniqueID,
            "setUniqueId", &Item::setUniqueID,
            "getActionId", &Item::getActionID,
            "setActionId", &Item::setActionID,
            "getText", &Item::getText,
            "setText", &Item::setText,
            "getDescription", &Item::getDescription,
            "setDescription", &Item::setDescription,
            "isStackable", &Item::isStackable,
            "isMoveable", &Item::isMoveable,
            "isGround", &Item::isGroundTile,
            "isWall", &Item::isWall,
            "isDoor", &Item::isDoor
        );

        lua["Item"]["create"] = [](uint16_t id) { return Item::Create(id); };
    }
}