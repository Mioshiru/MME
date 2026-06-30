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
#include <wx/settings.h>

// ============================================================================
// Brush Palette Panel
// A common class for terrain/doodad/item/raw palette

BEGIN_EVENT_TABLE(BrushPalettePanel, PalettePanel)
EVT_BUTTON(wxID_ADD, BrushPalettePanel::OnClickAddItemToTileset)
EVT_BUTTON(wxID_NEW, BrushPalettePanel::OnClickAddTileset)
EVT_CHOICEBOOK_PAGE_CHANGING(wxID_ANY, BrushPalettePanel::OnSwitchingPage)
EVT_CHOICEBOOK_PAGE_CHANGED(wxID_ANY, BrushPalettePanel::OnPageChanged)
EVT_CHOICE(PALETTE_TILESET_CHOICE, BrushPalettePanel::OnTilesetChoice)
EVT_TEXT(PALETTE_SEARCH_BOX, BrushPalettePanel::OnSearchTextChanged)
END_EVENT_TABLE()

BrushPalettePanel::BrushPalettePanel(wxWindow* parent, const TilesetContainer& tilesets, TilesetCategoryType category, wxWindowID id) :
	PalettePanel(parent, id),
	palette_type(category),
	choicebook(nullptr),
	size_panel(nullptr),
	tileset_choice(nullptr),
	search_box(nullptr),
	search_panel(nullptr) {
	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	// Create the tileset panel
	wxSizer* ts_sizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Tileset");
	tileset_choice = newd wxChoice(this, PALETTE_TILESET_CHOICE);
	search_box = newd wxTextCtrl(this, PALETTE_SEARCH_BOX, "", wxDefaultPosition, wxDefaultSize, 0);
	search_box->SetHint("Search brushes...");
	search_box->Bind(wxEVT_CHAR_HOOK, [](wxKeyEvent& event) {
		if (event.GetKeyCode() == WXK_ESCAPE) {
			event.Skip(); // Let escape bubble up
		} else {
			event.DoAllowNextEvent(); // Allow text input, but block accelerators
		}
	});

	ts_sizer->Add(tileset_choice, 0, wxEXPAND | wxBOTTOM, 5);
	ts_sizer->Add(search_box, 0, wxEXPAND | wxBOTTOM, 5);

	wxChoicebook* tmp_choicebook = newd wxChoicebook(this, wxID_ANY, wxDefaultPosition, wxSize(180, 250));
	ts_sizer->Add(tmp_choicebook, 1, wxEXPAND);
	topsizer->Add(ts_sizer, 1, wxEXPAND);

	if (g_settings.getBoolean(Config::SHOW_TILESET_EDITOR)) {
		wxSizer* tmpsizer = newd wxBoxSizer(wxHORIZONTAL);
		wxButton* buttonAddTileset = newd wxButton(this, wxID_NEW, "Add new Tileset");
		tmpsizer->Add(buttonAddTileset, wxSizerFlags(0).Center());

		wxButton* buttonAddItemToTileset = newd wxButton(this, wxID_ADD, "Add new Item");
		tmpsizer->Add(buttonAddItemToTileset, wxSizerFlags(0).Center());

		topsizer->Add(tmpsizer, 0, wxCENTER, 10);
	}

	search_panel = newd BrushPanel(this);
	ts_sizer->Add(search_panel, 1, wxEXPAND);
	search_panel->Hide();
	
	std::vector<Brush*> global_brushes;

	for (TilesetContainer::const_iterator iter = tilesets.begin(); iter != tilesets.end(); ++iter) {
		const TilesetCategory* tcg = iter->second->getCategory(category);
		if (tcg && tcg->size() > 0) {
			BrushPanel* panel = newd BrushPanel(tmp_choicebook);
			panel->AssignTileset(tcg);
			tmp_choicebook->AddPage(panel, wxstr(iter->second->name));
			tileset_choice->Append(wxstr(iter->second->name));
			if (tcg->size() > 0) {
				for(Brush* b : tcg->brushlist) {
					global_brushes.push_back(b);
				}
			}
		}
	}
	search_panel->AssignBrushes(global_brushes);

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

	SetSizerAndFit(topsizer);

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
	if (search_panel) search_panel->InvalidateContents();
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
	if (search_panel) search_panel->LoadContents();
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
	if (search_panel) search_panel->SetListType(ltype);
}

void BrushPalettePanel::SetListType(wxString ltype) {
	if (!choicebook) {
		return;
	}
	for (size_t iz = 0; iz < choicebook->GetPageCount(); ++iz) {
		BrushPanel* panel = dynamic_cast<BrushPanel*>(choicebook->GetPage(iz));
		panel->SetListType(ltype);
	}
	if (search_panel) search_panel->SetListType(ltype);
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
			choicebook->ChangeSelection(iz);
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
		choicebook->SetSelection(sel);
		if (search_box) {
			search_box->SetValue("");
		}
	}
}

