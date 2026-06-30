#include "main.h"
#include "lua/lua_api.h"
#include "creature.h"
#include "editor.h"
#include "gui.h"
#include "live_peer.h"

namespace LuaAPI {
    void registerCreatureAPI(sol::state& lua);

    void registerCreature(sol::state& lua) {
        registerCreatureAPI(lua);
    }

    void registerCreatureAPI(sol::state& lua) {
        // Creature-Klasse (NPCs/Monster auf der Map)
        lua.new_usertype<Creature>("Creature",
            "getName", &Creature::getName,
            "getDirection", &Creature::getDirection,
            "setDirection", &Creature::setDirection,
            "getSpawnTime", &Creature::getSpawnTime,
            "setDirection", &Creature::setDirection,
            "isNpc", &Creature::isNpc,
            "isSelected", &Creature::isSelected,
            "getLookType", [](Creature& c) { return c.getLookType().lookType; }
        );

        // Player-API (Für Live-Multiplayer Kontext)
        auto player_api = lua["player"].get_or_create<sol::table>();
        
        player_api["getLocalName"] = []() -> std::string {
            return "Mioshiro"; // Rückgabe des konfigurierten Nutzernamens
        };

        player_api["getLatency"] = []() -> uint32_t {
            Editor* ed = g_gui.GetCurrentEditor();
            if (ed && ed->IsLive() && !ed->IsLiveServer()) {
                return g_gui.latencies[ed->GetLiveClient()];
            }
            return 0;
        };

        // Outfit-Tabelle für Player-Visuals (Multiplayer Labels)
        lua.new_usertype<Outfit>("Outfit",
            "lookType", &Outfit::lookType,
            "lookHead", &Outfit::lookHead,
            "lookBody", &Outfit::lookBody,
            "lookLegs", &Outfit::lookLegs,
            "lookFeet", &Outfit::lookFeet,
            "lookAddon", &Outfit::lookAddon,
            "lookMount", &Outfit::lookMount
        );

        lua["Creature"]["create"] = [](const std::string& name) { return new Creature(name); };
    }
}