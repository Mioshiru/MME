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
#include "radio_player.h"
#include "pngfiles.h"
#include "artprovider.h"
#include <wx/artprov.h>
#include <wx/mstream.h>
#include <wx/stdpaths.h>

namespace {
wxBitmap LoadBitmapFromFileCandidates(const wxSize& icon_size, const std::vector<wxString>& candidates) {
	for (const wxString& candidate : candidates) {
		if (!wxFileExists(candidate)) {
			continue;
		}
		wxImage image;
		if (image.LoadFile(candidate, wxBITMAP_TYPE_ANY)) {
			if (icon_size.IsFullySpecified() && icon_size.GetWidth() > 0 && icon_size.GetHeight() > 0) {
				image = image.Scale(icon_size.GetWidth(), icon_size.GetHeight(), wxIMAGE_QUALITY_HIGH);
			}
			wxBitmap bitmap(image);
			if (bitmap.IsOk()) {
				return bitmap;
			}
		}
	}
	return wxNullBitmap;
}
}

const wxString MainToolBar::BRUSHES_BAR_NAME = "brushes_toolbar";
const wxString MainToolBar::POSITION_BAR_NAME = "position_toolbar";
const wxString MainToolBar::SIZES_BAR_NAME = "sizes_toolbar";

#define loadPNGFile(name) _wxGetBitmapFromMemory(name, sizeof(name), wxDefaultSize)
#define loadPNGFileSized(name, size) _wxGetBitmapFromMemory(name, sizeof(name), size)
inline wxBitmap* _wxGetBitmapFromMemory(const unsigned char* data, int length, const wxSize& target_size) {
	wxMemoryInputStream is(data, length);
	wxImage img(is, "image/png");
	if (!img.IsOk()) {
		return nullptr;
	}
	if (target_size.IsFullySpecified() && target_size.GetWidth() > 0 && target_size.GetHeight() > 0 &&
		(img.GetWidth() != target_size.GetWidth() || img.GetHeight() != target_size.GetHeight())) {
		img = img.Scale(target_size.GetWidth(), target_size.GetHeight(), wxIMAGE_QUALITY_HIGH);
	}
	return newd wxBitmap(img, -1);
}

