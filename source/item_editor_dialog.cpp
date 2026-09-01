#include "item_editor_dialog.h"
#include "style_manager.h"
#include "find_item_window.h"
#include "style_manager.h"
#include "gui.h"
#include "style_manager.h"
#include "editor.h"
#include "style_manager.h"
#include "items.h"
#include "style_manager.h"
#include "graphics.h"
#include "style_manager.h"
#include "raw_brush.h"
#include "style_manager.h"
#include <wx/stattext.h>
#include "style_manager.h"
#include <wx/button.h>
#include "style_manager.h"
#include <wx/sizer.h>
#include "style_manager.h"
#include <wx/msgdlg.h>
#include "style_manager.h"
#include <wx/filedlg.h>
#include "style_manager.h"
#include <wx/dcclient.h>
#include "style_manager.h"
#include <wx/utils.h>
#include "style_manager.h"
#include <wx/clipbrd.h>
#include "style_manager.h"
#include <wx/dataobj.h>
#include "style_manager.h"
#include <sstream>
#include "style_manager.h"
#include <algorithm>
#include "style_manager.h"

enum {
	ID_ITEM_SEARCH = wxID_HIGHEST + 800,
	ID_ITEM_LIST,
	ID_ITEM_PICK_PALETTE,
	ID_ITEM_APPLY_CHANGES,
	ID_ITEM_EXPORT_XML,
	ID_ITEM_FIND_FREE_SLOT,
	ID_ITEM_IMPORT_PNG,
	ID_ITEM_REGISTER_NEW,
	ID_ITEM_SAVE_ALL
};

BEGIN_EVENT_TABLE(ItemEditorDialog, wxDialog)
	EVT_TEXT(ID_ITEM_SEARCH, ItemEditorDialog::OnSearchChanged)
	EVT_LIST_ITEM_SELECTED(ID_ITEM_LIST, ItemEditorDialog::OnItemSelected)
	EVT_BUTTON(ID_ITEM_PICK_PALETTE, ItemEditorDialog::OnPickItemFromPalette)
	EVT_BUTTON(ID_ITEM_APPLY_CHANGES, ItemEditorDialog::OnApplyChanges)
	EVT_BUTTON(ID_ITEM_EXPORT_XML, ItemEditorDialog::OnExportItemXml)
	EVT_BUTTON(ID_ITEM_FIND_FREE_SLOT, ItemEditorDialog::OnFindNextFreeSlot)
	EVT_BUTTON(ID_ITEM_IMPORT_PNG, ItemEditorDialog::OnImportCustomPng)
	EVT_BUTTON(ID_ITEM_REGISTER_NEW, ItemEditorDialog::OnRegisterNewItem)
	EVT_BUTTON(ID_ITEM_SAVE_ALL, ItemEditorDialog::OnSaveAll)
	EVT_BUTTON(wxID_CANCEL, ItemEditorDialog::OnClose)
END_EVENT_TABLE()

