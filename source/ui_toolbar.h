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

        // Hintergrund-Panel zeichnen (#333333)
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, width, height, Theme::CornerRadius); // Annahme: Theme::CornerRadius ist definiert
        nvgFillColor(vg, Theme::Panel);
        nvgFill(vg);

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
