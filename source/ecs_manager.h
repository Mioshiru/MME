#ifndef RME_ECS_MANAGER_H
#define RME_ECS_MANAGER_H

#include <entt/entt.hpp>
#include "position.h"
#include "sprites.h"
#include <random>

class BaseMap;

namespace RME::Core::ECS {

    // Komponenten-Definitionen
    struct PositionComponent {
        int x, y, z;
    };

    struct AppearanceComponent {
        uint16_t lookType;
        uint16_t lookHead, lookBody, lookLegs, lookFeet;
        uint16_t lookAddon;
        uint16_t lookMount;
    };

    struct AIComponent {
        uint32_t lastUpdateTick;
        float walkSpeed;
        bool isNpc;
    };

    struct MetadataComponent {
        std::string name;
        uint16_t spawnRadius;
    };

    class ECSManager {
    public:
        ECSManager() = default;
        
        entt::registry& getRegistry() { return m_registry; }

        void updateAISystem(BaseMap& map, uint32_t currentTick) {
            auto view = m_registry.view<PositionComponent, AIComponent>();
            
            // Zufallsgenerator für das Wandern
            static std::mt19937 gen(std::random_device{}());
            std::uniform_int_distribution<> dis(-1, 1);

            for (auto entity : view) {
                auto& pos = view.get<PositionComponent>(entity);
                auto& ai = view.get<AIComponent>(entity);

                // Prüfen, ob der NPC basierend auf seiner Geschwindigkeit bereit für den nächsten Schritt ist
                if (currentTick - ai.lastUpdateTick > static_cast<uint32_t>(1000.0f / ai.walkSpeed)) {
                    int nextX = pos.x + dis(gen);
                    int nextY = pos.y + dis(gen);

                    // Einfache Validierung: Tile muss existieren (Logik kann um Collision-Flags erweitert werden)
                    if (nextX >= 0 && nextY >= 0) {
                        // Hier könnte map.getTile(nextX, nextY, pos.z) geprüft werden
                        pos.x = nextX;
                        pos.y = nextY;
                    }

                    ai.lastUpdateTick = currentTick;
                }
            }
        }

    private:
        entt::registry m_registry;
    };
}

#endif
