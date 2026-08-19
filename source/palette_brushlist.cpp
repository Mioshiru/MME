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

#include "palette_brushlist.h"
#include "gui.h"
#include "brush.h"
#include "add_tileset_window.h"
#include "add_item_window.h"
#include "materials.h"
#include "creature_brush.h"
#include "palette_window.h"
#include <wx/settings.h>
#include <wx/wrapsizer.h>
#include <wx/dcbuffer.h>

// ============================================================================
// Brush Palette Panel
// A common class for terrain/doodad/item/raw palette

BEGIN_EVENT_TABLE(BrushPalettePanel, PalettePanel)
EVT_BUTTON(wxID_ADD, BrushPalettePanel::OnClickAddItemToTileset)
EVT_BUTTON(wxID_NEW, BrushPalettePanel::OnClickAddTileset)
EVT_CHOICEBOOK_PAGE_CHANGING(wxID_ANY, BrushPalettePanel::OnSwitchingPage)
EVT_CHOICEBOOK_PAGE_CHANGED(wxID_ANY, BrushPalettePanel::OnPageChanged)
EVT_CHOICE(PALETTE_TILESET_CHOICE, BrushPalettePanel::OnTilesetChoice)
END_EVENT_TABLE()

BrushPalettePanel::BrushPalettePanel(wxWindow* parent, const TilesetContainer& tilesets, TilesetCategoryType category, wxWindowID id) :
	PalettePanel(parent, id),
	palette_type(category),
	choicebook(nullptr),
	size_panel(nullptr),
	tileset_choice(nullptr) {
	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	// Create the tileset panel
	wxStaticBox* ts_box = new wxStaticBox(this, wxID_ANY, "Tileset");
	wxSizer* ts_sizer = newd wxStaticBoxSizer(ts_box, wxVERTICAL);
	tileset_choice = newd wxChoice(this, PALETTE_TILESET_CHOICE);
	ts_sizer->Add(tileset_choice, 0, wxEXPAND | wxBOTTOM, 5);

	wxChoicebook* tmp_choicebook = newd wxChoicebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
	ts_sizer->Add(tmp_choicebook, 1, wxEXPAND);
	topsizer->Add(ts_sizer, 1, wxEXPAND | wxALL, 5);

	if (g_settings.getBoolean(Config::SHOW_TILESET_EDITOR)) {
		wxSizer* tmpsizer = newd wxBoxSizer(wxHORIZONTAL);
		wxButton* buttonAddTileset = newd wxButton(this, wxID_NEW, "Add new Tileset");
		tmpsizer->Add(buttonAddTileset, wxSizerFlags(0).Center());

		wxButton* buttonAddItemToTileset = newd wxButton(this, wxID_ADD, "Add new Item");
		tmpsizer->Add(buttonAddItemToTileset, wxSizerFlags(0).Center());

		topsizer->Add(tmpsizer, 0, wxCENTER, 10);
	}

	if (category == TILESET_FAVORITE) {
		Tileset* favs = g_materials.tilesets["Favorites"];
		if (favs) {
			struct FavCat {
				TilesetCategoryType type;
				wxString name;
			} subcats[] = {
				{ TILESET_FAVORITE, "All Favorites" },
				{ TILESET_TERRAIN, "Terrain" },
				{ TILESET_DOODAD, "Doodads" },
				{ TILESET_ITEM, "Items" },
				{ TILESET_CREATURE, "Monster" },
				{ TILESET_NPC, "NPCs" }
			};

			for (const auto& sc : subcats) {
				const TilesetCategory* tcg = favs->getCategory(sc.type);
				if (tcg) {
					BrushPanel* panel = newd BrushPanel(tmp_choicebook);
					panel->AssignTileset(tcg);
					tmp_choicebook->AddPage(panel, sc.name);
					tileset_choice->Append(sc.name);
				}
			}
		}
	} else {
		for (TilesetContainer::const_iterator iter = tilesets.begin(); iter != tilesets.end(); ++iter) {
			if (iter->second->name == "Favorites") continue;
			const TilesetCategory* tcg = iter->second->getCategory(category);
			if (tcg && tcg->size() > 0) {
				BrushPanel* panel = newd BrushPanel(tmp_choicebook);
				panel->AssignTileset(tcg);
				tmp_choicebook->AddPage(panel, wxstr(iter->second->name));
				tileset_choice->Append(wxstr(iter->second->name));
			}
		}
	}

	if (tileset_choice->GetCount() > 0) {
		int caveIdx = tileset_choice->FindString("Cave");
		if (caveIdx != wxNOT_FOUND) {
			tileset_choice->SetSelection(caveIdx);
			tmp_choicebook->SetSelection(caveIdx);
		} else {
			tileset_choice->SetSelection(0);
			tmp_choicebook->SetSelection(0);
		}
	}
	tmp_choicebook->GetChoiceCtrl()->Hide();

	SetSizer(topsizer);

	choicebook = tmp_choicebook;
}

BrushPalettePanel::~BrushPalettePanel() {
	////
}

void BrushPalettePanel::InvalidateContents() {
	for (size_t iz = 0; iz < choicebook->GetPageCount(); ++iz) {
		BrushPanel* panel = dynamic_cast<BrushPanel*>(choicebook->GetPage(iz));
		panel->InvalidateContents();
	}
	PalettePanel::InvalidateContents();
}

