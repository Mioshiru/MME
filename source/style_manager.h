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
        NVGcolor background;  // Dark Obsidian Navy #122036
        NVGcolor panel;       // Dark Sapphire Input #0C1626
        NVGcolor accent;      // Corporate Gold #B49632
        NVGcolor text;        // Crisp White #F0F5FF
        NVGcolor border;      // Gold borders
        float cornerRadius;   // 2-3px
        const char* fontFace; // Inter/Roboto
    };

    class StyleManager {
    public:
        static const ThemeData& GetTheme() {
            static ThemeData darkTheme = {
                nvgRGBA(18, 32, 54, 255),   // Dark Obsidian Navy #122036
                nvgRGBA(12, 22, 38, 255),   // Dark Sapphire Input #0C1626
                nvgRGBA(180, 150, 50, 255), // Corporate Gold #B49632
                nvgRGBA(240, 245, 255, 255),// Crisp Text
                nvgRGBA(180, 150, 50, 255), // Gold borders
                2.5f,
                "sans"
            };
            return darkTheme;
        }

        static bool IsPointInRect(float x, float y, float rx, float ry, float rw, float rh) {
            return x >= rx && x <= rx + rw && y >= ry && y <= ry + rh;
        }

        // Recursively styles windows and controls in cohesive Corporate Design
        static void ApplyThemeRecursively(wxWindow* win, const ThemeData& theme) {
            if (!win) return;

            if (win->GetClassInfo() && wxString(win->GetClassInfo()->GetClassName()).Contains("Grid")) {
                return;
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
            wxColor goldAccent(240, 210, 120);

            if (win->IsKindOf(wxCLASSINFO(wxDialog))) {
                win->SetBackgroundColour(bgDark);
                win->SetForegroundColour(fgText);
            }
            else if (win->IsKindOf(wxCLASSINFO(wxButton))) {
                // If button hasn't been given a specific custom color (e.g. green/red)
                wxColour curBg = win->GetBackgroundColour();
                if (!curBg.IsOk() || curBg == wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE) || curBg == wxColour(240, 240, 240)) {
                    win->SetBackgroundColour(wxColour(28, 46, 74));
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

            for (auto* child : win->GetChildren()) {
                ApplyThemeRecursively(child, theme);
            }
        }
    };
}

#endif // RME_STYLE_MANAGER_H
