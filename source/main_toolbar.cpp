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
#include "main_toolbar.h"
#include "gui.h"
#include "editor.h"
#include "settings.h"
#include "brush.h"
#include "pngfiles.h"
#include "artprovider.h"
#include <wx/artprov.h>
#include <wx/mstream.h>

const wxString MainToolBar::BRUSHES_BAR_NAME = "brushes_toolbar";
const wxString MainToolBar::POSITION_BAR_NAME = "position_toolbar";
const wxString MainToolBar::SIZES_BAR_NAME = "sizes_toolbar";

#define loadPNGFile(name) _wxGetBitmapFromMemory(name, sizeof(name))
inline wxBitmap* _wxGetBitmapFromMemory(const unsigned char* data, int length) {
	wxMemoryInputStream is(data, length);
	wxImage img(is, "image/png");
	if (!img.IsOk()) {
		return nullptr;
	}
	return newd wxBitmap(img, -1);
}

MainToolBar::MainToolBar(wxWindow* parent, wxAuiManager* manager) {
	wxSize icon_size = FROM_DIP(parent, wxSize(16, 16));

	wxBitmap* border_bitmap = loadPNGFile(optional_border_small_png);
	wxBitmap* eraser_bitmap = loadPNGFile(eraser_small_png);
	wxBitmap pz_bitmap = wxArtProvider::GetBitmap(ART_PZ_BRUSH, wxART_TOOLBAR, icon_size);
	wxBitmap nopvp_bitmap = wxArtProvider::GetBitmap(ART_NOPVP_BRUSH, wxART_TOOLBAR, icon_size);
	wxBitmap nologout_bitmap = wxArtProvider::GetBitmap(ART_NOLOOUT_BRUSH, wxART_TOOLBAR, icon_size);
	wxBitmap pvp_bitmap = wxArtProvider::GetBitmap(ART_PVP_BRUSH, wxART_TOOLBAR, icon_size);
	wxBitmap normal_bitmap = wxArtProvider::GetBitmap(ART_DOOR_NORMAL_SMALL, wxART_TOOLBAR, icon_size);
	wxBitmap locked_bitmap = wxArtProvider::GetBitmap(ART_DOOR_LOCKED_SMALL, wxART_TOOLBAR, icon_size);
	wxBitmap magic_bitmap = wxArtProvider::GetBitmap(ART_DOOR_MAGIC_SMALL, wxART_TOOLBAR, icon_size);
	wxBitmap quest_bitmap = wxArtProvider::GetBitmap(ART_DOOR_QUEST_SMALL, wxART_TOOLBAR, icon_size);
	wxBitmap normal_alt_bitmap = wxArtProvider::GetBitmap(ART_DOOR_NORMAL_ALT_SMALL, wxART_TOOLBAR, icon_size);

	wxBitmap* hatch_bitmap = loadPNGFile(window_hatch_small_png);
	wxBitmap* window_bitmap = loadPNGFile(window_normal_small_png);

	brushes_toolbar = newd wxAuiToolBar(parent, TOOLBAR_BRUSHES, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE);
	brushes_toolbar->SetToolBitmapSize(icon_size);
	brushes_toolbar->AddTool(PALETTE_TERRAIN_OPTIONAL_BORDER_TOOL, wxEmptyString, *border_bitmap, wxNullBitmap, wxITEM_CHECK, "Border", wxEmptyString, NULL);
	brushes_toolbar->AddTool(PALETTE_TERRAIN_ERASER, wxEmptyString, *eraser_bitmap, wxNullBitmap, wxITEM_CHECK, "Eraser", wxEmptyString, NULL);
	brushes_toolbar->AddSeparator();
	brushes_toolbar->AddTool(PALETTE_TERRAIN_PZ_TOOL, wxEmptyString, pz_bitmap, wxNullBitmap, wxITEM_CHECK, "Protected Zone", wxEmptyString, NULL);
	brushes_toolbar->AddTool(PALETTE_TERRAIN_NOPVP_TOOL, wxEmptyString, nopvp_bitmap, wxNullBitmap, wxITEM_CHECK, "No PvP Zone", wxEmptyString, NULL);
	brushes_toolbar->AddTool(PALETTE_TERRAIN_NOLOGOUT_TOOL, wxEmptyString, nologout_bitmap, wxNullBitmap, wxITEM_CHECK, "No Logout Zone", wxEmptyString, NULL);
	brushes_toolbar->AddTool(PALETTE_TERRAIN_PVPZONE_TOOL, wxEmptyString, pvp_bitmap, wxNullBitmap, wxITEM_CHECK, "PvP Zone", wxEmptyString, NULL);
	brushes_toolbar->AddSeparator();

	brushes_toolbar->AddTool(PALETTE_TERRAIN_NORMAL_DOOR, wxEmptyString, normal_bitmap, wxNullBitmap, wxITEM_CHECK, "Normal Door", wxEmptyString, NULL);
	brushes_toolbar->AddTool(PALETTE_TERRAIN_LOCKED_DOOR, wxEmptyString, locked_bitmap, wxNullBitmap, wxITEM_CHECK, "Locked Door", wxEmptyString, NULL);
	brushes_toolbar->AddTool(PALETTE_TERRAIN_MAGIC_DOOR, wxEmptyString, magic_bitmap, wxNullBitmap, wxITEM_CHECK, "Magic Door", wxEmptyString, NULL);
	brushes_toolbar->AddTool(PALETTE_TERRAIN_QUEST_DOOR, wxEmptyString, quest_bitmap, wxNullBitmap, wxITEM_CHECK, "Quest Door", wxEmptyString, NULL);
	brushes_toolbar->AddTool(PALETTE_TERRAIN_NORMAL_ALT_DOOR, wxEmptyString, normal_alt_bitmap, wxNullBitmap, wxITEM_CHECK, "Normal Door (alt)", wxEmptyString, NULL);
	brushes_toolbar->AddSeparator();
	brushes_toolbar->AddTool(PALETTE_TERRAIN_HATCH_DOOR, wxEmptyString, *hatch_bitmap, wxNullBitmap, wxITEM_CHECK, "Hatch Window", wxEmptyString, NULL);
	brushes_toolbar->AddTool(PALETTE_TERRAIN_WINDOW_DOOR, wxEmptyString, *window_bitmap, wxNullBitmap, wxITEM_CHECK, "Window", wxEmptyString, NULL);
	brushes_toolbar->Realize();

	wxBitmap go_bitmap = wxArtProvider::GetBitmap(ART_POSITION_GO, wxART_TOOLBAR, icon_size);

	position_toolbar = newd wxAuiToolBar(parent, TOOLBAR_POSITION, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORZ_TEXT);
	position_toolbar->SetToolBitmapSize(icon_size);
	
	wxArrayString z_levels;
	for (int i = 0; i <= MAP_MAX_LAYER; ++i) {
		z_levels.Add(wxString::Format("Floor %d", i));
	}
	z_choice = newd wxChoice(position_toolbar, wxID_ANY, wxDefaultPosition, FROM_DIP(parent, wxSize(100, -1)), z_levels);
	z_choice->SetToolTip("Select Z-Level (Floor)");
	
	position_toolbar->AddControl(z_choice);
	position_toolbar->Realize();

	wxBitmap circular_bitmap = wxArtProvider::GetBitmap(ART_CIRCULAR, wxART_TOOLBAR, icon_size);
	wxBitmap rectangular_bitmap = wxArtProvider::GetBitmap(ART_RECTANGULAR, wxART_TOOLBAR, icon_size);
	wxBitmap size1_bitmap = wxArtProvider::GetBitmap(ART_RECTANGULAR_1, wxART_TOOLBAR, icon_size);
	wxBitmap size2_bitmap = wxArtProvider::GetBitmap(ART_RECTANGULAR_2, wxART_TOOLBAR, icon_size);
	wxBitmap size3_bitmap = wxArtProvider::GetBitmap(ART_RECTANGULAR_3, wxART_TOOLBAR, icon_size);
	wxBitmap size4_bitmap = wxArtProvider::GetBitmap(ART_RECTANGULAR_4, wxART_TOOLBAR, icon_size);
	wxBitmap size5_bitmap = wxArtProvider::GetBitmap(ART_RECTANGULAR_5, wxART_TOOLBAR, icon_size);
	wxBitmap size6_bitmap = wxArtProvider::GetBitmap(ART_RECTANGULAR_6, wxART_TOOLBAR, icon_size);
	wxBitmap size7_bitmap = wxArtProvider::GetBitmap(ART_RECTANGULAR_7, wxART_TOOLBAR, icon_size);

	sizes_toolbar = newd wxAuiToolBar(parent, TOOLBAR_SIZES, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE);
	sizes_toolbar->SetToolBitmapSize(icon_size);
	sizes_toolbar->AddTool(TOOLBAR_SIZES_RECTANGULAR, wxEmptyString, rectangular_bitmap, wxNullBitmap, wxITEM_CHECK, "Rectangular Brush", wxEmptyString, NULL);
	sizes_toolbar->AddTool(TOOLBAR_SIZES_CIRCULAR, wxEmptyString, circular_bitmap, wxNullBitmap, wxITEM_CHECK, "Circular Brush", wxEmptyString, NULL);
	sizes_toolbar->AddSeparator();
	sizes_toolbar->AddTool(TOOLBAR_SIZES_1, wxEmptyString, size1_bitmap, wxNullBitmap, wxITEM_CHECK, "Size 1", wxEmptyString, NULL);
	sizes_toolbar->AddTool(TOOLBAR_SIZES_2, wxEmptyString, size2_bitmap, wxNullBitmap, wxITEM_CHECK, "Size 2", wxEmptyString, NULL);
	sizes_toolbar->AddTool(TOOLBAR_SIZES_3, wxEmptyString, size3_bitmap, wxNullBitmap, wxITEM_CHECK, "Size 3", wxEmptyString, NULL);
	sizes_toolbar->AddTool(TOOLBAR_SIZES_4, wxEmptyString, size4_bitmap, wxNullBitmap, wxITEM_CHECK, "Size 4", wxEmptyString, NULL);
	sizes_toolbar->AddTool(TOOLBAR_SIZES_5, wxEmptyString, size5_bitmap, wxNullBitmap, wxITEM_CHECK, "Size 5", wxEmptyString, NULL);
	sizes_toolbar->AddTool(TOOLBAR_SIZES_6, wxEmptyString, size6_bitmap, wxNullBitmap, wxITEM_CHECK, "Size 6", wxEmptyString, NULL);
	sizes_toolbar->AddTool(TOOLBAR_SIZES_7, wxEmptyString, size7_bitmap, wxNullBitmap, wxITEM_CHECK, "Size 7", wxEmptyString, NULL);
	sizes_toolbar->Realize();
	sizes_toolbar->ToggleTool(TOOLBAR_SIZES_RECTANGULAR, true);
	sizes_toolbar->ToggleTool(TOOLBAR_SIZES_1, true);

	manager->AddPane(brushes_toolbar, wxAuiPaneInfo().Name(BRUSHES_BAR_NAME).ToolbarPane().Top().Row(0).Position(0).Floatable(false).CloseButton(false).Gripper(false));
	manager->AddPane(sizes_toolbar, wxAuiPaneInfo().Name(SIZES_BAR_NAME).ToolbarPane().Top().Row(0).Position(1).Floatable(false).CloseButton(false).Gripper(false));
	manager->AddPane(position_toolbar, wxAuiPaneInfo().Name(POSITION_BAR_NAME).ToolbarPane().Top().Row(0).Position(2).Floatable(false).CloseButton(false).Gripper(false));

	brushes_toolbar->Bind(wxEVT_COMMAND_MENU_SELECTED, &MainToolBar::OnBrushesButtonClick, this);
	z_choice->Bind(wxEVT_CHOICE, &MainToolBar::OnZChoiceChanged, this);
	sizes_toolbar->Bind(wxEVT_COMMAND_MENU_SELECTED, &MainToolBar::OnSizesButtonClick, this);

	LoadPerspective();
}

