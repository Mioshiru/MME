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
#include "style_manager.h"

#include "palette_house.h"

#include "settings.h"

#include "brush.h"
#include "editor.h"
#include "map.h"

#include "application.h"
#include "map_display.h"

#include "house_brush.h"
#include "house_exit_brush.h"
#include "house_wizard_dialog.h"
#include "spawn_brush.h"
#include "common_windows.h"

// Menu command IDs for house palette context menu
enum {
	PALETTE_HOUSE_CMD_ADD_QUICK = 10510,
	PALETTE_HOUSE_CMD_ADD_WIZARD = 10511,
	PALETTE_HOUSE_CMD_EDIT_PROPS = 10512,
	PALETTE_HOUSE_CMD_EDIT_WIZARD = 10513,
	PALETTE_HOUSE_CMD_DELETE = 10514,
	PALETTE_HOUSE_CMD_CLEAR_TILES = 10515,
	PALETTE_HOUSE_CMD_HOUSE_BRUSH = 10516,
	PALETTE_HOUSE_CMD_SET_EXIT = 10517,
	PALETTE_HOUSE_CMD_GOTO = 10518,
	PALETTE_HOUSE_CMD_MANAGE_TOWNS = 10519,
	PALETTE_HOUSE_CMD_CLEAR_INVALID_TILES = 10520,
};

// ============================================================================
// House palette

BEGIN_EVENT_TABLE(HousePalettePanel, PalettePanel)
EVT_TIMER(PALETTE_LAYOUT_FIX_TIMER, HousePalettePanel::OnLayoutFixTimer)

EVT_CHOICE(PALETTE_HOUSE_TOWN_CHOICE, HousePalettePanel::OnTownChange)

EVT_LISTBOX(PALETTE_HOUSE_LISTBOX, HousePalettePanel::OnListBoxChange)
EVT_LISTBOX_DCLICK(PALETTE_HOUSE_LISTBOX, HousePalettePanel::OnListBoxDoubleClick)

EVT_BUTTON(PALETTE_HOUSE_ADD_HOUSE, HousePalettePanel::OnClickAddHouse)
EVT_BUTTON(PALETTE_HOUSE_EDIT_HOUSE, HousePalettePanel::OnClickEditHouse)
EVT_BUTTON(PALETTE_HOUSE_REMOVE_HOUSE, HousePalettePanel::OnClickRemoveHouse)
EVT_MENU(PALETTE_HOUSE_ADD_HOUSE, HousePalettePanel::OnClickAddHouse)
EVT_MENU(PALETTE_HOUSE_EDIT_HOUSE, HousePalettePanel::OnClickEditHouse)
EVT_MENU(PALETTE_HOUSE_REMOVE_HOUSE, HousePalettePanel::OnClickRemoveHouse)
EVT_TOGGLEBUTTON(PALETTE_HOUSE_BRUSH_BUTTON, HousePalettePanel::OnClickHouseBrushButton)
EVT_TOGGLEBUTTON(PALETTE_HOUSE_SET_EXIT_BUTTON, HousePalettePanel::OnClickSetExit)
END_EVENT_TABLE()

HousePalettePanel::HousePalettePanel(wxWindow* parent, wxWindowID id) :
	PalettePanel(parent, id),
	map(nullptr),
	is_exit_mode(false),
	do_resize_on_display(true),
	fix_size_timer(this, PALETTE_LAYOUT_FIX_TIMER) {
	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	wxSizer* sidesizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Houses");
	town_choice = newd wxChoice(this, PALETTE_HOUSE_TOWN_CHOICE, wxDefaultPosition, wxDefaultSize, (int)0, (const wxString*)nullptr);
	sidesizer->Add(town_choice, 0, wxEXPAND | wxBOTTOM, 4);

	house_list = newd SortableListBox(this, PALETTE_HOUSE_LISTBOX);
	house_list->Bind(wxEVT_CONTEXT_MENU, &HousePalettePanel::OnListBoxContextMenu, this);
	this->Bind(wxEVT_CONTEXT_MENU, &HousePalettePanel::OnListBoxContextMenu, this);
	house_list->Bind(wxEVT_KEY_DOWN, [this](wxKeyEvent& event) {
		if (event.GetKeyCode() == WXK_DELETE || event.GetKeyCode() == WXK_NUMPAD_DELETE) {
			wxCommandEvent ev;
			OnClickRemoveHouse(ev);
		} else {
			event.Skip();
		}
	});
#ifdef __APPLE__
	// Used for detecting a deselect
	house_list->Bind(wxEVT_LEFT_UP, &HousePalettePanel::OnListBoxClick, this);
#endif
	sidesizer->Add(house_list, 1, wxEXPAND);

	add_house_button = nullptr;
	edit_house_button = nullptr;
	remove_house_button = nullptr;
	house_brush_button = nullptr;
	set_exit_button = nullptr;
	auto_mode_checkbox = nullptr;

	topsizer->Add(sidesizer, 1, wxEXPAND);

	SetSizerAndFit(topsizer);
	RME::UI::StyleManager::ApplyThemeRecursively(this, RME::UI::StyleManager::GetTheme());
}

