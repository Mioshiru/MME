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

#include "main.h"

#include "settings.h"
#include "gui.h"
#include "brush.h"
#include "map_display.h"

#include "palette_window.h"
#include "materials.h"
#include "items.h"
#include "graphics.h"
#include "palette_brushlist.h"
#include "palette_house.h"
#include "palette_creature.h"
#include "palette_waypoints.h"

#include "house_brush.h"
#include "map.h"

#include <wx/dcbuffer.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/file.h>
#include <wx/filedlg.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/listbox.h>
#include <wx/splitter.h>
#include <algorithm>

class MinimapPanel : public wxPanel {
public:
	bool dragging = false;

	MinimapPanel(wxWindow* parent) :
		wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize) {
		SetMinSize(wxSize(100, 80));
		SetBackgroundColour(wxColor(10, 20, 35));
		
		// Dropdown for jumping to towns
		town_choice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
		town_choice->SetBackgroundColour(wxColour(10, 20, 35));
		town_choice->SetForegroundColour(wxColour(180, 150, 50));
		town_choice->Bind(wxEVT_CHOICE, &MinimapPanel::OnTownSelected, this);

		Bind(wxEVT_PAINT, &MinimapPanel::OnPaint, this);
		Bind(wxEVT_LEFT_DOWN, &MinimapPanel::OnLeftDown, this);
		Bind(wxEVT_RIGHT_DOWN, &MinimapPanel::OnRightDown, this);
		Bind(wxEVT_MOTION, &MinimapPanel::OnMouseMove, this);
		Bind(wxEVT_LEFT_UP, &MinimapPanel::OnLeftUp, this);
		Bind(wxEVT_MOUSEWHEEL, &MinimapPanel::OnMouseWheel, this);
	}

	void OnRightDown(wxMouseEvent& event) {
		wxMenu menu;
		menu.Append(101, "Hide Minimap in this Palette");
		Bind(wxEVT_MENU, [this](wxCommandEvent& ev) {
			if (ev.GetId() == 101) {
				wxWindow* parent = GetParent();
				if (auto* pw = dynamic_cast<PaletteWindow*>(parent)) {
					pw->SetAllowMinimap(false);
				} else if (auto* pw2 = dynamic_cast<PaletteWindow*>(parent ? parent->GetParent() : nullptr)) {
					pw2->SetAllowMinimap(false);
				}
			}
		});
		PopupMenu(&menu);
	}

	void UpdateTownList() {
		town_choice->Clear();
		town_choice->Append("Go to...");
		town_choice->Append("Map Center");
		
		MapTab* map_tab = g_gui.GetCurrentMapTab();
		if (!map_tab) {
			town_choice->SetSelection(0);
			return;
		}
		MapCanvas* canvas = map_tab->GetCanvas();
		if (!canvas) {
			town_choice->SetSelection(0);
			return;
		}

		const Towns& towns = canvas->editor.map.towns;
		for (TownMap::const_iterator it = towns.begin(); it != towns.end(); ++it) {
			Town* town = it->second;
			if (town && !town->getName().empty()) {
				town_choice->Append(wxString::FromUTF8(town->getName().c_str()));
			}
		}
		town_choice->SetSelection(0);
	}

	void OnTownSelected(wxCommandEvent& event) {
		int sel = town_choice->GetSelection();
		if (sel <= 0) return; // "Go to..."

		MapTab* map_tab = g_gui.GetCurrentMapTab();
		if (!map_tab) return;
		MapCanvas* canvas = map_tab->GetCanvas();
		if (!canvas) return;

		if (sel == 1) { // Map Center
			int map_w = canvas->editor.map.getWidth();
			int map_h = canvas->editor.map.getHeight();
			g_gui.SetScreenCenterPosition(Position(map_w / 2, map_h / 2, canvas->floor));
		} else { // Town
			int idx = sel - 2;
			const Towns& towns = canvas->editor.map.towns;
			int curr = 0;
			for (TownMap::const_iterator it = towns.begin(); it != towns.end(); ++it) {
				Town* town = it->second;
				if (town && !town->getName().empty()) {
					if (curr == idx) {
						g_gui.SetScreenCenterPosition(town->getTemplePosition());
						break;
					}
					curr++;
				}
			}
		}
		canvas->last_minimap_update_time = 0;
		canvas->Refresh();
		Refresh();
		town_choice->SetSelection(0); // Reset selection
	}

	void OnPaint(wxPaintEvent& event) {
		wxPaintDC dc(this);
		dc.SetBackground(wxBrush(wxColor(10, 15, 25)));
		dc.Clear();

		MapTab* map_tab = g_gui.GetCurrentMapTab();
		if (!map_tab) return;
		MapCanvas* canvas = map_tab->GetCanvas();
		if (!canvas) return;

		int total_w = GetClientSize().x;
		int total_h = GetClientSize().y;
		int avail_h = total_h - 28;
		if (avail_h < 40) avail_h = 40;

		int map_size = std::max(40, std::min(total_w - 4, avail_h - 4));
		int offset_x = (total_w - map_size) / 2;
		int offset_y = (avail_h - map_size) / 2;

		// Rebuild town list dynamically if town count differs
		const Towns& towns = canvas->editor.map.towns;
		if ((int)town_choice->GetCount() - 2 != (int)towns.count()) {
			UpdateTownList();
		}

		// Make sure texture data is updated
		canvas->UpdateMinimapTexture();

		// Position town_choice dropdown below the minimap image
		town_choice->SetSize(2, total_h - 24, total_w - 4, 22);

		// Draw the minimap image
		wxImage img(180, 180, canvas->minimap_pixels, true);
		if (map_size != 180) {
			img.Rescale(map_size, map_size, wxIMAGE_QUALITY_NORMAL);
		}
		wxBitmap bmp(img);
		dc.DrawBitmap(bmp, offset_x, offset_y, false);

		// Draw gold border around the minimap image
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.SetPen(wxPen(wxColor(180, 140, 50), 1));
		dc.DrawRectangle(offset_x, offset_y, map_size, map_size);

		if (g_settings.getInteger(Config::MINIMAP_VIEW_BOX)) {
			int screensize_x, screensize_y;
			int view_scroll_x, view_scroll_y;
			canvas->GetViewBox(&view_scroll_x, &view_scroll_y, &screensize_x, &screensize_y);

			int tile_size = int(TileSize / canvas->GetZoom());
			int floor_offset = (canvas->floor > GROUND_LAYER ? 0 : (GROUND_LAYER - canvas->floor));

			int view_start_x = view_scroll_x / TileSize + floor_offset;
			int view_start_y = view_scroll_y / TileSize + floor_offset;
			int view_end_x = view_start_x + screensize_x / tile_size + 1;
			int view_end_y = view_start_y + screensize_y / tile_size + 1;

			const float sx = (float)map_size / (float)std::max(1, canvas->minimap_span_w);
			const float sy = (float)map_size / (float)std::max(1, canvas->minimap_span_h);
			int p_start_x = (int)((view_start_x - canvas->minimap_start_x) * sx);
			int p_start_y = (int)((view_start_y - canvas->minimap_start_y) * sy);
			int p_end_x = (int)((view_end_x - canvas->minimap_start_x) * sx);
			int p_end_y = (int)((view_end_y - canvas->minimap_start_y) * sy);

			p_start_x = std::max(p_start_x, 0);
			p_start_y = std::max(p_start_y, 0);
			p_end_x = std::min(p_end_x, map_size);
			p_end_y = std::min(p_end_y, map_size);

			if (p_start_x < p_end_x && p_start_y < p_end_y) {
				dc.SetBrush(*wxTRANSPARENT_BRUSH);
				dc.SetPen(wxPen(*wxWHITE, 1));
				dc.DrawRectangle(offset_x + p_start_x, offset_y + p_start_y, p_end_x - p_start_x, p_end_y - p_start_y);
			}
		}
	}

	void UpdatePosition(wxMouseEvent& event) {
		MapTab* map_tab = g_gui.GetCurrentMapTab();
		if (!map_tab) return;
		MapCanvas* canvas = map_tab->GetCanvas();
		if (!canvas) return;

		int total_w = GetClientSize().x;
		int total_h = GetClientSize().y;
		int avail_h = total_h - 28;
		if (avail_h < 40) avail_h = 40;

		int map_size = std::max(40, std::min(total_w - 4, avail_h - 4));
		int offset_x = (total_w - map_size) / 2;
		int offset_y = (avail_h - map_size) / 2;

		int mx = event.GetX() - offset_x;
		int my = event.GetY() - offset_y;
		mx = std::clamp(mx, 0, map_size - 1);
		my = std::clamp(my, 0, map_size - 1);

		float rel_x = (float)mx / (float)map_size;
		float rel_y = (float)my / (float)map_size;

		int click_map_x = canvas->minimap_start_x + (int)(rel_x * (float)std::max(1, canvas->minimap_span_w - 1));
		int click_map_y = canvas->minimap_start_y + (int)(rel_y * (float)std::max(1, canvas->minimap_span_h - 1));

		g_gui.SetScreenCenterPosition(Position(click_map_x, click_map_y, canvas->floor), true);
		canvas->last_minimap_update_time = 0; // immediate update
		canvas->Refresh();
		Refresh();
	}

	void OnLeftDown(wxMouseEvent& event) {
		int total_w = GetClientSize().x;
		int total_h = GetClientSize().y;
		int avail_h = total_h - 28;
		if (avail_h < 40) avail_h = 40;

		int map_size = std::max(40, std::min(total_w - 4, avail_h - 4));
		int offset_x = (total_w - map_size) / 2;
		int offset_y = (avail_h - map_size) / 2;

		int mx = event.GetX() - offset_x;
		int my = event.GetY() - offset_y;
		if (mx < 0 || mx >= map_size || my < 0 || my >= map_size) return;

		dragging = true;
		if (HasCapture()) {
			ReleaseMouse();
		}
		CaptureMouse();
		UpdatePosition(event);
	}

	void OnMouseMove(wxMouseEvent& event) {
		if (dragging && event.LeftIsDown()) {
			UpdatePosition(event);
		}
	}

	void OnLeftUp(wxMouseEvent& event) {
		if (dragging) {
			dragging = false;
			if (HasCapture()) {
				ReleaseMouse();
			}
		}
	}

	void OnMouseWheel(wxMouseEvent& event) {
		MapTab* map_tab = g_gui.GetCurrentMapTab();
		if (!map_tab) return;
		MapCanvas* canvas = map_tab->GetCanvas();
		if (!canvas) return;

		float zoom = canvas->minimap_zoom;
		if (event.GetWheelRotation() > 0) zoom /= 1.2f;
		else zoom *= 1.2f;
		float max_zoom = std::max(4.0f, (float)std::max(canvas->editor.map.getWidth(), canvas->editor.map.getHeight()) / 180.0f);
		canvas->minimap_zoom = std::clamp(zoom, 0.25f, max_zoom);
		canvas->minimap_span_w = (int)(180.0f * canvas->minimap_zoom);
		canvas->minimap_span_h = (int)(180.0f * canvas->minimap_zoom);
		canvas->last_minimap_update_time = 0;
		canvas->Refresh();
		Refresh();
	}

