#include "main.h"
#include "gui.h"
#include "settings.h"
#include "editor_tabs.h"
#include "map_tab.h"
#include "map_display.h"
#include <wx/display.h>

void GUI::LoadPerspective() {
  ScopedAction action("GUI::LoadPerspective");
  if (!IsVersionLoaded()) {
    if (g_settings.getInteger(Config::WINDOW_MAXIMIZED)) {
      root->Maximize();
    } else {
      root->SetSize(wxSize(g_settings.getInteger(Config::WINDOW_WIDTH),
                           g_settings.getInteger(Config::WINDOW_HEIGHT)));
    }
  } else {
    // Destroy any existing palettes before recreating from saved layout
    // to prevent the snowball duplication bug where each restart would
    // accumulate more and more palette copies.
    // Always clean up palettes and enforce exactly ONE clean main palette docked on the right
    DestroyPalettes();

    PaletteWindow *palette = CreatePalette();
    if (palette) {
      wxAuiPaneInfo &info = aui_manager->GetPane(palette);
      info.Right().Layer(1).Position(1).Dockable(true).LeftDockable(true).RightDockable(true).TopDockable(false).BottomDockable(false).CloseButton(true).Floatable(true).BestSize(270, 560).MinSize(wxSize(palette->FromDIP(160), 100)).Show(true);
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