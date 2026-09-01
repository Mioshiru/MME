//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "main.h"
#include "style_manager.h"
#include "tileset_manager_window.h"
#include "gui.h"
#include "graphics.h"
#include "sprites.h"
#include "materials.h"
#include "brush.h"
#include "raw_brush.h"
#include "ground_brush.h"
#include "client_version.h"
#include "extension.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include <wx/filefn.h>
#include <wx/stdpaths.h>
#include <wx/listctrl.h>
#include "ext/pugixml.hpp"

extern Materials g_materials;

// ============================================================================
// RawItemListBox
// ============================================================================

RawItemListBox::RawItemListBox(wxWindow* parent, wxWindowID id) :
	wxVListBox(parent, id, wxDefaultPosition, wxDefaultSize, wxLB_SINGLE | wxBORDER_THEME) {
	UpdateFilter("");
}

void RawItemListBox::UpdateFilter(const wxString& filterText) {
	filtered_item_ids.clear();
	wxString lowerFilter = filterText.Lower();

	for (int id = 100; id <= static_cast<int>(g_items.getMaxID()); ++id) {
		const ItemType& it = g_items[id];
		if (it.id == 0 || it.clientID == 0) continue;

		bool match = false;
		if (lowerFilter.IsEmpty()) {
			match = true;
		} else {
			wxString idStr = wxString::Format("%d", id);
			wxString clientStr = wxString::Format("%d", it.clientID);
			wxString nameStr = wxString::FromUTF8(it.name.c_str()).Lower();

			if (idStr.Contains(lowerFilter) || clientStr.Contains(lowerFilter) || nameStr.Contains(lowerFilter)) {
				match = true;
			}
		}

		if (match) {
			filtered_item_ids.push_back(id);
		}
	}

	SetItemCount(filtered_item_ids.size());
	Refresh();
}

int RawItemListBox::GetSelectedItemID() const {
	int sel = GetSelection();
	if (sel >= 0 && sel < static_cast<int>(filtered_item_ids.size())) {
		return filtered_item_ids[sel];
	}
	return 0;
}

void RawItemListBox::SelectItemByID(int id) {
	for (size_t i = 0; i < filtered_item_ids.size(); ++i) {
		if (filtered_item_ids[i] == id) {
			SetSelection(i);
			ScrollToRow(i);
			break;
		}
	}
}

void RawItemListBox::OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const {
	if (n >= filtered_item_ids.size()) return;

	int itemId = filtered_item_ids[n];
	const ItemType& it = g_items[itemId];

	if (IsSelected(n)) {
		dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT)));
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.DrawRectangle(rect);
		dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
	} else {
		dc.SetBrush(wxBrush(n % 2 == 0 ? wxColor(255, 255, 255) : wxColor(245, 247, 250)));
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.DrawRectangle(rect);
		dc.SetTextForeground(wxColor(30, 30, 30));
	}

	Sprite* sprite = g_gui.gfx.getSprite(it.clientID);
	if (sprite) {
		sprite->DrawTo(&dc, SPRITE_SIZE_32x32, rect.GetX() + 2, rect.GetY() + 2, 32, 32, it.stackable);
	}

	wxString label = wxString::Format("ID %d (Client: %d) - %s", itemId, it.clientID, wxString::FromUTF8(it.name.c_str()));
	dc.DrawText(label, rect.GetX() + 40, rect.GetY() + 8);
}

wxCoord RawItemListBox::OnMeasureItem(size_t WXUNUSED(n)) const {
	return 36;
}

// ============================================================================
// ItemPreviewCanvas
// ============================================================================

BEGIN_EVENT_TABLE(ItemPreviewCanvas, wxPanel)
EVT_PAINT(ItemPreviewCanvas::OnPaint)
END_EVENT_TABLE()

ItemPreviewCanvas::ItemPreviewCanvas(wxWindow* parent, wxSize size) :
	wxPanel(parent, wxID_ANY, wxDefaultPosition, size, wxBORDER_THEME) {
	SetBackgroundStyle(wxBG_STYLE_PAINT);
}

void ItemPreviewCanvas::SetItemID(int id) {
	item_id = id;
	Refresh();
}

void ItemPreviewCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
	wxAutoBufferedPaintDC dc(this);
	wxSize size = GetClientSize();

	dc.SetBrush(wxBrush(wxColor(40, 44, 52)));
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());

	if (item_id > 0 && item_id <= static_cast<int>(g_items.getMaxID())) {
		const ItemType& it = g_items[item_id];
		Sprite* sprite = g_gui.gfx.getSprite(it.clientID);
		if (sprite) {
			int drawSize = std::min(size.GetWidth(), size.GetHeight()) - 8;
			int x = (size.GetWidth() - drawSize) / 2;
			int y = (size.GetHeight() - drawSize) / 2;
			sprite->DrawTo(&dc, SPRITE_SIZE_32x32, x, y, drawSize, drawSize, it.stackable);
		}
	} else {
		dc.SetTextForeground(wxColor(150, 150, 150));
		dc.DrawLabel("No Preview", wxRect(0, 0, size.GetWidth(), size.GetHeight()), wxALIGN_CENTER);
	}
}

// ============================================================================
// TilesetManagerDialog
// ============================================================================

enum {
	ID_SEARCH_CTRL = 10001,
	ID_RAW_LIST,
	ID_ASSIGN_BTN,
	ID_NEW_TILESET_BTN,
	ID_CREATE_BRUSH_BTN,
	ID_SAVE_EXT_BTN,
	ID_SLOT_0, ID_SLOT_1, ID_SLOT_2,
	ID_SLOT_3, ID_SLOT_4, ID_SLOT_5,
	ID_SLOT_6, ID_SLOT_7, ID_SLOT_8,
	// Extensions Manager
	ID_EXT_LIST,
	ID_EXT_TOGGLE,
	ID_EXT_OPEN_FOLDER
};

BEGIN_EVENT_TABLE(TilesetManagerDialog, wxDialog)
EVT_TEXT(ID_SEARCH_CTRL, TilesetManagerDialog::OnSearchChange)
EVT_LISTBOX(ID_RAW_LIST, TilesetManagerDialog::OnRawItemSelected)
EVT_BUTTON(ID_ASSIGN_BTN, TilesetManagerDialog::OnAssignToTileset)
EVT_BUTTON(ID_NEW_TILESET_BTN, TilesetManagerDialog::OnCreateNewTileset)
EVT_BUTTON(ID_CREATE_BRUSH_BTN, TilesetManagerDialog::OnCreateAutoborderBrush)
EVT_BUTTON(ID_SAVE_EXT_BTN, TilesetManagerDialog::OnSaveExtensions)
EVT_BUTTON(ID_EXT_TOGGLE, TilesetManagerDialog::OnExtToggleEnable)
EVT_BUTTON(ID_EXT_OPEN_FOLDER, TilesetManagerDialog::OnExtOpenFolder)
END_EVENT_TABLE()