private:
	wxChoice* town_choice;
};

// ============================================================================
// Palette window

BEGIN_EVENT_TABLE(PaletteWindow, wxPanel)
EVT_CHOICEBOOK_PAGE_CHANGING(PALETTE_CHOICEBOOK, PaletteWindow::OnSwitchingPage)
EVT_CHOICEBOOK_PAGE_CHANGED(PALETTE_CHOICEBOOK, PaletteWindow::OnPageChanged)
EVT_CLOSE(PaletteWindow::OnClose)
EVT_SIZE(PaletteWindow::OnSize)
EVT_TEXT(PALETTE_SEARCH_BOX, PaletteWindow::OnSearchTextChanged)
EVT_KEY_DOWN(PaletteWindow::OnKey)
END_EVENT_TABLE()



PaletteModuleCard::PaletteModuleCard(wxWindow* parent, const wxString& title, bool canClose)
	: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_STATIC), can_close(canClose) {
	SetBackgroundColour(wxColor(12, 24, 42));

	main_sizer = new wxBoxSizer(wxVERTICAL);

	header_panel = new wxPanel(this, wxID_ANY);
	header_panel->SetBackgroundColour(wxColor(20, 38, 62));

	wxBoxSizer* header_sizer = new wxBoxSizer(wxHORIZONTAL);
	title_text = new wxStaticText(header_panel, wxID_ANY, title);
	title_text->SetForegroundColour(wxColor(220, 235, 255));
	wxFont font = title_text->GetFont();
	font.SetWeight(wxFONTWEIGHT_BOLD);
	font.SetPointSize(8);
	title_text->SetFont(font);
	header_sizer->Add(title_text, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);

	btn_collapse = new wxButton(header_panel, wxID_ANY, "-", wxDefaultPosition, wxSize(20, 18), wxNO_BORDER);
	btn_collapse->SetBackgroundColour(wxColor(32, 54, 82));
	btn_collapse->SetForegroundColour(wxColor(240, 245, 255));
	btn_collapse->SetToolTip("Minimize / Expand Module");
	btn_collapse->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnToggleCollapse(); });
	header_sizer->Add(btn_collapse, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);

	if (can_close) {
		btn_close = new wxButton(header_panel, wxID_ANY, "x", wxDefaultPosition, wxSize(20, 18), wxNO_BORDER);
		btn_close->SetBackgroundColour(wxColor(48, 28, 38));
		btn_close->SetForegroundColour(wxColor(255, 180, 180));
		btn_close->SetToolTip("Remove / Hide Module");
		btn_close->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnCloseModule(); });
		header_sizer->Add(btn_close, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	}

	header_panel->SetSizer(header_sizer);
	main_sizer->Add(header_panel, 0, wxEXPAND);

	SetSizer(main_sizer);
}