HousePalettePanel::~HousePalettePanel() {
	////
}

void HousePalettePanel::SetMap(Map* m) {
	if (g_gui.house_brush) {
		g_gui.house_brush->setHouse(nullptr);
	}
	if (g_gui.house_exit_brush) {
		g_gui.house_exit_brush->setHouse(nullptr);
	}
	map = m;
	OnUpdate();
}

void HousePalettePanel::OnSwitchIn() {
	PalettePanel::OnSwitchIn();
	// Layout fix hack
	if (do_resize_on_display) {
		fix_size_timer.Start(100, true);
		do_resize_on_display = false;
	}
}

void HousePalettePanel::OnLayoutFixTimer(wxTimerEvent& WXUNUSED(event)) {
	wxWindow* w = this;
	while ((w = w->GetParent()) && dynamic_cast<PaletteWindow*>(w) == nullptr)
		;

	if (w) {
		w->SetSize(w->GetSize().GetWidth(), w->GetSize().GetHeight() + 1);
		w->SetSize(w->GetSize().GetWidth(), w->GetSize().GetHeight() - 1);
	}
}

void HousePalettePanel::SelectFirstBrush() {
	SelectHouseBrush();
}

Brush* HousePalettePanel::GetSelectedBrush() const {
	if (is_exit_mode && g_gui.house_exit_brush) {
		House* house = g_gui.house_exit_brush->getHouse();
		if (!house) house = GetCurrentlySelectedHouse();
		if (house) {
			g_gui.house_exit_brush->setHouse(house);
			return g_gui.house_exit_brush;
		}
	} else if (g_gui.house_brush) {
		House* house = g_gui.house_brush->getHouse();
		if (!house) house = GetCurrentlySelectedHouse();
		if (house) {
			g_gui.house_brush->setHouse(house);
			return g_gui.house_brush;
		}
	}
	return nullptr;
}

bool HousePalettePanel::SelectBrush(const Brush* whatbrush) {
	if (!whatbrush || !town_choice || !house_list) {
		return false;
	}

	if (whatbrush->isHouse() && map) {
		const HouseBrush* house_brush = static_cast<const HouseBrush*>(whatbrush);
		for (HouseMap::iterator house_iter = map->houses.begin(); house_iter != map->houses.end(); ++house_iter) {
			if (house_iter->second && house_iter->second->getID() == house_brush->getHouseID()) {
				for (uint32_t i = 0; i < town_choice->GetCount(); ++i) {
					Town* town = reinterpret_cast<Town*>(town_choice->GetClientData(i));
					bool is_no_town_option = (town == nullptr);
					bool house_has_no_valid_town = (map->towns.getTown(house_iter->second->townid) == nullptr);

					if ((is_no_town_option && house_has_no_valid_town) || (town != nullptr && town->getID() == house_iter->second->townid)) {
						SelectTown(i);
						for (uint32_t j = 0; j < house_list->GetCount(); ++j) {
							if (house_iter->second->getID() == reinterpret_cast<House*>(house_list->GetClientData(j))->getID()) {
								SelectHouse(j);
								return true;
							}
						}
						return true;
					}
				}
			}
		}
	} else if (whatbrush->isHouseExit()) {
		SelectExitBrush();
		return true;
	}
	return false;
}

int HousePalettePanel::GetSelectedBrushSize() const {
	return 0;
}

PaletteType HousePalettePanel::GetType() const {
	return TILESET_HOUSE;
}

void HousePalettePanel::SelectTown(size_t index) {
	if (map == nullptr || !town_choice || town_choice->GetCount() == 0 || index >= town_choice->GetCount()) {
		if (add_house_button) {
			add_house_button->Enable(false);
		}
		return;
	}

	Town* what_town = reinterpret_cast<Town*>(town_choice->GetClientData(index));

	// Clear the old houselist
	if (house_list) {
		house_list->Clear();
	}

	for (HouseMap::iterator house_iter = map->houses.begin(); house_iter != map->houses.end(); ++house_iter) {
		if (house_iter->second == nullptr) {
			continue;
		}
		if (what_town) {
			if (house_iter->second->townid == what_town->getID()) {
				if (house_list) {
					house_list->Append(wxstr(house_iter->second->getDescription()), house_iter->second);
				}
			}
		} else {
			// "No Town" selected!
			if (map->towns.getTown(house_iter->second->townid) == nullptr) {
				if (house_list) {
					house_list->Append(wxstr(house_iter->second->getDescription()), house_iter->second);
				}
			}
		}
	}
	if (house_list) {
		house_list->Sort();
	}

	// Select first house
	SelectHouse(0);
	town_choice->SetSelection(index);
	if (add_house_button) {
		add_house_button->Enable(what_town != nullptr);
	}
}