MainToolBar::MainToolBar(wxWindow* parent, wxAuiManager* manager) {
	int scale_percent = g_settings.getInteger(Config::UI_SCALE);
	if (scale_percent < 100) scale_percent = 100;
	if (scale_percent > 200) scale_percent = 200;
	wxSize base_size(16 * scale_percent / 100, 16 * scale_percent / 100);
	wxSize icon_size = FROM_DIP(parent, base_size);

	wxBitmap border_bitmap = LoadBitmapFromFileCandidates(icon_size, {
		"icons/auto_border.png",
		"../icons/auto_border.png",
		"Map Editor/icons/auto_border.png",
		"../Map Editor/icons/auto_border.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "auto_border.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "auto_border.png"
	});
	if (!border_bitmap.IsOk()) {
		wxBitmap* mem_bmp = loadPNGFileSized(optional_border_small_png, icon_size);
		if (mem_bmp) border_bitmap = *mem_bmp;
	}

	wxBitmap pointer_bitmap = LoadBitmapFromFileCandidates(icon_size, {
		"icons/pointer.png",
		"../icons/pointer.png",
		"Map Editor/icons/pointer.png",
		"../Map Editor/icons/pointer.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "pointer.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "pointer.png"
	});
	wxBitmap pencil_bitmap = LoadBitmapFromFileCandidates(icon_size, {
		"icons/pencil.png",
		"../icons/pencil.png",
		"Map Editor/icons/pencil.png",
		"../Map Editor/icons/pencil.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "pencil.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "pencil.png"
	});
	wxBitmap bucket_bitmap = LoadBitmapFromFileCandidates(icon_size, {
		"icons/bucket.png",
		"../icons/bucket.png",
		"Map Editor/icons/bucket.png",
		"../Map Editor/icons/bucket.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "bucket.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "bucket.png"
	});
	wxBitmap wand_bitmap = LoadBitmapFromFileCandidates(icon_size, {
		"icons/magic-wand.png",
		"../icons/magic-wand.png",
		"Map Editor/icons/magic-wand.png",
		"../Map Editor/icons/magic-wand.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "magic-wand.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "magic-wand.png"
	});
	wxBitmap prefab_bitmap = LoadBitmapFromFileCandidates(icon_size, {
		"icons/prefab.png",
		"../icons/prefab.png",
		"Map Editor/icons/prefab.png",
		"../Map Editor/icons/prefab.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "prefab.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "prefab.png"
	});
	wxBitmap pz_bitmap = LoadBitmapFromFileCandidates(icon_size, {
		"icons/protected_zone.png",
		"../icons/protected_zone.png",
		"Map Editor/icons/protected_zone.png",
		"../Map Editor/icons/protected_zone.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "protected_zone.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "protected_zone.png"
	});
	if (!pz_bitmap.IsOk()) pz_bitmap = wxArtProvider::GetBitmap(ART_PZ_BRUSH, wxART_TOOLBAR, icon_size);

	wxBitmap eraser_bitmap = LoadBitmapFromFileCandidates(icon_size, {
		"icons/eraser.png", "../icons/eraser.png", "Map Editor/icons/eraser.png", "../Map Editor/icons/eraser.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "eraser.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "eraser.png"
	});
	if (!eraser_bitmap.IsOk()) {
		wxBitmap* mem_bmp = loadPNGFileSized(eraser_small_png, icon_size);
		if (mem_bmp) eraser_bitmap = *mem_bmp;
	}

	wxBitmap normal_bitmap = LoadBitmapFromFileCandidates(icon_size, {
		"icons/door.png", "../icons/door.png", "Map Editor/icons/door.png", "../Map Editor/icons/door.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "door.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "door.png"
	});
	if (!normal_bitmap.IsOk()) normal_bitmap = wxArtProvider::GetBitmap(ART_DOOR_NORMAL_SMALL, wxART_TOOLBAR, icon_size);

	wxBitmap window_bitmap = LoadBitmapFromFileCandidates(icon_size, {
		"icons/window.png", "../icons/window.png", "Map Editor/icons/window.png", "../Map Editor/icons/window.png",
		wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "window.png",
		wxGetCwd() + wxFILE_SEP_PATH + "icons" + wxFILE_SEP_PATH + "window.png"
	});
	if (!window_bitmap.IsOk()) {
		wxBitmap* mem_bmp = _wxGetBitmapFromMemory(window_hatch_small_png, sizeof(window_hatch_small_png), icon_size);
		if (mem_bmp) window_bitmap = *mem_bmp;
	}

	brushes_toolbar = newd wxAuiToolBar(parent, TOOLBAR_BRUSHES, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE);
	brushes_toolbar->SetToolBitmapSize(icon_size);
	if (pointer_bitmap.IsOk()) {
		brushes_toolbar->AddTool(PALETTE_TERRAIN_SELECTION_TOOL, wxEmptyString, pointer_bitmap, wxNullBitmap, wxITEM_CHECK, "Selection Tool", wxEmptyString, NULL);
	}
	if (pencil_bitmap.IsOk()) {
		brushes_toolbar->AddTool(PALETTE_TERRAIN_PENCIL_TOOL, wxEmptyString, pencil_bitmap, wxNullBitmap, wxITEM_CHECK, "Pencil Tool", wxEmptyString, NULL);
	}
	if (bucket_bitmap.IsOk()) {
		brushes_toolbar->AddTool(PALETTE_TERRAIN_BUCKET_TOOL, wxEmptyString, bucket_bitmap, wxNullBitmap, wxITEM_CHECK, "Bucket Fill", wxEmptyString, NULL);
	}
	brushes_toolbar->AddTool(PALETTE_TERRAIN_ERASER, wxEmptyString, eraser_bitmap, wxNullBitmap, wxITEM_CHECK, "Eraser", wxEmptyString, NULL);
	if (prefab_bitmap.IsOk()) {
		brushes_toolbar->AddTool(PALETTE_TERRAIN_PREFAB_CREATOR_TOOL, wxEmptyString, prefab_bitmap, wxNullBitmap, wxITEM_CHECK, "Prefab Creator", wxEmptyString, NULL);
	}
	brushes_toolbar->AddTool(PALETTE_TERRAIN_OPTIONAL_BORDER_TOOL, wxEmptyString, border_bitmap, wxNullBitmap, wxITEM_CHECK, "Autoborder", wxEmptyString, NULL);
	brushes_toolbar->AddSeparator();

	brushes_toolbar->AddTool(PALETTE_TERRAIN_ZONES_DROPDOWN, "Zones", pz_bitmap, wxNullBitmap, wxITEM_NORMAL, "Zones (PZ, No-Logout, No-PVP, PVP)", wxEmptyString, NULL);
	brushes_toolbar->AddSeparator();
	brushes_toolbar->AddTool(PALETTE_TERRAIN_DOORS_DROPDOWN, "Doors", normal_bitmap, wxNullBitmap, wxITEM_NORMAL, "Doors", wxEmptyString, NULL);
	brushes_toolbar->AddSeparator();
	brushes_toolbar->AddTool(PALETTE_TERRAIN_WINDOWS_DROPDOWN, "Windows", window_bitmap, wxNullBitmap, wxITEM_NORMAL, "Windows (Hatch Window, Window)", wxEmptyString, NULL);
	brushes_toolbar->Realize();

	wxBitmap go_bitmap = wxArtProvider::GetBitmap(ART_POSITION_GO, wxART_TOOLBAR, icon_size);

	position_toolbar = newd wxAuiToolBar(parent, TOOLBAR_POSITION, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORZ_TEXT);
	position_toolbar->SetToolBitmapSize(icon_size);
	
	wxArrayString z_levels;
	for (int i = 0; i <= MAP_MAX_LAYER; ++i) {
		z_levels.Add(wxString::Format("Floor %d", i));
	}
	z_choice = newd wxChoice(position_toolbar, wxID_ANY, wxDefaultPosition, FROM_DIP(parent, wxSize(75, -1)), z_levels);
	z_choice->SetToolTip("Select Z-Level (Floor)");
	z_choice->SetBackgroundColour(wxColour(10, 20, 35));
	z_choice->SetForegroundColour(wxColour(180, 150, 50));
	
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

	brushes_toolbar->EnableTool(PALETTE_TERRAIN_SELECTION_TOOL, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_PENCIL_TOOL, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_OPTIONAL_BORDER_TOOL, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_BUCKET_TOOL, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_ERASER, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_ZONES_DROPDOWN, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_DOORS_DROPDOWN, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_WINDOWS_DROPDOWN, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_PREFAB_CREATOR_TOOL, has_map);

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
	static const int tool_ids[] = {
		PALETTE_TERRAIN_SELECTION_TOOL,
		PALETTE_TERRAIN_PENCIL_TOOL,
		PALETTE_TERRAIN_OPTIONAL_BORDER_TOOL,
		PALETTE_TERRAIN_BUCKET_TOOL,
		PALETTE_TERRAIN_ERASER,
		PALETTE_TERRAIN_PZ_TOOL,
		PALETTE_TERRAIN_NOPVP_TOOL,
		PALETTE_TERRAIN_NOLOGOUT_TOOL,
		PALETTE_TERRAIN_PVPZONE_TOOL,
		PALETTE_TERRAIN_NORMAL_DOOR,
		PALETTE_TERRAIN_LOCKED_DOOR,
		PALETTE_TERRAIN_MAGIC_DOOR,
		PALETTE_TERRAIN_QUEST_DOOR,
		PALETTE_TERRAIN_HATCH_DOOR,
		PALETTE_TERRAIN_WINDOW_DOOR,
		PALETTE_TERRAIN_PREFAB_CREATOR_TOOL
	};
	for (int id : tool_ids) {
		if (brushes_toolbar->FindTool(id)) {
			brushes_toolbar->ToggleTool(id, false);
		}
	}

	Brush* brush = g_gui.GetCurrentBrush();
	const bool fill_mode = g_gui.IsFillBrushMode();
	const bool selection_mode = g_gui.IsSelectionMode();

	if (selection_mode) {
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_SELECTION_TOOL, true);
	} else if (fill_mode) {
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_BUCKET_TOOL, true);
	} else if (brush == g_gui.eraser) {
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_ERASER, true);
	} else if (brush == g_gui.optional_brush) {
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_OPTIONAL_BORDER_TOOL, true);
	} else if (brush == g_gui.pz_brush) {
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_PZ_TOOL, true);
	} else if (brush == g_gui.rook_brush) {
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_NOPVP_TOOL, true);
	} else if (brush == g_gui.nolog_brush) {
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_NOLOGOUT_TOOL, true);
	} else if (brush == g_gui.pvp_brush) {
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_PVPZONE_TOOL, true);
	} else if (brush == g_gui.normal_door_brush) {
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_NORMAL_DOOR, true);
	} else if (brush == g_gui.locked_door_brush) {
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_LOCKED_DOOR, true);
	} else if (brush == g_gui.magic_door_brush) {
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_MAGIC_DOOR, true);
	} else if (brush == g_gui.quest_door_brush) {
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_QUEST_DOOR, true);
	} else if (brush == g_gui.hatch_door_brush) {
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_HATCH_DOOR, true);
	} else if (brush == g_gui.window_door_brush) {
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_WINDOW_DOOR, true);
	} else if (brush == g_gui.prefab_creator_brush) {
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_PREFAB_CREATOR_TOOL, true);
	} else {
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_PENCIL_TOOL, true);
	}

	brushes_toolbar->Refresh();
	g_gui.GetAuiManager()->Update();
}

