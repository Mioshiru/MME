//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//////////////////////////////////////////////////////////////////////

#ifndef RME_TILESET_MANAGER_WINDOW_H_
#define RME_TILESET_MANAGER_WINDOW_H_

#include "main.h"
#include <wx/dialog.h>
#include <wx/vscroll.h>
#include <wx/notebook.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/statbmp.h>
#include <wx/listctrl.h>
#include "item.h"
#include "items.h"
#include "materials.h"
#include "tileset.h"
#include "ground_brush.h"

// ListBox for RAW Items with sprite icons
class RawItemListBox : public wxVListBox {
public:
	RawItemListBox(wxWindow* parent, wxWindowID id = wxID_ANY);
	virtual ~RawItemListBox() = default;

	void UpdateFilter(const wxString& filterText);
	int GetSelectedItemID() const;
	void SelectItemByID(int id);

protected:
	virtual void OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const override;
	virtual wxCoord OnMeasureItem(size_t n) const override;

private:
	std::vector<int> filtered_item_ids;
};

// Preview canvas for 32x32 / multi-tile sprite preview
class ItemPreviewCanvas : public wxPanel {
public:
	ItemPreviewCanvas(wxWindow* parent, wxSize size = wxSize(64, 64));
	void SetItemID(int id);
	void SetSpriteBorder(int borderTileID);

protected:
	void OnPaint(wxPaintEvent& event);

private:
	int item_id = 0;
	DECLARE_EVENT_TABLE();
};

// The Main Tileset Manager Dialog (combines Tileset/Autoborder + Extensions)
class TilesetManagerDialog : public wxDialog {
public:
	TilesetManagerDialog(wxWindow* parent);
	virtual ~TilesetManagerDialog() = default;

	void RefreshTilesets();
	void RefreshBrushes();

protected:
	// Tileset/Autoborder event handlers
	void OnSearchChange(wxCommandEvent& event);
	void OnRawItemSelected(wxCommandEvent& event);
	void OnAssignToTileset(wxCommandEvent& event);
	void OnCreateNewTileset(wxCommandEvent& event);
	void OnCreateAutoborderBrush(wxCommandEvent& event);
	void OnSaveExtensions(wxCommandEvent& event);
	void OnBorderSlotClick(wxCommandEvent& event);

	// Extension manager event handlers
	void OnExtListItemSelected(wxListEvent& event);
	void OnExtToggleEnable(wxCommandEvent& event);
	void OnExtOpenFolder(wxCommandEvent& event);
	void RefreshExtensionList();

	// GUI Controls - Tileset Tab
	wxTextCtrl* search_ctrl;
	RawItemListBox* raw_list;
	ItemPreviewCanvas* item_preview;
	wxStaticText* item_info_text;

	// Tagging & Tileset Assignment
	wxChoice* tileset_choice;
	wxChoice* category_choice;
	wxButton* assign_btn;
	wxButton* new_tileset_btn;

	// Autoborder 3x3 Grid
	wxButton* border_slots[9]; // NW, N, NE, W, Center, E, SW, S, SE
	int border_item_ids[9];
	wxTextCtrl* brush_name_ctrl;
	wxSpinCtrl* z_order_ctrl;
	wxSpinCtrl* look_id_ctrl;
	wxButton* create_brush_btn;

	// Extensions Manager Tab
	wxListCtrl* ext_list_ctrl;
	wxButton* ext_toggle_btn;
	wxButton* ext_open_folder_btn;
	wxStaticText* ext_info_text;
	int selected_ext_index = -1;

	DECLARE_EVENT_TABLE();
};

#endif