void HousePalettePanel::SelectHouse(size_t index) {
	if (!house_list || house_list->GetCount() == 0 || index >= house_list->GetCount()) {
		if (edit_house_button) edit_house_button->Enable(false);
		if (remove_house_button) remove_house_button->Enable(false);
		if (set_exit_button) {
			set_exit_button->Enable(false);
			set_exit_button->SetValue(false);
		}
		if (house_brush_button) {
			house_brush_button->Enable(false);
			house_brush_button->SetValue(false);
		}
		is_exit_mode = false;
		g_gui.RefreshView();
		return;
	}

	if (edit_house_button) edit_house_button->Enable(true);
	if (remove_house_button) remove_house_button->Enable(true);
	if (set_exit_button) set_exit_button->Enable(true);
	if (house_brush_button) house_brush_button->Enable(true);

	house_list->SetSelection(index);
	House* house = GetCurrentlySelectedHouse();
	if (house) {
		if (g_gui.house_brush) {
			g_gui.house_brush->setHouse(house);
		}
		if (g_gui.house_exit_brush) {
			g_gui.house_exit_brush->setHouse(house);
		}
	}
	g_gui.RefreshView();
}

House* HousePalettePanel::GetCurrentlySelectedHouse() const {
	if (!house_list || house_list->GetCount() == 0) {
		return nullptr;
	}
	int selection = house_list->GetSelection();
	if (selection != wxNOT_FOUND && selection >= 0 && (size_t)selection < house_list->GetCount()) {
		return reinterpret_cast<House*>(house_list->GetClientData(selection));
	}
	return nullptr;
}

void HousePalettePanel::SelectHouseBrush() {
	is_exit_mode = false;
	if (set_exit_button) {
		set_exit_button->SetValue(false);
	}
	if (house_brush_button) {
		house_brush_button->SetValue(true);
	}
}

void HousePalettePanel::SelectExitBrush() {
	is_exit_mode = true;
	if (set_exit_button) {
		set_exit_button->SetValue(true);
	}
	if (house_brush_button) {
		house_brush_button->SetValue(false);
	}
}

void HousePalettePanel::OnUpdate() {
	ScopedAction action("HousePalettePanel::OnUpdate");
	if (!town_choice || !house_list) {
		return;
	}

	int old_town_selection = town_choice->GetSelection();

	town_choice->Clear();
	house_list->Clear();

	if (map == nullptr) {
		return;
	}

	if (map->towns.count() != 0) {
		// Create choice control
		for (TownMap::iterator town_iter = map->towns.begin(); town_iter != map->towns.end(); ++town_iter) {
			town_choice->Append(wxstr(town_iter->second->getName()), town_iter->second);
		}
		town_choice->Append("No Town", (void*)(nullptr));
		if (old_town_selection <= 0) {
			SelectTown(0);
		} else if ((size_t)old_town_selection < town_choice->GetCount()) {
			SelectTown(old_town_selection);
		} else {
			SelectTown(old_town_selection - 1);
		}

		house_list->Enable(true);
		if (add_house_button) add_house_button->Enable(true);
	} else {
		town_choice->Append("No Town", (void*)(nullptr));
		if (set_exit_button) {
			set_exit_button->Enable(false);
			set_exit_button->SetValue(false);
		}
		if (house_brush_button) {
			house_brush_button->Enable(false);
			house_brush_button->SetValue(false);
		}
		is_exit_mode = false;
		if (add_house_button) add_house_button->Enable(false);
		if (edit_house_button) edit_house_button->Enable(false);
		if (remove_house_button) remove_house_button->Enable(false);

		SelectTown(0);
	}
}

void HousePalettePanel::OnTownChange(wxCommandEvent& event) {
	if (town_choice && (size_t)event.GetSelection() < town_choice->GetCount()) {
		SelectTown(event.GetSelection());
		g_gui.SelectBrush();
	}
}

void HousePalettePanel::OnListBoxChange(wxCommandEvent& event) {
	if (house_list && (size_t)event.GetSelection() < house_list->GetCount()) {
		SelectHouse(event.GetSelection());
		g_gui.SelectBrush();
	}
}