void BrushPalettePanel::LoadCurrentContents() {
	wxWindow* page = choicebook->GetCurrentPage();
	BrushPanel* panel = dynamic_cast<BrushPanel*>(page);
	if (panel) {
		panel->OnSwitchIn();
	}
	PalettePanel::LoadCurrentContents();
}

void BrushPalettePanel::LoadAllContents() {
	for (size_t iz = 0; iz < choicebook->GetPageCount(); ++iz) {
		BrushPanel* panel = dynamic_cast<BrushPanel*>(choicebook->GetPage(iz));
		panel->LoadContents();
	}
	PalettePanel::LoadAllContents();
}

PaletteType BrushPalettePanel::GetType() const {
	return palette_type;
}

void BrushPalettePanel::SetListType(BrushListType ltype) {
	if (!choicebook) {
		return;
	}
	for (size_t iz = 0; iz < choicebook->GetPageCount(); ++iz) {
		BrushPanel* panel = dynamic_cast<BrushPanel*>(choicebook->GetPage(iz));
		panel->SetListType(ltype);
	}
}

void BrushPalettePanel::SetListType(wxString ltype) {
	if (!choicebook) {
		return;
	}
	for (size_t iz = 0; iz < choicebook->GetPageCount(); ++iz) {
		BrushPanel* panel = dynamic_cast<BrushPanel*>(choicebook->GetPage(iz));
		panel->SetListType(ltype);
	}
}

Brush* BrushPalettePanel::GetSelectedBrush() const {
	if (!choicebook) {
		return nullptr;
	}
	wxWindow* page = choicebook->GetCurrentPage();
	BrushPanel* panel = dynamic_cast<BrushPanel*>(page);
	Brush* res = nullptr;
	if (panel) {
		for (ToolBarList::const_iterator iter = tool_bars.begin(); iter != tool_bars.end(); ++iter) {
			res = (*iter)->GetSelectedBrush();
			if (res) {
				return res;
			}
		}
		res = panel->GetSelectedBrush();
	}
	return res;
}

void BrushPalettePanel::SelectFirstBrush() {
	if (!choicebook) {
		return;
	}
	wxWindow* page = choicebook->GetCurrentPage();
	BrushPanel* panel = dynamic_cast<BrushPanel*>(page);
	panel->SelectFirstBrush();
}

bool BrushPalettePanel::SelectBrush(const Brush* whatbrush) {
	if (!choicebook) {
		return false;
	}

	BrushPanel* panel = dynamic_cast<BrushPanel*>(choicebook->GetCurrentPage());
	if (!panel) {
		return false;
	}

	for (PalettePanel* toolBar : tool_bars) {
		if (toolBar->SelectBrush(whatbrush)) {
			panel->SelectBrush(nullptr);
			return true;
		}
	}

	if (panel->SelectBrush(whatbrush)) {
		for (PalettePanel* toolBar : tool_bars) {
			toolBar->SelectBrush(nullptr);
		}
		return true;
	}

	for (size_t iz = 0; iz < choicebook->GetPageCount(); ++iz) {
		if ((int)iz == choicebook->GetSelection()) {
			continue;
		}

		panel = dynamic_cast<BrushPanel*>(choicebook->GetPage(iz));
		if (panel && panel->SelectBrush(whatbrush)) {
			choicebook->SetSelection(iz);
			if (tileset_choice) {
				tileset_choice->SetSelection(iz);
			}
			for (PalettePanel* toolBar : tool_bars) {
				toolBar->SelectBrush(nullptr);
			}
			return true;
		}
	}
	return false;
}

void BrushPalettePanel::OnSwitchingPage(wxChoicebookEvent& event) {
	event.Skip();
	if (!choicebook) {
		return;
	}
	BrushPanel* old_panel = dynamic_cast<BrushPanel*>(choicebook->GetCurrentPage());
	if (old_panel) {
		old_panel->OnSwitchOut();
		for (ToolBarList::iterator iter = tool_bars.begin(); iter != tool_bars.end(); ++iter) {
			Brush* tmp = (*iter)->GetSelectedBrush();
			if (tmp) {
				remembered_brushes[old_panel] = tmp;
			}
		}
	}

	wxWindow* page = choicebook->GetPage(event.GetSelection());
	BrushPanel* panel = dynamic_cast<BrushPanel*>(page);
	if (panel) {
		panel->OnSwitchIn();
		for (ToolBarList::iterator iter = tool_bars.begin(); iter != tool_bars.end(); ++iter) {
			(*iter)->SelectBrush(remembered_brushes[panel]);
		}
	}
}

void BrushPalettePanel::OnPageChanged(wxChoicebookEvent& event) {
	if (!choicebook) {
		return;
	}
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush();
}

void BrushPalettePanel::OnTilesetChoice(wxCommandEvent& event) {
	int sel = event.GetSelection();
	if (sel != wxNOT_FOUND && choicebook) {
		PaletteWindow* pw = GetParentPalette();
		if (pw && pw->GetSearchBox()) {
			pw->GetSearchBox()->ChangeValue("");
		}
		for (size_t i = 0; i < choicebook->GetPageCount(); ++i) {
			BrushPanel* panel = dynamic_cast<BrushPanel*>(choicebook->GetPage(i));
			if (panel) {
				panel->Filter("");
			}
		}
		choicebook->SetSelection(sel);
	}
}

