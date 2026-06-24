#ifndef RME_OVERLAY_MANAGER_H_
#define RME_OVERLAY_MANAGER_H_

#include <nanovg.h>
#include <string>
#include <vector>
#include <memory>

namespace RME::UI {

struct TooltipData {
    float x, y;
    std::string text;
    uint8_t r, g, b;
};

struct ToolbarButton {
    int iconId;
    std::string tooltip;
    bool hovered;
    float x, y, size;
};

class OverlayManager {
public:
    OverlayManager(NVGcontext* vcg) : m_nvg(vcg) {}

    void AddTooltip(float x, float y, const std::string& text, uint8_t r, uint8_t g, uint8_t b) {
        m_tooltips.push_back({x, y, text, r, g, b});
    }

    void Clear() { m_tooltips.clear(); }

    void RenderTooltips(float zoom, float tileSize);
    void RenderHUD(int sw, int sh, int mx, int my, int mz, float fps);
    void RenderRPGToolbar(int sw, int sh);
    
    // SVG Support
    int LoadSVG(const std::string& path);
    void DrawIcon(int iconId, float x, float y, float size);
    void AddToolbarButton(int iconId, const std::string& tooltip);

private:
    NVGcontext* m_nvg;
    std::vector<TooltipData> m_tooltips;
    std::vector<ToolbarButton> m_toolbar_buttons;
};

} // namespace RME::UI

#endif
