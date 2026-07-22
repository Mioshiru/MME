//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#ifndef RME_PALETTE_H_
#define RME_PALETTE_H_

#include "palette_common.h"

class BrushPalettePanel;
class CreaturePalettePanel;
class HousePalettePanel;
class WaypointPalettePanel;
class BrushIconBox;

class PrefabPalettePanel : public PalettePanel {
public:
	PrefabPalettePanel(wxWindow* parent);
	virtual ~PrefabPalettePanel();

	PaletteType GetType() const override { return TILESET_PREFAB; }
	wxString GetName() const override { return "Prefabs"; }

	void LoadCurrentContents() override;
	void InvalidateContents() override;
	Brush* GetSelectedBrush() const override;
	bool SelectBrush(const Brush* whatbrush) override;

	void OnSelect(wxCommandEvent& event);
	void OnContextMenu(wxContextMenuEvent& event);
	void OnExportPrefab(wxCommandEvent& event);

protected:
	wxListBox* listbox;
	std::vector<PrefabBrush*> prefabs;

	DECLARE_EVENT_TABLE()
};

class FavoritesBox : public wxPanel {
public:
	FavoritesBox(wxWindow* parent);
	virtual ~FavoritesBox();

	void RefreshFavorites();
	Brush* GetSelectedBrush() const { return selected_brush; }

protected:
	void OnPaint(wxPaintEvent& event);
	void OnClick(wxMouseEvent& event);
	void OnRightClick(wxMouseEvent& event);
	void OnMouseMove(wxMouseEvent& event);

private:
	Brush* selected_brush = nullptr;
	DECLARE_EVENT_TABLE()
};

class PaletteWindow : public wxPanel {
public:
	PaletteWindow(wxWindow* parent, const TilesetContainer& tilesets);
	~PaletteWindow();

	// Interface
	// Reloads layout g_settings from g_settings (and using map)
	void ReloadSettings(Map* from);
	// Flushes all pages and forces them to be reloaded from the palette data again
	void InvalidateContents();
	// (Re)Loads all currently displayed data, called from InvalidateContents implicitly
	void LoadCurrentContents();
	// Goes to the selected page and selects any brush there
	void SelectPage(PaletteType palette);
	// The currently selected brush in this palette
	Brush* GetSelectedBrush() const;
	// The currently selected brush size in this palette
	int GetSelectedBrushSize() const;
	// The currently selected page (terrain, doodad...)
	PaletteType GetSelectedPage() const;
	BrushPalettePanel* collection_palette;
	void InvalidatePrefabPalette();
	void RefreshFavoritesBox();
	wxTextCtrl* GetSearchBox() const { return search_box; }

	// Custom Event handlers (something has changed?)
	// Finds the brush pointed to by whatbrush and selects it as the current brush (also changes page)
	// Returns if the brush was found in this palette
	virtual bool OnSelectBrush(const Brush* whatbrush, PaletteType primary = TILESET_UNKNOWN);
	// Updates the palette window to use the current brush size
	virtual void OnUpdateBrushSize(BrushShape shape, int size);
	// Updates the content of the palette (eg. houses, creatures)
	virtual void OnUpdate(Map* map);

	// wxWidgets Event Handlers
	void OnSwitchingPage(wxChoicebookEvent& event);
	void OnPageChanged(wxChoicebookEvent& event);
	void OnSearchTextChanged(wxCommandEvent& event);
	// Forward key events to the parent window (The Map Window)
	void OnKey(wxKeyEvent& event);
	void OnClose(wxCloseEvent&);

protected:
	static PalettePanel* CreateTerrainPalette(wxWindow* parent, const TilesetContainer& tilesets);
	static PalettePanel* CreateDoodadPalette(wxWindow* parent, const TilesetContainer& tilesets);
	static PalettePanel* CreateItemPalette(wxWindow* parent, const TilesetContainer& tilesets);
	static PalettePanel* CreateCollectionPalette(wxWindow* parent, const TilesetContainer& tilesets);
	static PalettePanel* CreateCreaturePalette(wxWindow* parent, const TilesetContainer& tilesets);
	static PalettePanel* CreateHousePalette(wxWindow* parent, const TilesetContainer& tilesets);
	static PalettePanel* CreateWaypointPalette(wxWindow* parent, const TilesetContainer& tilesets);
	static PalettePanel* CreateRAWPalette(wxWindow* parent, const TilesetContainer& tilesets);
	static PalettePanel* CreatePrefabPalette(wxWindow* parent);
	static PalettePanel* CreateFavoritesPalette(wxWindow* parent, const TilesetContainer& tilesets);

	wxChoicebook* choicebook;
	wxChoice* palette_choice;
	wxTextCtrl* search_box;

	BrushPalettePanel* terrain_palette;
	BrushPalettePanel* doodad_palette;
	BrushPalettePanel* item_palette;
	CreaturePalettePanel* creature_palette;
	HousePalettePanel* house_palette;
	WaypointPalettePanel* waypoint_palette;
	BrushPalettePanel* raw_palette;
	PrefabPalettePanel* prefab_palette;
	BrushPalettePanel* favorites_palette;

public:
	wxPanel* minimap_panel = nullptr;
	void UpdateMinimapVisibility();

	DECLARE_EVENT_TABLE()
};

#endif
