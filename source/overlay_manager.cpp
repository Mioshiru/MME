#include "main.h"
#include "overlay_manager.h"
#include <nanovg.h>
#include <format>
#include "style_manager.h"

namespace RME::UI {

void OverlayManager::RenderTooltips(float zoom, float tileSize) {
    nvgBeginFrame(m_nvg, 800, 600, 1.0f); // Dummy-Werte, werden von MapDrawer überschrieben
    for (const auto& tt : m_tooltips) {
        // Zeichne modernere Tooltips (wie zuvor in MapDrawer, aber hier entkoppelt)
        nvgBeginPath(m_nvg);
        nvgRoundedRect(m_nvg, tt.x, tt.y, 120, 20, 4.0f);
        nvgFillColor(m_nvg, nvgRGBA(tt.r, tt.g, tt.b, 200));
        nvgFill(m_nvg);
        
        nvgFillColor(m_nvg, nvgRGBA(255, 255, 255, 255));
        nvgText(m_nvg, tt.x + 5, tt.y + 15, tt.text.c_str(), nullptr);
    }
    nvgEndFrame(m_nvg);
    m_tooltips.clear();
}

void OverlayManager::RenderHUD(int sw, int sh, int mx, int my, int mz, float fps) {
    nvgBeginFrame(m_nvg, sw, sh, 1.0f);
    
    // Hintergrund-Bar unten
    nvgBeginPath(m_nvg);
    nvgRect(m_nvg, 0, sh - 30, sw, 30);
    nvgFillColor(m_nvg, nvgRGBA(10, 15, 25, 220));
    nvgFill(m_nvg);

    // FPS Anzeige (Rechts)
    std::string fpsText = std::format("FPS: {:.1f}", fps);
    nvgFontSize(m_nvg, 14.0f);
    nvgFillColor(m_nvg, nvgRGBA(218, 165, 32, 255)); // Gold
    nvgTextAlign(m_nvg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
    nvgText(m_nvg, (float)sw - 20, (float)sh - 15, fpsText.c_str(), nullptr);

    // Koordinaten (Links)
    std::string posText = std::format("X: {}  Y: {}  Z: {}", mx, my, mz);
    nvgTextAlign(m_nvg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgText(m_nvg, 20.0f, (float)sh - 15, posText.c_str(), nullptr);

    nvgEndFrame(m_nvg);
}

void OverlayManager::RenderRPGToolbar(int sw, int sh) {
    if (m_toolbar_buttons.empty()) return;

    float buttonSize = 40.0f;
    float padding = 10.0f;
    float totalWidth = m_toolbar_buttons.size() * (buttonSize + padding) + padding;
    float startX = (sw - totalWidth) / 2.0f;
    float startY = 20.0f;

    nvgBeginFrame(m_nvg, (float)sw, (float)sh, 1.0f);

    // Toolbar Background (Modern Floating Glass Look)
    nvgBeginPath(m_nvg);
    nvgRoundedRect(m_nvg, startX, startY, totalWidth, buttonSize + padding * 2, 8.0f);
    nvgFillColor(m_nvg, nvgRGBA(10, 15, 25, 200));
    nvgStrokeColor(m_nvg, nvgRGBA(180, 140, 50, 255));
    nvgStrokeWidth(m_nvg, 2.0f);
    nvgFill(m_nvg);
    nvgStroke(m_nvg);

    for (size_t i = 0; i < m_toolbar_buttons.size(); ++i) {
        float bx = startX + padding + i * (buttonSize + padding);
        float by = startY + padding;

        // Button Highlight bei Hover
        if (m_toolbar_buttons[i].hovered) {
            nvgBeginPath(m_nvg);
            nvgRoundedRect(m_nvg, bx - 2, by - 2, buttonSize + 4, buttonSize + 4, 4.0f);
            nvgFillColor(m_nvg, nvgRGBA(218, 165, 32, 50));
            nvgFill(m_nvg);
        }

        DrawIcon(m_toolbar_buttons[i].iconId, bx, by, buttonSize);
    }

    nvgEndFrame(m_nvg);
}

int OverlayManager::LoadSVG(const std::string& path) {
    // REDUX: Lädt SVG und skaliert es automatisch für die aktuelle Auflösung
    return nvgCreateImage(m_nvg, path.c_str(), 0);
}

void OverlayManager::DrawIcon(int iconId, float x, float y, float size) {
    NVGpaint imgPaint = nvgImagePattern(m_nvg, x, y, size, size, 0.0f, iconId, 1.0f);
    nvgBeginPath(m_nvg);
    nvgRect(m_nvg, x, y, size, size);
    nvgFillPaint(m_nvg, imgPaint);
    nvgFill(m_nvg);
}

} // namespace RME::UI
