#include "main.h"
#include "lua/lua_api.h"
#include "item.h"

namespace LuaAPI {
    void registerItemAPI(sol::state& lua) {
        lua.new_usertype<Item>("Item",
            "getId", &Item::getID,
            "getClientId", &Item::getClientID,
            "getName", &Item::getName,
            "getFullName", &Item::getFullName,
            "getWeight", &Item::getWeight,
            "getCount", &Item::getCount,
            "setCount", &Item::setSubtype,
            
            // Attribute
            "getUniqueId", &Item::getUniqueID,
            "setUniqueId", &Item::setUniqueID,
            "getActionId", &Item::getActionID,
            "setActionId", &Item::setActionID,
            "getText", &Item::getText,
            "setText", &Item::setText,
            "getDescription", &Item::getDescription,
            "setDescription", &Item::setDescription,
            
            // Flags
            "isStackable", &Item::isStackable,
            "isMoveable", &Item::isMoveable,
            "isGround", &Item::isGroundTile,
            "isWall", &Item::isWall,
            "isDoor", &Item::isDoor,
            
            // Vergleiche
            "equals", [](Item& a, Item* b) { return b && a.getID() == b->getID(); }
        );

        lua["Item"]["create"] = [](uint16_t id) { return Item::Create(id); };
    }
}