void BrushPalettePanel::DoSearch(const wxString& query) {
	if (!choicebook) return;

	if (query.IsEmpty()) {
		for (size_t i = 0; i < choicebook->GetPageCount(); ++i) {
			BrushPanel* panel = dynamic_cast<BrushPanel*>(choicebook->GetPage(i));
			if (panel) {
				panel->Filter("");
			}
		}
		BrushPanel* active_panel = dynamic_cast<BrushPanel*>(choicebook->GetCurrentPage());
		if (active_panel) {
			active_panel->SelectFirstBrush();
		}
		return;
	}

	wxString lower_query = query.Lower();

	int current_sel = choicebook->GetSelection();
	BrushPanel* target_panel = nullptr;
	Brush* target_brush = nullptr;
	int target_page_idx = -1;

	// Check current page first
	if (current_sel != wxNOT_FOUND) {
		BrushPanel* panel = dynamic_cast<BrushPanel*>(choicebook->GetPage(current_sel));
		if (panel) {
			for (Brush* brush : panel->GetBrushes()) {
				if (wxstr(brush->getName()).Lower().Contains(lower_query)) {
					target_panel = panel;
					target_brush = brush;
					target_page_idx = current_sel;
					break;
				}
			}
		}
	}

	// If not found on current page, check other pages
	if (!target_brush) {
		for (size_t i = 0; i < choicebook->GetPageCount(); ++i) {
			if ((int)i == current_sel) continue;
			BrushPanel* panel = dynamic_cast<BrushPanel*>(choicebook->GetPage(i));
			if (panel) {
				for (Brush* brush : panel->GetBrushes()) {
					if (wxstr(brush->getName()).Lower().Contains(lower_query)) {
						target_panel = panel;
						target_brush = brush;
						target_page_idx = i;
						break;
					}
				}
			}
			if (target_brush) break;
		}
	}

	if (target_brush && target_panel) {
		if (target_page_idx != current_sel) {
			choicebook->SetSelection(target_page_idx);
			if (tileset_choice) {
				tileset_choice->SetSelection(target_page_idx);
			}
		}
		target_panel->Filter(query);
		target_panel->SelectBrush(target_brush);
		g_gui.SelectBrush(target_brush, GetType());
	} else {
		BrushPanel* active_panel = dynamic_cast<BrushPanel*>(choicebook->GetCurrentPage());
		if (active_panel) {
			active_panel->Filter(query);
		}
	}
}

void BrushPalettePanel::OnSwitchIn() {
	LoadCurrentContents();
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SetBrushSizeInternal(last_brush_size);
	OnUpdateBrushSize(g_gui.GetBrushShape(), last_brush_size);
}

void BrushPalettePanel::OnClickAddTileset(wxCommandEvent& WXUNUSED(event)) {
	if (!choicebook) {
		return;
	}

	wxDialog* w = newd AddTilesetWindow(g_gui.root, palette_type);
	int ret = w->ShowModal();
	w->Destroy();

	if (ret != 0) {
		g_gui.DestroyPalettes();
		g_gui.NewPalette();
	}
}

void BrushPalettePanel::OnClickAddItemToTileset(wxCommandEvent& WXUNUSED(event)) {
	if (!choicebook) {
		return;
	}
	std::string tilesetName = choicebook->GetPageText(choicebook->GetSelection()).ToStdString();

	auto _it = g_materials.tilesets.find(tilesetName);
	if (_it != g_materials.tilesets.end()) {
		wxDialog* w = newd AddItemWindow(g_gui.root, palette_type, _it->second);
		int ret = w->ShowModal();
		w->Destroy();

		if (ret != 0) {
			g_gui.RebuildPalettes();
		}
	}
}

// ============================================================================
// Brush Panel
// A container of brush buttons

BEGIN_EVENT_TABLE(BrushPanel, wxPanel)
// Listbox style
EVT_LISTBOX(wxID_ANY, BrushPanel::OnClickListBoxRow)
END_EVENT_TABLE()

BrushPanel::BrushPanel(wxWindow* parent) :
	wxPanel(parent, wxID_ANY),
	tileset(nullptr),
	brushbox(nullptr),
	loaded(false),
	list_type(BRUSHLIST_LISTBOX) {
	sizer = newd wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);
	SetMinSize(wxSize(180, 200));
}

BrushPanel::~BrushPanel() {
	////
}

void BrushPanel::AssignTileset(const TilesetCategory* _tileset) {
	if (_tileset != tileset) {
		InvalidateContents();
		tileset = _tileset;
		if (tileset) {
			all_brushes = tileset->brushlist;
		}
	}
}

void BrushPanel::AssignBrushes(const std::vector<Brush*>& brushes) {
	InvalidateContents();
	tileset = nullptr;
	all_brushes = brushes;
}

void BrushPanel::SetListType(BrushListType ltype) {
	if (list_type != ltype) {
		InvalidateContents();
		list_type = ltype;
	}
}

void BrushPanel::SetListType(wxString ltype) {
	if (ltype == "small icons") {
		SetListType(BRUSHLIST_SMALL_ICONS);
	} else if (ltype == "large icons") {
		SetListType(BRUSHLIST_LARGE_ICONS);
	} else if (ltype == "listbox") {
		SetListType(BRUSHLIST_LISTBOX);
	} else if (ltype == "textlistbox") {
		SetListType(BRUSHLIST_TEXT_LISTBOX);
	}
}

