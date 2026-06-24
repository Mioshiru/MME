#include "lighting_system.h"

LightingSystem::LightingSystem() : ambient_intensity(0.2f) {
}

LightingSystem::~LightingSystem() {
}

void LightingSystem::addLight(const LightNode& light) {
    m_lights.push_back(light);
}

void LightingSystem::clearLights() {
    m_lights.clear();
}

std::vector<LightNode> LightingSystem::getVisibleLights(float view_min_x, float view_min_y, float view_max_x, float view_max_y) const {
    std::vector<LightNode> visible;
    for (const auto& light : m_lights) {
        // Simple AABB intersection with light circle
        if (light.x + light.radius >= view_min_x && light.x - light.radius <= view_max_x &&
            light.y + light.radius >= view_min_y && light.y - light.radius <= view_max_y) {
            visible.push_back(light);
        }
    }
    return visible;
}
