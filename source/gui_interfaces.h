#ifndef RME_GUI_INTERFACES_H
#define RME_GUI_INTERFACES_H

class PaletteWindow;
#include "tileset.h"
typedef TilesetCategoryType PaletteType;

// Interface für Fenster-Management und Layouts
class IGuiLayout {
public:
    virtual ~IGuiLayout() = default;
    virtual void LoadPerspective() = 0;
    virtual void SavePerspective() = 0;
    virtual PaletteWindow* CreatePalette() = 0;
    virtual void SelectPalettePage(PaletteType pt) = 0;
};

// Interface für Asset-Management und Versionen
class IGuiAssets {
public:
    virtual ~IGuiAssets() = default;
    virtual bool LoadVersion(ClientVersionID ver, wxString& error, wxArrayString& warnings, bool force = false) = 0;
    virtual void UnloadVersion() = 0;
    virtual bool IsVersionLoaded() const = 0;
};

#endif




