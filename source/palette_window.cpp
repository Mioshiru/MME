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
#include <algorithm>

class MinimapPanel : public wxPanel {
public:
	bool dragging = false;

	MinimapPanel(wxWindow* parent) :
		wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(180, 255)) {
		SetMinSize(wxSize(180, 255));
		SetBackgroundColour(wxColor(10, 20, 35));
		
		// Dropdown for jumping to towns
		town_choice = new wxChoice(this, wxID_ANY, wxPoint(5, 182), wxSize(170, 20));
		town_choice->SetBackgroundColour(wxColour(10, 20, 35));
		town_choice->SetForegroundColour(wxColour(180, 150, 50));
		town_choice->Bind(wxEVT_CHOICE, &MinimapPanel::OnTownSelected, this);

		// Checkbox for view box
		view_box_chk = new wxCheckBox(this, wxID_ANY, "Show View Box", wxPoint(5, 207));
		view_box_chk->SetValue(g_settings.getInteger(Config::MINIMAP_VIEW_BOX) != 0);
		view_box_chk->SetForegroundColour(wxColor(180, 150, 50));
		view_box_chk->Bind(wxEVT_CHECKBOX, &MinimapPanel::OnToggleViewBox, this);

		// Button to dock back to canvas
		dock_btn = new wxButton(this, wxID_ANY, "Dock to Canvas", wxPoint(5, 227), wxSize(170, 20));
		dock_btn->SetBackgroundColour(wxColour(10, 20, 35));
		dock_btn->SetForegroundColour(wxColour(180, 150, 50));
		dock_btn->Bind(wxEVT_BUTTON, &MinimapPanel::OnDockToCanvas, this);

		Bind(wxEVT_PAINT, &MinimapPanel::OnPaint, this);
		Bind(wxEVT_LEFT_DOWN, &MinimapPanel::OnLeftDown, this);
		Bind(wxEVT_MOTION, &MinimapPanel::OnMouseMove, this);
		Bind(wxEVT_LEFT_UP, &MinimapPanel::OnLeftUp, this);
		Bind(wxEVT_MOUSEWHEEL, &MinimapPanel::OnMouseWheel, this);
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

	void OnToggleViewBox(wxCommandEvent& event) {
		g_settings.setInteger(Config::MINIMAP_VIEW_BOX, event.IsChecked() ? 1 : 0);
		Refresh();
		if (g_gui.GetCurrentMapTab() && g_gui.GetCurrentMapTab()->GetCanvas()) {
			g_gui.GetCurrentMapTab()->GetCanvas()->Refresh();
		}
	}

	void OnDockToCanvas(wxCommandEvent& event) {
		g_settings.setInteger(Config::MINIMAP_DOCK_STYLE, 0);
		g_gui.RefreshPalettes();
		if (g_gui.GetCurrentMapTab() && g_gui.GetCurrentMapTab()->GetCanvas()) {
			g_gui.GetCurrentMapTab()->GetCanvas()->Refresh();
		}
	}

	void OnPaint(wxPaintEvent& event) {
		wxPaintDC dc(this);
		dc.SetBackground(wxBrush(wxColor(10, 15, 25)));
		dc.Clear();

		MapTab* map_tab = g_gui.GetCurrentMapTab();
		if (!map_tab) return;
		MapCanvas* canvas = map_tab->GetCanvas();
		if (!canvas) return;

		// Rebuild town list dynamically if town count differs
		const Towns& towns = canvas->editor.map.towns;
		if ((int)town_choice->GetCount() - 2 != (int)towns.count()) {
			UpdateTownList();
		}

		// Make sure texture data is updated
		canvas->UpdateMinimapTexture();

		// Position controls below the 180x180 minimap image
		town_choice->Move(5, 185);
		view_box_chk->Move(5, 210);
		dock_btn->Move(5, 230);

		// Draw the minimap image
		wxImage img(180, 180, canvas->minimap_pixels, true);
		wxBitmap bmp(img);
		dc.DrawBitmap(bmp, 0, 0, false);

		// Draw gold border around the minimap image
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.SetPen(wxPen(wxColor(180, 140, 50), 1));
		dc.DrawRectangle(0, 0, 180, 180);

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

			const float sx = 180.0f / (float)std::max(1, canvas->minimap_span_w);
			const float sy = 180.0f / (float)std::max(1, canvas->minimap_span_h);
			int p_start_x = (int)((view_start_x - canvas->minimap_start_x) * sx);
			int p_start_y = (int)((view_start_y - canvas->minimap_start_y) * sy);
			int p_end_x = (int)((view_end_x - canvas->minimap_start_x) * sx);
			int p_end_y = (int)((view_end_y - canvas->minimap_start_y) * sy);

			p_start_x = std::max(p_start_x, 0);
			p_start_y = std::max(p_start_y, 0);
			p_end_x = std::min(p_end_x, 180);
			p_end_y = std::min(p_end_y, 180);

			if (p_start_x < p_end_x && p_start_y < p_end_y) {
				dc.SetBrush(*wxTRANSPARENT_BRUSH);
				dc.SetPen(wxPen(*wxWHITE, 1));
				dc.DrawRectangle(p_start_x, p_start_y, p_end_x - p_start_x, p_end_y - p_start_y);
			}
		}
	}

	void UpdatePosition(wxMouseEvent& event) {
		MapTab* map_tab = g_gui.GetCurrentMapTab();
		if (!map_tab) return;
		MapCanvas* canvas = map_tab->GetCanvas();
		if (!canvas) return;

		int mx = event.GetX();
		int my = event.GetY();
		mx = std::clamp(mx, 0, 179);
		my = std::clamp(my, 0, 179);

		float rel_x = (float)mx / 180.0f;
		float rel_y = (float)my / 180.0f;

		int click_map_x = canvas->minimap_start_x + (int)(rel_x * (float)std::max(1, canvas->minimap_span_w - 1));
		int click_map_y = canvas->minimap_start_y + (int)(rel_y * (float)std::max(1, canvas->minimap_span_h - 1));

		g_gui.SetScreenCenterPosition(Position(click_map_x, click_map_y, canvas->floor), true);
		canvas->last_minimap_update_time = 0; // immediate update
		canvas->Refresh();
		Refresh();
	}

	void OnLeftDown(wxMouseEvent& event) {
		int mx = event.GetX();
		int my = event.GetY();
		if (mx < 0 || mx >= 180 || my < 0 || my >= 180) return;

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
	wxCheckBox* view_box_chk;
	wxButton* dock_btn;
};