MainToolBar::~MainToolBar() {
	brushes_toolbar->Unbind(wxEVT_COMMAND_MENU_SELECTED, &MainToolBar::OnBrushesButtonClick, this);
	z_choice->Unbind(wxEVT_CHOICE, &MainToolBar::OnZChoiceChanged, this);
	sizes_toolbar->Unbind(wxEVT_COMMAND_MENU_SELECTED, &MainToolBar::OnSizesButtonClick, this);

}

void MainToolBar::UpdateButtons() {
	Editor* editor = g_gui.GetCurrentEditor();

	bool has_map = editor != nullptr;
	bool is_host = has_map && !editor->IsLiveClient();

	brushes_toolbar->EnableTool(PALETTE_TERRAIN_OPTIONAL_BORDER_TOOL, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_ERASER, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_PZ_TOOL, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_NOPVP_TOOL, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_NOLOGOUT_TOOL, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_PVPZONE_TOOL, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_NORMAL_DOOR, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_LOCKED_DOOR, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_MAGIC_DOOR, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_QUEST_DOOR, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_NORMAL_ALT_DOOR, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_HATCH_DOOR, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_WINDOW_DOOR, has_map);

	if (z_choice) {
		z_choice->Enable(has_map);
		if (has_map) {
			SetFloor(g_gui.GetCurrentFloor());
		}
	}
	sizes_toolbar->EnableTool(TOOLBAR_SIZES_CIRCULAR, has_map);
	sizes_toolbar->EnableTool(TOOLBAR_SIZES_RECTANGULAR, has_map);
	sizes_toolbar->EnableTool(TOOLBAR_SIZES_1, has_map);
	sizes_toolbar->EnableTool(TOOLBAR_SIZES_2, has_map);
	sizes_toolbar->EnableTool(TOOLBAR_SIZES_3, has_map);
	sizes_toolbar->EnableTool(TOOLBAR_SIZES_4, has_map);
	sizes_toolbar->EnableTool(TOOLBAR_SIZES_5, has_map);
	sizes_toolbar->EnableTool(TOOLBAR_SIZES_6, has_map);
	sizes_toolbar->EnableTool(TOOLBAR_SIZES_7, has_map);

}

