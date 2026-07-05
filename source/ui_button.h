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
        : UIElement(x, y, w, h), label(label), svgData(svgData ? svgData : ""), iconImageHandle(-1) {}

    UIButton(const std::string& label, float x, float y, float w, float h, const std::string& imagePath)
        : UIElement(x, y, w, h), label(label), iconImagePath(imagePath), iconImageHandle(-1) {}

    void render(NVGcontext* vg) override {
        if (!visible) return;

        // Einheitlicher Button-Hintergrund fuer alle Icon-Typen
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, width, height, Theme::CornerRadius);
        nvgFillColor(vg, Theme::Button);
        nvgFill(vg);

        // Rahmen inkl. einheitlichem Hover-Ring
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, width, height, Theme::CornerRadius);
        nvgStrokeColor(vg, hovered ? Theme::Accent : nvgRGBA(100, 100, 100, 255));
        nvgStrokeWidth(vg, hovered ? 2.0f : 1.0f);
        nvgStroke(vg);

        if (hovered) {
            nvgBeginPath(vg);
            nvgRoundedRect(vg, x, y, width, height, Theme::CornerRadius);
            nvgFillColor(vg, nvgRGBA(255, 255, 255, 18));
            nvgFill(vg);
        }

        // Icon zeichnen (falls vorhanden)
        if (!iconImagePath.empty()) {
            if (!renderImage(vg)) {
                renderFallbackText(vg);
            }
        } else if (!svgData.empty()) {
            renderSVG(vg);
        } else {
            renderFallbackText(vg);
        }
    }

private:
    void renderFallbackText(NVGcontext* vg) {
        nvgFontSize(vg, Theme::FontSize);
        nvgFontFace(vg, Theme::MainFont);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, Theme::Text);
        nvgText(vg, x + width / 2.0f, y + height / 2.0f, label.c_str(), nullptr);
    }

    bool renderImage(NVGcontext* vg) {
        if (iconImagePath.empty()) {
            return false;
        }

        if (iconImageHandle < 0) {
            iconImageHandle = nvgCreateImage(vg, iconImagePath.c_str(), 0);
        }

        if (iconImageHandle < 0) {
            return false;
        }

        float iconSize = std::min(width, height) * 0.60f;
        float ox = x + (width - iconSize) / 2.0f;
        float oy = y + (height - iconSize) / 2.0f;

        NVGpaint paint = nvgImagePattern(vg, ox, oy, iconSize, iconSize, 0.0f, iconImageHandle, 1.0f);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, ox, oy, iconSize, iconSize, 2.0f);
        nvgFillPaint(vg, paint);
        nvgFill(vg);

        if (hovered) {
            nvgBeginPath(vg);
            nvgRoundedRect(vg, ox, oy, iconSize, iconSize, 2.0f);
            nvgFillColor(vg, nvgRGBA(255, 255, 255, 24));
            nvgFill(vg);
        }
        return true;
    }

    void renderSVG(NVGcontext* vg) {
        if (svgData.empty()) return;
        std::string mutableData(svgData);
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
    std::string svgData;
    std::string iconImagePath;
    int iconImageHandle;
};

} // namespace RME::UI

#endif // RME_UI_BUTTON_H