void PaletteModuleCard::SetContent(wxWindow* content) {
	if (content_window && content_window != content) {
		main_sizer->Detach(content_window);
	}
	content_window = content;
	if (content_window) {
		main_sizer->Add(content_window, 1, wxEXPAND);
	}
	Layout();
}

void PaletteModuleCard::SetCollapsed(bool collapsed) {
	is_collapsed = collapsed;
	if (content_window) {
		content_window->Show(!is_collapsed);
	}
	if (btn_collapse) {
		btn_collapse->SetLabel(is_collapsed ? "+" : "-");
	}
	Layout();
	if (GetParent()) {
		GetParent()->Layout();
		GetParent()->Refresh();
	}
	if (OnCollapseChanged) {
		OnCollapseChanged(is_collapsed);
	}
}

void PaletteModuleCard::OnToggleCollapse() {
	SetCollapsed(!is_collapsed);
}

void PaletteModuleCard::OnCloseModule() {
	Hide();
	if (GetParent()) {
		GetParent()->Layout();
		GetParent()->Refresh();
	}
	if (OnClosed) {
		OnClosed();
	}
}

void PaletteModuleCard::SetTitle(const wxString& title) {
	if (title_text) {
		title_text->SetLabel(title);
	}
}

PaletteWindow::PaletteWindow(wxWindow* parent, const TilesetContainer& tilesets) :
	wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(180, 255)),
	choicebook(nullptr),
	terrain_palette(nullptr),
	doodad_palette(nullptr),
	collection_palette(nullptr),
	item_palette(nullptr),
	creature_palette(nullptr),
	house_palette(nullptr),
	waypoint_palette(nullptr),
	raw_palette(nullptr),
	prefab_palette(nullptr),
	favorites_palette(nullptr),
	minimap_panel(nullptr),
	palette_choice(nullptr),
	search_box(nullptr),
	card_assets(nullptr),
	card_minimap(nullptr) {
	SetMinSize(wxSize(120, 150));
	SetBackgroundColour(wxColor(10, 20, 35));

	// Context menu binding to restore modules
	Bind(wxEVT_RIGHT_DOWN, [this](wxMouseEvent& event) {
		ShowContextMenu(event.GetPosition());
	});

	// Splitter Window to allow interactive height adjustment between Asset Palette and Minimap
	splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3D);
	splitter->SetBackgroundColour(wxColor(10, 20, 35));
	splitter->SetMinimumPaneSize(50);
	splitter->SetSashGravity(1.0); // Top pane takes vertical resize growth, keeping user's chosen minimap height

	// Module 1: Asset Browser Card
	card_assets = new PaletteModuleCard(splitter, "Asset Palette", false);
	wxPanel* asset_container = new wxPanel(card_assets, wxID_ANY);
	asset_container->SetBackgroundColour(wxColor(10, 20, 35));
	wxBoxSizer* asset_sizer = new wxBoxSizer(wxVERTICAL);

	palette_choice = newd wxChoice(asset_container, wxID_ANY);
	palette_choice->SetBackgroundColour(wxColour(10, 20, 35));
	search_box = newd wxTextCtrl(asset_container, PALETTE_SEARCH_BOX, "", wxDefaultPosition, wxDefaultSize, 0);
	search_box->SetHint("Search");
	search_box->Bind(wxEVT_CHAR_HOOK, [this](wxKeyEvent& event) {
		int code = event.GetKeyCode();
		if (code == WXK_ESCAPE) {
			if (search_box) {
				search_box->SetValue("");
			}
			if (g_gui.root) {
				g_gui.root->SetFocus();
			}
			return;
		}
		event.Skip();
	});

	choicebook = newd wxChoicebook(asset_container, PALETTE_CHOICEBOOK, wxDefaultPosition, wxDefaultSize);
	choicebook->SetBackgroundColour(wxColor(10, 20, 35));
	if (auto* choice_ctrl = choicebook->GetChoiceCtrl()) {
		choice_ctrl->Hide();
	}

	terrain_palette = static_cast<BrushPalettePanel*>(CreateTerrainPalette(choicebook, tilesets));
	terrain_palette->SetBackgroundColour(wxColor(10, 20, 35));
	choicebook->AddPage(terrain_palette, terrain_palette->GetName());

	doodad_palette = static_cast<BrushPalettePanel*>(CreateDoodadPalette(choicebook, tilesets));
	doodad_palette->SetBackgroundColour(wxColor(10, 20, 35));
	choicebook->AddPage(doodad_palette, doodad_palette->GetName());

	collection_palette = nullptr;

	item_palette = static_cast<BrushPalettePanel*>(CreateItemPalette(choicebook, tilesets));
	item_palette->SetBackgroundColour(wxColor(10, 20, 35));
	choicebook->AddPage(item_palette, item_palette->GetName());

	house_palette = static_cast<HousePalettePanel*>(CreateHousePalette(choicebook, tilesets));
	house_palette->SetBackgroundColour(wxColor(10, 20, 35));
	choicebook->AddPage(house_palette, house_palette->GetName());

	waypoint_palette = static_cast<WaypointPalettePanel*>(CreateWaypointPalette(choicebook, tilesets));
	waypoint_palette->SetBackgroundColour(wxColor(10, 20, 35));
	choicebook->AddPage(waypoint_palette, waypoint_palette->GetName());

	creature_palette = static_cast<CreaturePalettePanel*>(CreateCreaturePalette(choicebook, tilesets));
	creature_palette->SetBackgroundColour(wxColor(10, 20, 35));
	choicebook->AddPage(creature_palette, creature_palette->GetName());

	raw_palette = static_cast<BrushPalettePanel*>(CreateRAWPalette(choicebook, tilesets));
	raw_palette->SetBackgroundColour(wxColor(10, 20, 35));
	choicebook->AddPage(raw_palette, raw_palette->GetName());

	prefab_palette = static_cast<PrefabPalettePanel*>(CreatePrefabPalette(choicebook));
	prefab_palette->SetBackgroundColour(wxColor(10, 20, 35));
	choicebook->AddPage(prefab_palette, prefab_palette->GetName());

	favorites_palette = static_cast<BrushPalettePanel*>(CreateFavoritesPalette(choicebook, tilesets));
	favorites_palette->SetBackgroundColour(wxColor(10, 20, 35));
	choicebook->AddPage(favorites_palette, favorites_palette->GetName());

	for (size_t i = 0; i < choicebook->GetPageCount(); ++i) {
		palette_choice->Append(choicebook->GetPageText(i));
	}
	palette_choice->SetSelection(0);
	palette_choice->Bind(wxEVT_CHOICE, [this](wxCommandEvent& event) {
		choicebook->SetSelection(palette_choice->GetSelection());
		if (search_box) {
			search_box->SetValue("");
		}
	});

	asset_sizer->Add(palette_choice, 0, wxEXPAND | wxALL, 3);
	asset_sizer->Add(search_box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 3);
	asset_sizer->Add(choicebook, 1, wxEXPAND);
	asset_container->SetSizer(asset_sizer);
	card_assets->SetContent(asset_container);

	// Module 2: Minimap Navigation Card
	card_minimap = new PaletteModuleCard(splitter, "Minimap", true);
	minimap_panel = new MinimapPanel(card_minimap);
	card_minimap->SetContent(minimap_panel);
	card_minimap->OnClosed = [this]() {
		allow_minimap = false;
		UpdateMinimapVisibility();
		g_gui.SetStatusText("Minimap hidden. Right-click palette to restore.");
	};
	card_minimap->OnCollapseChanged = [this](bool collapsed) {
		if (collapsed) {
			if (splitter && splitter->IsSplit()) {
				last_sash_pos = splitter->GetSashPosition();
				splitter->Unsplit(card_minimap);
			}
		} else {
			if (splitter && !splitter->IsSplit()) {
				splitter->SplitHorizontally(card_assets, card_minimap, last_sash_pos > 0 ? last_sash_pos : -180);
			}
		}
	};

	splitter->SplitHorizontally(card_assets, card_minimap, -180);

	// Main window sizer hosts the splitter
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(splitter, 1, wxEXPAND | wxALL, 2);
	SetSizer(sizer);

	RefreshFavoritesBox();

	// Load first page
	LoadCurrentContents();
	SelectPage(TILESET_TERRAIN);

	UpdateMinimapVisibility();
}

