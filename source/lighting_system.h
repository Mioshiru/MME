#ifndef RME_LIGHTING_SYSTEM_H
#define RME_LIGHTING_SYSTEM_H

#include <vector>
#include <cstdint>

struct LightNode {
    float x, y;
    float radius;
    uint32_t color_rgba;
    float intensity;
};

class LightingSystem {
public:
    LightingSystem();
    ~LightingSystem();

    void addLight(const LightNode& light);
    void clearLights();

    // In a real implementation, this would use a Quadtree.
    // For now, we return all lights since frustum culling can be added later.
    std::vector<LightNode> getVisibleLights(float view_min_x, float view_min_y, float view_max_x, float view_max_y) const;

    float getAmbientIntensity() const { return ambient_intensity; }
    void setAmbientIntensity(float intensity) { ambient_intensity = intensity; }

private:
    std::vector<LightNode> m_lights;
    float ambient_intensity;
};

#endif // RME_LIGHTING_SYSTEM_H