void MainToolBar::UpdateBrushButtons() {
	Brush* brush = g_gui.GetCurrentBrush();
	if (brush) {
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_OPTIONAL_BORDER_TOOL, brush == g_gui.optional_brush);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_ERASER, brush == g_gui.eraser);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_PZ_TOOL, brush == g_gui.pz_brush);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_NOPVP_TOOL, brush == g_gui.rook_brush);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_NOLOGOUT_TOOL, brush == g_gui.nolog_brush);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_PVPZONE_TOOL, brush == g_gui.pvp_brush);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_NORMAL_DOOR, brush == g_gui.normal_door_brush);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_LOCKED_DOOR, brush == g_gui.locked_door_brush);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_MAGIC_DOOR, brush == g_gui.magic_door_brush);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_QUEST_DOOR, brush == g_gui.quest_door_brush);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_NORMAL_ALT_DOOR, brush == g_gui.normal_door_alt_brush);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_HATCH_DOOR, brush == g_gui.hatch_door_brush);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_WINDOW_DOOR, brush == g_gui.window_door_brush);
	} else {
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_OPTIONAL_BORDER_TOOL, false);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_ERASER, false);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_PZ_TOOL, false);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_NOPVP_TOOL, false);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_NOLOGOUT_TOOL, false);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_PVPZONE_TOOL, false);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_NORMAL_DOOR, false);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_LOCKED_DOOR, false);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_MAGIC_DOOR, false);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_QUEST_DOOR, false);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_NORMAL_ALT_DOOR, false);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_HATCH_DOOR, false);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_WINDOW_DOOR, false);
	}
	g_gui.GetAuiManager()->Update();
}