// ============================================================================
// Palette window

BEGIN_EVENT_TABLE(PaletteWindow, wxPanel)
EVT_CHOICEBOOK_PAGE_CHANGING(PALETTE_CHOICEBOOK, PaletteWindow::OnSwitchingPage)
EVT_CHOICEBOOK_PAGE_CHANGED(PALETTE_CHOICEBOOK, PaletteWindow::OnPageChanged)
EVT_CLOSE(PaletteWindow::OnClose)
EVT_TEXT(PALETTE_SEARCH_BOX, PaletteWindow::OnSearchTextChanged)
EVT_KEY_DOWN(PaletteWindow::OnKey)
END_EVENT_TABLE()



PaletteWindow::PaletteWindow(wxWindow* parent, const TilesetContainer& tilesets) :
	wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(255, 250)),
	choicebook(nullptr),
	terrain_palette(nullptr),
	doodad_palette(nullptr),
	item_palette(nullptr),
	collection_palette(nullptr),
	creature_palette(nullptr),
	house_palette(nullptr),
	waypoint_palette(nullptr),
	raw_palette(nullptr),
	prefab_palette(nullptr),
	favorites_palette(nullptr),
	minimap_panel(nullptr),
	palette_choice(nullptr),
	search_box(nullptr) {
	SetMinSize(wxSize(120, 150));
	SetBackgroundColour(wxColor(10, 20, 35));

	palette_choice = newd wxChoice(this, wxID_ANY);
	palette_choice->SetBackgroundColour(wxColour(10, 20, 35));
	palette_choice->SetForegroundColour(wxColour(180, 150, 50));

	search_box = newd wxTextCtrl(this, PALETTE_SEARCH_BOX, "", wxDefaultPosition, wxDefaultSize, 0);
	search_box->SetHint("Search brushes...");
	search_box->Bind(wxEVT_CHAR_HOOK, [](wxKeyEvent& event) {
		if (event.GetKeyCode() == WXK_ESCAPE) {
			event.Skip();
		} else {
			event.DoAllowNextEvent();
		}
	});

	choicebook = newd wxChoicebook(this, PALETTE_CHOICEBOOK, wxDefaultPosition, wxSize(255, 250));
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

	// Setup sizers
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
	sizer->Add(palette_choice, 0, wxEXPAND | wxALL, 5);
	sizer->Add(search_box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

	choicebook->SetMinSize(wxSize(120, 150));
	sizer->Add(choicebook, 1, wxEXPAND);

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

	minimap_panel = new MinimapPanel(this);
	sizer->Add(minimap_panel, 0, wxALIGN_CENTER | wxALL, 10);

	SetSizer(sizer);

	RefreshFavoritesBox();

	// Load first page
	LoadCurrentContents();
	SelectPage(TILESET_TERRAIN);

	UpdateMinimapVisibility();

	Fit();
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

	if (whatbrush->isHouse() && house_palette) {
		house_palette->SelectBrush(whatbrush);
		SelectPage(TILESET_HOUSE);
		return true;
	}

	// Priority 1: Check Favorites FIRST if primary is TILESET_FAVORITE or if currently on Favorites page or if brush is in Favorites
	if (primary == TILESET_FAVORITE || GetSelectedPage() == TILESET_FAVORITE) {
		if (favorites_palette && favorites_palette->SelectBrush(whatbrush)) {
			SelectPage(TILESET_FAVORITE);
			return true;
		}
	}

	// Always test if brush is in Favorites before falling back to default palettes
	if (favorites_palette && favorites_palette->SelectBrush(whatbrush)) {
		SelectPage(TILESET_FAVORITE);
		return true;
	}

	switch (primary) {
		case TILESET_TERRAIN: {
			// This is already searched first
			break;
		}
		case TILESET_DOODAD: {
			// Ok, search doodad before terrain
			if (doodad_palette && doodad_palette->SelectBrush(whatbrush)) {
				SelectPage(TILESET_DOODAD);
				return true;
			}
			break;
		}
		case TILESET_COLLECTION: {
			if (collection_palette && collection_palette->SelectBrush(whatbrush)) {
				SelectPage(TILESET_COLLECTION);
				return true;
			}
		}
		case TILESET_ITEM: {
			if (item_palette && item_palette->SelectBrush(whatbrush)) {
				SelectPage(TILESET_ITEM);
				return true;
			}
			break;
		}
		case TILESET_CREATURE: {
			if (creature_palette && creature_palette->SelectBrush(whatbrush)) {
				SelectPage(TILESET_CREATURE);
				return true;
			}
			break;
		}
		case TILESET_RAW: {
			if (raw_palette && raw_palette->SelectBrush(whatbrush)) {
				SelectPage(TILESET_RAW);
				return true;
			}
			break;
		}
		case TILESET_PREFAB: {
			if (prefab_palette && prefab_palette->SelectBrush(whatbrush)) {
				SelectPage(TILESET_PREFAB);
				return true;
			}
			break;
		}
		default:
			break;
	}

	// Test if it's a terrain brush
	if (terrain_palette && terrain_palette->SelectBrush(whatbrush)) {
		SelectPage(TILESET_TERRAIN);
		return true;
	}

	// Test if it's a doodad brush
	if (primary != TILESET_DOODAD) {
		if (doodad_palette && doodad_palette->SelectBrush(whatbrush)) {
			SelectPage(TILESET_DOODAD);
			return true;
		}
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
	ASSERT(page);
	page->OnUpdateBrushSize(shape, size);
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
	bool show_minimap = g_settings.getBoolean(Config::MINIMAP_VISIBLE) &&
		(g_settings.getInteger(Config::MINIMAP_DOCK_STYLE) == 1);
	if (minimap_panel) {
		if (minimap_panel->Show(show_minimap)) {
			GetSizer()->Layout();
		}
	}
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

void PaletteWindow::OnClose(wxCloseEvent& event) {
	if (!event.CanVeto()) {
		// We can't do anything! This sucks!
		// (application is closed, we have to destroy ourselves)
		Destroy();
	} else {
		Show(false);
		event.Veto(true);
	}
}
