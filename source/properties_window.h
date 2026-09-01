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

#ifndef RME_PROPERTIES_WINDOW_H_
#define RME_PROPERTIES_WINDOW_H_

#include "main.h"
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/grid.h>
#include <wx/notebook.h>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>

class ContainerItemButton;
class ItemAttribute;
class Tile;
class Item;
class Map;

class PropertiesWindow : public wxDialog {
public:
  PropertiesWindow(wxWindow *parent, const Map *map, const Tile *tile,
                   Item *item, wxPoint position = wxDefaultPosition);
  virtual ~PropertiesWindow();

  void OnClickOK(wxCommandEvent &event);
  void OnClickCancel(wxCommandEvent &event);
  void OnClose(wxCloseEvent &event);
  void OnClickAddAttribute(wxCommandEvent &event);
  void OnClickRemoveAttribute(wxCommandEvent &event);
  void OnClickTown(wxCommandEvent &event);
  void OnGridValueChanged(wxGridEvent &event);

  void Update();

  Item *getItemBeingEdited() { return edit_item; }

protected:
  const Map *edit_map = nullptr;
  const Tile *edit_tile = nullptr;
  Item *edit_item = nullptr;

  // Panels
  wxWindow *createGeneralPanel(wxWindow *parent);
  void saveGeneralPanel();

  std::vector<ContainerItemButton *> container_items;
  wxWindow *createContainerPanel(wxWindow *parent);
  void saveContainerPanel();

  wxChoice *door_type_choice = nullptr;
  wxSpinCtrl *door_req_level = nullptr;
  wxSpinCtrl *door_action_id = nullptr;
  wxSpinCtrl *door_storage_key = nullptr;
  wxWindow *createDoorSpecialPanel(wxWindow *parent);
  void saveDoorSpecialPanel();

  wxChoice *chest_mode_choice = nullptr;
  wxPanel *chest_quest_panel = nullptr;
  wxSpinCtrl *chest_req_level = nullptr;
  wxSpinCtrl *chest_action_id = nullptr;
  wxTextCtrl *chest_reward_msg = nullptr;
  void saveContainerSpecialPanel();

  wxSpinCtrl *tele_dest_x = nullptr;
  wxSpinCtrl *tele_dest_y = nullptr;
  wxSpinCtrl *tele_dest_z = nullptr;
  wxWindow *createTeleportSpecialPanel(wxWindow *parent);
  void saveTeleportSpecialPanel();

  wxGrid *attributesGrid = nullptr;
  wxWindow *createAttributesPanel(wxWindow *parent);
  void saveAttributesPanel();
  void SetGridValue(wxGrid *grid, int rowIndex, std::string name,
                    const ItemAttribute &attr);

  wxTextCtrl *waypoint_name_field = nullptr;
  wxWindow *createWaypointPanel(wxWindow *parent);
  void saveWaypointPanel();
  bool validateWaypointPanel();

protected:
  wxNotebook *notebook = nullptr;

  wxSpinCtrl *action_id_field = nullptr;
  wxSpinCtrl *unique_id_field = nullptr;
  wxChoice *ore_type_choice = nullptr;
  wxSpinCtrl *count_field = nullptr;
  wxTextCtrl *text_field = nullptr;
  wxChoice *depot_town_field = nullptr;
  wxCheckBox *locked_door_checkbox = nullptr;

  DECLARE_EVENT_TABLE()
};

#endif
