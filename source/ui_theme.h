#ifndef RME_UI_THEME_H
#define RME_UI_THEME_H

#include <nanovg.h>

namespace RME::UI::Theme {
    enum class Type { Dark, Light };

    // Aktuelle Farben (werden beim Umschalten aktualisiert)
    inline NVGcolor Background = nvgRGBA(13, 17, 23, 255);
    inline NVGcolor Panel = nvgRGBA(21, 27, 36, 255);
    inline NVGcolor Accent = nvgRGBA(229, 193, 88, 255);
    inline NVGcolor Text = nvgRGBA(240, 244, 248, 255);
    inline NVGcolor Button = nvgRGBA(28, 36, 48, 255);

    static constexpr float CornerRadius = 3.0f; // 3px Eckenradius
    static const char* MainFont = "sans";
    static constexpr float FontSize = 13.0f;

    inline void SetTheme(Type type) {
        if (type == Type::Dark) {
            Background = nvgRGBA(13, 17, 23, 255);   // Dark Obsidian Basalt #0D1117
            Panel      = nvgRGBA(21, 27, 36, 255);   // Deep Slate Panel #151B24
            Accent     = nvgRGBA(229, 193, 88, 255); // Mystic Gold #E5C158
            Text       = nvgRGBA(240, 244, 248, 255);
            Button     = nvgRGBA(28, 36, 48, 255);
        } else {
            Background = nvgRGBA(240, 240, 240, 255);
            Panel      = nvgRGBA(225, 225, 225, 255);
            Accent     = nvgRGBA(212, 175, 55, 255);
            Text       = nvgRGBA(30, 30, 30, 255);
            Button     = nvgRGBA(200, 200, 200, 255);
        }
    }
}

#endif // RME_UI_THEME_H
