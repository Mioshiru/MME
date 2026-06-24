#include "main.h"
#include "gui.h"
#include "settings.h"
#include "editor_tabs.h"
#include "map_tab.h"
#include "map_display.h"
#include <wx/display.h>

void GUI::LoadPerspective() {
  if (!IsVersionLoaded()) {
    if (g_settings.getInteger(Config::WINDOW_MAXIMIZED)) {
      root->Maximize();
    } else {
      root->SetSize(wxSize(g_settings.getInteger(Config::WINDOW_WIDTH),
                           g_settings.getInteger(Config::WINDOW_HEIGHT)));
    }
  } else {
    std::string tmp;
    std::string layout = g_settings.getString(Config::PALETTE_LAYOUT);

    std::vector<std::string> palette_list;
    for (char c : layout) {
      if (c == '|') {
        palette_list.push_back(tmp);
        tmp.clear();
      } else {
        tmp.push_back(c);
      }
    }

    if (!tmp.empty()) {
      palette_list.push_back(tmp);
    }

    for (const std::string &name : palette_list) {
      PaletteWindow *palette = CreatePalette();

      wxAuiPaneInfo &info = aui_manager->GetPane(palette);
      aui_manager->LoadPaneInfo(wxstr(name), info);

      if (info.IsFloatable()) {
        bool offscreen = true;
        for (uint32_t index = 0; index < wxDisplay::GetCount(); ++index) {
          wxDisplay display(index);
          wxRect rect = display.GetClientArea();
          if (rect.Contains(info.floating_pos)) {
            offscreen = false;
            break;
          }
        }

        if (offscreen) {
          info.Dock();
        }
      }
    }

    if (aui_manager) {
      aui_manager->SetFlags(aui_manager->GetFlags() |
                            wxAUI_MGR_TRANSPARENT_HINT | wxAUI_MGR_LIVE_RESIZE |
                            wxAUI_MGR_HINT_FADE);
    }

    aui_manager->Update();
    root->UpdateMenubar();
  }

  root->GetAuiToolBar()->LoadPerspective();
}

void GUI::SavePerspective() {
  g_settings.setInteger(Config::WINDOW_MAXIMIZED, root->IsMaximized());
  g_settings.setInteger(Config::WINDOW_WIDTH, root->GetSize().GetWidth());
  g_settings.setInteger(Config::WINDOW_HEIGHT, root->GetSize().GetHeight());

  wxString pinfo;
  for (auto &palette : palettes) {
    if (aui_manager->GetPane(palette).IsShown()) {
      pinfo << aui_manager->SavePaneInfo(aui_manager->GetPane(palette)) << "|";
    }
  }
  g_settings.setString(Config::PALETTE_LAYOUT, nstr(pinfo));

  if (MapTab *tab = GetCurrentMapTab()) {
    if (MapCanvas *canvas = tab->GetCanvas()) {
      if (auto *toolbar = canvas->GetUIToolbar()) {
        g_settings.setInteger(Config::UI_TOOLBAR_X, (int)toolbar->getX());
        g_settings.setInteger(Config::UI_TOOLBAR_Y, (int)toolbar->getY());
      }
    }
  }
  root->GetAuiToolBar()->SavePerspective();
}