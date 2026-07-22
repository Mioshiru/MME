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

#include "properties_window.h"
#include "tile.h"
#include "map.h"
#include "town.h"
#include "editor.h"
#include "gui.h"

#include "gui_ids.h"
#include "complexitem.h"
#include <wx/wrapsizer.h>
#include "container_properties_window.h"

#include <wx/grid.h>
#include <wx/textctrl.h>
#include <wx/msgdlg.h>

BEGIN_EVENT_TABLE(PropertiesWindow, wxDialog)
EVT_BUTTON(wxID_OK, PropertiesWindow::OnClickOK)
EVT_BUTTON(wxID_CANCEL, PropertiesWindow::OnClickCancel)

EVT_BUTTON(ITEM_PROPERTIES_ADD_ATTRIBUTE, PropertiesWindow::OnClickAddAttribute)
EVT_BUTTON(ITEM_PROPERTIES_REMOVE_ATTRIBUTE, PropertiesWindow::OnClickRemoveAttribute)
EVT_BUTTON(ITEM_PROPERTIES_TOWN_BTN, PropertiesWindow::OnClickTown)

EVT_NOTEBOOK_PAGE_CHANGED(wxID_ANY, PropertiesWindow::OnNotebookPageChanged)

EVT_GRID_CELL_CHANGED(PropertiesWindow::OnGridValueChanged)
END_EVENT_TABLE()

PropertiesWindow::PropertiesWindow(wxWindow* parent, const Map* map, const Tile* tile_parent, Item* item, wxPoint pos) :
	ObjectPropertiesWindowBase(parent, "Item Properties", map, tile_parent, item, pos),
	action_id_field(nullptr),
	unique_id_field(nullptr),
	count_field(nullptr),
	waypoint_name_field(nullptr),
	currentPanel(nullptr) {
	ASSERT(edit_item);
	notebook = newd wxNotebook(this, wxID_ANY, wxDefaultPosition, wxSize(600, 300));

	notebook->AddPage(createGeneralPanel(notebook), "Simple", true);
	if (dynamic_cast<Container*>(item)) {
		notebook->AddPage(createContainerPanel(notebook), "Contents");
	}
	notebook->AddPage(createAttributesPanel(notebook), "Advanced");
	if (edit_tile) {
		notebook->AddPage(createWaypointPanel(notebook), "Waypoint");
	}

	wxSizer* topSizer = newd wxBoxSizer(wxVERTICAL);
	topSizer->Add(notebook, wxSizerFlags(1).DoubleBorder());

	wxSizer* optSizer = newd wxBoxSizer(wxHORIZONTAL);
	optSizer->Add(newd wxButton(this, wxID_OK, "OK"), wxSizerFlags(0).Center());
	optSizer->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(0).Center());
	topSizer->Add(optSizer, wxSizerFlags(0).Center().DoubleBorder());

	SetSizerAndFit(topSizer);
	Centre(wxBOTH);
}

PropertiesWindow::~PropertiesWindow() {
	;
}

void PropertiesWindow::Update() {
	Container* container = dynamic_cast<Container*>(edit_item);
	if (container) {
		for (uint32_t i = 0; i < container->getVolume(); ++i) {
			container_items[i]->setItem(container->getItem(i));
		}
	}
	wxDialog::Update();
}