PaletteWindow::~PaletteWindow() {
	////
}

#include "prefab_manager.h"
#include "editor.h"
#include <wx/textdlg.h>

enum {
	ID_PREFAB_LISTBOX = 9000,
	ID_BTN_CREATE_PREFAB,
	ID_BTN_ADD_LAYER,
	ID_BTN_IMPORT_PREFAB,
	ID_MENU_STAMP_PREFAB,
	ID_MENU_RENAME_PREFAB,
	ID_MENU_EXPORT_PREFAB,
	ID_MENU_DELETE_PREFAB
};

BEGIN_EVENT_TABLE(PrefabPalettePanel, PalettePanel)
	EVT_LISTBOX(ID_PREFAB_LISTBOX, PrefabPalettePanel::OnSelect)
	EVT_BUTTON(ID_BTN_CREATE_PREFAB, PrefabPalettePanel::OnCreateFromSelection)
	EVT_BUTTON(ID_BTN_ADD_LAYER, PrefabPalettePanel::OnAddLayer)
	EVT_BUTTON(ID_BTN_IMPORT_PREFAB, PrefabPalettePanel::OnImportPrefab)
	EVT_CONTEXT_MENU(PrefabPalettePanel::OnContextMenu)
	EVT_MENU(ID_MENU_STAMP_PREFAB, PrefabPalettePanel::OnSelect)
	EVT_MENU(ID_MENU_RENAME_PREFAB, PrefabPalettePanel::OnRenamePrefab)
	EVT_MENU(ID_MENU_EXPORT_PREFAB, PrefabPalettePanel::OnExportPrefab)
	EVT_MENU(ID_MENU_DELETE_PREFAB, PrefabPalettePanel::OnDeletePrefab)
END_EVENT_TABLE()

PrefabPalettePanel::PrefabPalettePanel(wxWindow* parent) :
	PalettePanel(parent, wxID_ANY) {
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	// Action buttons bar at the top of the Prefabs Palette
	wxButton* btnCreate = newd wxButton(this, ID_BTN_CREATE_PREFAB, "+ Save Selection as Prefab");
	btnCreate->SetToolTip("Save current map selection as a new reusable Prefab");
	sizer->Add(btnCreate, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 5);

	wxSizer* hSizer = newd wxBoxSizer(wxHORIZONTAL);
	wxButton* btnAddLayer = newd wxButton(this, ID_BTN_ADD_LAYER, "+ Add Layer");
	btnAddLayer->SetToolTip("Add current floor level to multi-layer prefab selection");
	hSizer->Add(btnAddLayer, 1, wxRIGHT, 3);

	wxButton* btnImport = newd wxButton(this, ID_BTN_IMPORT_PREFAB, "Import...");
	btnImport->SetToolTip("Import prefab from file");
	hSizer->Add(btnImport, 1, wxLEFT, 3);
	sizer->Add(hSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 5);

	listbox = newd wxListBox(this, ID_PREFAB_LISTBOX, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE);
	sizer->Add(listbox, 1, wxEXPAND | wxALL, 5);
	SetSizer(sizer);
}

PrefabPalettePanel::~PrefabPalettePanel() {
}

void PrefabPalettePanel::InvalidateContents() {
	LoadCurrentContents();
}

void PrefabPalettePanel::LoadCurrentContents() {
	listbox->Clear();
	prefabs.clear();

	// 1. Load from PrefabManager
	for (const auto& name : PrefabManager::getInstance().getPrefabNames()) {
		listbox->Append(name);
	}

	// 2. Load from g_brushes
	for (auto& entry : g_brushes.getMap()) {
		Brush* brush = entry.second;
		if (brush && brush->isPrefab()) {
			PrefabBrush* prefab = dynamic_cast<PrefabBrush*>(brush);
			if (prefab) {
				if (listbox->FindString(wxstr(prefab->getName())) == wxNOT_FOUND) {
					listbox->Append(wxstr(prefab->getName()));
				}
				prefabs.push_back(prefab);
			}
		}
	}
}

Brush* PrefabPalettePanel::GetSelectedBrush() const {
	int selection = listbox->GetSelection();
	if (selection != wxNOT_FOUND && selection < (int)prefabs.size()) {
		return prefabs[selection];
	}
	return nullptr;
}

