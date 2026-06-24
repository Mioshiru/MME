#ifndef RME_UI_TOOLBAR_H
#define RME_UI_TOOLBAR_H

#include "ui_button.h"
#include <vector>
#include <memory>

namespace RME::UI {

/**
 * UIToolbar: Ein Container für UIButton-Elemente.
 * Nutzt das Panel-Design aus dem UITheme.
 */
class UIToolbar : public UIElement {
public:
    UIToolbar(float x, float y, float w, float h)
        : UIElement(x, y, w, h) {}

    void addButton(const std::string& label, const char* svgIcon, std::function<void()> callback) {
        float btnHeight = 30.0f;
        float btnWidth = width - 10.0f;
        float btnX = x + 5.0f;
        float btnY = y + 5.0f + (buttons.size() * (btnHeight + 5.0f));

        auto btn = std::make_unique<UIButton>(label, btnX, btnY, btnWidth, btnHeight, svgIcon);
        btn->setCallback(callback);
        buttons.push_back(std::move(btn));
    }

    void render(NVGcontext* vg) override {
        if (!visible) return;

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
    std::vector<std::unique_ptr<UIButton>> buttons;
};

} // namespace RME::UI

#endif // RME_UI_TOOLBAR_H