void BrushPalettePanel::OnSearchTextChanged(wxCommandEvent& event) {
	if (!choicebook) return;
	wxString query = event.GetString();
	if (query.IsEmpty()) {
		search_panel->Hide();
		choicebook->Show();
		tileset_choice->Show();
		Layout();
	} else {
		choicebook->Hide();
		tileset_choice->Hide();
		search_panel->Show();
		search_panel->LoadContents(); // Ensure it's loaded when shown
		search_panel->Filter(query);
		Layout();
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
	SetSizerAndFit(sizer);
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
			// Sort brushes by name for consistent display
			std::sort(all_brushes.begin(), all_brushes.end(), [](const Brush* a, const Brush* b) {
				return a->getName() < b->getName();
			});
		}
	}
}

void BrushPanel::AssignBrushes(const std::vector<Brush*>& brushes) {
	InvalidateContents();
	tileset = nullptr;
	all_brushes = brushes;
	// Sort brushes by name for consistent display
	std::sort(all_brushes.begin(), all_brushes.end(), [](const Brush* a, const Brush* b) {
		return a->getName() < b->getName();
	});
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
	sizer->Clear(true);
	loaded = false;
	brushbox = nullptr;
}

void BrushPanel::LoadContents() {
	if (loaded) {
		return;
	}
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
	ASSERT(brushbox != nullptr);
	sizer->Add(brushbox->GetSelfWindow(), 1, wxEXPAND);
	Fit();
	if (!search_query.IsEmpty()) {
		brushbox->Filter(search_query);
	} else {
		brushbox->SelectFirstBrush();
	}
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
// Listbox style
EVT_TOGGLEBUTTON(wxID_ANY, BrushIconBox::OnClickBrushButton)
END_EVENT_TABLE()

BrushIconBox::BrushIconBox(wxWindow* parent, const std::vector<Brush*>& brushes, RenderSize rsz) :
	wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL),
	BrushBoxInterface(brushes),
	icon_size(rsz) {
	int width;
	if (icon_size == RENDER_SIZE_32x32) {
		width = max(g_settings.getInteger(Config::PALETTE_COL_COUNT) / 2 + 1, 1);
	} else {
		width = max(g_settings.getInteger(Config::PALETTE_COL_COUNT) + 1, 1);
	}

	// Create buttons
	wxSizer* stacksizer = newd wxBoxSizer(wxVERTICAL);
	wxSizer* rowsizer = nullptr;
	int item_counter = 0;
	for (BrushVector::const_iterator iter = all_brushes.begin(); iter != all_brushes.end(); ++iter) {
		ASSERT(*iter);
		++item_counter;

		if (!rowsizer) {
			rowsizer = newd wxBoxSizer(wxHORIZONTAL);
		}

		BrushButton* bb = newd BrushButton(this, *iter, rsz);
		rowsizer->Add(bb);
		brush_buttons.push_back(bb);

		if (item_counter % width == 0) { // newd row
			stacksizer->Add(rowsizer);
			rowsizer = nullptr;
		}
	}
	if (rowsizer) {
		stacksizer->Add(rowsizer);
	}

	SetScrollbars(20, 20, 8, item_counter / width, 0, 0);
	SetSizer(stacksizer);
}

BrushIconBox::~BrushIconBox() {
	////
}

void BrushIconBox::SelectFirstBrush() {
	DeselectAll();
	for (BrushButton* bb : brush_buttons) {
		if (bb->IsShown()) {
			bb->SetValue(true);
			EnsureVisible(bb);
			break;
		}
	}
}

Brush* BrushIconBox::GetSelectedBrush() const {
	for (std::vector<BrushButton*>::const_iterator it = brush_buttons.begin(); it != brush_buttons.end(); ++it) {
		if ((*it)->GetValue() && (*it)->IsShown()) {
			return (*it)->brush;
		}
	}
	return nullptr;
}

bool BrushIconBox::SelectBrush(const Brush* whatbrush) {
	DeselectAll();
	for (std::vector<BrushButton*>::iterator it = brush_buttons.begin(); it != brush_buttons.end(); ++it) {
		if ((*it)->brush == whatbrush) {
			(*it)->SetValue(true);
			EnsureVisible(*it);
			return true;
		}
	}
	return false;
}

void BrushIconBox::DeselectAll() {
	for (std::vector<BrushButton*>::iterator it = brush_buttons.begin(); it != brush_buttons.end(); ++it) {
		(*it)->SetValue(false);
	}
}

void BrushIconBox::EnsureVisible(BrushButton* btn) {
	int windowSizeX, windowSizeY;
	GetVirtualSize(&windowSizeX, &windowSizeY);

	int scrollUnitX;
	int scrollUnitY;
	GetScrollPixelsPerUnit(&scrollUnitX, &scrollUnitY);

	wxRect rect = btn->GetRect();
	int y;
	CalcUnscrolledPosition(0, rect.y, nullptr, &y);

	int maxScrollPos = windowSizeY / scrollUnitY;
	int scrollPosY = std::min(maxScrollPos, (y / scrollUnitY));

	int startScrollPosY;
	GetViewStart(nullptr, &startScrollPosY);

	int clientSizeX, clientSizeY;
	GetClientSize(&clientSizeX, &clientSizeY);
	int endScrollPosY = startScrollPosY + clientSizeY / scrollUnitY;

	if (scrollPosY < startScrollPosY || scrollPosY > endScrollPosY) {
		// only scroll if the button isnt visible
		Scroll(-1, scrollPosY);
	}
}

