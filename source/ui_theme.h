#ifndef RME_UI_THEME_H
#define RME_UI_THEME_H

#include <nanovg.h>

namespace RME::UI::Theme {
    enum class Type { Dark, Light };

    // Aktuelle Farben (werden beim Umschalten aktualisiert)
    inline NVGcolor Background;
    inline NVGcolor Panel;
    inline NVGcolor Accent;
    inline NVGcolor Text;
    inline NVGcolor Button;

    static constexpr float CornerRadius = 3.0f; // 2-3px Eckenradius
    static const char* MainFont = "sans";
    static constexpr float FontSize = 13.0f;

    inline void SetTheme(Type type) {
        if (type == Type::Dark) {
            Background = nvgRGBA(30, 30, 30, 255);  // #1e1e1e
            Panel      = nvgRGBA(45, 45, 48, 255);  // VS-Style Dark
            Accent     = nvgRGBA(0, 120, 215, 255); // Blau
            Text       = nvgRGBA(240, 240, 240, 255);
            Button     = nvgRGBA(60, 60, 60, 255);
        } else {
            Background = nvgRGBA(240, 240, 240, 255);
            Panel      = nvgRGBA(225, 225, 225, 255);
            Accent     = nvgRGBA(0, 90, 180, 255);
            Text       = nvgRGBA(30, 30, 30, 255);
            Button     = nvgRGBA(200, 200, 200, 255);
        }
    }
}

#endif // RME_UI_THEME_H