void MainToolBar::UpdateBrushSize(BrushShape shape, int size) {
	if (shape == BRUSHSHAPE_CIRCLE) {
		sizes_toolbar->ToggleTool(TOOLBAR_SIZES_CIRCULAR, true);
		sizes_toolbar->ToggleTool(TOOLBAR_SIZES_RECTANGULAR, false);

		wxSize icon_size = wxSize(16, 16);
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_1, wxArtProvider::GetBitmap(ART_CIRCULAR_1, wxART_TOOLBAR, icon_size));
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_2, wxArtProvider::GetBitmap(ART_CIRCULAR_2, wxART_TOOLBAR, icon_size));
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_3, wxArtProvider::GetBitmap(ART_CIRCULAR_3, wxART_TOOLBAR, icon_size));
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_4, wxArtProvider::GetBitmap(ART_CIRCULAR_4, wxART_TOOLBAR, icon_size));
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_5, wxArtProvider::GetBitmap(ART_CIRCULAR_5, wxART_TOOLBAR, icon_size));
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_6, wxArtProvider::GetBitmap(ART_CIRCULAR_6, wxART_TOOLBAR, icon_size));
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_7, wxArtProvider::GetBitmap(ART_CIRCULAR_7, wxART_TOOLBAR, icon_size));
	} else {
		sizes_toolbar->ToggleTool(TOOLBAR_SIZES_CIRCULAR, false);
		sizes_toolbar->ToggleTool(TOOLBAR_SIZES_RECTANGULAR, true);

		wxSize icon_size = wxSize(16, 16);
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_1, wxArtProvider::GetBitmap(ART_RECTANGULAR_1, wxART_TOOLBAR, icon_size));
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_2, wxArtProvider::GetBitmap(ART_RECTANGULAR_2, wxART_TOOLBAR, icon_size));
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_3, wxArtProvider::GetBitmap(ART_RECTANGULAR_3, wxART_TOOLBAR, icon_size));
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_4, wxArtProvider::GetBitmap(ART_RECTANGULAR_4, wxART_TOOLBAR, icon_size));
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_5, wxArtProvider::GetBitmap(ART_RECTANGULAR_5, wxART_TOOLBAR, icon_size));
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_6, wxArtProvider::GetBitmap(ART_RECTANGULAR_6, wxART_TOOLBAR, icon_size));
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_7, wxArtProvider::GetBitmap(ART_RECTANGULAR_7, wxART_TOOLBAR, icon_size));
	}

	sizes_toolbar->ToggleTool(TOOLBAR_SIZES_1, size == 0);
	sizes_toolbar->ToggleTool(TOOLBAR_SIZES_2, size == 1);
	sizes_toolbar->ToggleTool(TOOLBAR_SIZES_3, size == 2);
	sizes_toolbar->ToggleTool(TOOLBAR_SIZES_4, size == 4);
	sizes_toolbar->ToggleTool(TOOLBAR_SIZES_5, size == 6);
	sizes_toolbar->ToggleTool(TOOLBAR_SIZES_6, size == 8);
	sizes_toolbar->ToggleTool(TOOLBAR_SIZES_7, size == 11);

	g_gui.GetAuiManager()->Update();
}

