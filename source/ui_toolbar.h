#ifndef RME_UI_TOOLBAR_H
#define RME_UI_TOOLBAR_H

#include "ui_button.h"
#include <vector>
#include <memory>
#include <algorithm>

namespace RME::UI {

/**
 * UIToolbar: Ein Container für UIButton-Elemente.
 * Nutzt das Panel-Design aus dem UITheme.
 */
class UIToolbar : public UIElement {
public:
    UIToolbar(float x, float y, float w, float h, float scale = 1.0f)
        : UIElement(x, y, w, h), uiScale(std::max(1.0f, scale)) {}

    void setScale(float scale) {
        uiScale = std::max(1.0f, scale);
    }

    void addButton(const std::string& label, const char* svgIcon, std::function<void()> callback) {
        auto btn = std::make_unique<UIButton>(label, 0.0f, 0.0f, 0.0f, 0.0f, svgIcon);
        btn->setCallback(callback);
        buttons.push_back(std::move(btn));
        relayoutButtons();
    }

    void addButtonImage(const std::string& label, const std::string& imagePath, std::function<void()> callback) {
        auto btn = std::make_unique<UIButton>(label, 0.0f, 0.0f, 0.0f, 0.0f, imagePath);
        btn->setCallback(callback);
        buttons.push_back(std::move(btn));
        relayoutButtons();
    }

    void render(NVGcontext* vg) override {
        if (!visible) return;

        relayoutButtons();

        // macOS Floating Dock: Frosted Glass Panel mit weichem Schatten & Glanzkante
        const float radius = 10.0f * uiScale;

        // Subtiler dunkler Schatten
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y + 2.0f, width, height, radius);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 80));
        nvgFill(vg);

        // Frosted Glass Panel Hintergrund
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, width, height, radius);
        nvgFillColor(vg, Theme::Panel);
        nvgFill(vg);

        // Feine 1px Lichtreflexkante oben/rundherum
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, width, height, radius);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 38));
        nvgStrokeWidth(vg, 1.0f);
        nvgStroke(vg);

        // Buttons rendern
        for (auto& btn : buttons) {
            btn->render(vg);
        }
    }

    bool onMouseDown(float mx, float my, int button) override {
        if (!visible) return false;
        // Prüfe erst, ob ein Button getroffen wurde
        for (auto& btn : buttons) {
            if (btn->isPointInside(mx, my)) return false; // Buttons blockieren Drag
        }
        return isPointInside(mx, my) ? UIElement::onMouseDown(mx, my, button) : false;
    }

    bool onMouseClick(float mx, float my, int button) override {
        for (auto& btn : buttons) {
            if (btn->onMouseClick(mx, my, button)) return true;
        }
        bool wasDragging = dragging;
        dragging = false;
        return wasDragging || isPointInside(mx, my);
    }

    void onHover(float mx, float my) override {
        UIElement::onHover(mx, my);
        if (dragging) {
            onMouseDrag(mx, my);
        }
        for (auto& btn : buttons) {
            btn->onHover(mx, my);
        }
    }

private:
    void relayoutButtons() {
        const float margin = 5.0f * uiScale;
        const float spacing = 5.0f * uiScale;
        const float btnHeight = 30.0f * uiScale;
        const float btnWidth = std::max(24.0f * uiScale, width - (2.0f * margin));

        float contentBottom = y + margin;
        for (size_t i = 0; i < buttons.size(); ++i) {
            const float btnX = x + margin;
            const float btnY = y + margin + (float)i * (btnHeight + spacing);
            buttons[i]->setPosition(btnX, btnY);
            buttons[i]->setSize(btnWidth, btnHeight);
            contentBottom = btnY + btnHeight;
        }

        const float desiredHeight = (contentBottom - y) + margin;
        if (desiredHeight > height) {
            height = desiredHeight;
        }
    }

private:
    float uiScale;
    std::vector<std::unique_ptr<UIButton>> buttons;
};

} // namespace RME::UI

#endif // RME_UI_TOOLBAR_H
