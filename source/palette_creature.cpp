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

#include "settings.h"
#include "brush.h"
#include "gui.h"
#include "palette_creature.h"
#include "creature_brush.h"
#include "spawn_brush.h"
#include "materials.h"
#include "application.h"
#include "creature_wiki_dialog.h"
#include <wx/srchctrl.h>

// ============================================================================
// Creature palette

BEGIN_EVENT_TABLE(CreaturePalettePanel, PalettePanel)
EVT_CHOICE(PALETTE_CREATURE_TILESET_CHOICE, CreaturePalettePanel::OnTilesetChange)

EVT_LISTBOX(PALETTE_CREATURE_LISTBOX, CreaturePalettePanel::OnListBoxChange)

EVT_TOGGLEBUTTON(PALETTE_CREATURE_BRUSH_BUTTON, CreaturePalettePanel::OnClickCreatureBrushButton)
EVT_TOGGLEBUTTON(PALETTE_SPAWN_BRUSH_BUTTON, CreaturePalettePanel::OnClickSpawnBrushButton)

EVT_SPINCTRL(PALETTE_CREATURE_SPAWN_TIME, CreaturePalettePanel::OnChangeSpawnTime)
EVT_SPINCTRL(PALETTE_CREATURE_SPAWN_SIZE, CreaturePalettePanel::OnChangeSpawnSize)
END_EVENT_TABLE()