void MainToolBar::UpdateBrushSize(BrushShape shape, int size) {
	if (shape == BRUSHSHAPE_CIRCLE) {
		sizes_toolbar->ToggleTool(TOOLBAR_SIZES_CIRCULAR, true);
		sizes_toolbar->ToggleTool(TOOLBAR_SIZES_RECTANGULAR, false);

		wxSize icon_size = sizes_toolbar->GetToolBitmapSize();
		if (!icon_size.IsFullySpecified() || icon_size.GetWidth() <= 0 || icon_size.GetHeight() <= 0) {
			int scale_percent = g_settings.getInteger(Config::UI_SCALE);
			if (scale_percent < 100) scale_percent = 100;
			if (scale_percent > 200) scale_percent = 200;
			wxSize base_size(16 * scale_percent / 100, 16 * scale_percent / 100);
			icon_size = FROM_DIP(sizes_toolbar, base_size);
		}
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

		wxSize icon_size = sizes_toolbar->GetToolBitmapSize();
		if (!icon_size.IsFullySpecified() || icon_size.GetWidth() <= 0 || icon_size.GetHeight() <= 0) {
			int scale_percent = g_settings.getInteger(Config::UI_SCALE);
			if (scale_percent < 100) scale_percent = 100;
			if (scale_percent > 200) scale_percent = 200;
			wxSize base_size(16 * scale_percent / 100, 16 * scale_percent / 100);
			icon_size = FROM_DIP(sizes_toolbar, base_size);
		}
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
		case PALETTE_TERRAIN_SELECTION_TOOL:
			g_gui.SetFillBrushMode(false);
			g_gui.SetSelectionMode();
			break;
		case PALETTE_TERRAIN_PENCIL_TOOL:
			g_gui.SetFillBrushMode(false);
			g_gui.SetDrawingMode();
			break;
		case PALETTE_TERRAIN_OPTIONAL_BORDER_TOOL:
			g_gui.SelectBrush(g_gui.optional_brush);
			break;
		case PALETTE_TERRAIN_BUCKET_TOOL:
			g_gui.SetDrawingMode();
			g_gui.SetFillBrushMode(!g_gui.IsFillBrushMode());
			break;
		case PALETTE_TERRAIN_ERASER:
			g_gui.SelectBrush(g_gui.eraser);
			break;
		case PALETTE_TERRAIN_ZONES_DROPDOWN:
			OnZonesDropdown(event);
			break;
		case PALETTE_TERRAIN_DOORS_DROPDOWN:
			OnDoorsDropdown(event);
			break;
		case PALETTE_TERRAIN_WINDOWS_DROPDOWN:
			OnWindowsDropdown(event);
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
		case PALETTE_TERRAIN_HATCH_DOOR:
			g_gui.SelectBrush(g_gui.hatch_door_brush);
			break;
		case PALETTE_TERRAIN_WINDOW_DOOR:
			g_gui.SelectBrush(g_gui.window_door_brush);
			break;
		case PALETTE_TERRAIN_PREFAB_CREATOR_TOOL:
			g_gui.SelectBrush(g_gui.prefab_creator_brush);
			break;
		default:
			break;
	}
}