void MainToolBar::Show(ToolBarID id, bool show) {
	wxAuiManager* manager = g_gui.GetAuiManager();
	if (manager) {
		wxAuiPaneInfo& pane = GetPane(id);
		if (pane.IsOk()) {
			pane.Show(true);
			manager->Update();
		}
	}
}

void MainToolBar::HideAll(bool update) {
	// Locked down: do not hide any toolbars
}

void MainToolBar::LoadPerspective() {
	bool show_brushes = true;
	bool show_sizes = true;
	bool show_position = true;

	GetPane(TOOLBAR_BRUSHES).Show(show_brushes);
	GetPane(TOOLBAR_SIZES).Show(show_sizes);
	GetPane(TOOLBAR_POSITION).Show(show_position);

	if (wxAuiManager* manager = g_gui.GetAuiManager()) {
		manager->Update();
		
		LogErrorToFile("=== AUI Panes Diagnostics ===");
		wxAuiPaneInfoArray& panes = manager->GetAllPanes();
		for (int i = 0, count = panes.GetCount(); i < count; ++i) {
			wxAuiPaneInfo& pane = panes.Item(i);
			LogErrorToFile(wxString::Format("Pane: name='%s', is_toolbar=%d, is_shown=%d, is_valid=%d, rect=(%d, %d, %d, %d), row=%d, col=%d",
				pane.name.c_str(), pane.IsToolbar(), pane.IsShown(), pane.IsOk(),
				pane.rect.x, pane.rect.y, pane.rect.width, pane.rect.height,
				pane.dock_row, pane.dock_pos).ToStdString());
		}
		LogErrorToFile("=============================");
	}
}

void MainToolBar::SavePerspective() {
	// Toolbars are now fixed and their layout isn't saved anymore
}

