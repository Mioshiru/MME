#ifndef RME_UI_BUTTON_H
#define RME_UI_BUTTON_H

#include "ui_element.h"
#include "ui_theme.h"
#include <nanosvg.h>
#include <string>

namespace RME::UI {

class UIButton : public UIElement {
public:
    UIButton(const std::string& label, float x, float y, float w, float h, const char* svgData = nullptr)
        : UIElement(x, y, w, h), label(label), iconData(svgData) {}

    void render(NVGcontext* vg) override {
        if (!visible) return;

        // Hintergrund zeichnen
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, width, height, Theme::CornerRadius);
        
        if (hovered) {
            nvgFillColor(vg, Theme::Accent); // Akzentfarbe bei Hover
        } else {
            nvgFillColor(vg, nvgRGBA(60, 60, 60, 255)); // Etwas heller als Panel
        }
        nvgFill(vg);

        // Rahmen
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, width, height, Theme::CornerRadius);
        nvgStrokeColor(vg, nvgRGBA(100, 100, 100, 255));
        nvgStrokeWidth(vg, 1.0f);
        nvgStroke(vg);

        // Icon zeichnen (falls vorhanden)
        if (iconData) {
            renderSVG(vg);
        } else {
            // Text zeichnen (nur wenn kein Icon da ist oder als Fallback)
            nvgFontSize(vg, Theme::FontSize);
            nvgFontFace(vg, Theme::MainFont);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, Theme::Text);
            nvgText(vg, x + width / 2.0f, y + height / 2.0f, label.c_str(), nullptr);
        }
    }

private:
    void renderSVG(NVGcontext* vg) {
        if (!iconData) return;
        std::string mutableData(iconData);
        NSVGimage* img = nsvgParse(mutableData.data(), "px", 96.0f);
        if (!img) return;

        float iconSize = std::min(width, height) * 0.6f;
        float ox = x + (width - iconSize) / 2.0f;
        float oy = y + (height - iconSize) / 2.0f;
        
        nvgSave(vg);
        nvgTranslate(vg, ox, oy);
        nvgScale(vg, iconSize / img->width, iconSize / img->height);

        for (NSVGshape* shape = img->shapes; shape != NULL; shape = shape->next) {
            for (NSVGpath* path = shape->paths; path != NULL; path = path->next) {
                nvgBeginPath(vg);
                nvgMoveTo(vg, path->pts[0], path->pts[1]);
                for (int i = 0; i < path->npts - 1; i += 3) {
                    float* p = &path->pts[i * 2];
                    nvgBezierTo(vg, p[2], p[3], p[4], p[5], p[6], p[7]);
                }
                if (path->closed) nvgClosePath(vg);
                
                if (shape->fill.type == NSVG_PAINT_COLOR) {
                    uint32_t c = shape->fill.color;
                    nvgFillColor(vg, nvgRGBA(c & 0xff, (c >> 8) & 0xff, (c >> 16) & 0xff, (c >> 24) & 0xff));
                    nvgFill(vg);
                }
            }
        }
        
        nvgRestore(vg);
        nsvgDelete(img);
    }

private:
    std::string label;
    const char* iconData;
};

} // namespace RME::UI

#endif // RME_UI_BUTTON_H
