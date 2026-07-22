#ifndef RME_STYLE_MANAGER_H
#define RME_STYLE_MANAGER_H

#include "nanovg.h"

#include <wx/dialog.h>
#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/aui/auibook.h>
#include <wx/control.h>

namespace RME::UI {

    struct ThemeData {
        NVGcolor background;  // #1e1e1e
        NVGcolor panel;       // #333333
        NVGcolor accent;      // Blau
        NVGcolor text;        // Hellgrau/Weiß
        NVGcolor border;      // Panel-Abgrenzung
        float cornerRadius;    // 2-3px
        const char* fontFace; // Inter/Roboto
    };

    class StyleManager {
    public:
        static const ThemeData& GetTheme() {
            static ThemeData darkTheme = {
                nvgRGBA(10, 20, 35, 255),   // Loadingbar Dark Blue #0A1423
                nvgRGBA(16, 28, 48, 255),   // Dark Sapphire Panel #101C30
                nvgRGBA(180, 150, 50, 255), // Corporate Gold #B49632
                nvgRGBA(240, 245, 255, 255),// Crisp Text
                nvgRGBA(180, 150, 50, 255), // Gold borders
                2.5f,
                "sans"
            };
            return darkTheme;
        }

        // Hilfsmethode für Bounding-Box Checks (Observer Pattern Basis)
        static bool IsPointInRect(float x, float y, float rx, float ry, float rw, float rh) {
            return x >= rx && x <= rx + rw && y >= ry && y <= ry + rh;
        }

        // Recursively styles windows and child controls (white text on dark background)
        static void ApplyThemeRecursively(wxWindow* win, const ThemeData& theme) {
            if (!win) return;

            if (win->IsKindOf(wxCLASSINFO(wxDialog)) ||
                win->IsKindOf(wxCLASSINFO(wxPanel)) ||
                win->IsKindOf(wxCLASSINFO(wxStaticText)) ||
                win->IsKindOf(wxCLASSINFO(wxCheckBox)) ||
                win->IsKindOf(wxCLASSINFO(wxStaticBox)) ||
                win->IsKindOf(wxCLASSINFO(wxControl)) ||
                win->IsKindOf(wxCLASSINFO(wxAuiNotebook))) 
            {
                wxColor bg(static_cast<unsigned char>(theme.background.r * 255),
                           static_cast<unsigned char>(theme.background.g * 255),
                           static_cast<unsigned char>(theme.background.b * 255));
                wxColor fg(static_cast<unsigned char>(theme.text.r * 255),
                           static_cast<unsigned char>(theme.text.g * 255),
                           static_cast<unsigned char>(theme.text.b * 255));
                win->SetBackgroundColour(bg);
                win->SetForegroundColour(fg);
                
                if (win->IsKindOf(wxCLASSINFO(wxAuiNotebook))) {
                    auto* notebook = wxDynamicCast(win, wxAuiNotebook);
                    if (notebook) {
                        auto* art = notebook->GetArtProvider();
                        if (art) {
                            art->SetActiveColour(bg);
                            art->SetColour(bg);
                        }
                    }
                }
                win->Refresh();
            }
            
            for (auto* child : win->GetChildren()) {
                ApplyThemeRecursively(child, theme);
            }
        }
    };
}

#endif // RME_STYLE_MANAGER_H