void MainToolBar::OnBrushesButtonClick(wxCommandEvent& event) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	switch (event.GetId()) {
		case PALETTE_TERRAIN_OPTIONAL_BORDER_TOOL:
			g_gui.SelectBrush(g_gui.optional_brush);
			break;
		case PALETTE_TERRAIN_ERASER:
			g_gui.SelectBrush(g_gui.eraser);
			break;
		case PALETTE_TERRAIN_PZ_TOOL:
			g_gui.SelectBrush(g_gui.pz_brush);
			break;
		case PALETTE_TERRAIN_NOPVP_TOOL:
			g_gui.SelectBrush(g_gui.rook_brush);
			break;
		case PALETTE_TERRAIN_NOLOGOUT_TOOL:
			g_gui.SelectBrush(g_gui.nolog_brush);
			break;
		case PALETTE_TERRAIN_PVPZONE_TOOL:
			g_gui.SelectBrush(g_gui.pvp_brush);
			break;
		case PALETTE_TERRAIN_NORMAL_DOOR:
			g_gui.SelectBrush(g_gui.normal_door_brush);
			break;
		case PALETTE_TERRAIN_LOCKED_DOOR:
			g_gui.SelectBrush(g_gui.locked_door_brush);
			break;
		case PALETTE_TERRAIN_MAGIC_DOOR:
			g_gui.SelectBrush(g_gui.magic_door_brush);
			break;
		case PALETTE_TERRAIN_QUEST_DOOR:
			g_gui.SelectBrush(g_gui.quest_door_brush);
			break;
		case PALETTE_TERRAIN_NORMAL_ALT_DOOR:
			g_gui.SelectBrush(g_gui.normal_door_alt_brush);
			break;
		case PALETTE_TERRAIN_HATCH_DOOR:
			g_gui.SelectBrush(g_gui.hatch_door_brush);
			break;
		case PALETTE_TERRAIN_WINDOW_DOOR:
			g_gui.SelectBrush(g_gui.window_door_brush);
			break;
		default:
			break;
	}
}

void MainToolBar::SetFloor(int floor) {
	if (z_choice && floor >= 0 && floor <= MAP_MAX_LAYER) {
		z_choice->SetSelection(floor);
	}
}

void MainToolBar::OnZChoiceChanged(wxCommandEvent& event) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}
	int z = event.GetSelection();
	if (z >= 0 && z <= MAP_MAX_LAYER) {
		g_gui.ChangeFloor(z);
	}
}

void MainToolBar::OnSizesButtonClick(wxCommandEvent& event) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	switch (event.GetId()) {
		case TOOLBAR_SIZES_CIRCULAR:
			g_gui.SetBrushShape(BRUSHSHAPE_CIRCLE);
			break;
		case TOOLBAR_SIZES_RECTANGULAR:
			g_gui.SetBrushShape(BRUSHSHAPE_SQUARE);
			break;
		case TOOLBAR_SIZES_1:
			g_gui.SetBrushSize(0);
			break;
		case TOOLBAR_SIZES_2:
			g_gui.SetBrushSize(1);
			break;
		case TOOLBAR_SIZES_3:
			g_gui.SetBrushSize(2);
			break;
		case TOOLBAR_SIZES_4:
			g_gui.SetBrushSize(4);
			break;
		case TOOLBAR_SIZES_5:
			g_gui.SetBrushSize(6);
			break;
		case TOOLBAR_SIZES_6:
			g_gui.SetBrushSize(8);
			break;
		case TOOLBAR_SIZES_7:
			g_gui.SetBrushSize(11);
			break;
		default:
			break;
	}
}

wxAuiPaneInfo& MainToolBar::GetPane(ToolBarID id) {
	wxAuiManager* manager = g_gui.GetAuiManager();
	if (!manager) {
		return wxAuiNullPaneInfo;
	}

	switch (id) {
		case TOOLBAR_BRUSHES:
			return manager->GetPane(BRUSHES_BAR_NAME);
		case TOOLBAR_POSITION:
			return manager->GetPane(POSITION_BAR_NAME);
		case TOOLBAR_SIZES:
			return manager->GetPane(SIZES_BAR_NAME);

		default:
			return wxAuiNullPaneInfo;
	}
}