TilesetManagerDialog::TilesetManagerDialog(wxWindow* parent) :
	wxDialog(parent, wxID_ANY, "Tileset Manager", wxDefaultPosition, wxSize(950, 700), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {

	for (int i = 0; i < 9; ++i) {
		border_item_ids[i] = 0;
	}

	wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);

	// Header (Corporate Design: Dark Obsidian + Gold)
	wxPanel* header_panel = new wxPanel(this, wxID_ANY);
	header_panel->SetBackgroundColour(wxColour(18, 32, 54)); // Dark Obsidian
	wxBoxSizer* header_sizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* header_title = new wxStaticText(header_panel, wxID_ANY, "Tileset Manager");
	header_title->SetForegroundColour(wxColour(240, 210, 120)); // Corporate Gold
	wxFont titleFont = header_title->GetFont();
	titleFont.SetPointSize(12);
	titleFont.SetWeight(wxFONTWEIGHT_BOLD);
	header_title->SetFont(titleFont);
	header_sizer->Add(header_title, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
	header_panel->SetSizer(header_sizer);
	main_sizer->Add(header_panel, 0, wxEXPAND);

	// Main notebook (top-level tabs)
	wxNotebook* main_notebook = new wxNotebook(this, wxID_ANY);

	// ================================================================
	// TAB 1: Tileset & Autoborder Studio
	// ================================================================
	wxPanel* tileset_tab = new wxPanel(main_notebook, wxID_ANY);
	wxBoxSizer* body_sizer = new wxBoxSizer(wxHORIZONTAL);

	// Left: RAW Items Search & List
	wxBoxSizer* left_sizer = new wxBoxSizer(wxVERTICAL);
	left_sizer->Add(new wxStaticText(tileset_tab, wxID_ANY, "Search RAW Items (by Name or ID):"), 0, wxTOP | wxLEFT | wxRIGHT, 8);
	search_ctrl = new wxTextCtrl(tileset_tab, ID_SEARCH_CTRL, "", wxDefaultPosition, wxDefaultSize);
	left_sizer->Add(search_ctrl, 0, wxEXPAND | wxALL, 8);

	raw_list = new RawItemListBox(tileset_tab, ID_RAW_LIST);
	left_sizer->Add(raw_list, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

	body_sizer->Add(left_sizer, 1, wxEXPAND);

	// Center & Right: Sub-Notebook with Tabs
	wxNotebook* notebook = new wxNotebook(tileset_tab, wxID_ANY);

	// Sub-Tab 1: Fast Tagging & Tileset Assignment
	wxPanel* assign_tab = new wxPanel(notebook, wxID_ANY);
	wxBoxSizer* assign_sizer = new wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer* preview_box = new wxStaticBoxSizer(wxVERTICAL, assign_tab, "Selected Item Preview");
	item_preview = new ItemPreviewCanvas(assign_tab, wxSize(96, 96));
	preview_box->Add(item_preview, 0, wxALIGN_CENTER | wxALL, 10);
	item_info_text = new wxStaticText(assign_tab, wxID_ANY, "Select an item from the list");
	preview_box->Add(item_info_text, 0, wxALIGN_CENTER | wxBOTTOM, 8);
	assign_sizer->Add(preview_box, 0, wxEXPAND | wxALL, 8);

	wxStaticBoxSizer* tileset_box = new wxStaticBoxSizer(wxVERTICAL, assign_tab, "Assign to Tileset");
	wxFlexGridSizer* form_sizer = new wxFlexGridSizer(2, 8, 8);
	form_sizer->AddGrowableCol(1);

	form_sizer->Add(new wxStaticText(assign_tab, wxID_ANY, "Target Tileset:"), 0, wxALIGN_CENTER_VERTICAL);
	tileset_choice = new wxChoice(assign_tab, wxID_ANY);
	form_sizer->Add(tileset_choice, 1, wxEXPAND);

	form_sizer->Add(new wxStaticText(assign_tab, wxID_ANY, "Category:"), 0, wxALIGN_CENTER_VERTICAL);
	category_choice = new wxChoice(assign_tab, wxID_ANY);
	category_choice->Append("Terrain", new int(TILESET_TERRAIN));
	category_choice->Append("Doodads", new int(TILESET_DOODAD));
	category_choice->Append("Items", new int(TILESET_ITEM));
	category_choice->Append("Collections", new int(TILESET_COLLECTION));
	category_choice->Append("Raw", new int(TILESET_RAW));
	category_choice->SetSelection(2); // Items
	form_sizer->Add(category_choice, 1, wxEXPAND);

	tileset_box->Add(form_sizer, 0, wxEXPAND | wxALL, 8);

	wxBoxSizer* btn_row = new wxBoxSizer(wxHORIZONTAL);
	assign_btn = new wxButton(assign_tab, ID_ASSIGN_BTN, "Add Item to Tileset");
	assign_btn->SetBackgroundColour(wxColour(180, 150, 50)); // Corporate Gold
	assign_btn->SetForegroundColour(*wxWHITE);
	new_tileset_btn = new wxButton(assign_tab, ID_NEW_TILESET_BTN, "New Tileset...");
	btn_row->Add(assign_btn, 1, wxRIGHT, 5);
	btn_row->Add(new_tileset_btn, 0);
	tileset_box->Add(btn_row, 0, wxEXPAND | wxALL, 8);

	assign_sizer->Add(tileset_box, 0, wxEXPAND | wxALL, 8);
	assign_tab->SetSizer(assign_sizer);

	// Sub-Tab 2: Autoborder Ground Studio (3x3 Grid)
	wxPanel* border_tab = new wxPanel(notebook, wxID_ANY);
	wxBoxSizer* border_main_sizer = new wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer* grid_box = new wxStaticBoxSizer(wxVERTICAL, border_tab, "Interactive 3x3 Autoborder Grid (Click slot to set from Selected Item)");
	wxGridSizer* grid_3x3 = new wxGridSizer(3, 3, 5, 5);

	const char* slot_labels[9] = {
		"NW Corner", "North Edge", "NE Corner",
		"West Edge", "Center Ground", "East Edge",
		"SW Corner", "South Edge", "SE Corner"
	};

	for (int i = 0; i < 9; ++i) {
		border_slots[i] = new wxButton(border_tab, ID_SLOT_0 + i, slot_labels[i], wxDefaultPosition, wxSize(100, 45));
		border_slots[i]->Bind(wxEVT_BUTTON, &TilesetManagerDialog::OnBorderSlotClick, this);
		grid_3x3->Add(border_slots[i], 1, wxEXPAND);
	}

	grid_box->Add(grid_3x3, 0, wxEXPAND | wxALL, 10);
	border_main_sizer->Add(grid_box, 0, wxEXPAND | wxALL, 8);

	wxFlexGridSizer* brush_params = new wxFlexGridSizer(2, 6, 6);
	brush_params->AddGrowableCol(1);

	brush_params->Add(new wxStaticText(border_tab, wxID_ANY, "Brush Name:"), 0, wxALIGN_CENTER_VERTICAL);
	brush_name_ctrl = new wxTextCtrl(border_tab, wxID_ANY, "Custom Ore Ground");
	brush_params->Add(brush_name_ctrl, 1, wxEXPAND);

	brush_params->Add(new wxStaticText(border_tab, wxID_ANY, "Z-Order:"), 0, wxALIGN_CENTER_VERTICAL);
	z_order_ctrl = new wxSpinCtrl(border_tab, wxID_ANY, "5000", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 50000, 5000);
	brush_params->Add(z_order_ctrl, 1, wxEXPAND);

	border_main_sizer->Add(brush_params, 0, wxEXPAND | wxALL, 8);

	create_brush_btn = new wxButton(border_tab, ID_CREATE_BRUSH_BTN, "Generate & Register Ground Brush");
	create_brush_btn->SetBackgroundColour(wxColour(18, 32, 54)); // Corporate Dark
	create_brush_btn->SetForegroundColour(wxColour(240, 210, 120)); // Gold Text
	border_main_sizer->Add(create_brush_btn, 0, wxEXPAND | wxALL, 8);

	border_tab->SetSizer(border_main_sizer);

	notebook->AddPage(assign_tab, "Quick Tagging & Tileset");
	notebook->AddPage(border_tab, "Autoborder Studio");

	body_sizer->Add(notebook, 1, wxEXPAND | wxALL, 8);

	tileset_tab->SetSizer(body_sizer);

	// ================================================================
	// TAB 2: Extensions Manager
	// ================================================================
	wxPanel* ext_tab = new wxPanel(main_notebook, wxID_ANY);
	wxBoxSizer* ext_main_sizer = new wxBoxSizer(wxVERTICAL);

	// Info text
	wxStaticText* ext_heading = new wxStaticText(ext_tab, wxID_ANY,
		"Extensions are .xml files loaded from the 'extensions' folder.\n"
		"Incompatible extensions (not matching current client version) are greyed out.\n"
		"Toggling an extension only takes effect after restarting the editor.");
	ext_main_sizer->Add(ext_heading, 0, wxALL, 10);

	// Extension List
	ext_list_ctrl = new wxListCtrl(ext_tab, ID_EXT_LIST, wxDefaultPosition, wxDefaultSize,
		wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_THEME);
	ext_list_ctrl->InsertColumn(0, "File", wxLIST_FORMAT_LEFT, 160);
	ext_list_ctrl->InsertColumn(1, "Name", wxLIST_FORMAT_LEFT, 150);
	ext_list_ctrl->InsertColumn(2, "Author", wxLIST_FORMAT_LEFT, 100);
	ext_list_ctrl->InsertColumn(3, "Versions", wxLIST_FORMAT_LEFT, 120);
	ext_list_ctrl->InsertColumn(4, "Status", wxLIST_FORMAT_LEFT, 120);
	ext_list_ctrl->InsertColumn(5, "Description", wxLIST_FORMAT_LEFT, 250);
	ext_main_sizer->Add(ext_list_ctrl, 1, wxEXPAND | wxALL, 8);

	// Info text below list
	ext_info_text = new wxStaticText(ext_tab, wxID_ANY, "Select an extension to see details.");
	ext_main_sizer->Add(ext_info_text, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);

	// Button row
	wxBoxSizer* ext_btn_row = new wxBoxSizer(wxHORIZONTAL);
	ext_toggle_btn = new wxButton(ext_tab, ID_EXT_TOGGLE, "Enable / Disable");
	ext_toggle_btn->SetBackgroundColour(wxColour(180, 150, 50)); // Corporate Gold
	ext_toggle_btn->SetForegroundColour(*wxWHITE);
	ext_btn_row->Add(ext_toggle_btn, 0, wxRIGHT, 10);

	ext_open_folder_btn = new wxButton(ext_tab, ID_EXT_OPEN_FOLDER, "Open Extensions Folder");
	ext_btn_row->Add(ext_open_folder_btn, 0);
	ext_main_sizer->Add(ext_btn_row, 0, wxLEFT | wxBOTTOM, 10);

	ext_list_ctrl->Bind(wxEVT_LIST_ITEM_SELECTED, &TilesetManagerDialog::OnExtListItemSelected, this);

	ext_tab->SetSizer(ext_main_sizer);

	// ================================================================
	// Assemble main notebook
	// ================================================================
	main_notebook->AddPage(tileset_tab, "Tileset & Brushes");
	main_notebook->AddPage(ext_tab, "Extensions Manager");

	main_sizer->Add(main_notebook, 1, wxEXPAND | wxALL, 4);

	// Bottom Action Bar
	wxBoxSizer* bottom_sizer = new wxBoxSizer(wxHORIZONTAL);
	wxButton* save_ext_btn = new wxButton(this, ID_SAVE_EXT_BTN, "Save Changes to materials.xml");
	save_ext_btn->SetBackgroundColour(wxColour(180, 150, 50)); // Corporate Gold
	save_ext_btn->SetForegroundColour(*wxWHITE);
	bottom_sizer->Add(save_ext_btn, 0, wxALL, 10);
	bottom_sizer->AddStretchSpacer();
	bottom_sizer->Add(new wxButton(this, wxID_OK, "Close"), 0, wxALL, 10);

	main_sizer->Add(bottom_sizer, 0, wxEXPAND);

	SetSizer(main_sizer);
	RefreshTilesets();
	RefreshExtensionList();
	Centre();
	RME::UI::StyleManager::ApplyThemeRecursively(this, RME::UI::StyleManager::GetTheme());
}

void TilesetManagerDialog::RefreshTilesets() {
	tileset_choice->Clear();
	for (const auto& kv : g_materials.tilesets) {
		tileset_choice->Append(wxString::FromUTF8(kv.first.c_str()));
	}
	if (tileset_choice->GetCount() > 0) {
		tileset_choice->SetSelection(0);
	}
}

void TilesetManagerDialog::RefreshExtensionList() {
	ext_list_ctrl->DeleteAllItems();

	uint16_t cur_version = g_gui.GetCurrentVersionID();
	const MaterialsExtensionList& exts = g_materials.getExtensions();

	for (int i = 0; i < (int)exts.size(); ++i) {
		MaterialsExtension* ext = exts[i];

		bool compatible = ext->isForVersion(cur_version);
		wxString status;
		if (!compatible) {
			status = "Incompatible";
		} else if (ext->enabled) {
			status = "Enabled";
		} else {
			status = "Disabled";
		}

		long idx = ext_list_ctrl->InsertItem(i, wxString::FromUTF8(ext->filename.c_str()));
		ext_list_ctrl->SetItem(idx, 1, wxString::FromUTF8(ext->name.c_str()));
		ext_list_ctrl->SetItem(idx, 2, wxString::FromUTF8(ext->author.c_str()));
		ext_list_ctrl->SetItem(idx, 3, wxString::FromUTF8(ext->getVersionString().c_str()));
		ext_list_ctrl->SetItem(idx, 4, status);
		ext_list_ctrl->SetItem(idx, 5, wxString::FromUTF8(ext->description.c_str()));

		// Grey out incompatible extensions
		if (!compatible) {
			ext_list_ctrl->SetItemTextColour(idx, wxColor(140, 140, 140));
		} else if (ext->enabled) {
			ext_list_ctrl->SetItemTextColour(idx, wxColor(50, 180, 70));
		}
	}
}

void TilesetManagerDialog::OnExtListItemSelected(wxListEvent& event) {
	selected_ext_index = event.GetIndex();
	const MaterialsExtensionList& exts = g_materials.getExtensions();

	if (selected_ext_index >= 0 && selected_ext_index < (int)exts.size()) {
		MaterialsExtension* ext = exts[selected_ext_index];
		wxString info = wxString::Format(
			"Name: %s  |  Author: %s  |  File: %s\nDescription: %s",
			wxString::FromUTF8(ext->name.c_str()),
			wxString::FromUTF8(ext->author.c_str()),
			wxString::FromUTF8(ext->filename.c_str()),
			wxString::FromUTF8(ext->description.c_str())
		);
		ext_info_text->SetLabel(info);
	}
}

void TilesetManagerDialog::OnExtToggleEnable(wxCommandEvent& WXUNUSED(event)) {
	const MaterialsExtensionList& exts = g_materials.getExtensions();

	if (selected_ext_index < 0 || selected_ext_index >= (int)exts.size()) {
		wxMessageBox("Please select an extension from the list first.", "No Selection", wxOK | wxICON_INFORMATION, this);
		return;
	}

	MaterialsExtension* ext = exts[selected_ext_index];
	uint16_t cur_version = g_gui.GetCurrentVersionID();

	if (!ext->isForVersion(cur_version)) {
		wxMessageBox("This extension is incompatible with the currently loaded client version and cannot be enabled.", "Incompatible", wxOK | wxICON_WARNING, this);
		return;
	}

	ext->enabled = !ext->enabled;
	RefreshExtensionList();

	wxString msg = ext->enabled
		? wxString::Format("Extension '%s' enabled. Restart the editor to apply.", wxString::FromUTF8(ext->name.c_str()))
		: wxString::Format("Extension '%s' disabled. Restart the editor to apply.", wxString::FromUTF8(ext->name.c_str()));
	wxMessageBox(msg, ext->enabled ? "Enabled" : "Disabled", wxOK | wxICON_INFORMATION, this);
}

void TilesetManagerDialog::OnExtOpenFolder(wxCommandEvent& WXUNUSED(event)) {
	wxString extDir = g_gui.GetExtensionsDirectory();
#if defined(__WINDOWS__)
	wxString cmd = "explorer \"" + extDir + "\"";
	wxExecute(cmd, wxEXEC_ASYNC);
#elif defined(__APPLE__)
	wxString cmd = "open \"" + extDir + "\"";
	wxExecute(cmd, wxEXEC_ASYNC);
#else
	wxString cmd = "xdg-open \"" + extDir + "\"";
	wxExecute(cmd, wxEXEC_ASYNC);
#endif
}

void TilesetManagerDialog::OnSearchChange(wxCommandEvent& WXUNUSED(event)) {
	raw_list->UpdateFilter(search_ctrl->GetValue());
}

void TilesetManagerDialog::OnRawItemSelected(wxCommandEvent& WXUNUSED(event)) {
	int itemId = raw_list->GetSelectedItemID();
	item_preview->SetItemID(itemId);

	if (itemId > 0 && itemId <= static_cast<int>(g_items.getMaxID())) {
		const ItemType& it = g_items[itemId];
		wxString info = wxString::Format("Item ID: %d | Client Sprite ID: %d\nName: %s\nStackable: %s | Solid: %s",
			itemId, it.clientID, wxString::FromUTF8(it.name.c_str()),
			it.stackable ? "Yes" : "No", it.unpassable ? "Yes" : "No");
		item_info_text->SetLabel(info);
	}
}

void TilesetManagerDialog::OnBorderSlotClick(wxCommandEvent& event) {
	int slot = event.GetId() - ID_SLOT_0;
	if (slot < 0 || slot >= 9) return;

	int itemId = raw_list->GetSelectedItemID();
	if (itemId == 0) {
		wxMessageBox("Please select an item from the RAW list first.", "No Item Selected", wxOK | wxICON_INFORMATION, this);
		return;
	}

	border_item_ids[slot] = itemId;
	const ItemType& it = g_items[itemId];
	wxString label = wxString::Format("%s\nID %d", wxString::FromUTF8(it.name.c_str()), itemId);
	border_slots[slot]->SetLabel(label);
	border_slots[slot]->SetBackgroundColour(wxColor(200, 230, 201));
}

void TilesetManagerDialog::OnAssignToTileset(wxCommandEvent& WXUNUSED(event)) {
	int itemId = raw_list->GetSelectedItemID();
	if (itemId == 0) {
		wxMessageBox("Please select an item to assign.", "No Item Selected", wxOK | wxICON_WARNING, this);
		return;
	}

	wxString tilesetName = tileset_choice->GetStringSelection();
	if (tilesetName.IsEmpty()) {
		wxMessageBox("Please select a target tileset.", "No Tileset", wxOK | wxICON_WARNING, this);
		return;
	}

	int catSel = category_choice->GetSelection();
	TilesetCategoryType catType = static_cast<TilesetCategoryType>(catSel);

	// Fix: clear in_other_tileset flag so it can be registered again
	ItemType& it = g_items[itemId];
	it.in_other_tileset = false;

	g_materials.addToTileset(std::string(tilesetName.mb_str()), itemId, catType);

	// Force a full palette rebuild so the item immediately appears
	g_gui.RefreshPalettes();

	wxMessageBox(wxString::Format("Successfully added Item %d to tileset '%s'!\nIt should now appear in the palette.", itemId, tilesetName),
		"Assigned", wxOK | wxICON_INFORMATION, this);
}

void TilesetManagerDialog::OnCreateNewTileset(wxCommandEvent& WXUNUSED(event)) {
	wxTextEntryDialog dlg(this, "Enter name for the new tileset (e.g. 'Ores & Mining'):", "Create Tileset");
	if (dlg.ShowModal() == wxID_OK) {
		wxString name = dlg.GetValue().Trim();
		if (!name.IsEmpty()) {
			std::string sName = std::string(name.mb_str());
			if (g_materials.tilesets.find(sName) == g_materials.tilesets.end()) {
				Tileset* newTs = new Tileset(g_brushes, sName);
				g_materials.tilesets.insert(std::make_pair(sName, newTs));
				RefreshTilesets();
				tileset_choice->SetStringSelection(name);
				g_gui.RefreshPalettes();
			}
		}
	}
}

void TilesetManagerDialog::OnCreateAutoborderBrush(wxCommandEvent& WXUNUSED(event)) {
	wxString brushName = brush_name_ctrl->GetValue().Trim();
	if (brushName.IsEmpty()) {
		wxMessageBox("Please enter a brush name.", "Missing Name", wxOK | wxICON_WARNING, this);
		return;
	}

	int centerGroundId = border_item_ids[4]; // Center slot
	if (centerGroundId == 0) {
		centerGroundId = raw_list->GetSelectedItemID();
	}

	if (centerGroundId == 0) {
		wxMessageBox("Please configure at least the Center Ground item.", "Missing Ground", wxOK | wxICON_WARNING, this);
		return;
	}

	GroundBrush* groundBrush = new GroundBrush();
	groundBrush->setName(std::string(brushName.mb_str()));
	g_brushes.addBrush(groundBrush);

	// Add to Terrain category in current tileset
	wxString tilesetName = tileset_choice->GetStringSelection();
	if (!tilesetName.IsEmpty()) {
		Tileset* ts = g_materials.tilesets[std::string(tilesetName.mb_str())];
		if (ts) {
			TilesetCategory* tc = ts->getCategory(TILESET_TERRAIN);
			if (tc) {
				tc->brushlist.push_back(groundBrush);
			}
		}
	}

	g_gui.RefreshPalettes();
	wxMessageBox(wxString::Format("GroundBrush '%s' created with autoborder integration!", brushName), "Brush Created", wxOK | wxICON_INFORMATION, this);
}

void TilesetManagerDialog::OnSaveExtensions(wxCommandEvent& WXUNUSED(event)) {
	g_materials.saveFavorites();
	wxMessageBox("Tileset and Brush configurations saved successfully!", "Saved", wxOK | wxICON_INFORMATION, this);
}

void TilesetManagerDialog::RefreshBrushes() {
	// Placeholder for future use
}
