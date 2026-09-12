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

    static constexpr float CornerRadius = 8.0f; // macOS Eckenradius
    static const char* MainFont = "sans";
    static constexpr float FontSize = 13.0f;

    inline void SetTheme(Type type) {
        if (type == Type::Dark) {
            Background = nvgRGBA(18, 22, 30, 255);   // macOS Dark Obsidian #12161E
            Panel      = nvgRGBA(24, 28, 38, 240);   // macOS Frosted Glass #181C26
            Accent     = nvgRGBA(75, 145, 240, 255); // macOS Apple Blue #4B91F0
            Text       = nvgRGBA(240, 244, 250, 255);
            Button     = nvgRGBA(36, 44, 58, 220);
        } else {
            Background = nvgRGBA(240, 240, 245, 255);
            Panel      = nvgRGBA(225, 228, 235, 240);
            Accent     = nvgRGBA(0, 122, 255, 255);
            Text       = nvgRGBA(30, 30, 30, 255);
            Button     = nvgRGBA(210, 215, 225, 220);
        }
    }
}

#endif // RME_UI_THEME_H