void MainToolBar::OnZonesDropdown(wxCommandEvent& WXUNUSED(event)) {
	wxMenu menu;
	menu.Append(PALETTE_TERRAIN_PZ_TOOL, "Protected Zone");
	menu.Append(PALETTE_TERRAIN_NOLOGOUT_TOOL, "No Logout Zone");
	menu.Append(PALETTE_TERRAIN_NOPVP_TOOL, "No PvP Zone");
	menu.Append(PALETTE_TERRAIN_PVPZONE_TOOL, "PvP Zone");
	brushes_toolbar->PopupMenu(&menu);
}

void MainToolBar::OnDoorsDropdown(wxCommandEvent& WXUNUSED(event)) {
	wxMenu menu;
	menu.Append(PALETTE_TERRAIN_NORMAL_DOOR, "Normal Door");
	menu.Append(PALETTE_TERRAIN_LOCKED_DOOR, "Locked Door");
	menu.Append(PALETTE_TERRAIN_MAGIC_DOOR, "Magic Door");
	menu.Append(PALETTE_TERRAIN_QUEST_DOOR, "Quest Door");
	brushes_toolbar->PopupMenu(&menu);
}

void MainToolBar::OnWindowsDropdown(wxCommandEvent& WXUNUSED(event)) {
	wxMenu menu;
	menu.Append(PALETTE_TERRAIN_HATCH_DOOR, "Hatch Window");
	menu.Append(PALETTE_TERRAIN_WINDOW_DOOR, "Window");
	brushes_toolbar->PopupMenu(&menu);
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

	const bool bucket_tool = event.GetId() == PALETTE_TERRAIN_BUCKET_TOOL;
	if (!bucket_tool) {
		g_gui.SetFillBrushMode(false);
	}

	switch (event.GetId()) {
		case TOOLBAR_SIZES_CIRCULAR:
			g_gui.SetBrushShape(BRUSHSHAPE_CIRCLE);
			break;
		case TOOLBAR_SIZES_RECTANGULAR:
			g_gui.SetBrushShape(BRUSHSHAPE_SQUARE);
			break;
		case PALETTE_TERRAIN_BUCKET_TOOL:
			g_gui.SetDrawingMode();
			g_gui.SetFillBrushMode(true);
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