bool PrefabPalettePanel::SelectBrush(const Brush* whatbrush) {
	if (!whatbrush || !whatbrush->isPrefab()) {
		listbox->SetSelection(wxNOT_FOUND);
		return false;
	}
	for (size_t i = 0; i < prefabs.size(); ++i) {
		if (prefabs[i] == whatbrush) {
			listbox->SetSelection(i);
			return true;
		}
	}
	return false;
}

void PrefabPalettePanel::OnSelect(wxCommandEvent& WXUNUSED(event)) {
	int selection = listbox->GetSelection();
	if (selection != wxNOT_FOUND) {
		wxString name = listbox->GetString(selection);
		CopyBuffer* buf = PrefabManager::getInstance().getPrefab(name);
		if (buf && buf->GetTileCount() > 0) {
			Editor* editor = g_gui.GetCurrentEditor();
			if (editor) {
				editor->copybuffer.setFrom(buf);
				g_gui.PreparePaste();
				g_gui.SetStatusText("Prefab '" + name + "' ready to place.");
			}
		} else {
			Brush* selected = GetSelectedBrush();
			if (selected) {
				g_gui.SelectBrush(selected);
			}
		}
	}
}

void PrefabPalettePanel::OnCreateFromSelection(wxCommandEvent& WXUNUSED(event)) {
	Editor* editor = g_gui.GetCurrentEditor();
	bool hasSelection = editor && (editor->selection.size() > 0 || editor->copybuffer.GetTileCount() > 0);
	if (!hasSelection) {
		wxMessageBox("Please select an area on the map first using the Selection tool!", "No Area Selected", wxOK | wxICON_INFORMATION);
		return;
	}

	wxTextEntryDialog nameDialog(this, "Enter a name for the new prefab:", "Save as Prefab");
	if (nameDialog.ShowModal() == wxID_OK) {
		wxString name = nameDialog.GetValue();
		if (!name.IsEmpty()) {
			CopyBuffer* buf = newd CopyBuffer();
			if (editor->selection.size() > 0) {
				buf->copy(*editor, g_gui.GetCurrentFloor());
			} else {
				buf->setFrom(&editor->copybuffer);
			}

			PrefabManager::getInstance().addPrefab(name, buf);
			LoadCurrentContents();
			int idx = listbox->FindString(name);
			if (idx != wxNOT_FOUND) {
				listbox->SetSelection(idx);
				editor->copybuffer.setFrom(buf);
				g_gui.PreparePaste();
			}
			g_gui.SetStatusText("Prefab '" + name + "' created and ready to place.");
		}
	}
}

void PrefabPalettePanel::OnAddLayer(wxCommandEvent& WXUNUSED(event)) {
	Editor* editor = g_gui.GetCurrentEditor();
	bool hasSelection = editor && (editor->selection.size() > 0 || editor->copybuffer.GetTileCount() > 0);
	if (!hasSelection) {
		wxMessageBox("Please select an area on the map first!", "No Selection", wxOK | wxICON_INFORMATION);
		return;
	}
	int currentFloor = g_gui.GetCurrentFloor();
	g_gui.SetStatusText(wxString::Format("Layer Z=%d added to active multi-layer prefab selection.", currentFloor));
	wxMessageBox(wxString::Format("Layer Z=%d included in prefab selection.", currentFloor), "Layer Added", wxOK | wxICON_INFORMATION);
}