void BrushPanel::Filter(const wxString& query) {
	search_query = query;
	if (loaded && brushbox) {
		brushbox->Filter(query);
	}
}

void BrushPanel::InvalidateContents() {
	if (tileset) {
		all_brushes = tileset->brushlist;
	}
	if (loaded && brushbox) {
		if (BrushIconBox* iconbox = dynamic_cast<BrushIconBox*>(brushbox->GetSelfWindow())) {
			iconbox->SetBrushes(all_brushes);
		}
	}
}

void BrushPanel::LoadContents() {
	if (loaded) {
		return;
	}
	Freeze();
	loaded = true;
	switch (list_type) {
		case BRUSHLIST_LARGE_ICONS:
			brushbox = newd BrushIconBox(this, all_brushes, RENDER_SIZE_32x32);
			break;
		case BRUSHLIST_SMALL_ICONS:
			brushbox = newd BrushIconBox(this, all_brushes, RENDER_SIZE_16x16);
			break;
		case BRUSHLIST_LISTBOX:
			brushbox = newd BrushListBox(this, all_brushes);
			break;
		default:
			break;
	}
	sizer->Add(brushbox->GetSelfWindow(), 1, wxEXPAND);
	Layout();
	if (BrushIconBox* iconbox = dynamic_cast<BrushIconBox*>(brushbox->GetSelfWindow())) {
		iconbox->UpdateLayout();
	}
	if (!search_query.IsEmpty()) {
		brushbox->Filter(search_query);
	} else {
		brushbox->SelectFirstBrush();
	}
	Thaw();
}

void BrushPanel::SelectFirstBrush() {
	if (loaded) {
		ASSERT(brushbox != nullptr);
		brushbox->SelectFirstBrush();
	}
}

Brush* BrushPanel::GetSelectedBrush() const {
	if (loaded) {
		ASSERT(brushbox != nullptr);
		return brushbox->GetSelectedBrush();
	}

	if (all_brushes.size() > 0) {
		return all_brushes[0];
	}
	return nullptr;
}

bool BrushPanel::SelectBrush(const Brush* whatbrush) {
	if (loaded) {
		// std::cout << loaded << std::endl;
		// std::cout << brushbox << std::endl;
		ASSERT(brushbox != nullptr);
		return brushbox->SelectBrush(whatbrush);
	}

	for (BrushVector::const_iterator iter = all_brushes.begin(); iter != all_brushes.end(); ++iter) {
		if (*iter == whatbrush) {
			LoadContents();
			return brushbox->SelectBrush(whatbrush);
		}
	}
	return false;
}

void BrushPanel::OnSwitchIn() {
	LoadContents();
}

void BrushPanel::OnSwitchOut() {
	////
}

void BrushPanel::OnClickListBoxRow(wxCommandEvent& event) {
	// We just notify the GUI of the action, it will take care of everything else
	ASSERT(brushbox);

	wxWindow* w = this;
	while ((w = w->GetParent()) && dynamic_cast<PaletteWindow*>(w) == nullptr)
		;

	if (w) {
		g_gui.ActivatePalette(static_cast<PaletteWindow*>(w));
	}

	Brush* selected = brushbox->GetSelectedBrush();
	if (selected) {
		TilesetCategoryType catType = tileset ? tileset->getType() : TILESET_UNKNOWN;
		g_gui.SelectBrush(selected, catType);
	}
}

// ============================================================================
// BrushIconBox

BEGIN_EVENT_TABLE(BrushIconBox, wxScrolledWindow)
EVT_PAINT(BrushIconBox::OnPaint)
EVT_LEFT_DOWN(BrushIconBox::OnClick)
EVT_RIGHT_DOWN(BrushIconBox::OnRightClick)
EVT_MOTION(BrushIconBox::OnMouseMove)
EVT_SIZE(BrushIconBox::OnSize)
END_EVENT_TABLE()

BrushIconBox::BrushIconBox(wxWindow* parent, const std::vector<Brush*>& brushes, RenderSize rsz) :
	wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxFULL_REPAINT_ON_RESIZE),
	BrushBoxInterface(brushes),
	icon_size(rsz) {
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetScrollRate(0, 20);
	UpdateLayout();
}

BrushIconBox::~BrushIconBox() {
}

void BrushIconBox::SelectFirstBrush() {
	selected_brush = nullptr;
	for (size_t i = 0; i < visible_brushes.size(); ++i) {
		if (visible_brushes[i] && !visible_brushes[i]->isSeparator()) {
			selected_brush = visible_brushes[i];
			EnsureVisible(i);
			break;
		}
	}
	Refresh();
}

Brush* BrushIconBox::GetSelectedBrush() const {
	return selected_brush;
}

bool BrushIconBox::SelectBrush(const Brush* whatbrush) {
	if (whatbrush && whatbrush->isSeparator()) return false;
	for (size_t i = 0; i < visible_brushes.size(); ++i) {
		if (visible_brushes[i] == whatbrush) {
			selected_brush = const_cast<Brush*>(whatbrush);
			EnsureVisible(i);
			Refresh();
			return true;
		}
	}
	return false;
}

void BrushIconBox::SetBrushes(const std::vector<Brush*>& brushes) {
	all_brushes = brushes;
	visible_brushes = brushes;
	UpdateLayout();
	Refresh();
}

