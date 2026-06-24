#include "main.h"
#include "gui_preview.h"
#include "materials.h"
#include "palette_brushlist.h"
#include "gui.h"
#include "settings.h"
#include "application.h"
#include "palette_window.h"
#include "result_window.h"
#include "main_menubar.h"
#include "map_display.h"
#include <wx/display.h>

// Note: GUI::LoadPerspective, SavePerspective, CreatePalette, NewPalette,
// ActivatePalette, DestroyPalettes, RebuildPalettes, ShowPalette,
// SelectPalettePage, GetPalette, HideSearchWindow, ShowSearchWindow,
// and ShowToolbar are all defined in gui.cpp. This file only contains
// GetPalettes() which is unique to this translation unit.
const std::list<PaletteWindow*>& GUI::GetPalettes() { return palettes; }