void HousePalettePanel::OnListBoxDoubleClick(wxCommandEvent& event) {
	if (!house_list || house_list->GetCount() == 0 || !map) {
		return;
	}
	int selection = house_list->GetSelection();
	if (selection == wxNOT_FOUND || (size_t)selection >= house_list->GetCount()) {
		return;
	}
	House* house = reinterpret_cast<House*>(house_list->GetClientData(selection));
	if (house) {
		SelectHouse(selection);
		g_gui.SelectBrush();

		Position target_pos = house->getExit();
		if (!target_pos.isValid() || target_pos == Position(0, 0, 0)) {
			target_pos = house->getFirstTilePosition();
		}
		if (target_pos.isValid() && target_pos != Position(0, 0, 0)) {
			g_gui.SetScreenCenterPosition(target_pos);
		}
	}
}

void HousePalettePanel::OnListBoxContextMenu(wxContextMenuEvent& event) {
	if (!house_list || !map) {
		return;
	}

	wxPoint mousePos = event.GetPosition();
	wxPoint clientPos;
	if (mousePos == wxDefaultPosition || (mousePos.x == -1 && mousePos.y == -1)) {
		clientPos = wxPoint(10, 10);
	} else {
		clientPos = house_list->ScreenToClient(mousePos);
	}

	int itemIndex = wxNOT_FOUND;
	if (clientPos.x >= 0 && clientPos.y >= 0 && clientPos.y < house_list->GetSize().y) {
		itemIndex = house_list->HitTest(clientPos);
	}
	if (itemIndex == wxNOT_FOUND) {
		itemIndex = house_list->GetSelection();
	}

	wxMenu menu;
	menu.Append(PALETTE_HOUSE_CMD_ADD_QUICK, "New");

	if (itemIndex != wxNOT_FOUND && (size_t)itemIndex < house_list->GetCount()) {
		SelectHouse(itemIndex);
		g_gui.SelectBrush();

		menu.Append(PALETTE_HOUSE_CMD_EDIT_PROPS, "Edit");
		menu.Append(PALETTE_HOUSE_CMD_DELETE, "Delete");
	}

	menu.AppendSeparator();
	menu.Append(PALETTE_HOUSE_CMD_MANAGE_TOWNS, "Towns");
	menu.Append(PALETTE_HOUSE_CMD_CLEAR_INVALID_TILES, "Clear invalid House Tiles");

	int chosenId = house_list->GetPopupMenuSelectionFromUser(menu, clientPos);
	if (chosenId == wxID_NONE) {
		return;
	}

	switch (chosenId) {
		case PALETTE_HOUSE_CMD_ADD_QUICK: {
			wxCommandEvent e;
			OnClickAddHouseQuick(e);
			break;
		}
		case PALETTE_HOUSE_CMD_ADD_WIZARD: {
			wxCommandEvent e;
			OnClickAddHouseWizard(e);
			break;
		}
		case PALETTE_HOUSE_CMD_EDIT_PROPS: {
			wxCommandEvent e;
			OnClickEditHouse(e);
			break;
		}
		case PALETTE_HOUSE_CMD_EDIT_WIZARD: {
			wxCommandEvent e;
			OnClickEditHouseWizard(e);
			break;
		}
		case PALETTE_HOUSE_CMD_DELETE: {
			wxCommandEvent e;
			OnClickRemoveHouse(e);
			break;
		}
		case PALETTE_HOUSE_CMD_CLEAR_TILES: {
			wxCommandEvent e;
			OnClickClearHouseTiles(e);
			break;
		}
		case PALETTE_HOUSE_CMD_HOUSE_BRUSH: {
			SelectHouseBrush();
			g_gui.SelectBrush();
			g_gui.SetStatusText("Click and drag on the map to paint house tiles.");
			break;
		}
		case PALETTE_HOUSE_CMD_SET_EXIT: {
			SelectExitBrush();
			g_gui.SelectBrush();
			g_gui.SetStatusText("Click on the map to place the house exit doorway.");
			break;
		}
		case PALETTE_HOUSE_CMD_GOTO: {
			if (itemIndex != wxNOT_FOUND && (size_t)itemIndex < house_list->GetCount()) {
				House* h = reinterpret_cast<House*>(house_list->GetClientData(itemIndex));
				if (h) {
					Position p = h->getExit();
					if (!p.isValid() || p == Position(0, 0, 0)) p = h->getFirstTilePosition();
					if (p.isValid() && p != Position(0, 0, 0)) g_gui.SetScreenCenterPosition(p);
				}
			}
			break;
		}
		case PALETTE_HOUSE_CMD_MANAGE_TOWNS: {
			Editor* editor = g_gui.GetCurrentEditor();
			if (editor) {
				uint32_t sel_town_id = 0;
				if (town_choice && town_choice->GetSelection() != wxNOT_FOUND) {
					Town* t = reinterpret_cast<Town*>(town_choice->GetClientData(town_choice->GetSelection()));
					if (t) sel_town_id = t->getID();
				}
				EditTownsDialog dlg(this, *editor, sel_town_id);
				dlg.ShowModal();
				OnUpdate();
			}
			break;
		}
		case PALETTE_HOUSE_CMD_CLEAR_INVALID_TILES: {
			Editor* editor = g_gui.GetCurrentEditor();
			if (editor) {
				int ret = g_gui.PopupDialog("Clear Invalid House Tiles", "Are you sure you want to remove all house tiles that do not belong to a house (this action cannot be undone)?", wxYES | wxNO);
				if (ret == wxID_YES) {
					editor->clearInvalidHouseTiles(true);
					g_gui.RefreshView();
				}
			}
			break;
		}
		default:
			break;
	}
}