void BrushIconBox::EnsureVisible(size_t n) {
	if (n >= visible_brushes.size()) return;
	Brush* target = visible_brushes[n];
	for (const auto& item : item_layout) {
		if (item.brush == target) {
			int scroll_unit_y = 20;
			GetScrollPixelsPerUnit(nullptr, &scroll_unit_y);
			if (scroll_unit_y > 0) {
				int start_scroll_y;
				GetViewStart(nullptr, &start_scroll_y);
				int client_height = GetClientSize().y;

				int start_y = start_scroll_y * scroll_unit_y;
				int end_y = start_y + client_height;

				if (item.y < start_y || item.y + item.height > end_y) {
					Scroll(-1, item.y / scroll_unit_y);
				}
			}
			break;
		}
	}
}

void BrushIconBox::Filter(const wxString& query) {
	visible_brushes.clear();
	wxString lower_query = query.Lower();
	for (Brush* brush : all_brushes) {
		if (!brush) continue;
		if (brush->isSeparator()) {
			if (lower_query.IsEmpty()) {
				visible_brushes.push_back(brush);
			}
		} else if (lower_query.IsEmpty() || wxstr(brush->getName()).Lower().Contains(lower_query)) {
			visible_brushes.push_back(brush);
		}
	}
	UpdateLayout();
	SelectFirstBrush();
	Refresh();
}

void BrushIconBox::OnSize(wxSizeEvent& event) {
	UpdateLayout();
	Refresh();
	event.Skip();
}

void BrushIconBox::UpdateLayout() {
	item_layout.clear();
	int client_width = GetClientSize().x;
	int btn_width = 36;
	int scale_percent = g_settings.getInteger(Config::UI_SCALE);
	if (scale_percent < 100) scale_percent = 100;
	if (scale_percent > 200) scale_percent = 200;

	if (icon_size == RENDER_SIZE_16x16) {
		btn_width = 20;
	} else if (icon_size == RENDER_SIZE_32x32) {
		btn_width = 36;
	}
	btn_width = btn_width * scale_percent / 100;

	int columns = client_width / btn_width;
	if (columns < 1) columns = 1;

	int total_items_width = columns * btn_width;
	int left_padding = (client_width - total_items_width) / 2;
	if (left_padding < 0) left_padding = 0;

	int cur_x = left_padding;
	int cur_y = 4;
	int col_in_row = 0;
	int sep_height = 22 * scale_percent / 100;
	bool current_collapsed = false;

	for (Brush* brush : visible_brushes) {
		if (!brush) continue;
		if (brush->isSeparator()) {
			SeparatorBrush* sep = brush->asSeparator();
			current_collapsed = sep ? sep->isCollapsed() : false;

			if (col_in_row > 0) {
				cur_y += btn_width + 4;
				cur_x = left_padding;
				col_in_row = 0;
			}
			BrushItemPos pos;
			pos.brush = brush;
			pos.x = 4;
			pos.y = cur_y;
			pos.width = std::max(10, client_width - 8);
			pos.height = sep_height;
			pos.is_separator = true;
			pos.is_collapsed = current_collapsed;
			pos.label = brush->getName();
			item_layout.push_back(pos);

			cur_y += sep_height + 4;
		} else {
			if (current_collapsed) {
				continue; // Skip items in collapsed section!
			}

			BrushItemPos pos;
			pos.brush = brush;
			pos.x = cur_x;
			pos.y = cur_y;
			pos.width = btn_width;
			pos.height = btn_width;
			pos.is_separator = false;
			item_layout.push_back(pos);

			col_in_row++;
			if (col_in_row >= columns) {
				col_in_row = 0;
				cur_x = left_padding;
				cur_y += btn_width;
			} else {
				cur_x += btn_width;
			}
		}
	}

	if (col_in_row > 0) {
		cur_y += btn_width;
	}
	int total_height = std::max(150, cur_y + 10);
	SetVirtualSize(client_width, total_height);
}

