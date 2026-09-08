#ifndef RME_STYLE_MANAGER_H
#define RME_STYLE_MANAGER_H

#include "nanovg.h"

#include <wx/dialog.h>
#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <wx/radiobut.h>
#include <wx/statbox.h>
#include <wx/aui/auibook.h>
#include <wx/notebook.h>
#include <wx/control.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/spinctrl.h>
#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/listbox.h>
#include <wx/listctrl.h>
#include <wx/treectrl.h>

namespace RME::UI {

    struct ThemeData {
        NVGcolor background;  // Dark Runic Basalt #0D1117
        NVGcolor panel;       // Deep Obsidian Panel #151B24
        NVGcolor accent;      // Mystic Gold Accent #E5C158
        NVGcolor text;        // Crisp Rune Text #F0F4F8
        NVGcolor border;      // Polished Gold Border #D4AF37
        float cornerRadius;   // 3px
        const char* fontFace; // sans
    };

    class StyleManager {
    public:
        static const ThemeData& GetTheme() {
            static ThemeData darkTheme = {
                nvgRGBA(13, 17, 23, 255),    // Deep Basalt / Dark Obsidian #0D1117
                nvgRGBA(21, 27, 36, 255),    // Deep Runic Slate Input/Panel #151B24
                nvgRGBA(229, 193, 88, 255),  // Mystic Gold #E5C158
                nvgRGBA(240, 244, 248, 255), // Crisp Text #F0F4F8
                nvgRGBA(212, 175, 55, 255),  // Polished Gold Border #D4AF37
                3.0f,
                "sans"
            };
            return darkTheme;
        }

        static bool IsPointInRect(float x, float y, float rx, float ry, float rw, float rh) {
            return x >= rx && x <= rx + rw && y >= ry && y <= ry + rh;
        }

        // Recursively styles windows and controls in cohesive Fantasy RPG Design
        static void ApplyThemeRecursively(wxWindow* win, const ThemeData& theme) {
            if (!win) return;

            if (win->GetClassInfo()) {
                wxString clsName = win->GetClassInfo()->GetClassName();
                if (clsName.Contains("Grid") || clsName.Contains("Picker") || clsName.Contains("ColourDialog")) {
                    return;
                }
            }

            wxColor bgDark(static_cast<unsigned char>(theme.background.r * 255),
                           static_cast<unsigned char>(theme.background.g * 255),
                           static_cast<unsigned char>(theme.background.b * 255));
            wxColor bgInput(static_cast<unsigned char>(theme.panel.r * 255),
                            static_cast<unsigned char>(theme.panel.g * 255),
                            static_cast<unsigned char>(theme.panel.b * 255));
            wxColor fgText(static_cast<unsigned char>(theme.text.r * 255),
                           static_cast<unsigned char>(theme.text.g * 255),
                           static_cast<unsigned char>(theme.text.b * 255));
            wxColor goldAccent(229, 193, 88);

            try {
                if (win->IsKindOf(wxCLASSINFO(wxDialog))) {
                    win->SetBackgroundColour(bgDark);
                    win->SetForegroundColour(fgText);
                }
                else if (win->IsKindOf(wxCLASSINFO(wxButton))) {
                    // If button hasn't been given a specific custom color (e.g. green/red)
                    wxColour curBg = win->GetBackgroundColour();
                    if (!curBg.IsOk() || curBg == wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE) || curBg == wxColour(240, 240, 240)) {
                        win->SetBackgroundColour(wxColour(28, 36, 48));
                        win->SetForegroundColour(goldAccent);
                    }
                }
                else if (win->IsKindOf(wxCLASSINFO(wxTextCtrl)) || win->IsKindOf(wxCLASSINFO(wxSpinCtrl))) {
                    win->SetBackgroundColour(bgInput);
                    win->SetForegroundColour(fgText);
                }
                else if (win->IsKindOf(wxCLASSINFO(wxListBox)) || win->IsKindOf(wxCLASSINFO(wxListCtrl)) || win->IsKindOf(wxCLASSINFO(wxTreeCtrl))) {
                    win->SetBackgroundColour(bgInput);
                    win->SetForegroundColour(fgText);
                }
                else if (win->IsKindOf(wxCLASSINFO(wxChoice)) || win->IsKindOf(wxCLASSINFO(wxComboBox))) {
                    win->SetBackgroundColour(bgInput);
                    win->SetForegroundColour(fgText);
                    auto* ctrl = wxDynamicCast(win, wxControl);
                    if (ctrl) {
                        wxSize best = ctrl->GetBestSize();
                        if (best.x > 0) {
                            ctrl->SetMinSize(wxSize(best.x + 8, -1));
                        }
                    }
                }
                else if (win->IsKindOf(wxCLASSINFO(wxPanel)) ||
                         win->IsKindOf(wxCLASSINFO(wxStaticText)) ||
                         win->IsKindOf(wxCLASSINFO(wxCheckBox)) ||
                         win->IsKindOf(wxCLASSINFO(wxRadioButton)) ||
                         win->IsKindOf(wxCLASSINFO(wxStaticBox))) {
                    win->SetBackgroundColour(bgDark);
                    win->SetForegroundColour(fgText);
                }
                else if (win->IsKindOf(wxCLASSINFO(wxNotebook))) {
                    win->SetBackgroundColour(bgDark);
                    win->SetForegroundColour(fgText);
                }
                else if (win->IsKindOf(wxCLASSINFO(wxAuiNotebook))) {
                    auto* auiNotebook = wxDynamicCast(win, wxAuiNotebook);
                    if (auiNotebook) {
                        auto* art = auiNotebook->GetArtProvider();
                        if (art) {
                            art->SetActiveColour(goldAccent);
                            art->SetColour(bgDark);
                        }
                    }
                    win->SetBackgroundColour(bgDark);
                    win->SetForegroundColour(fgText);
                }

                win->Refresh();
            } catch (...) {
                // Catch any control-specific styling failures safely
            }

            for (auto* child : win->GetChildren()) {
                if (child) {
                    ApplyThemeRecursively(child, theme);
                }
            }
        }
    };
}

#endif // RME_STYLE_MANAGER_H