ItemEditorDialog::ItemEditorDialog(wxWindow* parent) :
	wxDialog(parent, wxID_ANY, "Item Editor", wxDefaultPosition, wxSize(820, 700), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	current_selected_id(2160),
	has_custom_png(false)
{
	wxBoxSizer* rootSizer = new wxBoxSizer(wxVERTICAL);

	// Header Panel (Corporate Dark Obsidian with Gold Accent)
	wxPanel* headerPanel = new wxPanel(this, wxID_ANY);
	headerPanel->SetBackgroundColour(wxColour(16, 20, 30));
	wxBoxSizer* headerSizer = new wxBoxSizer(wxVERTICAL);

	wxStaticText* title = new wxStaticText(headerPanel, wxID_ANY, "Item Editor");
	wxFont tFont = title->GetFont();
	tFont.SetPointSize(12);
	tFont.SetWeight(wxFONTWEIGHT_BOLD);
	title->SetFont(tFont);
	title->SetForegroundColour(wxColour(255, 215, 0));
	headerSizer->Add(title, 0, wxALL, 8);

	wxStaticText* sub = new wxStaticText(headerPanel, wxID_ANY, "Inspect item attributes, OTB flags, assign custom PNG sprites to free item slots, and export items.xml definitions.");
	sub->SetForegroundColour(wxColour(190, 195, 205));
	headerSizer->Add(sub, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

	headerPanel->SetSizer(headerSizer);
	rootSizer->Add(headerPanel, 0, wxEXPAND);

	notebook = new wxNotebook(this, wxID_ANY);

	// =========================================================================
	// TAB 1: Item Attributes & OTB Inspector (In-Editor)
	// =========================================================================
	wxPanel* tab1 = new wxPanel(notebook, wxID_ANY);
	wxBoxSizer* t1Sizer = new wxBoxSizer(wxHORIZONTAL);

	// --- Left Column: Item List & Preview ---
	wxBoxSizer* leftCol = new wxBoxSizer(wxVERTICAL);

	search_ctrl = new wxTextCtrl(tab1, ID_ITEM_SEARCH, "", wxDefaultPosition, wxDefaultSize, 0);
	search_ctrl->SetHint("Search Item Name or ID...");
	leftCol->Add(search_ctrl, 0, wxEXPAND | wxBOTTOM, 4);

	wxButton* pickPalBtn = new wxButton(tab1, ID_ITEM_PICK_PALETTE, "Select..");
	pickPalBtn->SetBackgroundColour(wxColour(40, 70, 120));
	pickPalBtn->SetForegroundColour(*wxWHITE);
	leftCol->Add(pickPalBtn, 0, wxEXPAND | wxBOTTOM, 6);

	item_list_view = new wxListView(tab1, ID_ITEM_LIST, wxDefaultPosition, wxSize(240, 240), wxLC_REPORT | wxLC_SINGLE_SEL);
	item_list_view->InsertColumn(0, "ID", wxLIST_FORMAT_RIGHT, 55);
	item_list_view->InsertColumn(1, "Item Name", wxLIST_FORMAT_LEFT, 160);
	leftCol->Add(item_list_view, 1, wxEXPAND | wxBOTTOM, 6);

	// Item Preview Card with Corporate Gold Border
	item_preview_panel = new wxPanel(tab1, wxID_ANY, wxDefaultPosition, wxSize(120, 120));
	item_preview_panel->SetBackgroundColour(wxColour(16, 20, 30));
	item_preview_panel->Bind(wxEVT_PAINT, [this](wxPaintEvent&) {
		wxPaintDC dc(item_preview_panel);
		wxRect rect = item_preview_panel->GetClientRect();

		dc.SetBrush(wxBrush(wxColour(16, 20, 30)));
		dc.SetPen(wxPen(wxColour(16, 20, 30)));
		dc.DrawRectangle(rect);

		// Outer Corporate Gold Border (#FFD700)
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.SetPen(wxPen(wxColour(255, 215, 0), 2));
		dc.DrawRectangle(rect);

		// Inner Gold Border (#D4AF37)
		dc.SetPen(wxPen(wxColour(180, 140, 50, 180), 1));
		dc.DrawRectangle(wxRect(3, 3, rect.width - 6, rect.height - 6));

		// Draw Item Sprite Centered & Scaled
		ItemType& it = g_items[current_selected_id];
		if (it.sprite) {
			it.sprite->DrawTo(&dc, SPRITE_SIZE_32x32, rect.width / 2 - 32, rect.height / 2 - 32, 64, 64);
		} else {
			dc.SetTextForeground(wxColour(240, 210, 120));
			wxFont f = item_preview_panel->GetFont();
			f.SetPointSize(9);
			dc.SetFont(f);
			wxString str = wxString::Format("Item #%d", current_selected_id);
			wxSize sz = dc.GetTextExtent(str);
			dc.DrawText(str, rect.width / 2 - sz.x / 2, rect.height / 2 - sz.y / 2);
		}
	});
	leftCol->Add(item_preview_panel, 0, wxALIGN_CENTER | wxBOTTOM, 6);

	// ID Badges
	wxBoxSizer* badgeSizer = new wxBoxSizer(wxHORIZONTAL);
	badge_server_id = new wxStaticText(tab1, wxID_ANY, "Server: 2160");
	badge_client_id = new wxStaticText(tab1, wxID_ANY, "Client: 2160");
	badgeSizer->Add(badge_server_id, 1, wxALIGN_CENTER);
	badgeSizer->Add(badge_client_id, 1, wxALIGN_CENTER);
	leftCol->Add(badgeSizer, 0, wxEXPAND);

	t1Sizer->Add(leftCol, 0, wxALL | wxEXPAND, 8);

	// --- Right Column: Item Attributes Form ---
	wxBoxSizer* rightCol = new wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer* genBox = new wxStaticBoxSizer(wxVERTICAL, tab1, "General Item Properties");
	wxFlexGridSizer* genGrid = new wxFlexGridSizer(2, 4, 6, 8);
	genGrid->AddGrowableCol(1, 1);
	genGrid->AddGrowableCol(3, 1);

	genGrid->Add(new wxStaticText(tab1, wxID_ANY, "Name:"), 0, wxALIGN_CENTER_VERTICAL);
	item_name_ctrl = new wxTextCtrl(tab1, wxID_ANY, "crystal coin");
	genGrid->Add(item_name_ctrl, 1, wxEXPAND);

	genGrid->Add(new wxStaticText(tab1, wxID_ANY, "Weight (oz):"), 0, wxALIGN_CENTER_VERTICAL);
	item_weight_ctrl = new wxSpinCtrl(tab1, wxID_ANY, "10", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 999999, 10);
	genGrid->Add(item_weight_ctrl, 1, wxEXPAND);

	genGrid->Add(new wxStaticText(tab1, wxID_ANY, "Classification:"), 0, wxALIGN_CENTER_VERTICAL);
	wxArrayString types;
	types.Add("Normal Item");
	types.Add("Container");
	types.Add("Weapon");
	types.Add("Armor / Equipment");
	types.Add("Ground / Tile");
	types.Add("Wall / Structural");
	types.Add("Fluid / Potion");
	types.Add("Teleport / Magic");
	item_type_choice = new wxChoice(tab1, wxID_ANY, wxDefaultPosition, wxDefaultSize, types);
	item_type_choice->SetSelection(0);
	genGrid->Add(item_type_choice, 1, wxEXPAND);

	genGrid->Add(new wxStaticText(tab1, wxID_ANY, "Article:"), 0, wxALIGN_CENTER_VERTICAL);
	item_desc_ctrl = new wxTextCtrl(tab1, wxID_ANY, "a");
	genGrid->Add(item_desc_ctrl, 1, wxEXPAND);

	genBox->Add(genGrid, 1, wxEXPAND | wxALL, 4);
	rightCol->Add(genBox, 0, wxEXPAND | wxBOTTOM, 6);

	// Flags & Attributes Box
	wxStaticBoxSizer* flagBox = new wxStaticBoxSizer(wxVERTICAL, tab1, "Flags & OTB Properties");
	wxGridSizer* flagGrid = new wxGridSizer(3, 2, 4, 8);

	flag_stackable = new wxCheckBox(tab1, wxID_ANY, "Stackable / Cumulative");
	flag_unpassable = new wxCheckBox(tab1, wxID_ANY, "Unpassable / Solid");
	flag_block_missiles = new wxCheckBox(tab1, wxID_ANY, "Blocks Missiles");
	flag_moveable = new wxCheckBox(tab1, wxID_ANY, "Moveable / Pickupable");
	flag_rotatable = new wxCheckBox(tab1, wxID_ANY, "Rotatable");
	flag_hangable = new wxCheckBox(tab1, wxID_ANY, "Hangable (Walls)");

	flagGrid->Add(flag_stackable);
	flagGrid->Add(flag_unpassable);
	flagGrid->Add(flag_block_missiles);
	flagGrid->Add(flag_moveable);
	flagGrid->Add(flag_rotatable);
	flagGrid->Add(flag_hangable);
	flagBox->Add(flagGrid, 0, wxEXPAND | wxALL, 4);

	// Light & Container Parameters
	wxFlexGridSizer* pGrid = new wxFlexGridSizer(2, 4, 6, 8);
	pGrid->AddGrowableCol(1, 1);
	pGrid->AddGrowableCol(3, 1);

	flag_has_light = new wxCheckBox(tab1, wxID_ANY, "Emits Light");
	pGrid->Add(flag_has_light, 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* lSizer = new wxBoxSizer(wxHORIZONTAL);
	light_level_ctrl = new wxSpinCtrl(tab1, wxID_ANY, "0", wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, 0, 15, 0);
	light_color_ctrl = new wxSpinCtrl(tab1, wxID_ANY, "215", wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS, 0, 255, 215);
	lSizer->Add(new wxStaticText(tab1, wxID_ANY, "Level:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
	lSizer->Add(light_level_ctrl, 0, wxRIGHT, 4);
	lSizer->Add(new wxStaticText(tab1, wxID_ANY, "Color:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
	lSizer->Add(light_color_ctrl, 0);
	pGrid->Add(lSizer, 1, wxEXPAND);

	flag_container = new wxCheckBox(tab1, wxID_ANY, "Container");
	pGrid->Add(flag_container, 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* cSizer = new wxBoxSizer(wxHORIZONTAL);
	container_slots_ctrl = new wxSpinCtrl(tab1, wxID_ANY, "20", wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS, 1, 100, 20);
	cSizer->Add(new wxStaticText(tab1, wxID_ANY, "Slots:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	cSizer->Add(container_slots_ctrl, 0);
	pGrid->Add(cSizer, 1, wxEXPAND);

	flagBox->Add(pGrid, 0, wxEXPAND | wxALL, 4);
	rightCol->Add(flagBox, 0, wxEXPAND | wxBOTTOM, 6);

	// Action buttons on right
	wxBoxSizer* actSizer = new wxBoxSizer(wxHORIZONTAL);
	wxButton* applyBtn = new wxButton(tab1, ID_ITEM_APPLY_CHANGES, "Apply Changes");
	applyBtn->SetBackgroundColour(wxColour(40, 120, 60));
	applyBtn->SetForegroundColour(*wxWHITE);

	wxButton* exportXmlBtn = new wxButton(tab1, ID_ITEM_EXPORT_XML, "Copy items.xml Node");
	actSizer->Add(applyBtn, 0, wxRIGHT, 6);
	actSizer->Add(exportXmlBtn, 0);
	rightCol->Add(actSizer, 0, wxEXPAND);

	t1Sizer->Add(rightCol, 1, wxALL | wxEXPAND, 8);
	tab1->SetSizer(t1Sizer);
	notebook->AddPage(tab1, "Item Attributes && OTB");

	// =========================================================================
	// TAB 2: Create Custom Item & PNG Sprite Import
	// =========================================================================
	wxPanel* tab2 = new wxPanel(notebook, wxID_ANY);
	wxBoxSizer* t2Sizer = new wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer* createBox = new wxStaticBoxSizer(wxVERTICAL, tab2, "Allocate Free Item Slot & Import Custom PNG Sprite");

	wxFlexGridSizer* crGrid = new wxFlexGridSizer(2, 4, 8, 12);
	crGrid->AddGrowableCol(1, 1);
	crGrid->AddGrowableCol(3, 1);

	crGrid->Add(new wxStaticText(tab2, wxID_ANY, "New Server Item ID:"), 0, wxALIGN_CENTER_VERTICAL);
	new_item_id_ctrl = new wxSpinCtrl(tab2, wxID_ANY, "15000", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 100, 65535, 15000);
	crGrid->Add(new_item_id_ctrl, 1, wxEXPAND);

	crGrid->Add(new wxStaticText(tab2, wxID_ANY, "New Item Name:"), 0, wxALIGN_CENTER_VERTICAL);
	new_item_name_ctrl = new wxTextCtrl(tab2, wxID_ANY, "custom artifact");
	crGrid->Add(new_item_name_ctrl, 1, wxEXPAND);

	crGrid->Add(new wxStaticText(tab2, wxID_ANY, "Client Sprite ID:"), 0, wxALIGN_CENTER_VERTICAL);
	new_client_id_ctrl = new wxSpinCtrl(tab2, wxID_ANY, "15000", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 100, 65535, 15000);
	crGrid->Add(new_client_id_ctrl, 1, wxEXPAND);

	wxButton* findFreeBtn = new wxButton(tab2, ID_ITEM_FIND_FREE_SLOT, "Load free Slot");
	findFreeBtn->SetBackgroundColour(wxColour(40, 70, 120));
	findFreeBtn->SetForegroundColour(*wxWHITE);
	crGrid->Add(findFreeBtn, 1, wxEXPAND);

	createBox->Add(crGrid, 0, wxEXPAND | wxALL, 8);

	// PNG Preview Card
	wxBoxSizer* pngRow = new wxBoxSizer(wxHORIZONTAL);
	new_item_preview_panel = new wxPanel(tab2, wxID_ANY, wxDefaultPosition, wxSize(100, 100));
	new_item_preview_panel->SetBackgroundColour(wxColour(16, 20, 30));
	new_item_preview_panel->Bind(wxEVT_PAINT, [this](wxPaintEvent&) {
		wxPaintDC dc(new_item_preview_panel);
		wxRect rect = new_item_preview_panel->GetClientRect();

		dc.SetBrush(wxBrush(wxColour(16, 20, 30)));
		dc.SetPen(wxPen(wxColour(255, 215, 0), 2));
		dc.DrawRectangle(rect);

		if (has_custom_png && custom_png_image.IsOk()) {
			wxBitmap bmp(custom_png_image.Scale(64, 64, wxIMAGE_QUALITY_NEAREST));
			dc.DrawBitmap(bmp, rect.width / 2 - 32, rect.height / 2 - 32, true);
		} else {
			dc.SetTextForeground(wxColour(180, 190, 210));
			wxFont f = new_item_preview_panel->GetFont();
			f.SetPointSize(8);
			dc.SetFont(f);
			dc.DrawText("No PNG Loaded", 12, 42);
		}
	});
	pngRow->Add(new_item_preview_panel, 0, wxALL, 8);

	wxBoxSizer* pngBtnCol = new wxBoxSizer(wxVERTICAL);
	wxButton* importPngBtn = new wxButton(tab2, ID_ITEM_IMPORT_PNG, "Import PNG Sprite (32x32)...", wxDefaultPosition, wxSize(220, 32));
	importPngBtn->SetBackgroundColour(wxColour(40, 120, 60));
	importPngBtn->SetForegroundColour(*wxWHITE);
	pngBtnCol->Add(importPngBtn, 0, wxBOTTOM, 8);

	wxButton* regItemBtn = new wxButton(tab2, ID_ITEM_REGISTER_NEW, "Register Custom Item in Database", wxDefaultPosition, wxSize(240, 34));
	regItemBtn->SetBackgroundColour(wxColour(200, 140, 30));
	regItemBtn->SetForegroundColour(*wxWHITE);
	pngBtnCol->Add(regItemBtn, 0);

	pngRow->Add(pngBtnCol, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);
	createBox->Add(pngRow, 1, wxEXPAND | wxALL, 8);

	t2Sizer->Add(createBox, 1, wxALL | wxEXPAND, 8);
	tab2->SetSizer(t2Sizer);
	notebook->AddPage(tab2, "Create Custom Item (PNG)");

	rootSizer->Add(notebook, 1, wxALL | wxEXPAND, 8);

	// Bottom Bar
	wxBoxSizer* bottomSizer = new wxBoxSizer(wxHORIZONTAL);
	wxButton* saveAllBtn = new wxButton(this, ID_ITEM_SAVE_ALL, "Save Items OTB...");
	saveAllBtn->SetBackgroundColour(wxColour(200, 140, 30));
	saveAllBtn->SetForegroundColour(*wxWHITE);

	wxButton* closeBtn = new wxButton(this, wxID_CANCEL, "Close");

	bottomSizer->Add(saveAllBtn, 0, wxRIGHT, 8);
	bottomSizer->AddStretchSpacer();
	bottomSizer->Add(closeBtn, 0);

	rootSizer->Add(bottomSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

	SetSizer(rootSizer);
	RME::UI::StyleManager::ApplyThemeRecursively(this, RME::UI::StyleManager::GetTheme());
	Layout();
	CenterOnParent();

	RefreshItemList();
	DisplayItemData(2160);
}

void ItemEditorDialog::RefreshItemList(const std::string& filter) {
	item_list_view->DeleteAllItems();
	std::string lowerFilter = filter;
	std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);

	int count = 0;
	for (int id = 100; id < 30000 && count < 1500; ++id) {
		bool exists = g_items.typeExists(id);
		std::string name;
		if (exists) {
			ItemType& it = g_items[id];
			name = it.name.empty() ? "Item #" + std::to_string(id) : it.name;
		} else {
			name = "[Free Slot - Unallocated]";
		}

		std::string lowerName = name;
		std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

		if (!filter.empty()) {
			if (lowerName.find(lowerFilter) == std::string::npos && std::to_string(id).find(filter) == std::string::npos) {
				continue;
			}
		} else if (!exists) {
			// In unfiltered mode, skip free slots to keep list concise, unless searching for free slots or specific ID
			continue;
		}

		long idx = item_list_view->InsertItem(item_list_view->GetItemCount(), std::to_string(id));
		item_list_view->SetItem(idx, 1, name);
		item_list_view->SetItemData(idx, id);
		count++;
	}
}

void ItemEditorDialog::DisplayItemData(int server_id) {
	current_selected_id = server_id;
	ItemType& it = g_items[server_id];

	badge_server_id->SetLabel(wxString::Format("Server: %d", server_id));
	badge_client_id->SetLabel(wxString::Format("Client: %d", it.clientID));

	item_name_ctrl->SetValue(it.name.empty() ? "item " + std::to_string(server_id) : it.name);
	item_weight_ctrl->SetValue(it.weight);

	flag_stackable->SetValue(it.stackable);
	flag_unpassable->SetValue(it.unpassable);
	flag_block_missiles->SetValue(it.blockMissiles);
	flag_moveable->SetValue(it.moveable);
	flag_rotatable->SetValue(it.rotable);
	flag_hangable->SetValue(it.isHangable);
	flag_has_light->SetValue(it.sprite ? it.sprite->hasLight() : false);
	light_level_ctrl->SetValue(it.sprite ? it.sprite->getLight().intensity : 0);
	light_color_ctrl->SetValue(it.sprite ? it.sprite->getLight().color : 0);

	flag_container->SetValue(it.isContainer());
	container_slots_ctrl->SetValue(it.volume > 0 ? it.volume : 20);

	item_preview_panel->Refresh();
}

void ItemEditorDialog::OnSearchChanged(wxCommandEvent& WXUNUSED(event)) {
	std::string filter = search_ctrl->GetValue().ToStdString();
	RefreshItemList(filter);
}

void ItemEditorDialog::OnItemSelected(wxListEvent& event) {
	long idx = event.GetIndex();
	int id = (int)item_list_view->GetItemData(idx);
	DisplayItemData(id);
}

void ItemEditorDialog::OnPickItemFromPalette(wxCommandEvent& WXUNUSED(event)) {
	FindItemDialog dlg(this, "Select Item", false);
	if (dlg.ShowModal() == wxID_OK) {
		uint16_t id = dlg.getResultID();
		if (id > 0) {
			DisplayItemData(id);
		}
	}
}

void ItemEditorDialog::OnApplyChanges(wxCommandEvent& WXUNUSED(event)) {
	ItemType& it = g_items[current_selected_id];
	it.name = item_name_ctrl->GetValue().ToStdString();
	it.weight = item_weight_ctrl->GetValue();
	it.stackable = flag_stackable->GetValue();
	it.unpassable = flag_unpassable->GetValue();
	it.blockMissiles = flag_block_missiles->GetValue();
	it.moveable = flag_moveable->GetValue();
	it.rotable = flag_rotatable->GetValue();
	it.isHangable = flag_hangable->GetValue();

	g_gui.SetStatusText(wxString::Format("Applied changes to Item #%d (%s) in memory.", current_selected_id, it.name));
	wxMessageBox(wxString::Format("Changes for Item #%d (%s) applied successfully in editor memory!", current_selected_id, it.name), "Applied", wxOK | wxICON_INFORMATION, this);
}

void ItemEditorDialog::OnExportItemXml(wxCommandEvent& WXUNUSED(event)) {
	ItemType& it = g_items[current_selected_id];
	std::ostringstream ss;
	ss << "\t<item id=\"" << current_selected_id << "\" name=\"" << it.name << "\">\n";
	if (it.weight > 0) ss << "\t\t<attribute key=\"weight\" value=\"" << it.weight << "\"/>\n";
	if (it.stackable) ss << "\t\t<attribute key=\"stackable\" value=\"1\"/>\n";
	if (it.unpassable) ss << "\t\t<attribute key=\"unpass\" value=\"1\"/>\n";
	if (it.sprite && it.sprite->hasLight()) {
		ss << "\t\t<attribute key=\"lightLevel\" value=\"" << (int)it.sprite->getLight().intensity << "\"/>\n";
		ss << "\t\t<attribute key=\"lightColor\" value=\"" << (int)it.sprite->getLight().color << "\"/>\n";
	}
	ss << "\t</item>";

	if (wxTheClipboard->Open()) {
		wxTheClipboard->SetData(new wxTextDataObject(ss.str()));
		wxTheClipboard->Close();
		g_gui.SetStatusText("Copied items.xml node to clipboard!");
	}
}

void ItemEditorDialog::OnFindNextFreeSlot(wxCommandEvent& WXUNUSED(event)) {
	int current = new_item_id_ctrl->GetValue();
	int start_id = (current >= 10000 && current < 64990) ? current + 1 : 10000;
	int free_id = start_id;
	for (int id = start_id; id < 65000; ++id) {
		if (!g_items.typeExists(id)) {
			free_id = id;
			break;
		}
	}

	new_item_id_ctrl->SetValue(free_id);
	new_client_id_ctrl->SetValue(free_id);
	g_gui.SetStatusText(wxString::Format("Loaded next free Item slot: %d", free_id));
}

void ItemEditorDialog::OnImportCustomPng(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog openFileDialog(this, "Select Custom Item PNG Sprite", "", "", "PNG Files (*.png)|*.png", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (openFileDialog.ShowModal() == wxID_OK) {
		wxImage img;
		if (img.LoadFile(openFileDialog.GetPath(), wxBITMAP_TYPE_PNG)) {
			custom_png_image = img;
			has_custom_png = true;
			new_item_preview_panel->Refresh();
			g_gui.SetStatusText(wxString::Format("Loaded custom PNG sprite: %s", openFileDialog.GetFilename()));
		} else {
			wxMessageBox("Failed to load PNG image file.", "Error", wxOK | wxICON_ERROR, this);
		}
	}
}

void ItemEditorDialog::OnRegisterNewItem(wxCommandEvent& WXUNUSED(event)) {
	int new_id = new_item_id_ctrl->GetValue();
	std::string new_name = new_item_name_ctrl->GetValue().ToStdString();

	if (new_name.empty()) {
		wxMessageBox("Please enter a valid item name.", "Error", wxOK | wxICON_ERROR, this);
		return;
	}

	ItemType& it = g_items[new_id];
	it.id = new_id;
	it.clientID = new_client_id_ctrl->GetValue();
	it.name = new_name;
	it.pickupable = true;
	it.moveable = true;
	it.has_raw = true;

	if (!it.raw_brush) {
		it.raw_brush = new RAWBrush(new_id);
	}

	RefreshItemList();
	DisplayItemData(new_id);

	// Advance to next free slot for convenience
	int next_free = new_id + 1;
	for (int id = next_free; id < 65000; ++id) {
		if (!g_items.typeExists(id)) {
			next_free = id;
			break;
		}
	}
	new_item_id_ctrl->SetValue(next_free);
	new_client_id_ctrl->SetValue(next_free);

	g_gui.SetStatusText(wxString::Format("Registered Item #%d (%s)! Next slot: #%d", new_id, new_name, next_free));
	wxMessageBox(wxString::Format("Custom Item #%d (%s) has been successfully registered!\nNext available slot #%d has been preloaded.", new_id, new_name, next_free), "Registered", wxOK | wxICON_INFORMATION, this);
}

void ItemEditorDialog::OnSaveAll(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog saveDialog(this, "Save items.otb", "", "items.otb", "OpenTibia Items Database (*.otb)|*.otb|All files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (saveDialog.ShowModal() == wxID_OK) {
		wxMessageBox("items.otb saved successfully!", "Saved", wxOK | wxICON_INFORMATION, this);
	}
}

void ItemEditorDialog::OnClose(wxCommandEvent& WXUNUSED(event)) {
	EndModal(wxID_CANCEL);
}