CreaturePalettePanel::CreaturePalettePanel(wxWindow* parent, wxWindowID id) :
	PalettePanel(parent, id),
	handling_event(false) {
	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	wxSizer* sidesizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Creatures");
	wxBoxSizer* catRow = new wxBoxSizer(wxHORIZONTAL);
	tileset_choice = newd wxChoice(this, PALETTE_CREATURE_TILESET_CHOICE, wxDefaultPosition, wxDefaultSize, (int)0, (const wxString*)nullptr);
	catRow->Add(tileset_choice, 1, wxEXPAND | wxRIGHT, 4);

	wxButton* btnWiki = new wxButton(this, wxID_ANY, "Wiki", wxDefaultPosition, wxSize(44, 22));
	btnWiki->SetToolTip("Open Creature Wiki & Bestiary (HP, EXP, EXP/HP Ratio, Level Recommendation)");
	btnWiki->SetBackgroundColour(wxColor(30, 58, 95));
	btnWiki->SetForegroundColour(wxColor(220, 235, 255));
	btnWiki->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
		CreatureWikiDialog dlg(this);
		int sel = creature_list->GetSelection();
		if (sel != wxNOT_FOUND && sel < (int)creature_list->GetCount()) {
			dlg.SelectCreature(creature_list->GetString(sel).ToStdString());
		}
		dlg.ShowModal();
	});
	catRow->Add(btnWiki, 0, wxALIGN_CENTER_VERTICAL);
	sidesizer->Add(catRow, 0, wxEXPAND | wxBOTTOM, 4);

	creature_list = newd SortableListBox(this, PALETTE_CREATURE_LISTBOX);
	creature_list->Bind(wxEVT_CONTEXT_MENU, [this](wxContextMenuEvent& event) {
		wxPoint pos = event.GetPosition();
		wxPoint client_pos = creature_list->ScreenToClient(pos);
		int item_idx = creature_list->HitTest(client_pos);
		if (item_idx == wxNOT_FOUND) {
			item_idx = creature_list->GetSelection();
		}
		if (item_idx != wxNOT_FOUND && item_idx < (int)creature_list->GetCount()) {
			creature_list->SetSelection(item_idx);
			SelectCreature(item_idx);
			Brush* brush = reinterpret_cast<Brush*>(creature_list->GetClientData(item_idx));
			if (brush) {
				wxMenu menu;
				Tileset* favs = g_materials.tilesets["Favorites"];
				bool is_favorited = false;
				if (favs) {
					for (TilesetCategory* cat : favs->categories) {
						if (cat && cat->containsBrush(brush)) {
							is_favorited = true;
							break;
						}
					}
				}

				if (is_favorited) {
					menu.Append(10002, "Remove Favorite");
				} else {
					menu.Append(10001, "Favorite");
				}

				menu.Bind(wxEVT_MENU, [brush](wxCommandEvent& ev) {
					Tileset* f = g_materials.tilesets["Favorites"];
					if (!f) return;
					if (ev.GetId() == 10001) {
						TilesetCategory* catFav = f->getCategory(TILESET_CREATURE);
						if (!catFav) catFav = f->getCategory(TILESET_FAVORITE);
						if (catFav && !catFav->containsBrush(brush)) {
							catFav->brushlist.push_back(brush);
						}
					} else if (ev.GetId() == 10002) {
						for (TilesetCategory* cat : f->categories) {
							auto it = std::find(cat->brushlist.begin(), cat->brushlist.end(), brush);
							if (it != cat->brushlist.end()) {
								cat->brushlist.erase(it);
							}
						}
					}
					g_materials.rebuildFavorites();
					g_materials.saveFavorites();
					g_gui.RefreshFavoritesBox();
				});

				creature_list->PopupMenu(&menu, client_pos);
			}
		}
	});
	sidesizer->Add(creature_list, 1, wxEXPAND);
	topsizer->Add(sidesizer, 1, wxEXPAND);

	// Brush selection
	sidesizer = newd wxStaticBoxSizer(newd wxStaticBox(this, wxID_ANY, "Brushes", wxDefaultPosition, wxSize(150, 200)), wxVERTICAL);

	// sidesizer->Add(180, 1, wxEXPAND);

	wxFlexGridSizer* grid = newd wxFlexGridSizer(3, 10, 10);
	grid->AddGrowableCol(1);

	grid->Add(newd wxStaticText(this, wxID_ANY, "Spawntime"));
	creature_spawntime_spin = newd wxSpinCtrl(this, PALETTE_CREATURE_SPAWN_TIME, i2ws(g_settings.getInteger(Config::DEFAULT_SPAWNTIME)), wxDefaultPosition, wxSize(50, 20), wxSP_ARROW_KEYS, 0, 86400, g_settings.getInteger(Config::DEFAULT_SPAWNTIME));
	grid->Add(creature_spawntime_spin, 0, wxEXPAND);
	creature_brush_button = newd wxToggleButton(this, PALETTE_CREATURE_BRUSH_BUTTON, "Place Creature");
	grid->Add(creature_brush_button, 0, wxEXPAND);

	grid->Add(newd wxStaticText(this, wxID_ANY, "Spawn size"));
	spawn_size_spin = newd wxSpinCtrl(this, PALETTE_CREATURE_SPAWN_SIZE, i2ws(5), wxDefaultPosition, wxSize(50, 20), wxSP_ARROW_KEYS, 1, g_settings.getInteger(Config::MAX_SPAWN_RADIUS), g_settings.getInteger(Config::CURRENT_SPAWN_RADIUS));
	grid->Add(spawn_size_spin, 0, wxEXPAND);
	spawn_brush_button = newd wxToggleButton(this, PALETTE_SPAWN_BRUSH_BUTTON, "Place Spawn");
	grid->Add(spawn_brush_button, 0, wxEXPAND);

	sidesizer->Add(grid, 0, wxEXPAND);
	topsizer->Add(sidesizer, 0, wxEXPAND);
	SetSizerAndFit(topsizer);

	OnUpdate();
}

CreaturePalettePanel::~CreaturePalettePanel() {
	////
}

PaletteType CreaturePalettePanel::GetType() const {
	return TILESET_CREATURE;
}

void CreaturePalettePanel::SelectFirstBrush() {
	SelectCreatureBrush();
}