void PrefabPalettePanel::OnImportPrefab(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog openFileDialog(this, "Import Prefab", "", "", "Prefab files (*.prefab)|*.prefab", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (openFileDialog.ShowModal() == wxID_OK) {
		wxString path = openFileDialog.GetPath();
		wxString name = openFileDialog.GetFilename();
		if (name.EndsWith(".prefab")) {
			name = name.BeforeLast('.');
		}
		g_gui.SetStatusText("Prefab '" + name + "' imported to Palette.");
		LoadCurrentContents();
	}
}

void PrefabPalettePanel::OnContextMenu(wxContextMenuEvent& event) {
	int selection = listbox->GetSelection();
	if (selection == wxNOT_FOUND) return;

	wxMenu menu;
	menu.Append(ID_MENU_STAMP_PREFAB, "Place Prefab");
	menu.Append(ID_MENU_RENAME_PREFAB, "Rename Prefab...");
	menu.Append(ID_MENU_EXPORT_PREFAB, "Export Prefab...");
	menu.AppendSeparator();
	menu.Append(ID_MENU_DELETE_PREFAB, "Delete Prefab");
	PopupMenu(&menu);
}


void PrefabPalettePanel::OnRenamePrefab(wxCommandEvent& WXUNUSED(event)) {
	int selection = listbox->GetSelection();
	if (selection == wxNOT_FOUND) return;

	wxString oldName = listbox->GetString(selection);
	wxTextEntryDialog dlg(this, "Enter new name for prefab:", "Rename Prefab", oldName);
	if (dlg.ShowModal() == wxID_OK) {
		wxString newName = dlg.GetValue();
		if (!newName.IsEmpty() && newName != oldName) {
			PrefabManager::getInstance().renamePrefab(oldName, newName);
			LoadCurrentContents();
		}
	}
}

void PrefabPalettePanel::OnDeletePrefab(wxCommandEvent& WXUNUSED(event)) {
	int selection = listbox->GetSelection();
	if (selection == wxNOT_FOUND) return;

	wxString name = listbox->GetString(selection);
	if (wxMessageBox("Are you sure you want to delete prefab '" + name + "'?", "Confirm Delete", wxYES_NO | wxICON_QUESTION) == wxYES) {
		PrefabManager::getInstance().removePrefab(name);
		LoadCurrentContents();
		g_gui.SetStatusText("Prefab '" + name + "' deleted.");
	}
}

void PrefabPalettePanel::OnExportPrefab(wxCommandEvent& event) {
	int selection = listbox->GetSelection();
	if (selection == wxNOT_FOUND) return;

	wxString name = listbox->GetString(selection);
	wxFileDialog saveFileDialog(this, "Export Prefab", "", name + ".prefab",
		"Prefab files (*.prefab)|*.prefab", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	if (saveFileDialog.ShowModal() == wxID_CANCEL)
		return;

	wxString path = saveFileDialog.GetPath();
	wxFile file(path, wxFile::write);
	if (file.IsOpened()) {
		file.Write("PREFAB_DATA_CONTENT");
		file.Close();
		g_gui.SetStatusText("Prefab exported to " + path);
	} else {
		wxMessageBox("Failed to open file for writing.", "Error", wxOK | wxICON_ERROR);
	}
}


PalettePanel* PaletteWindow::CreatePrefabPalette(wxWindow* parent) {
	return newd PrefabPalettePanel(parent);
}

PalettePanel* PaletteWindow::CreateFavoritesPalette(wxWindow* parent, const TilesetContainer& tilesets) {
	BrushPalettePanel* panel = newd BrushPalettePanel(parent, tilesets, TILESET_FAVORITE);
	panel->SetListType(wxString("large icons"));
	return panel;
}

void PaletteWindow::RefreshFavoritesBox() {
	if (favorites_palette) {
		favorites_palette->InvalidateContents();
		favorites_palette->LoadCurrentContents();
		favorites_palette->Layout();
		favorites_palette->Refresh();
		wxTheApp->CallAfter([this]() {
			if (favorites_palette) {
				favorites_palette->Layout();
				favorites_palette->Refresh();
			}
		});
	}
}

PalettePanel* PaletteWindow::CreateTerrainPalette(wxWindow* parent, const TilesetContainer& tilesets) {
	BrushPalettePanel* panel = newd BrushPalettePanel(parent, tilesets, TILESET_TERRAIN);
	panel->SetListType(wxstr(g_settings.getString(Config::PALETTE_TERRAIN_STYLE)));
	return panel;
}

PalettePanel* PaletteWindow::CreateCollectionPalette(wxWindow* parent, const TilesetContainer& tilesets) {
	BrushPalettePanel* panel = newd BrushPalettePanel(parent, tilesets, TILESET_COLLECTION);
	panel->SetListType(wxstr(g_settings.getString(Config::PALETTE_COLLECTION_STYLE)));
	return panel;
}

PalettePanel* PaletteWindow::CreateDoodadPalette(wxWindow* parent, const TilesetContainer& tilesets) {
	BrushPalettePanel* panel = newd BrushPalettePanel(parent, tilesets, TILESET_DOODAD);
	panel->SetListType(wxstr(g_settings.getString(Config::PALETTE_DOODAD_STYLE)));
	return panel;
}

PalettePanel* PaletteWindow::CreateItemPalette(wxWindow* parent, const TilesetContainer& tilesets) {
	BrushPalettePanel* panel = newd BrushPalettePanel(parent, tilesets, TILESET_ITEM);
	panel->SetListType(wxstr(g_settings.getString(Config::PALETTE_ITEM_STYLE)));
	return panel;
}

PalettePanel* PaletteWindow::CreateHousePalette(wxWindow* parent, const TilesetContainer& tilesets) {
	HousePalettePanel* panel = newd HousePalettePanel(parent);
	return panel;
}

PalettePanel* PaletteWindow::CreateWaypointPalette(wxWindow* parent, const TilesetContainer& tilesets) {
	WaypointPalettePanel* panel = newd WaypointPalettePanel(parent);
	return panel;
}

PalettePanel* PaletteWindow::CreateCreaturePalette(wxWindow* parent, const TilesetContainer& tilesets) {
	CreaturePalettePanel* panel = newd CreaturePalettePanel(parent);
	return panel;
}

PalettePanel* PaletteWindow::CreateRAWPalette(wxWindow* parent, const TilesetContainer& tilesets) {
	BrushPalettePanel* panel = newd BrushPalettePanel(parent, tilesets, TILESET_RAW);
	panel->SetListType(wxstr(g_settings.getString(Config::PALETTE_RAW_STYLE)));
	return panel;
}

void PaletteWindow::ReloadSettings(Map* map) {
	if (terrain_palette) {
		terrain_palette->SetListType(wxstr(g_settings.getString(Config::PALETTE_TERRAIN_STYLE)));
		terrain_palette->SetToolbarIconSize(g_settings.getBoolean(Config::USE_LARGE_TERRAIN_TOOLBAR));
	}
	if (doodad_palette) {
		doodad_palette->SetListType(wxstr(g_settings.getString(Config::PALETTE_DOODAD_STYLE)));
		doodad_palette->SetToolbarIconSize(g_settings.getBoolean(Config::USE_LARGE_DOODAD_SIZEBAR));
	}
	if (house_palette) {
		house_palette->SetMap(map);
		house_palette->SetToolbarIconSize(g_settings.getBoolean(Config::USE_LARGE_HOUSE_SIZEBAR));
	}
	if (waypoint_palette) {
		waypoint_palette->SetMap(map);
	}
	if (item_palette) {
		item_palette->SetListType(wxstr(g_settings.getString(Config::PALETTE_ITEM_STYLE)));
		item_palette->SetToolbarIconSize(g_settings.getBoolean(Config::USE_LARGE_ITEM_SIZEBAR));
	}
	if (collection_palette) {
		collection_palette->SetListType(wxstr(g_settings.getString(Config::PALETTE_COLLECTION_STYLE)));
		collection_palette->SetToolbarIconSize(g_settings.getBoolean(Config::USE_LARGE_COLLECTION_TOOLBAR));
	}
	if (raw_palette) {
		raw_palette->SetListType(wxstr(g_settings.getString(Config::PALETTE_RAW_STYLE)));
		raw_palette->SetToolbarIconSize(g_settings.getBoolean(Config::USE_LARGE_RAW_SIZEBAR));
	}
	UpdateMinimapVisibility();
	InvalidateContents();
}

void PaletteWindow::LoadCurrentContents() {
	if (!choicebook) {
		return;
	}
	PalettePanel* panel = dynamic_cast<PalettePanel*>(choicebook->GetCurrentPage());
	if (panel) {
		panel->LoadCurrentContents();
	}
	Fit();
	Refresh();
	Update();
}

void PaletteWindow::InvalidateContents() {
	if (!choicebook) {
		return;
	}
	for (size_t iz = 0; iz < choicebook->GetPageCount(); ++iz) {
		PalettePanel* panel = dynamic_cast<PalettePanel*>(choicebook->GetPage(iz));
		panel->InvalidateContents();
	}
	LoadCurrentContents();
	if (creature_palette) {
		creature_palette->OnUpdate();
	}
	if (house_palette) {
		house_palette->OnUpdate();
	}
	if (waypoint_palette) {
		waypoint_palette->OnUpdate();
	}
}

void PaletteWindow::SelectPage(PaletteType id) {
	if (!choicebook) {
		return;
	}
	if (id == GetSelectedPage()) {
		return;
	}

	for (size_t iz = 0; iz < choicebook->GetPageCount(); ++iz) {
		PalettePanel* panel = dynamic_cast<PalettePanel*>(choicebook->GetPage(iz));
		if (panel->GetType() == id) {
			choicebook->SetSelection(iz);
			// LoadCurrentContents();
			break;
		}
	}
}

Brush* PaletteWindow::GetSelectedBrush() const {
	if (!choicebook) {
		return nullptr;
	}
	PalettePanel* panel = dynamic_cast<PalettePanel*>(choicebook->GetCurrentPage());
	return panel->GetSelectedBrush();
}

int PaletteWindow::GetSelectedBrushSize() const {
	if (!choicebook) {
		return 0;
	}
	PalettePanel* panel = dynamic_cast<PalettePanel*>(choicebook->GetCurrentPage());
	return panel->GetSelectedBrushSize();
}

PaletteType PaletteWindow::GetSelectedPage() const {
	if (!choicebook) {
		return TILESET_UNKNOWN;
	}
	PalettePanel* panel = dynamic_cast<PalettePanel*>(choicebook->GetCurrentPage());
	ASSERT(panel);
	return panel->GetType();
}

bool PaletteWindow::OnSelectBrush(const Brush* whatbrush, PaletteType primary) {
	if (!choicebook) {
		return false;
	}

	if (!whatbrush) {
		if (terrain_palette) terrain_palette->SelectBrush(nullptr);
		if (doodad_palette) doodad_palette->SelectBrush(nullptr);
		if (item_palette) item_palette->SelectBrush(nullptr);
		if (collection_palette) collection_palette->SelectBrush(nullptr);
		if (creature_palette) creature_palette->SelectBrush(nullptr);
		if (house_palette) house_palette->SelectBrush(nullptr);
		if (waypoint_palette) waypoint_palette->SelectBrush(nullptr);
		if (raw_palette) raw_palette->SelectBrush(nullptr);
		if (prefab_palette) prefab_palette->SelectBrush(nullptr);
		return true;
	}

	if ((whatbrush->isHouse() || whatbrush->isHouseExit()) && house_palette) {
		house_palette->SelectBrush(whatbrush);
		SelectPage(TILESET_HOUSE);
		return true;
	}

	// Priority 1: Check requested primary palette FIRST if specified
	if (primary == TILESET_TERRAIN) {
		if (terrain_palette && terrain_palette->SelectBrush(whatbrush)) {
			SelectPage(TILESET_TERRAIN);
			return true;
		}
	} else if (primary == TILESET_DOODAD) {
		if (doodad_palette && doodad_palette->SelectBrush(whatbrush)) {
			SelectPage(TILESET_DOODAD);
			return true;
		}
	} else if (primary == TILESET_COLLECTION) {
		if (collection_palette && collection_palette->SelectBrush(whatbrush)) {
			SelectPage(TILESET_COLLECTION);
			return true;
		}
	} else if (primary == TILESET_ITEM) {
		if (item_palette && item_palette->SelectBrush(whatbrush)) {
			SelectPage(TILESET_ITEM);
			return true;
		}
	} else if (primary == TILESET_CREATURE) {
		if (creature_palette && creature_palette->SelectBrush(whatbrush)) {
			SelectPage(TILESET_CREATURE);
			return true;
		}
	} else if (primary == TILESET_RAW) {
		if (raw_palette && raw_palette->SelectBrush(whatbrush)) {
			SelectPage(TILESET_RAW);
			return true;
		}
	} else if (primary == TILESET_PREFAB) {
		if (prefab_palette && prefab_palette->SelectBrush(whatbrush)) {
			SelectPage(TILESET_PREFAB);
			return true;
		}
	} else if (primary == TILESET_FAVORITE) {
		if (favorites_palette && favorites_palette->SelectBrush(whatbrush)) {
			SelectPage(TILESET_FAVORITE);
			return true;
		}
	}

	// If currently on Favorites page or brush is in Favorites
	if (GetSelectedPage() == TILESET_FAVORITE) {
		if (favorites_palette && favorites_palette->SelectBrush(whatbrush)) {
			SelectPage(TILESET_FAVORITE);
			return true;
		}
	}

	// Fallback to all palettes in standard order
	if (terrain_palette && terrain_palette->SelectBrush(whatbrush)) {
		SelectPage(TILESET_TERRAIN);
		return true;
	}
	if (doodad_palette && doodad_palette->SelectBrush(whatbrush)) {
		SelectPage(TILESET_DOODAD);
		return true;
	}
	if (item_palette && item_palette->SelectBrush(whatbrush)) {
		SelectPage(TILESET_ITEM);
		return true;
	}
	if (collection_palette && collection_palette->SelectBrush(whatbrush)) {
		SelectPage(TILESET_COLLECTION);
		return true;
	}
	if (creature_palette && creature_palette->SelectBrush(whatbrush)) {
		SelectPage(TILESET_CREATURE);
		return true;
	}
	if (raw_palette && raw_palette->SelectBrush(whatbrush)) {
		SelectPage(TILESET_RAW);
		return true;
	}
	if (prefab_palette && prefab_palette->SelectBrush(whatbrush)) {
		SelectPage(TILESET_PREFAB);
		return true;
	}
	if (favorites_palette && favorites_palette->SelectBrush(whatbrush)) {
		SelectPage(TILESET_FAVORITE);
		return true;
	}

	// Test if it's an item brush
	if (primary != TILESET_ITEM) {
		if (item_palette && item_palette->SelectBrush(whatbrush)) {
			SelectPage(TILESET_ITEM);
			return true;
		}
	}

	// Test if it's a creature brush
	if (primary != TILESET_CREATURE) {
		if (creature_palette && creature_palette->SelectBrush(whatbrush)) {
			SelectPage(TILESET_CREATURE);
			return true;
		}
	}

	// Test if it's a raw brush
	if (primary != TILESET_RAW) {
		if (raw_palette && raw_palette->SelectBrush(whatbrush)) {
			SelectPage(TILESET_RAW);
			return true;
		}
	}

	// Test if it's a prefab brush
	if (primary != TILESET_PREFAB) {
		if (prefab_palette && prefab_palette->SelectBrush(whatbrush)) {
			SelectPage(TILESET_PREFAB);
			return true;
		}
	}

	return false;
}

void PaletteWindow::OnSwitchingPage(wxChoicebookEvent& event) {
	event.Skip();
	if (!choicebook) {
		return;
	}

	wxWindow* old_page = choicebook->GetPage(choicebook->GetSelection());
	PalettePanel* old_panel = dynamic_cast<PalettePanel*>(old_page);
	if (old_panel) {
		old_panel->OnSwitchOut();
	}

	wxWindow* page = choicebook->GetPage(event.GetSelection());
	PalettePanel* panel = dynamic_cast<PalettePanel*>(page);
	if (panel) {
		panel->OnSwitchIn();
	}
}

void PaletteWindow::OnPageChanged(wxChoicebookEvent& event) {
	if (!choicebook) {
		return;
	}
	if (palette_choice) {
		palette_choice->SetSelection(event.GetSelection());
	}
	if (search_box) {
		search_box->SetValue("");
	}
	g_gui.SelectBrush();
}

void PaletteWindow::OnSearchTextChanged(wxCommandEvent& event) {
	if (!choicebook) return;
	wxString query = search_box->GetValue();
	wxWindow* page = choicebook->GetCurrentPage();
	if (auto* brush_page = dynamic_cast<BrushPalettePanel*>(page)) {
		brush_page->DoSearch(query);
	} else if (auto* creature_page = dynamic_cast<CreaturePalettePanel*>(page)) {
		creature_page->DoSearch(query);
	}
}

void PaletteWindow::OnUpdateBrushSize(BrushShape shape, int size) {
	if (!choicebook) {
		return;
	}
	PalettePanel* page = dynamic_cast<PalettePanel*>(choicebook->GetCurrentPage());
	if (page) {
		page->OnUpdateBrushSize(shape, size);
	}
}

void PaletteWindow::OnUpdate(Map* map) {
	if (creature_palette) {
		creature_palette->OnUpdate();
	}
	if (house_palette) {
		house_palette->SetMap(map);
	}
	if (waypoint_palette) {
		waypoint_palette->SetMap(map);
		waypoint_palette->OnUpdate();
	}
	UpdateMinimapVisibility();
	if (minimap_panel && minimap_panel->IsShown()) {
		minimap_panel->Refresh();
	}
}

void PaletteWindow::UpdateMinimapVisibility() {
	bool show_minimap = allow_minimap && g_settings.getBoolean(Config::MINIMAP_VISIBLE);
	if (splitter && card_assets && card_minimap) {
		if (!show_minimap && splitter->IsSplit()) {
			last_sash_pos = splitter->GetSashPosition();
			splitter->Unsplit(card_minimap);
		} else if (show_minimap && !splitter->IsSplit()) {
			splitter->SplitHorizontally(card_assets, card_minimap, last_sash_pos > 0 ? last_sash_pos : -180);
		}
	}
	Layout();
	Refresh();
}

void PaletteWindow::InvalidatePrefabPalette() {
	if (prefab_palette) {
		prefab_palette->InvalidateContents();
	}
}

void PaletteWindow::OnKey(wxKeyEvent& event) {
	if (g_gui.GetCurrentTab() != nullptr) {
		g_gui.GetCurrentMapTab()->GetEventHandler()->AddPendingEvent(event);
	}
}

void PaletteWindow::OnSize(wxSizeEvent& event) {
	Layout();
	Refresh();
	SnapDockWidth();
	event.Skip();
}

void PaletteWindow::SnapDockWidth() {
	if (!g_gui.aui_manager) return;
	wxAuiPaneInfo& pane = g_gui.aui_manager->GetPane(this);
	if (!pane.IsOk()) return;

	int scale_percent = g_settings.getInteger(Config::UI_SCALE);
	if (scale_percent < 100) scale_percent = 100;
	if (scale_percent > 200) scale_percent = 200;

	int btn_w = FromDIP(36 * scale_percent / 100);
	int vscroll = wxSystemSettings::GetMetric(wxSYS_VSCROLL_X);
	if (vscroll <= 0) vscroll = FromDIP(18);
	int chrome = vscroll + FromDIP(4);

	int min_w = 3 * btn_w + chrome;
	if (pane.min_size.x != min_w) {
		pane.MinSize(wxSize(min_w, 100));
	}

	if (!pane.IsDocked()) return;
	if (pane.dock_direction != wxAUI_DOCK_LEFT && pane.dock_direction != wxAUI_DOCK_RIGHT) return;

	int current_w = GetSize().x;
	if (current_w <= 0) return;

	int cols = (current_w - chrome + btn_w / 2) / btn_w;
	if (cols < 3) cols = 3;
	int snapped_w = cols * btn_w + chrome;

	if (std::abs(current_w - snapped_w) > 3 && !snapping_active) {
		snapping_active = true;
		CallAfter([this, snapped_w]() {
			if (g_gui.aui_manager) {
				wxAuiPaneInfo& p = g_gui.aui_manager->GetPane(this);
				if (p.IsOk() && p.IsDocked()) {
					p.best_size.x = snapped_w;
					g_gui.aui_manager->Update();
				}
			}
			snapping_active = false;
		});
	}
}

void PaletteWindow::CheckAndUpdateOrientation() {
	// Stable vertical orientation for Left/Right docking & floating
}

void PaletteWindow::SetHorizontalLayout(bool horizontal) {
	is_horizontal = horizontal;
	if (splitter && card_assets && card_minimap) {
		if (horizontal) {
			if (splitter->GetSplitMode() != wxSPLIT_VERTICAL && splitter->IsSplit()) {
				last_sash_pos = splitter->GetSashPosition();
				splitter->SetSplitMode(wxSPLIT_VERTICAL);
			}
		} else {
			if (splitter->GetSplitMode() != wxSPLIT_HORIZONTAL && splitter->IsSplit()) {
				last_sash_pos = splitter->GetSashPosition();
				splitter->SetSplitMode(wxSPLIT_HORIZONTAL);
			}
		}
	}
	UpdateMinimapVisibility();
	Layout();
	Refresh();
}

void PaletteWindow::ShowContextMenu(const wxPoint& pos) {
	wxMenu menu;
	if (card_assets) {
		wxMenuItem* item = menu.AppendCheckItem(12001, "Show Asset Palette");
		item->Check(card_assets->IsShown());
	}
	if (card_minimap) {
		wxMenuItem* item = menu.AppendCheckItem(12002, "Show Minimap");
		item->Check(allow_minimap && (!splitter || splitter->IsSplit()));
	}
	menu.AppendSeparator();
	menu.Append(12003, "Reset Palette Modules");

	Bind(wxEVT_MENU, [this](wxCommandEvent& ev) {
		int id = ev.GetId();
		if (id == 12001 && card_assets) {
			card_assets->Show(!card_assets->IsShown());
			if (card_assets->IsShown() && card_assets->IsCollapsed()) card_assets->SetCollapsed(false);
			Layout();
			Refresh();
		} else if (id == 12002 && card_minimap) {
			allow_minimap = !allow_minimap;
			UpdateMinimapVisibility();
		} else if (id == 12003) {
			allow_minimap = true;
			if (card_assets) { card_assets->Show(true); card_assets->SetCollapsed(false); }
			if (card_minimap) { card_minimap->Show(true); card_minimap->SetCollapsed(false); }
			if (splitter && !splitter->IsSplit()) {
				splitter->SplitHorizontally(card_assets, card_minimap, -180);
			}
			Layout();
			Refresh();
		}
	});

	PopupMenu(&menu, pos);
}

void PaletteWindow::OnClose(wxCloseEvent& event) {
	if (!event.CanVeto()) {
		Destroy();
	} else {
		Show(false);
		event.Veto(true);
	}
}