void BrushIconBox::EnsureVisible(size_t n) {
	if (n < brush_buttons.size()) {
		EnsureVisible(brush_buttons[n]);
	}
}

void BrushIconBox::Filter(const wxString& query) {
	wxSizer* stacksizer = GetSizer();
	if (stacksizer) {
		stacksizer->Clear(false);
	} else {
		stacksizer = newd wxBoxSizer(wxVERTICAL);
		SetSizer(stacksizer);
	}

	int width;
	if (icon_size == RENDER_SIZE_32x32) {
		width = max(g_settings.getInteger(Config::PALETTE_COL_COUNT) / 2 + 1, 1);
	} else {
		width = max(g_settings.getInteger(Config::PALETTE_COL_COUNT) + 1, 1);
	}

	wxString lower_query = query.Lower();
	visible_brushes.clear();

	std::vector<BrushButton*> visible_buttons;
	for (BrushButton* bb : brush_buttons) {
		wxString brush_name = wxstr(bb->brush->getName());
		if (lower_query.IsEmpty() || brush_name.Lower().Contains(lower_query)) {
			bb->Show(true);
			visible_buttons.push_back(bb);
			visible_brushes.push_back(bb->brush);
		} else {
			bb->Show(false);
			bb->SetValue(false);
		}
	}

	wxSizer* rowsizer = nullptr;
	int item_counter = 0;
	for (BrushButton* bb : visible_buttons) {
		++item_counter;
		if (!rowsizer) {
			rowsizer = newd wxBoxSizer(wxHORIZONTAL);
		}
		rowsizer->Add(bb);

		if (item_counter % width == 0) {
			stacksizer->Add(rowsizer);
			rowsizer = nullptr;
		}
	}
	if (rowsizer) {
		stacksizer->Add(rowsizer);
	}

	stacksizer->Layout();
	SetScrollbars(20, 20, 8, std::max(1, item_counter / width), 0, 0);
	FitInside();

	if (visible_buttons.size() > 0) {
		Brush* current_sel = GetSelectedBrush();
		if (!current_sel) {
			SelectFirstBrush();
		}
	}
}

void BrushIconBox::OnClickBrushButton(wxCommandEvent& event) {
	wxObject* obj = event.GetEventObject();
	BrushButton* btn = dynamic_cast<BrushButton*>(obj);
	if (btn) {
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
		
		g_gui.SelectBrush(btn->brush, catType);
	}
}

// ============================================================================
// BrushListBox

BEGIN_EVENT_TABLE(BrushListBox, wxVListBox)
EVT_KEY_DOWN(BrushListBox::OnKey)
END_EVENT_TABLE()

BrushListBox::BrushListBox(wxWindow* parent, const std::vector<Brush*>& brushes) :
	wxVListBox(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLB_SINGLE),
	BrushBoxInterface(brushes) {
	SetItemCount(visible_brushes.size());
}

BrushListBox::~BrushListBox() {
	////
}

void BrushListBox::SelectFirstBrush() {
	if (visible_brushes.size() > 0) {
		SetSelection(0);
	} else {
		SetSelection(wxNOT_FOUND);
	}
	wxWindow::ScrollLines(-1);
}

Brush* BrushListBox::GetSelectedBrush() const {
	int n = GetSelection();
	if (n != wxNOT_FOUND && n < (int)visible_brushes.size()) {
		return visible_brushes[n];
	} else if (visible_brushes.size() > 0) {
		return visible_brushes[0];
	}
	return nullptr;
}

bool BrushListBox::SelectBrush(const Brush* whatbrush) {
	if (!whatbrush) {
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

void BrushListBox::OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const {
	ASSERT(n < visible_brushes.size());
	int look_id = visible_brushes[n]->getLookID();
	Sprite* spr = nullptr;
	if (look_id > 0 && g_items.typeExists(look_id)) {
		spr = g_gui.gfx.getSprite(g_items[look_id].clientID);
	} else {
		spr = g_gui.gfx.getSprite(look_id);
	}
	if (spr) {
		spr->DrawTo(&dc, SPRITE_SIZE_32x32, rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());
	}
	if (IsSelected(n)) {
		dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
	} else {
		dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
	}
	dc.DrawText(wxstr(visible_brushes[n]->getName()), rect.GetX() + 40, rect.GetY() + 6);
}

wxCoord BrushListBox::OnMeasureItem(size_t n) const {
	return 32;
}

void BrushListBox::Filter(const wxString& query) {
	visible_brushes.clear();
	wxString lower_query = query.Lower();
	for (Brush* brush : all_brushes) {
		if (lower_query.IsEmpty() || wxstr(brush->getName()).Lower().Contains(lower_query)) {
			visible_brushes.push_back(brush);
		}
	}
	SetItemCount(visible_brushes.size());
	Refresh();
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