void BrushIconBox::OnPaint(wxPaintEvent& event) {
	wxBufferedPaintDC dc(this);
	DoPrepareDC(dc);

	dc.SetBackground(wxBrush(wxColor(10, 20, 35)));
	dc.Clear();

	if (g_gui.gfx.isUnloaded()) {
		return;
	}

	int scale_percent = g_settings.getInteger(Config::UI_SCALE);
	if (scale_percent < 100) scale_percent = 100;
	if (scale_percent > 200) scale_percent = 200;

	int spr_w = (icon_size == RENDER_SIZE_16x16 ? 16 : 32) * scale_percent / 100;
	int offset = 2 * scale_percent / 100;

	static std::unique_ptr<wxPen> highlight_pen;
	static std::unique_ptr<wxPen> dark_highlight_pen;
	static std::unique_ptr<wxPen> light_shadow_pen;
	static std::unique_ptr<wxPen> shadow_pen;

	if (highlight_pen.get() == nullptr) {
		highlight_pen.reset(newd wxPen(wxColor(0xFF, 0xFF, 0xFF), 1, wxSOLID));
		dark_highlight_pen.reset(newd wxPen(wxColor(0xD4, 0xD0, 0xC8), 1, wxSOLID));
		light_shadow_pen.reset(newd wxPen(wxColor(0x80, 0x80, 0x80), 1, wxSOLID));
		shadow_pen.reset(newd wxPen(wxColor(0x40, 0x40, 0x40), 1, wxSOLID));
	}

	SpriteSize spr_sz = (icon_size == RENDER_SIZE_16x16 ? SPRITE_SIZE_16x16 : SPRITE_SIZE_32x32);

	int client_height = GetClientSize().y;
	int start_y = GetViewStart().y * 20;
	int end_y = start_y + client_height;

	for (const auto& item : item_layout) {
		if (item.y + item.height < start_y || item.y > end_y) {
			continue;
		}

		if (item.is_separator) {
			int line_y = item.y + item.height / 2;

			// Draw vector chevron
			int chevron_x = item.x + 6;
			dc.SetBrush(wxBrush(wxColor(130, 165, 205)));
			dc.SetPen(wxPen(wxColor(130, 165, 205), 1, wxSOLID));
			if (item.is_collapsed) {
				wxPoint pts[3] = {
					wxPoint(chevron_x, line_y - 4),
					wxPoint(chevron_x + 6, line_y),
					wxPoint(chevron_x, line_y + 4)
				};
				dc.DrawPolygon(3, pts);
			} else {
				wxPoint pts[3] = {
					wxPoint(chevron_x - 1, line_y - 3),
					wxPoint(chevron_x + 7, line_y - 3),
					wxPoint(chevron_x + 3, line_y + 3)
				};
				dc.DrawPolygon(3, pts);
			}

			int text_w = 0;
			if (!item.label.empty()) {
				dc.SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
				dc.SetTextForeground(wxColor(140, 175, 215));
				wxSize tsz = dc.GetTextExtent(wxstr(item.label));
				text_w = tsz.x;
				dc.DrawText(wxstr(item.label), item.x + 18, item.y + (item.height - tsz.y) / 2);
			}
			dc.SetPen(wxPen(wxColor(50, 75, 105), 1, wxSOLID));
			int line_start_x = item.x + (text_w > 0 ? text_w + 26 : 18);
			int line_end_x = item.x + item.width - 4;
			if (line_end_x > line_start_x) {
				dc.DrawLine(line_start_x, line_y, line_end_x, line_y);
			}
			continue;
		}

		int x = item.x;
		int y = item.y;
		int btn_width = item.width;
		Brush* brush = item.brush;
		bool is_selected = (brush == selected_brush);

		dc.SetBrush(*wxBLACK_BRUSH);
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.DrawRectangle(x, y, btn_width, btn_width);

		if (is_selected) {
			// High-contrast clear selection frame: outer dark outline + bright gold highlight frame
			dc.SetPen(wxPen(wxColor(0, 0, 0), 2, wxSOLID));
			dc.SetBrush(*wxTRANSPARENT_BRUSH);
			dc.DrawRectangle(x, y, btn_width, btn_width);
			dc.SetPen(wxPen(wxColor(255, 205, 50), 2, wxSOLID));
			dc.DrawRectangle(x + 1, y + 1, btn_width - 2, btn_width - 2);
		} else {
			dc.SetPen(*highlight_pen);
			dc.DrawLine(x, y, x + btn_width - 1, y);
			dc.DrawLine(x, y + 1, x, y + btn_width - 1);
			dc.SetPen(*dark_highlight_pen);
			dc.DrawLine(x + 1, y + 1, x + btn_width - 2, y + 1);
			dc.DrawLine(x + 1, y + 2, x + 1, y + btn_width - 2);
			dc.SetPen(*light_shadow_pen);
			dc.DrawLine(x + btn_width - 2, y + 1, x + btn_width - 2, y + btn_width - 2);
			dc.DrawLine(x + 1, y + btn_width - 2, x + btn_width - 1, y + btn_width - 2);
			dc.SetPen(*shadow_pen);
			dc.DrawLine(x + btn_width - 1, y, x + btn_width - 1, y + btn_width - 1);
			dc.DrawLine(x, y + btn_width - 1, x + btn_width, y + btn_width - 1);
		}

		if (brush) {
			Sprite* sprite = nullptr;
			int look_id = brush->getLookID();
			if (look_id > 0 && g_items.typeExists(look_id)) {
				sprite = g_gui.gfx.getSprite(g_items[look_id].clientID);
			} else {
				sprite = g_gui.gfx.getSprite(look_id);
			}

			if (sprite) {
				sprite->DrawTo(&dc, spr_sz, x + offset, y + offset, spr_w, spr_w);
			}
		}
	}
}

void BrushIconBox::OnClick(wxMouseEvent& event) {
	int logical_x, logical_y;
	CalcUnscrolledPosition(event.GetX(), event.GetY(), &logical_x, &logical_y);

	for (const auto& item : item_layout) {
		if (logical_x >= item.x && logical_x < item.x + item.width &&
			logical_y >= item.y && logical_y < item.y + item.height) {
			if (item.is_separator) {
				if (SeparatorBrush* sep = item.brush ? item.brush->asSeparator() : nullptr) {
					sep->toggleCollapsed();
					UpdateLayout();
					Refresh();
				}
				return;
			}

			Brush* clicked_brush = item.brush;
			if (!clicked_brush) return;
			SelectBrush(clicked_brush);

			wxWindow* w = this;
			while ((w = w->GetParent()) && dynamic_cast<PaletteWindow*>(w) == nullptr)
				;
			if (w) {
				g_gui.ActivatePalette(static_cast<PaletteWindow*>(w));
			}

			TilesetCategoryType catType = TILESET_UNKNOWN;
			wxWindow* pw = this;
			while (pw && dynamic_cast<BrushPalettePanel*>(pw) == nullptr) {
				pw = pw->GetParent();
			}
			if (pw) {
				catType = static_cast<BrushPalettePanel*>(pw)->GetType();
			}

			g_gui.SelectBrush(clicked_brush, catType);
			break;
		}
	}
	SetFocus();
}