void HousePalettePanel::OnClickHouseBrushButton(wxCommandEvent& event) {
	SelectHouseBrush();
	g_gui.SelectBrush();
}

void HousePalettePanel::OnClickSetExit(wxCommandEvent& event) {
	if (set_exit_button && set_exit_button->GetValue()) {
		SelectExitBrush();
	} else {
		SelectHouseBrush();
	}
	g_gui.SelectBrush();
}

void HousePalettePanel::OnClickAddHouse(wxCommandEvent& WXUNUSED(event)) {
	if (map == nullptr || !town_choice || !house_list) {
		return;
	}

	uint32_t default_town_id = 0;
	if (town_choice->GetSelection() != wxNOT_FOUND) {
		Town* town = reinterpret_cast<Town*>(town_choice->GetClientData(town_choice->GetSelection()));
		if (town) {
			default_town_id = town->getID();
		}
	}

	MapTab* mt = g_gui.GetCurrentMapTab();
	if (mt && mt->GetCanvas()) {
		mt->GetCanvas()->StartHouseCreationFlow(default_town_id);
	}
}

void HousePalettePanel::OnClickAddHouseQuick(wxCommandEvent& event) {
	OnClickAddHouse(event);
}

void HousePalettePanel::OnClickAddHouseWizard(wxCommandEvent& event) {
	OnClickAddHouse(event);
}

void HousePalettePanel::OnClickEditHouse(wxCommandEvent& event) {
	if (!house_list || house_list->GetCount() == 0 || !town_choice) {
		return;
	}
	if (map == nullptr) {
		return;
	}
	int selection = house_list->GetSelection();
	if (selection == wxNOT_FOUND || (size_t)selection >= house_list->GetCount()) {
		return;
	}
	House* house = reinterpret_cast<House*>(house_list->GetClientData(selection));
	if (house) {
		EditHouseDialog dialog(this, map, house);
		if (dialog.ShowModal() == 1) {
			house_list->SetString(selection, wxstr(house->getDescription()));
			house_list->Sort();
			map->doChange();

			SelectTown(town_choice->GetSelection());
			int idx = house_list->FindString(wxstr(house->getDescription()));
			if (idx != wxNOT_FOUND) {
				SelectHouse(idx);
			}
			g_gui.SelectBrush();
			refresh_timer.Start(300, true);
		}
	}
}

void HousePalettePanel::OnClickEditHouseWizard(wxCommandEvent& event) {
	OnClickEditHouse(event);
}

void HousePalettePanel::OnClickRemoveHouse(wxCommandEvent& event) {
	if (!house_list || map == nullptr) {
		return;
	}
	int selection = house_list->GetSelection();
	if (selection != wxNOT_FOUND && (size_t)selection < house_list->GetCount()) {
		House* house = reinterpret_cast<House*>(house_list->GetClientData(selection));
		if (!house) return;

		int ret = g_gui.PopupDialog(this, "Delete House", wxString::Format("Are you sure you want to delete house \"%s\" (ID: %u)?\nAll assigned tiles will be cleared.", wxstr(house->name), house->getID()), wxYES | wxNO);
		if (ret != wxID_YES) {
			return;
		}

		map->houses.removeHouse(house);
		map->doChange();
		house_list->Delete(selection);
		refresh_timer.Start(300, true);

		if (int(house_list->GetCount()) <= selection) {
			selection -= 1;
		}

		if (selection >= 0 && house_list->GetCount()) {
			house_list->SetSelection(selection);
		} else {
			if (set_exit_button) {
				set_exit_button->Enable(false);
				set_exit_button->SetValue(false);
			}
			if (house_brush_button) {
				house_brush_button->Enable(false);
				house_brush_button->SetValue(false);
			}
			is_exit_mode = false;
			if (edit_house_button) edit_house_button->Enable(false);
			if (remove_house_button) remove_house_button->Enable(false);
		}
		g_gui.SelectBrush();
	}
	g_gui.RefreshView();
}