wxWindow* PropertiesWindow::createGeneralPanel(wxWindow* parent) {
	wxPanel* panel = newd wxPanel(parent, ITEM_PROPERTIES_GENERAL_TAB);
	wxFlexGridSizer* gridsizer = newd wxFlexGridSizer(2, 10, 10);
	gridsizer->AddGrowableCol(1);

	gridsizer->Add(newd wxStaticText(panel, wxID_ANY, "ID " + i2ws(edit_item->getID())));
	gridsizer->Add(newd wxStaticText(panel, wxID_ANY, "\"" + wxstr(edit_item->getName()) + "\""));

	gridsizer->Add(newd wxStaticText(panel, wxID_ANY, "Action ID"));
	action_id_field = newd wxSpinCtrl(panel, wxID_ANY, i2ws(edit_item->getActionID()), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 0xFFFF, edit_item->getActionID());
	gridsizer->Add(action_id_field, wxSizerFlags(1).Expand());

	gridsizer->Add(newd wxStaticText(panel, wxID_ANY, "Unique ID"));
	unique_id_field = newd wxSpinCtrl(panel, wxID_ANY, i2ws(edit_item->getUniqueID()), wxDefaultPosition, wxSize(-1, 20), wxSP_ARROW_KEYS, 0, 0xFFFF, edit_item->getUniqueID());
	gridsizer->Add(unique_id_field, wxSizerFlags(1).Expand());

	if (edit_item->isStackable() || edit_item->isCharged() || edit_item->isFluidContainer() || edit_item->isSplash()) {
		int max_count = 100;
		if (edit_item->isFluidContainer() || edit_item->isSplash()) {
			max_count = 250;
		} else if (edit_item->isCharged()) {
			max_count = 65500;
		}
		gridsizer->Add(newd wxStaticText(panel, wxID_ANY, "Count/Subtype"));
		count_field = newd wxSpinCtrl(panel, wxID_ANY, i2ws(edit_item->getCount()), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, max_count, edit_item->getCount());
		if (edit_item->isHangable()) {
			count_field->Enable(false);
		}
		gridsizer->Add(count_field, wxSizerFlags(1).Expand());
	} else {
		count_field = nullptr;
	}

	if (edit_item->canHoldText() || edit_item->canHoldDescription()) {
		gridsizer->Add(newd wxStaticText(panel, wxID_ANY, "Text"));
		text_field = newd wxTextCtrl(panel, wxID_ANY, wxstr(edit_item->getText()), wxDefaultPosition, wxSize(-1, 80), wxTE_MULTILINE);
		gridsizer->Add(text_field, wxSizerFlags(1).Expand());
	} else {
		text_field = nullptr;
	}

	if (edit_item->isGroundTile() && !edit_item->hasProperty(BLOCKSOLID)) {
		Town* clicked_town = nullptr;
		if (edit_tile && edit_map) {
			Position click_pos = edit_tile->getPosition();
			for (const auto& pair : edit_map->towns) {
				if (pair.second->getTemplePosition() == click_pos) {
					clicked_town = pair.second;
					break;
				}
			}
		}

		wxString town_text_val = "None";
		if (clicked_town) {
			town_text_val = wxString::Format("%d - %s", clicked_town->getID(), wxstr(clicked_town->getName()));
		}

		gridsizer->Add(newd wxStaticText(panel, wxID_ANY, "Town"));

		wxSizer* town_sizer = newd wxBoxSizer(wxHORIZONTAL);
		wxStaticText* town_lbl = newd wxStaticText(panel, wxID_ANY, town_text_val);
		town_sizer->Add(town_lbl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
		wxButton* town_btn = newd wxButton(panel, ITEM_PROPERTIES_TOWN_BTN, "Edit Towns...");
		town_sizer->Add(town_btn, 0, wxALIGN_CENTER_VERTICAL);

		gridsizer->Add(town_sizer, wxSizerFlags(0));
	}

	panel->SetSizerAndFit(gridsizer);

	return panel;
}

wxWindow* PropertiesWindow::createContainerPanel(wxWindow* parent) {
	Container* container = (Container*)edit_item;
	wxPanel* panel = newd wxPanel(parent, ITEM_PROPERTIES_CONTAINER_TAB);
	wxSizer* topSizer = newd wxBoxSizer(wxVERTICAL);

	wxSizer* gridSizer = newd wxWrapSizer(wxHORIZONTAL);

	bool use_large_sprites = g_settings.getBoolean(Config::USE_LARGE_CONTAINER_ICONS);
	for (uint32_t i = 0; i < container->getVolume(); ++i) {
		Item* item = container->getItem(i);
		ContainerItemButton* containerItemButton = newd ContainerItemButton(panel, use_large_sprites, i, edit_map, item);

		container_items.push_back(containerItemButton);
		gridSizer->Add(containerItemButton, wxSizerFlags(0));
	}

	topSizer->Add(gridSizer, wxSizerFlags(1).Expand());

	/*
	wxSizer* optSizer = newd wxBoxSizer(wxHORIZONTAL);
	optSizer->Add(newd wxButton(panel, ITEM_PROPERTIES_ADD_ATTRIBUTE, "Add Item"), wxSizerFlags(0).Center());
	// optSizer->Add(newd wxButton(panel, ITEM_PROPERTIES_REMOVE_ATTRIBUTE, "Remove Attribute"), wxSizerFlags(0).Center());
	topSizer->Add(optSizer, wxSizerFlags(0).Center().DoubleBorder());
	*/

	panel->SetSizer(topSizer);
	return panel;
}

wxWindow* PropertiesWindow::createAttributesPanel(wxWindow* parent) {
	wxPanel* panel = newd wxPanel(parent, wxID_ANY);
	wxSizer* topSizer = newd wxBoxSizer(wxVERTICAL);

	attributesGrid = newd wxGrid(panel, ITEM_PROPERTIES_ADVANCED_TAB, wxDefaultPosition, wxSize(-1, 160));
	topSizer->Add(attributesGrid, wxSizerFlags(1).Expand());

	wxFont time_font(*wxSWISS_FONT);
	attributesGrid->SetDefaultCellFont(time_font);
	attributesGrid->CreateGrid(0, 3);
	attributesGrid->DisableDragRowSize();
	attributesGrid->DisableDragColSize();
	attributesGrid->SetSelectionMode(wxGrid::wxGridSelectRows);
	attributesGrid->SetRowLabelSize(0);
	// log->SetColLabelSize(0);
	// log->EnableGridLines(false);
	attributesGrid->EnableEditing(true);

	attributesGrid->SetColLabelValue(0, "Key");
	attributesGrid->SetColSize(0, 100);
	attributesGrid->SetColLabelValue(1, "Type");
	attributesGrid->SetColSize(1, 80);
	attributesGrid->SetColLabelValue(2, "Value");
	attributesGrid->SetColSize(2, 410);

	// contents
	ItemAttributeMap attrs = edit_item->getAttributes();
	attributesGrid->AppendRows(attrs.size());
	int i = 0;
	for (ItemAttributeMap::iterator aiter = attrs.begin(); aiter != attrs.end(); ++aiter, ++i) {
		SetGridValue(attributesGrid, i, aiter->first, aiter->second);
	}

	wxSizer* optSizer = newd wxBoxSizer(wxHORIZONTAL);
	optSizer->Add(newd wxButton(panel, ITEM_PROPERTIES_ADD_ATTRIBUTE, "Add Attribute"), wxSizerFlags(0).Center());
	optSizer->Add(newd wxButton(panel, ITEM_PROPERTIES_REMOVE_ATTRIBUTE, "Remove Attribute"), wxSizerFlags(0).Center());
	topSizer->Add(optSizer, wxSizerFlags(0).Center().DoubleBorder());

	panel->SetSizer(topSizer);

	return panel;
}

void PropertiesWindow::SetGridValue(wxGrid* grid, int rowIndex, std::string label, const ItemAttribute& attr) {
	wxArrayString types;
	types.Add("Number");
	types.Add("Float");
	types.Add("Boolean");
	types.Add("String");

	grid->SetCellValue(rowIndex, 0, label);
	switch (attr.type) {
		case ItemAttribute::STRING: {
			grid->SetCellValue(rowIndex, 1, "String");
			grid->SetCellValue(rowIndex, 2, wxstr(*attr.getString()));
			break;
		}
		case ItemAttribute::INTEGER: {
			grid->SetCellValue(rowIndex, 1, "Number");
			grid->SetCellValue(rowIndex, 2, i2ws(*attr.getInteger()));
			grid->SetCellEditor(rowIndex, 2, new wxGridCellNumberEditor);
			break;
		}
		case ItemAttribute::DOUBLE:
		case ItemAttribute::FLOAT: {
			grid->SetCellValue(rowIndex, 1, "Float");
			wxString f;
			f << *attr.getFloat();
			grid->SetCellValue(rowIndex, 2, f);
			grid->SetCellEditor(rowIndex, 2, new wxGridCellFloatEditor);
			break;
		}
		case ItemAttribute::BOOLEAN: {
			grid->SetCellValue(rowIndex, 1, "Boolean");
			grid->SetCellValue(rowIndex, 2, *attr.getBoolean() ? "1" : "");
			grid->SetCellRenderer(rowIndex, 2, new wxGridCellBoolRenderer);
			grid->SetCellEditor(rowIndex, 2, new wxGridCellBoolEditor);
			break;
		}
		default: {
			grid->SetCellValue(rowIndex, 1, "Unknown");
			grid->SetCellBackgroundColour(rowIndex, 1, *wxLIGHT_GREY);
			grid->SetCellBackgroundColour(rowIndex, 2, *wxLIGHT_GREY);
			grid->SetReadOnly(rowIndex, 1, true);
			grid->SetReadOnly(rowIndex, 2, true);
			break;
		}
	}
	grid->SetCellEditor(rowIndex, 1, new wxGridCellChoiceEditor(types));
}

void PropertiesWindow::OnResize(wxSizeEvent& evt) {
	/*
	if(wxGrid* grid = (wxGrid*)currentPanel->FindWindowByName("AdvancedGrid")) {
		int tWidth = 0;
		for(int i = 0; i < 3; ++i)
			tWidth += grid->GetColumnWidth(i);

		int wWidth = grid->GetParent()->GetSize().GetWidth();

		grid->SetColumnWidth(2, wWidth - 100 - 80);
	}
	*/
}

void PropertiesWindow::OnNotebookPageChanged(wxNotebookEvent& evt) {
	wxWindow* page = notebook->GetCurrentPage();

	// TODO: Save

	switch (page->GetId()) {
		case ITEM_PROPERTIES_GENERAL_TAB: {
			// currentPanel = createGeneralPanel(page);
			break;
		}
		case ITEM_PROPERTIES_ADVANCED_TAB: {
			// currentPanel = createAttributesPanel(page);
			break;
		}
		default:
			break;
	}
}

void PropertiesWindow::saveGeneralPanel() {
	if (action_id_field) {
		edit_item->setActionID(action_id_field->GetValue());
	}
	if (unique_id_field) {
		edit_item->setUniqueID(unique_id_field->GetValue());
	}
	if (count_field && (edit_item->isStackable() || edit_item->isCharged() || edit_item->isFluidContainer() || edit_item->isSplash())) {
		edit_item->setSubtype(count_field->GetValue());
	}
	if (text_field && (edit_item->canHoldText() || edit_item->canHoldDescription())) {
		edit_item->setText(nstr(text_field->GetValue()));
	}
}

void PropertiesWindow::saveContainerPanel() {
	////
}

void PropertiesWindow::saveAttributesPanel() {
	if (!attributesGrid) return;
	// Preserve general panel attributes before clearing
	std::string text_val = edit_item->getText();
	uint16_t aid_val = edit_item->getActionID();
	uint16_t uid_val = edit_item->getUniqueID();

	edit_item->clearAllAttributes();

	if (!text_val.empty()) edit_item->setText(text_val);
	if (aid_val > 0) edit_item->setActionID(aid_val);
	if (uid_val > 0) edit_item->setUniqueID(uid_val);

	for (int32_t rowIndex = 0; rowIndex < attributesGrid->GetNumberRows(); ++rowIndex) {
		ItemAttribute attr;
		wxString type = attributesGrid->GetCellValue(rowIndex, 1);
		if (type == "String") {
			attr.set(nstr(attributesGrid->GetCellValue(rowIndex, 2)));
		} else if (type == "Float") {
			double value;
			if (attributesGrid->GetCellValue(rowIndex, 2).ToDouble(&value)) {
				attr.set(value);
			}
		} else if (type == "Number") {
			long value;
			if (attributesGrid->GetCellValue(rowIndex, 2).ToLong(&value)) {
				attr.set(static_cast<int32_t>(value));
			}
		} else if (type == "Boolean") {
			attr.set(attributesGrid->GetCellValue(rowIndex, 2) == "1");
		} else {
			continue;
		}
		std::string key = nstr(attributesGrid->GetCellValue(rowIndex, 0));
		if (!key.empty()) {
			edit_item->setAttribute(key, attr);
		}
	}
}

void PropertiesWindow::OnGridValueChanged(wxGridEvent& event) {
	if (event.GetCol() == 1) {
		wxString newType = attributesGrid->GetCellValue(event.GetRow(), 1);
		if (newType == event.GetString()) {
			return;
		}

		ItemAttribute attr;
		if (newType == "String") {
			attr.set("");
		} else if (newType == "Float") {
			attr.set(0.0f);
		} else if (newType == "Number") {
			attr.set(0);
		} else if (newType == "Boolean") {
			attr.set(false);
		}
		SetGridValue(attributesGrid, event.GetRow(), nstr(attributesGrid->GetCellValue(event.GetRow(), 0)), attr);
	}
}

void PropertiesWindow::OnClickOK(wxCommandEvent&) {
	if (!validateWaypointPanel()) {
		return;
	}
	saveGeneralPanel();
	saveAttributesPanel();
	saveWaypointPanel();
	EndModal(1);
}

void PropertiesWindow::OnClickAddAttribute(wxCommandEvent&) {
	attributesGrid->AppendRows(1);
	ItemAttribute attr(0);
	SetGridValue(attributesGrid, attributesGrid->GetNumberRows() - 1, "", attr);
}

void PropertiesWindow::OnClickRemoveAttribute(wxCommandEvent&) {
	wxArrayInt rowIndexes = attributesGrid->GetSelectedRows();
	if (rowIndexes.Count() != 1) {
		return;
	}

	int rowIndex = rowIndexes[0];
	attributesGrid->DeleteRows(rowIndex, 1);
}

void PropertiesWindow::OnClickTown(wxCommandEvent&) {
	if (!edit_tile) return;
	Position click_pos = edit_tile->getPosition();

	Town* clicked_town = nullptr;
	for (const auto& pair : edit_map->towns) {
		if (pair.second->getTemplePosition() == click_pos) {
			clicked_town = pair.second;
			break;
		}
	}

	if (!clicked_town) {
		uint32_t max_id = 0;
		for (const auto& pair : edit_map->towns) {
			if (pair.second->getID() > max_id) {
				max_id = pair.second->getID();
			}
		}
		Map* map = const_cast<Map*>(edit_map);
		clicked_town = newd Town(max_id + 1);
		clicked_town->setName("Unnamed Town");
		clicked_town->setTemplePosition(click_pos);
		map->towns.addTown(clicked_town);
		Tile* tile = map->getOrCreateTile(click_pos);
		if (tile) {
			tile->getLocation()->increaseTownCount();
		}
		map->doChange();
	} else {
		clicked_town->setTemplePosition(click_pos);
	}

	if (g_gui.GetCurrentEditor()) {
		wxDialog* town_dialog = newd EditTownsDialog(this, *g_gui.GetCurrentEditor(), clicked_town->getID());
		town_dialog->ShowModal();
		town_dialog->Destroy();
	}
}

void PropertiesWindow::OnClickCancel(wxCommandEvent&) {
	EndModal(0);
}

wxWindow* PropertiesWindow::createWaypointPanel(wxWindow* parent) {
	wxPanel* panel = newd wxPanel(parent, ITEM_PROPERTIES_WAYPOINT_TAB);
	wxFlexGridSizer* gridsizer = newd wxFlexGridSizer(2, 10, 10);
	gridsizer->AddGrowableCol(1);

	gridsizer->Add(newd wxStaticText(panel, wxID_ANY, "Create Waypoint"));
	gridsizer->AddSpacer(0);

	gridsizer->Add(newd wxStaticText(panel, wxID_ANY, "Waypoint Name:"));
	
	std::string current_wp_name = "";
	if (edit_tile && edit_map) {
		Map* mutable_map = const_cast<Map*>(edit_map);
		Tile* mutable_tile = const_cast<Tile*>(edit_tile);
		Waypoint* wp = mutable_map->waypoints.getWaypoint(mutable_tile->getLocation());
		if (wp) {
			current_wp_name = wp->name;
		}
	}

	waypoint_name_field = newd wxTextCtrl(panel, wxID_ANY, wxstr(current_wp_name));
	gridsizer->Add(waypoint_name_field, wxSizerFlags(1).Expand());

	panel->SetSizerAndFit(gridsizer);
	return panel;
}

bool PropertiesWindow::validateWaypointPanel() {
	if (!edit_tile || !edit_map || !waypoint_name_field) return true;

	std::string new_name = nstr(waypoint_name_field->GetValue());
	new_name.erase(0, new_name.find_first_not_of(" \t\r\n"));
	new_name.erase(new_name.find_last_not_of(" \t\r\n") + 1);

	if (new_name.empty()) return true;

	Map* mutable_map = const_cast<Map*>(edit_map);
	Tile* mutable_tile = const_cast<Tile*>(edit_tile);
	Waypoint* current_wp = mutable_map->waypoints.getWaypoint(mutable_tile->getLocation());
	Waypoint* existing_wp = mutable_map->waypoints.getWaypoint(new_name);

	if (existing_wp && (!current_wp || current_wp->name != new_name)) {
		wxMessageBox("There already is a waypoint with this name.", "Error", wxOK | wxICON_ERROR, this);
		return false;
	}
	return true;
}

void PropertiesWindow::saveWaypointPanel() {
	if (!edit_tile || !edit_map || !waypoint_name_field) return;

	std::string new_name = nstr(waypoint_name_field->GetValue());
	new_name.erase(0, new_name.find_first_not_of(" \t\r\n"));
	new_name.erase(new_name.find_last_not_of(" \t\r\n") + 1);

	Map* mutable_map = const_cast<Map*>(edit_map);
	Tile* mutable_tile = const_cast<Tile*>(edit_tile);
	Waypoint* current_wp = mutable_map->waypoints.getWaypoint(mutable_tile->getLocation());

	if (current_wp) {
		std::string old_name = current_wp->name;
		if (new_name.empty()) {
			// User cleared the name, remove the waypoint
			mutable_map->waypoints.removeWaypoint(old_name);
			g_gui.RefreshPalettes();
		} else if (old_name != new_name) {
			// Rename the waypoint
			mutable_map->waypoints.removeWaypoint(old_name);

			Waypoint* nwp = newd Waypoint(new_name, edit_tile->getPosition());
			mutable_map->waypoints.addWaypoint(nwp);
			g_gui.RefreshPalettes();
		}
	} else {
		if (!new_name.empty()) {
			Waypoint* nwp = newd Waypoint(new_name, edit_tile->getPosition());
			mutable_map->waypoints.addWaypoint(nwp);
			g_gui.RefreshPalettes();
		}
	}
}