static void AddFavoriteBrushIconBox(Brush* brush) {
	if (!brush || brush->isSeparator()) return;
	Tileset* favs = g_materials.tilesets["Favorites"];
	if (!favs) return;

	TilesetCategory* catFav = favs->getCategory(TILESET_FAVORITE);
	if (catFav && !catFav->containsBrush(brush)) {
		catFav->brushlist.push_back(brush);
	}

	g_materials.rebuildFavorites();
	g_materials.saveFavorites();
}

static void RemoveFavoriteBrushIconBox(Brush* brush) {
	if (!brush || brush->isSeparator()) return;
	Tileset* favs = g_materials.tilesets["Favorites"];
	if (!favs) return;

	for (TilesetCategory* cat : favs->categories) {
		auto it = std::find(cat->brushlist.begin(), cat->brushlist.end(), brush);
		if (it != cat->brushlist.end()) {
			cat->brushlist.erase(it);
		}
	}

	g_materials.rebuildFavorites();
	g_materials.saveFavorites();
}

void BrushIconBox::OnRightClick(wxMouseEvent& event) {
	int logical_x, logical_y;
	CalcUnscrolledPosition(event.GetX(), event.GetY(), &logical_x, &logical_y);

	Brush* clicked_brush = nullptr;
	for (const auto& item : item_layout) {
		if (item.is_separator) continue;
		if (logical_x >= item.x && logical_x < item.x + item.width &&
			logical_y >= item.y && logical_y < item.y + item.height) {
			clicked_brush = item.brush;
			break;
		}
	}

	if (clicked_brush) {
		wxMenu menu;
		Tileset* favs = g_materials.tilesets["Favorites"];
		bool is_favorited = false;
		if (favs) {
			const TilesetCategory* cat = favs->getCategory(TILESET_FAVORITE);
			if (cat && cat->containsBrush(clicked_brush)) {
				is_favorited = true;
			}
		}

		if (is_favorited) {
			menu.Append(10002, "Remove Favorite");
		} else {
			menu.Append(10001, "Favorite");
		}

		Bind(wxEVT_MENU, [this, clicked_brush](wxCommandEvent& ev) {
			if (ev.GetId() == 10001) {
				AddFavoriteBrushIconBox(clicked_brush);
			} else if (ev.GetId() == 10002) {
				RemoveFavoriteBrushIconBox(clicked_brush);
			}
			g_gui.RefreshFavoritesBox();
		});

		PopupMenu(&menu);
	}
}

void BrushIconBox::OnMouseMove(wxMouseEvent& event) {
	int logical_x, logical_y;
	CalcUnscrolledPosition(event.GetX(), event.GetY(), &logical_x, &logical_y);

	Brush* hovered = nullptr;
	for (const auto& item : item_layout) {
		if (item.is_separator) continue;
		if (logical_x >= item.x && logical_x < item.x + item.width &&
			logical_y >= item.y && logical_y < item.y + item.height) {
			hovered = item.brush;
			break;
		}
	}

	if (hovered) {
		wxString tip = wxstr(hovered->getName());
		if (RAWBrush* raw = dynamic_cast<RAWBrush*>(hovered)) {
			tip << " (ID: " << raw->getItemID() << ")";
		}
		SetToolTip(tip);
		g_gui.SetStatusText(tip);
	} else {
		UnsetToolTip();
	}
	event.Skip();
}

// ============================================================================
// BrushListBox

BEGIN_EVENT_TABLE(BrushListBox, wxVListBox)
EVT_KEY_DOWN(BrushListBox::OnKey)
EVT_LEFT_DOWN(BrushListBox::OnLeftDown)
END_EVENT_TABLE()

BrushListBox::BrushListBox(wxWindow* parent, const std::vector<Brush*>& brushes) :
	wxVListBox(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLB_SINGLE),
	BrushBoxInterface(brushes) {
	UpdateVisibleList();
}

BrushListBox::~BrushListBox() {
	////
}

void BrushListBox::UpdateVisibleList() {
	visible_brushes.clear();
	wxString lower_query = current_query.Lower();
	bool in_collapsed = false;

	for (Brush* brush : all_brushes) {
		if (!brush) continue;
		if (brush->isSeparator()) {
			SeparatorBrush* sep = brush->asSeparator();
			in_collapsed = sep ? sep->isCollapsed() : false;
			if (lower_query.IsEmpty()) {
				visible_brushes.push_back(brush);
			}
		} else {
			if (in_collapsed && lower_query.IsEmpty()) {
				continue;
			}
			if (lower_query.IsEmpty() || wxstr(brush->getName()).Lower().Contains(lower_query)) {
				visible_brushes.push_back(brush);
			}
		}
	}
	SetItemCount(visible_brushes.size());
	Refresh();
}

void BrushListBox::SelectFirstBrush() {
	for (size_t n = 0; n < visible_brushes.size(); ++n) {
		if (visible_brushes[n] && !visible_brushes[n]->isSeparator()) {
			SetSelection(n);
			return;
		}
	}
	SetSelection(wxNOT_FOUND);
	wxWindow::ScrollLines(-1);
}