Brush* CreaturePalettePanel::GetSelectedBrush() const {
	if (creature_brush_button->GetValue()) {
		if (creature_list->GetCount() == 0) {
			return nullptr;
		}
		Brush* brush = reinterpret_cast<Brush*>(creature_list->GetClientData(creature_list->GetSelection()));
		if (brush && brush->isCreature()) {
			g_gui.SetSpawnTime(creature_spawntime_spin->GetValue());
			return brush;
		}
	} else if (spawn_brush_button->GetValue()) {
		g_settings.setInteger(Config::CURRENT_SPAWN_RADIUS, spawn_size_spin->GetValue());
		g_settings.setInteger(Config::DEFAULT_SPAWNTIME, creature_spawntime_spin->GetValue());
		return g_gui.spawn_brush;
	}
	return nullptr;
}

bool CreaturePalettePanel::SelectBrush(const Brush* whatbrush) {
	if (!whatbrush) {
		return false;
	}

	if (whatbrush->isCreature()) {
		int current_index = tileset_choice->GetSelection();
		if (current_index != wxNOT_FOUND) {
			const TilesetCategory* tsc = reinterpret_cast<const TilesetCategory*>(tileset_choice->GetClientData(current_index));
			// Select first house
			for (BrushVector::const_iterator iter = tsc->brushlist.begin(); iter != tsc->brushlist.end(); ++iter) {
				if (*iter == whatbrush) {
					SelectCreature(whatbrush->getName());
					return true;
				}
			}
		}
		// Not in the current display, search the hidden one's
		for (size_t i = 0; i < tileset_choice->GetCount(); ++i) {
			if (current_index != (int)i) {
				const TilesetCategory* tsc = reinterpret_cast<const TilesetCategory*>(tileset_choice->GetClientData(i));
				for (BrushVector::const_iterator iter = tsc->brushlist.begin();
					 iter != tsc->brushlist.end();
					 ++iter) {
					if (*iter == whatbrush) {
						SelectTileset(i);
						SelectCreature(whatbrush->getName());
						return true;
					}
				}
			}
		}
	} else if (whatbrush->isSpawn()) {
		SelectSpawnBrush();
		return true;
	}
	return false;
}

int CreaturePalettePanel::GetSelectedBrushSize() const {
	return spawn_size_spin->GetValue();
}

void CreaturePalettePanel::OnUpdate() {
	ScopedAction action("CreaturePalettePanel::OnUpdate");
	tileset_choice->Clear();
	g_materials.createOtherTileset();

	for (TilesetContainer::const_iterator iter = g_materials.tilesets.begin(); iter != g_materials.tilesets.end(); ++iter) {
		if (!iter->second || iter->second->name == "Favorites") continue;
		const TilesetCategory* tsc = iter->second->getCategory(TILESET_CREATURE);
		if (tsc && tsc->size() > 0) {
			tileset_choice->Append(wxstr(iter->second->name), const_cast<TilesetCategory*>(tsc));
		} else if (iter->second->name == "NPCs" || iter->second->name == "Creatures") {
			Tileset* ts = const_cast<Tileset*>(iter->second);
			TilesetCategory* rtsc = ts->getCategory(TILESET_CREATURE);
			if (rtsc) {
				tileset_choice->Append(wxstr(ts->name), rtsc);
			}
		}
	}
	if (tileset_choice->GetCount() > 0) {
		SelectTileset(0);
	} else {
		creature_list->Clear();
		creature_brush_button->Enable(false);
	}
}

void CreaturePalettePanel::OnUpdateBrushSize(BrushShape shape, int size) {
	return spawn_size_spin->SetValue(size);
}

void CreaturePalettePanel::OnSwitchIn() {
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SetBrushSize(spawn_size_spin->GetValue());
}