void HousePalettePanel::OnClickClearHouseTiles(wxCommandEvent& event) {
	if (!house_list || map == nullptr) {
		return;
	}
	int selection = house_list->GetSelection();
	if (selection == wxNOT_FOUND || (size_t)selection >= house_list->GetCount()) {
		return;
	}
	House* house = reinterpret_cast<House*>(house_list->GetClientData(selection));
	if (!house) return;

	int ret = g_gui.PopupDialog(this, "Clear House Tiles", wxString::Format("Are you sure you want to unassign all tiles and exit for house \"%s\" (ID: %u)?", wxstr(house->name), house->getID()), wxYES | wxNO);
	if (ret != wxID_YES) {
		return;
	}

	house->clean();
	map->doChange();
	g_gui.RefreshView();
	refresh_timer.Start(300, true);
}

#ifdef __APPLE__
void HousePalettePanel::OnListBoxClick(wxMouseEvent& event) {
	if (house_list->GetSelection() == wxNOT_FOUND) {
		if (set_exit_button) {
			set_exit_button->Enable(false);
			set_exit_button->SetValue(false);
		}
		if (house_brush_button) {
			house_brush_button->Enable(false);
			house_brush_button->SetValue(false);
		}
		is_exit_mode = false;
		edit_house_button->Enable(false);
		remove_house_button->Enable(false);
		g_gui.SelectBrush();
	}
}
#endif

// ============================================================================
// House Edit Dialog

BEGIN_EVENT_TABLE(EditHouseDialog, wxDialog)
EVT_SET_FOCUS(EditHouseDialog::OnFocusChange)
EVT_BUTTON(ID_HOUSE_RANDOM_NAME, EditHouseDialog::OnClickRandomName)
EVT_BUTTON(wxID_OK, EditHouseDialog::OnClickOK)
EVT_BUTTON(wxID_CANCEL, EditHouseDialog::OnClickCancel)
END_EVENT_TABLE()