Brush* BrushListBox::GetSelectedBrush() const {
	int n = GetSelection();
	if (n != wxNOT_FOUND && n < (int)visible_brushes.size() && visible_brushes[n] && !visible_brushes[n]->isSeparator()) {
		return visible_brushes[n];
	} else if (visible_brushes.size() > 0) {
		for (Brush* b : visible_brushes) {
			if (b && !b->isSeparator()) return b;
		}
	}
	return nullptr;
}

bool BrushListBox::SelectBrush(const Brush* whatbrush) {
	if (!whatbrush || whatbrush->isSeparator()) {
		SetSelection(wxNOT_FOUND);
		return false;
	}
	for (size_t n = 0; n < visible_brushes.size(); ++n) {
		if (visible_brushes[n] == whatbrush) {
			SetSelection(n);
			return true;
		}
	}
	return false;
}

void BrushListBox::OnLeftDown(wxMouseEvent& event) {
	int item_hit = HitTest(event.GetPosition());
	if (item_hit != wxNOT_FOUND && item_hit < (int)visible_brushes.size()) {
		Brush* b = visible_brushes[item_hit];
		if (b && b->isSeparator()) {
			if (SeparatorBrush* sep = b->asSeparator()) {
				sep->toggleCollapsed();
				UpdateVisibleList();
			}
			return;
		}
	}
	event.Skip();
}

void BrushListBox::OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const {
	ASSERT(n < visible_brushes.size());
	Brush* brush = visible_brushes[n];
	if (brush && brush->isSeparator()) {
		int line_y = rect.GetY() + rect.GetHeight() / 2;
		SeparatorBrush* sep = brush->asSeparator();
		bool collapsed = sep ? sep->isCollapsed() : false;

		// Draw chevron
		int chevron_x = rect.GetX() + 6;
		dc.SetBrush(wxBrush(wxColor(130, 165, 205)));
		dc.SetPen(wxPen(wxColor(130, 165, 205), 1, wxSOLID));
		if (collapsed) {
			wxPoint pts[3] = {
				wxPoint(chevron_x, line_y - 4),
				wxPoint(chevron_x + 6, line_y),
				wxPoint(chevron_x, line_y + 4)
			};
			dc.DrawPolygon(3, pts);
		} else {
			wxPoint pts[3] = {
				wxPoint(chevron_x - 1, line_y - 3),
				wxPoint(chevron_x + 7, line_y - 3),
				wxPoint(chevron_x + 3, line_y + 3)
			};
			dc.DrawPolygon(3, pts);
		}

		std::string label = brush->getName();
		int text_w = 0;
		if (!label.empty()) {
			dc.SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
			dc.SetTextForeground(wxColor(140, 175, 215));
			wxSize tsz = dc.GetTextExtent(wxstr(label));
			text_w = tsz.x;
			dc.DrawText(wxstr(label), rect.GetX() + 18, rect.GetY() + (rect.GetHeight() - tsz.y) / 2);
		}
		dc.SetPen(wxPen(wxColor(50, 75, 105), 1, wxSOLID));
		int line_start_x = rect.GetX() + (text_w > 0 ? text_w + 26 : 18);
		int line_end_x = rect.GetRight() - 4;
		if (line_end_x > line_start_x) {
			dc.DrawLine(line_start_x, line_y, line_end_x, line_y);
		}
		return;
	}

	int look_id = brush ? brush->getLookID() : 0;
	Sprite* spr = nullptr;
	if (look_id > 0 && g_items.typeExists(look_id)) {
		spr = g_gui.gfx.getSprite(g_items[look_id].clientID);
	} else {
		spr = g_gui.gfx.getSprite(look_id);
	}
	bool count100 = (look_id > 0 && g_items.typeExists(look_id) && g_items[look_id].stackable);
	if (spr) {
		spr->DrawTo(&dc, SPRITE_SIZE_32x32, rect.GetX(), rect.GetY(), rect.GetHeight(), rect.GetHeight(), count100);
	}
	if (IsSelected(n)) {
		dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
	} else {
		dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
	}
	if (brush) {
		dc.DrawText(wxstr(brush->getName()), rect.GetX() + 40, rect.GetY() + 6);
	}
}

wxCoord BrushListBox::OnMeasureItem(size_t n) const {
	if (n < visible_brushes.size() && visible_brushes[n] && visible_brushes[n]->isSeparator()) {
		return 22;
	}
	return 32;
}

void BrushListBox::Filter(const wxString& query) {
	current_query = query;
	UpdateVisibleList();
	if (visible_brushes.size() > 0) {
		SelectFirstBrush();
	}
}

void BrushListBox::OnKey(wxKeyEvent& event) {
	switch (event.GetKeyCode()) {
		case WXK_UP:
		case WXK_DOWN:
		case WXK_LEFT:
		case WXK_RIGHT:
			if (g_settings.getInteger(Config::LISTBOX_EATS_ALL_EVENTS)) {
				case WXK_PAGEUP:
				case WXK_PAGEDOWN:
				case WXK_HOME:
				case WXK_END:
					event.Skip(true);
			} else {
				[[fallthrough]];
				default:
					if (g_gui.GetCurrentTab() != nullptr) {
						g_gui.GetCurrentMapTab()->GetEventHandler()->AddPendingEvent(event);
					}
			}
	}
}