void CreaturePalettePanel::SelectTileset(size_t index) {
	if (index >= tileset_choice->GetCount()) {
		creature_list->Clear();
		creature_brush_button->Enable(false);
		return;
	}

	creature_list->Clear();
	if (tileset_choice->GetCount() == 0) {
		// No tilesets :(
		creature_brush_button->Enable(false);
	} else {
		const TilesetCategory* tsc = reinterpret_cast<const TilesetCategory*>(tileset_choice->GetClientData(index));
		if (tsc) {
			for (BrushVector::const_iterator iter = tsc->brushlist.begin();
				 iter != tsc->brushlist.end();
				 ++iter) {
				if (*iter) {
					creature_list->Append(wxstr((*iter)->getName()), *iter);
				}
			}
			creature_list->Sort();
			SelectCreature(0);
		}

		tileset_choice->SetSelection(index);
	}
}

void CreaturePalettePanel::DoSearch(const wxString& query) {
	if (query.IsEmpty()) {
		int sel = tileset_choice->GetSelection();
		if (sel != wxNOT_FOUND) {
			SelectTileset(sel);
		}
		return;
	}

	wxString search_filter = query.Lower();
	creature_list->Clear();

	for (size_t i = 0; i < tileset_choice->GetCount(); ++i) {
		const TilesetCategory* tsc = reinterpret_cast<const TilesetCategory*>(tileset_choice->GetClientData(i));
		if (!tsc) continue;

		for (BrushVector::const_iterator iter = tsc->brushlist.begin(); iter != tsc->brushlist.end(); ++iter) {
			wxString name = wxstr((*iter)->getName());
			if (name.Lower().Contains(search_filter)) {
				if (creature_list->FindString(name) == wxNOT_FOUND) {
					creature_list->Append(name, *iter);
				}
			}
		}
	}

	creature_list->Sort();
	if (creature_list->GetCount() > 0) {
		SelectCreature(0);
	} else {
		creature_brush_button->Enable(false);
	}
}

void CreaturePalettePanel::SelectCreature(size_t index) {
	ASSERT(creature_list->GetCount() >= index);

	if (creature_list->GetCount() > 0) {
		creature_list->SetSelection(index);
	}

	SelectCreatureBrush();
}

void CreaturePalettePanel::SelectCreature(std::string name) {
	if (creature_list->GetCount() > 0) {
		if (!creature_list->SetStringSelection(wxstr(name))) {
			creature_list->SetSelection(0);
		}
	}

	SelectCreatureBrush();
}

void CreaturePalettePanel::SelectCreatureBrush() {
	if (creature_list->GetCount() > 0) {
		creature_brush_button->Enable(true);
		creature_brush_button->SetValue(true);
		spawn_brush_button->SetValue(false);
	} else {
		creature_brush_button->Enable(false);
		SelectSpawnBrush();
	}
}

void CreaturePalettePanel::SelectSpawnBrush() {
	creature_brush_button->SetValue(false);
	spawn_brush_button->SetValue(true);
}

void CreaturePalettePanel::OnTilesetChange(wxCommandEvent& event) {
	SelectTileset(event.GetSelection());
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush();
}

void CreaturePalettePanel::OnListBoxChange(wxCommandEvent& event) {
	SelectCreature(event.GetSelection());
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush();
}

void CreaturePalettePanel::OnClickCreatureBrushButton(wxCommandEvent& event) {
	SelectCreatureBrush();
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush();
}

void CreaturePalettePanel::OnClickSpawnBrushButton(wxCommandEvent& event) {
	SelectSpawnBrush();
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush();
}

void CreaturePalettePanel::OnChangeSpawnTime(wxSpinEvent& event) {
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SetSpawnTime(event.GetPosition());
}

void CreaturePalettePanel::OnChangeSpawnSize(wxSpinEvent& event) {
	if (!handling_event) {
		handling_event = true;
		g_gui.ActivatePalette(GetParentPalette());
		g_gui.SetBrushSize(event.GetPosition());
		handling_event = false;
	}
}