EditHouseDialog::EditHouseDialog(wxWindow* parent, Map* map, House* house) :
	wxDialog(parent, wxID_ANY, "House Properties", wxDefaultPosition, wxSize(520, 390), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	map(map),
	what_house(house),
	name_field(nullptr),
	town_id_field(nullptr),
	id_field(nullptr),
	rent_field(nullptr),
	guildhall_field(nullptr) {
	ASSERT(map);
	ASSERT(what_house);

	SetBackgroundColour(wxColour(16, 24, 38));
	SetForegroundColour(wxColour(240, 245, 255));

	wxFont default_font(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Arial");
	wxFont bold_font(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, "Arial");
	wxFont title_font(13, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, "Arial");

	SetFont(default_font);

	wxBoxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	// Header Banner Box
	wxPanel* header_panel = newd wxPanel(this, wxID_ANY);
	header_panel->SetBackgroundColour(wxColour(12, 18, 30));
	wxBoxSizer* header_sizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticText* title_lbl = newd wxStaticText(header_panel, wxID_ANY, "House Properties");
	title_lbl->SetFont(title_font);
	title_lbl->SetForegroundColour(wxColour(255, 215, 80));
	header_sizer->Add(title_lbl, 0, wxBOTTOM, 4);

	wxStaticText* subtitle_lbl = newd wxStaticText(header_panel, wxID_ANY,
		"Configure the house name, town, rent and guildhall settings.");
	subtitle_lbl->SetFont(default_font);
	subtitle_lbl->SetForegroundColour(wxColour(180, 195, 215));
	header_sizer->Add(subtitle_lbl, 0, wxEXPAND);

	header_panel->SetSizer(header_sizer);
	topsizer->Add(header_panel, 0, wxEXPAND | wxALL, 12);

	// Form Grid (5 rows, 2 columns with ample spacing)
	wxFlexGridSizer* grid = newd wxFlexGridSizer(5, 2, 12, 14);
	grid->AddGrowableCol(1);

	house_name = wxString::FromUTF8(what_house->name);
	house_id = wxString::Format("%u", what_house->getID());
	house_rent = wxString::Format("%u", what_house->rent);

	// 1. House Name with Random button
	wxStaticText* name_lbl = newd wxStaticText(this, wxID_ANY, "Name:");
	name_lbl->SetFont(bold_font);
	name_lbl->SetForegroundColour(wxColour(230, 235, 245));
	grid->Add(name_lbl, 0, wxALIGN_CENTER_VERTICAL);

	wxBoxSizer* name_row = newd wxBoxSizer(wxHORIZONTAL);
	name_field = newd wxTextCtrl(this, wxID_ANY, house_name, wxDefaultPosition, wxDefaultSize, 0, wxTextValidator(wxFILTER_ASCII, &house_name));
	name_field->SetFont(default_font);
	name_field->SetBackgroundColour(wxColour(10, 15, 26));
	name_field->SetForegroundColour(wxColour(245, 245, 255));
	name_row->Add(name_field, 1, wxEXPAND | wxRIGHT, 8);

	wxButton* random_btn = newd wxButton(this, ID_HOUSE_RANDOM_NAME, "Random", wxDefaultPosition, wxSize(80, 28));
	random_btn->SetFont(default_font);
	random_btn->SetBackgroundColour(wxColour(30, 60, 110));
	random_btn->SetForegroundColour(wxColour(255, 225, 120));
	random_btn->SetToolTip("Generate a random English fantasy house name");
	name_row->Add(random_btn, 0, wxALIGN_CENTER_VERTICAL);
	grid->Add(name_row, 1, wxEXPAND);

	// 2. Town Selection
	wxStaticText* town_lbl = newd wxStaticText(this, wxID_ANY, "Town / City:");
	town_lbl->SetFont(bold_font);
	town_lbl->SetForegroundColour(wxColour(230, 235, 245));
	grid->Add(town_lbl, 0, wxALIGN_CENTER_VERTICAL);

	const Towns& towns = map->towns;
	town_id_field = newd wxChoice(this, wxID_ANY);
	town_id_field->SetFont(default_font);
	town_id_field->SetBackgroundColour(wxColour(10, 15, 26));
	town_id_field->SetForegroundColour(wxColour(245, 245, 255));

	int to_select_index = 0;
	uint32_t houseTownId = what_house->townid;

	if (towns.count() > 0) {
		bool found = false;
		for (TownMap::const_iterator town_iter = towns.begin(); town_iter != towns.end(); ++town_iter) {
			if (town_iter->second->getID() == houseTownId) {
				found = true;
			}
			town_id_field->Append(wxString::FromUTF8(town_iter->second->getName()), newd int(town_iter->second->getID()));
			if (!found) {
				++to_select_index;
			}
		}

		if (!found) {
			if (houseTownId != 0) {
				town_id_field->Append("Undefined Town (id:" + wxString::Format("%u", houseTownId) + ")", newd int(houseTownId));
				++to_select_index;
			}
		}
	} else {
		town_id_field->Append("No Town", newd int(0));
	}
	town_id_field->SetSelection(to_select_index);
	grid->Add(town_id_field, 1, wxEXPAND);

	// 3. House ID
	wxStaticText* id_lbl = newd wxStaticText(this, wxID_ANY, "House ID:");
	id_lbl->SetFont(bold_font);
	id_lbl->SetForegroundColour(wxColour(230, 235, 245));
	grid->Add(id_lbl, 0, wxALIGN_CENTER_VERTICAL);

	id_field = newd wxSpinCtrl(this, wxID_ANY, house_id, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 999999, what_house->getID());
	id_field->SetFont(default_font);
	id_field->SetBackgroundColour(wxColour(10, 15, 26));
	id_field->SetForegroundColour(wxColour(245, 245, 255));
	grid->Add(id_field, 1, wxEXPAND);

	// 4. Rent
	wxStaticText* rent_lbl = newd wxStaticText(this, wxID_ANY, "Rent (Gold):");
	rent_lbl->SetFont(bold_font);
	rent_lbl->SetForegroundColour(wxColour(230, 235, 245));
	grid->Add(rent_lbl, 0, wxALIGN_CENTER_VERTICAL);

	rent_field = newd wxTextCtrl(this, wxID_ANY, house_rent, wxDefaultPosition, wxDefaultSize, 0, wxTextValidator(wxFILTER_NUMERIC, &house_rent));
	rent_field->SetFont(default_font);
	rent_field->SetBackgroundColour(wxColour(10, 15, 26));
	rent_field->SetForegroundColour(wxColour(245, 245, 255));
	grid->Add(rent_field, 1, wxEXPAND);

	// 5. Guildhall Checkbox
	wxStaticText* gh_lbl = newd wxStaticText(this, wxID_ANY, "Guildhall:");
	gh_lbl->SetFont(bold_font);
	gh_lbl->SetForegroundColour(wxColour(230, 235, 245));
	grid->Add(gh_lbl, 0, wxALIGN_CENTER_VERTICAL);

	guildhall_field = newd wxCheckBox(this, wxID_ANY, "Designate this house as a Guildhall");
	guildhall_field->SetFont(default_font);
	guildhall_field->SetForegroundColour(wxColour(200, 215, 235));
	guildhall_field->SetValue(what_house->guildhall);
	grid->Add(guildhall_field, 0, wxALIGN_CENTER_VERTICAL);

	topsizer->Add(grid, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 14);

	// OK/Cancel buttons
	wxBoxSizer* buttonsSizer = newd wxBoxSizer(wxHORIZONTAL);
	buttonsSizer->AddStretchSpacer();

	wxButton* ok_btn = newd wxButton(this, wxID_OK, "OK", wxDefaultPosition, wxSize(100, 30));
	ok_btn->SetFont(bold_font);
	ok_btn->SetBackgroundColour(wxColour(25, 80, 150));
	ok_btn->SetForegroundColour(wxColour(255, 225, 120));
	buttonsSizer->Add(ok_btn, 0, wxRIGHT, 8);

	wxButton* cancel_btn = newd wxButton(this, wxID_CANCEL, "Cancel", wxDefaultPosition, wxSize(90, 30));
	cancel_btn->SetFont(default_font);
	cancel_btn->SetBackgroundColour(wxColour(30, 40, 60));
	cancel_btn->SetForegroundColour(wxColour(210, 220, 235));
	buttonsSizer->Add(cancel_btn, 0);

	topsizer->Add(buttonsSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 14);

	SetSizerAndFit(topsizer);
	SetMinSize(GetSize());
	RME::UI::StyleManager::ApplyThemeRecursively(this, RME::UI::StyleManager::GetTheme());
	Centre(wxBOTH);
}

EditHouseDialog::~EditHouseDialog() {
	if (town_id_field) {
		for (size_t i = 0; i < town_id_field->GetCount(); ++i) {
			int* data = reinterpret_cast<int*>(town_id_field->GetClientData(i));
			delete data;
		}
	}
}

void EditHouseDialog::OnClickRandomName(wxCommandEvent& WXUNUSED(event)) {
	if (name_field) {
		name_field->SetValue(wxString::FromUTF8(HouseWizardDialog::GenerateRandomHouseName()));
	}
}

void EditHouseDialog::OnFocusChange(wxFocusEvent& event) {
	wxWindow* win = event.GetWindow();
	if (wxSpinCtrl* spin = dynamic_cast<wxSpinCtrl*>(win)) {
		spin->SetSelection(-1, -1);
	} else if (wxTextCtrl* text = dynamic_cast<wxTextCtrl*>(win)) {
		text->SetSelection(-1, -1);
	}
}

void EditHouseDialog::OnClickOK(wxCommandEvent& WXUNUSED(event)) {
	if (Validate() && TransferDataFromWindow()) {
		// Verify the new rent information
		long new_house_rent = 0;
		house_rent.ToLong(&new_house_rent);
		if (new_house_rent < 0) {
			g_gui.PopupDialog(this, "Error", "House rent cannot be less than 0.", wxOK);
			return;
		}

		// Verify the new house id
		uint32_t new_house_id = id_field->GetValue();
		if (new_house_id < 1) {
			g_gui.PopupDialog(this, "Error", "House ID cannot be less than 1.", wxOK);
			return;
		}

		// Verify the new house name
		if (house_name.length() == 0) {
			g_gui.PopupDialog(this, "Error", "House name cannot be empty.", wxOK);
			return;
		}

		bool is_existing_house = (map->houses.getHouse(what_house->getID()) == what_house);

		if (g_settings.getInteger(Config::WARN_FOR_DUPLICATE_ID)) {
			Houses& houses = map->houses;
			for (HouseMap::const_iterator house_iter = houses.begin(); house_iter != houses.end(); ++house_iter) {
				House* house = house_iter->second;
				if (!house) continue;

				if (house->getID() == new_house_id && (is_existing_house ? (new_house_id != what_house->getID()) : true)) {
					g_gui.PopupDialog(this, "Error", "This House ID is already in use by another house.", wxOK);
					return;
				}

				if (wxstr(house->name) == house_name && (is_existing_house ? (house->getID() != what_house->getID()) : true)) {
					int ret = g_gui.PopupDialog(this, "Warning", "This house name is already in use. Are you sure you want to continue?", wxYES | wxNO);
					if (ret == wxID_NO) {
						return;
					}
				}
			}
		}

		if (new_house_id != what_house->getID()) {
			if (is_existing_house) {
				int ret = g_gui.PopupDialog(this, "Warning", "Changing existing house IDs on a production server WILL HAVE DATABASE CONSEQUENCES such as potential item loss, house owner change or invalidating guest lists.\n\nAre you sure you want to continue?", wxYES | wxNO);
				if (ret == wxID_NO) {
					return;
				}

				uint32_t old_house_id = what_house->getID();
				map->convertHouseTiles(old_house_id, new_house_id);
				map->houses.changeId(what_house, new_house_id);
			} else {
				what_house->setID(new_house_id);
			}
		}

		// Transfer to house
		uint32_t new_town_id = 0;
		int sel = town_id_field->GetSelection();
		if (sel != wxNOT_FOUND && town_id_field->GetClientData(sel) != nullptr) {
			new_town_id = static_cast<uint32_t>(*reinterpret_cast<int*>(town_id_field->GetClientData(sel)));
		}

		what_house->name = nstr(house_name);
		what_house->rent = new_house_rent;
		what_house->guildhall = guildhall_field->GetValue();
		what_house->townid = new_town_id;

		EndModal(1);
	}
}

void EditHouseDialog::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	// Just close this window
	EndModal(0);
}
