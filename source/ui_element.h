#ifndef RME_UI_ELEMENT_H
#define RME_UI_ELEMENT_H

#include <nanovg.h>
#include <functional>

namespace RME::UI {

/**
 * UIElement: Universelle Basisklasse für das neue NanoVG-System.
 * Integriert Bounding-Box-Kollision und ein leichtgewichtiges Callback-System.
 */
class UIElement {
public:
    UIElement(float x, float y, float w, float h)
        : x(x), y(y), width(w), height(h), visible(true), hovered(false), dragging(false), 
          dragOffsetX(0), dragOffsetY(0) {}
    virtual ~UIElement() = default;

    // Jedes UI-Element muss seine eigene Zeichenlogik implementieren
    virtual void render(NVGcontext* vg) = 0;

    // Event-Schnittstellen für Interaktionen
    virtual bool onMouseDown(float mx, float my, int button) {
        if (visible && isPointInside(mx, my)) {
            dragging = true;
            dragOffsetX = mx - x;
            dragOffsetY = my - y;
            return true;
        }
        return false;
    }

    virtual bool onMouseClick(float mx, float my, int button) {
        if (visible && isPointInside(mx, my)) {
            dragging = false;
            if (onClick) onClick();
            return true; // Event konsumiert (Bounding Box Hit)
        }
        return false;
    }

    virtual void onHover(float mx, float my) {
        hovered = isPointInside(mx, my);
    }

    virtual void onMouseDrag(float mx, float my) {
        if (dragging) {
            x = mx - dragOffsetX;
            y = my - dragOffsetY;
        }
    }

    // Hilfsfunktionen für Bounding-Box-Logik
    bool isPointInside(float mx, float my) const {
        return mx >= x && mx <= x + width && my >= y && my <= y + height;
    }

    void setCallback(std::function<void()> cb) { onClick = cb; }
    void setVisible(bool v) { visible = v; }
    bool isVisible() const { return visible; }

    float getX() const { return x; }
    float getY() const { return y; }

protected:
    float x, y, width, height;
    bool visible;
    bool hovered;
    bool dragging;
    float dragOffsetX, dragOffsetY;
    std::function<void()> onClick; // Callback-System (Observer Pattern)
};

} // namespace RME::UI

#endif // RME_UI_ELEMENT_H
