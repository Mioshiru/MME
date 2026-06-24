#include "imgui_palette.h"
#include "gui.h"
#include "materials.h"
#include "tileset.h"
#include "brush.h"
#include "map.h"
#include "town.h"
#include "house.h"
#include "waypoint_brush.h"
#include "house_brush.h"
#include "house_exit_brush.h"
#include <imgui.h>
#include <algorithm>
#include <sstream>

TilesetCategoryType ImGuiPalette::selected_category = TILESET_TERRAIN;

void ImGuiPalette::DrawToolsWindow() {
	if (ImGui::Begin("Tools Palette", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		// Draw standard tools as ImGui buttons
		if (ImGui::Button("Selection")) g_gui.SetSelectionMode();
		ImGui::SameLine();
		if (ImGui::Button("Eraser")) {
			Brush* eraser = g_brushes.getBrush("Eraser");
			if (eraser) g_gui.SelectBrush(eraser);
		}
		
		ImGui::Separator();
		
		// Draw terrain tools
		if (ImGui::Button("Terrain")) {
			g_gui.SelectBrush(g_brushes.getBrush("Terrain"));
			selected_category = TILESET_TERRAIN;
		}
		ImGui::SameLine();
		if (ImGui::Button("Doodad")) {
			g_gui.SelectBrush(g_brushes.getBrush("Doodad"));
			selected_category = TILESET_DOODAD;
		}
		
		ImGui::Separator();
		
		if (ImGui::Button("Spawn")) {
			g_gui.SelectBrush(g_brushes.getBrush("Spawn"));
			selected_category = TILESET_CREATURE;
		}
		ImGui::SameLine();
		if (ImGui::Button("Waypoint")) {
			g_gui.SelectBrush(g_brushes.getBrush("Waypoint"));
			selected_category = TILESET_WAYPOINT;
		}
		ImGui::SameLine();
		if (ImGui::Button("House (Prefab)")) {
			g_gui.SelectBrush(g_brushes.getBrush("House"));
			selected_category = TILESET_HOUSE;
		}
	}
	ImGui::End();
}

void ImGuiPalette::DrawBrushSizeWindow() {
	if (ImGui::Begin("Brush Size", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		int size = g_gui.GetBrushSize();
		if (ImGui::SliderInt("Size", &size, 0, 20)) {
			g_gui.SetBrushSize(size);
		}
		
		// Optional: Brush shape
		int shape = g_gui.GetBrushShape();
		const char* shapes[] = { "Square", "Circle" };
		if (ImGui::Combo("Shape", &shape, shapes, IM_ARRAYSIZE(shapes))) {
			g_gui.SetBrushShape(static_cast<BrushShape>(shape));
		}
	}
	ImGui::End();
}

void ImGuiPalette::DrawTilesetWindow() {
	if (g_materials.tilesets.empty()) {
		if (ImGui::Begin("Tileset & Brushes", nullptr)) {
			ImGui::Text("No tilesets loaded.");
		}
		ImGui::End();
		return;
	}

	if (ImGui::Begin("Tileset & Brushes", nullptr, ImGuiWindowFlags_None)) {
		Map* map = g_gui.IsEditorOpen() ? &g_gui.GetCurrentMap() : nullptr;

		// 1. Tileset selection Combo box
		static std::string active_tileset_name = "";
		if (active_tileset_name.empty() || g_materials.tilesets.find(active_tileset_name) == g_materials.tilesets.end()) {
			active_tileset_name = g_materials.tilesets.begin()->first;
		}

		if (selected_category == TILESET_TERRAIN || selected_category == TILESET_DOODAD || 
			selected_category == TILESET_ITEM || selected_category == TILESET_RAW || 
			selected_category == TILESET_CREATURE) {
			
			if (ImGui::BeginCombo("Tileset", active_tileset_name.c_str())) {
				for (auto& pair : g_materials.tilesets) {
					const TilesetCategory* tcg = pair.second->getCategory(selected_category);
					bool is_creature_spec = (selected_category == TILESET_CREATURE && (pair.first == "NPCs" || pair.first == "Others"));
					if ((tcg && tcg->size() > 0) || is_creature_spec) {
						bool is_selected = (active_tileset_name == pair.first);
						if (ImGui::Selectable(pair.first.c_str(), is_selected)) {
							active_tileset_name = pair.first;
						}
						if (is_selected) {
							ImGui::SetItemDefaultFocus();
						}
					}
				}
				ImGui::EndCombo();
			}
			ImGui::Separator();
		}

		// 2. Category selection: Tab Bar
		static TilesetCategoryType last_selected_category = TILESET_UNKNOWN;
		bool force_tab_select = (selected_category != last_selected_category);
		last_selected_category = selected_category;

		if (ImGui::BeginTabBar("CategoryTabs")) {
			auto draw_tab = [&](const char* label, TilesetCategoryType cat) {
				ImGuiTabItemFlags flags = (selected_category == cat && force_tab_select) ? ImGuiTabItemFlags_SetSelected : 0;
				if (ImGui::BeginTabItem(label, nullptr, flags)) {
					selected_category = cat;
					ImGui::EndTabItem();
				}
			};
			draw_tab("Terrain", TILESET_TERRAIN);
			draw_tab("Doodad", TILESET_DOODAD);
			draw_tab("Item", TILESET_ITEM);
			draw_tab("RAW", TILESET_RAW);
			draw_tab("Creatures", TILESET_CREATURE);
			draw_tab("House", TILESET_HOUSE);
			draw_tab("Waypoint", TILESET_WAYPOINT);
			ImGui::EndTabBar();
		}

		ImGui::Spacing();

		// 3. Search and View Mode toggles
		static char search_query[128] = "";
		ImGui::InputTextWithHint("##SearchBrushes", "Search...", search_query, IM_ARRAYSIZE(search_query));
		ImGui::SameLine();
		
		static bool list_view_mode = true;
		if (ImGui::Button(list_view_mode ? "Grid View" : "List View")) {
			list_view_mode = !list_view_mode;
		}

		ImGui::Separator();
		ImGui::Spacing();

		// 4. Content Area
		if (selected_category == TILESET_HOUSE) {
			// HOUSE PALETTE
			if (!map) {
				ImGui::Text("No active map.");
			} else {
				// Town combo
				static Town* selected_town = nullptr;
				std::string town_combo_label = selected_town ? selected_town->getName() : "No Town";
				if (ImGui::BeginCombo("Town", town_combo_label.c_str())) {
					for (auto town_iter = map->towns.begin(); town_iter != map->towns.end(); ++town_iter) {
						bool is_selected = (selected_town == town_iter->second);
						if (ImGui::Selectable(town_iter->second->getName().c_str(), is_selected)) {
							selected_town = town_iter->second;
						}
					}
					bool is_selected = (selected_town == nullptr);
					if (ImGui::Selectable("No Town", is_selected)) {
						selected_town = nullptr;
					}
					ImGui::EndCombo();
				}

				// House list
				std::vector<House*> filtered_houses;
				for (auto& house_pair : map->houses) {
					House* house = house_pair.second;
					if (selected_town) {
						if (house->townid == selected_town->getID()) {
							filtered_houses.push_back(house);
						}
					} else {
						if (map->towns.getTown(house->townid) == nullptr) {
							filtered_houses.push_back(house);
						}
					}
				}

				static House* selected_house = nullptr;
				if (selected_house && std::find(filtered_houses.begin(), filtered_houses.end(), selected_house) == filtered_houses.end()) {
					selected_house = filtered_houses.empty() ? nullptr : filtered_houses.front();
				}

				ImGui::Text("Houses in Town:");
				if (ImGui::BeginChild("HouseListChild", ImVec2(0, 150), true)) {
					for (House* house : filtered_houses) {
						bool is_selected = (selected_house == house);
						if (ImGui::Selectable(house->getDescription().c_str(), is_selected)) {
							selected_house = house;
						}
						if (is_selected && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
							if (house->getExit() != Position(0, 0, 0)) {
								g_gui.SetScreenCenterPosition(house->getExit());
							}
						}
					}
					ImGui::EndChild();
				}

				// Actions
				if (ImGui::Button("Add House")) {
					House* new_house = newd House(*map);
					new_house->setID(map->houses.getEmptyID());
					std::ostringstream os;
					os << "Unnamed House #" << new_house->getID();
					new_house->name = os.str();
					if (selected_town) {
						new_house->townid = selected_town->getID();
					} else {
						new_house->townid = 0;
					}
					map->houses.addHouse(new_house);
					selected_house = new_house;
				}
				ImGui::SameLine();
				if (selected_house && ImGui::Button("Remove House")) {
					map->houses.removeHouse(selected_house);
					selected_house = nullptr;
				}
				ImGui::SameLine();
				
				static bool open_edit_house_popup = false;
				if (selected_house && ImGui::Button("Edit House")) {
					open_edit_house_popup = true;
				}

				if (open_edit_house_popup && selected_house) {
					ImGui::OpenPopup("Edit House Dialog");
					open_edit_house_popup = false;
				}

				if (ImGui::BeginPopupModal("Edit House Dialog", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
					static char name_buf[128] = "";
					static int id_val = 1;
					static int rent_val = 0;
					static bool guildhall_val = false;
					static int town_id_val = 0;
					static bool init_edit = true;

					if (init_edit && selected_house) {
						strncpy(name_buf, selected_house->name.c_str(), sizeof(name_buf));
						id_val = selected_house->getID();
						rent_val = selected_house->rent;
						guildhall_val = selected_house->guildhall;
						town_id_val = selected_house->townid;
						init_edit = false;
					}

					ImGui::InputText("Name", name_buf, IM_ARRAYSIZE(name_buf));
					ImGui::InputInt("ID", &id_val);
					ImGui::InputInt("Rent", &rent_val);
					ImGui::Checkbox("Guildhall", &guildhall_val);

					// Town selection in popup
					std::string pop_town_label = "No Town";
					Town* curr_pop_town = map->towns.getTown(town_id_val);
					if (curr_pop_town) pop_town_label = curr_pop_town->getName();
					if (ImGui::BeginCombo("Town##Popup", pop_town_label.c_str())) {
						for (auto town_iter = map->towns.begin(); town_iter != map->towns.end(); ++town_iter) {
							if (ImGui::Selectable(town_iter->second->getName().c_str(), town_id_val == town_iter->second->getID())) {
								town_id_val = town_iter->second->getID();
							}
						}
						if (ImGui::Selectable("No Town", town_id_val == 0)) {
							town_id_val = 0;
						}
						ImGui::EndCombo();
					}

					if (ImGui::Button("Save", ImVec2(120, 0))) {
						if (strlen(name_buf) > 0 && id_val >= 1) {
							selected_house->name = name_buf;
							selected_house->rent = rent_val;
							selected_house->guildhall = guildhall_val;
							selected_house->townid = town_id_val;
							if (id_val != selected_house->getID()) {
								map->convertHouseTiles(selected_house->getID(), id_val);
								map->houses.changeId(selected_house, id_val);
							}
							init_edit = true;
							ImGui::CloseCurrentPopup();
						}
					}
					ImGui::SameLine();
					if (ImGui::Button("Cancel", ImVec2(120, 0))) {
						init_edit = true;
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}

				ImGui::Separator();
				ImGui::Spacing();

				// Brushes section
				bool is_house_tiles = (g_gui.GetCurrentBrush() == g_gui.house_brush);
				bool is_select_exit = (g_gui.GetCurrentBrush() == g_gui.house_exit_brush);

				if (is_house_tiles) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 0.9f, 0.6f));
				if (ImGui::Button("House tiles")) {
					if (selected_house) {
						g_gui.house_brush->setHouse(selected_house);
						g_gui.SelectBrush(g_gui.house_brush);
					}
				}
				if (is_house_tiles) ImGui::PopStyleColor();

				ImGui::SameLine();

				if (is_select_exit) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 0.9f, 0.6f));
				if (ImGui::Button("Select Exit")) {
					if (selected_house) {
						g_gui.house_exit_brush->setHouse(selected_house);
						g_gui.SelectBrush(g_gui.house_exit_brush);
					}
				}
				if (is_select_exit) ImGui::PopStyleColor();
			}
		} 
		else if (selected_category == TILESET_WAYPOINT) {
			// WAYPOINT PALETTE
			if (!map) {
				ImGui::Text("No active map.");
			} else {
				static Waypoint* selected_waypoint = nullptr;
				std::vector<Waypoint*> filtered_waypoints;
				
				std::string q = search_query;
				std::transform(q.begin(), q.end(), q.begin(), ::tolower);

				for (auto wp_iter = map->waypoints.begin(); wp_iter != map->waypoints.end(); ++wp_iter) {
					std::string name = wp_iter->second->name;
					std::string name_lower = name;
					std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
					if (q.empty() || name_lower.find(q) != std::string::npos) {
						filtered_waypoints.push_back(wp_iter->second);
					}
				}

				if (ImGui::BeginChild("WaypointListChild", ImVec2(0, 180), true)) {
					for (Waypoint* wp : filtered_waypoints) {
						bool is_selected = (selected_waypoint == wp);
						if (ImGui::Selectable(wp->name.empty() ? "(Unnamed Waypoint)" : wp->name.c_str(), is_selected)) {
							selected_waypoint = wp;
							g_gui.waypoint_brush->setWaypoint(wp);
							g_gui.SelectBrush(g_gui.waypoint_brush);
						}
						if (is_selected) {
							if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
								g_gui.SetScreenCenterPosition(wp->pos);
							}
						}
					}
					ImGui::EndChild();
				}

				static bool open_add_wp_popup = false;
				if (ImGui::Button("Add Waypoint")) {
					open_add_wp_popup = true;
				}

				if (open_add_wp_popup) {
					ImGui::OpenPopup("Add Waypoint Popup");
					open_add_wp_popup = false;
				}

				if (ImGui::BeginPopupModal("Add Waypoint Popup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
					static char wp_name_buf[128] = "";
					ImGui::InputText("Name", wp_name_buf, IM_ARRAYSIZE(wp_name_buf));
					if (ImGui::Button("Create", ImVec2(120, 0))) {
						if (strlen(wp_name_buf) > 0) {
							if (map->waypoints.getWaypoint(wp_name_buf) == nullptr) {
								Waypoint* nwp = newd Waypoint();
								nwp->name = wp_name_buf;
								map->waypoints.addWaypoint(nwp);
								selected_waypoint = nwp;
								g_gui.waypoint_brush->setWaypoint(nwp);
								g_gui.SelectBrush(g_gui.waypoint_brush);
								wp_name_buf[0] = '\0';
								ImGui::CloseCurrentPopup();
							} else {
								g_gui.SetStatusText("There already is a waypoint with this name.");
							}
						}
					}
					ImGui::SameLine();
					if (ImGui::Button("Cancel", ImVec2(120, 0))) {
						wp_name_buf[0] = '\0';
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}

				ImGui::SameLine();
				if (selected_waypoint && ImGui::Button("Remove Waypoint")) {
					if (map->getTile(selected_waypoint->pos)) {
						map->getTileL(selected_waypoint->pos)->decreaseWaypointCount();
					}
					map->waypoints.removeWaypoint(selected_waypoint->name);
					selected_waypoint = nullptr;
				}
			}
		} 
		else {
			// STANDARD BRUSH CATEGORIES (Terrain, Doodad, Item, RAW, Creature)
			Tileset* active_tileset = nullptr;
			auto it = g_materials.tilesets.find(active_tileset_name);
			if (it != g_materials.tilesets.end()) {
				active_tileset = it->second;
			}

			if (active_tileset) {
				const TilesetCategory* category = active_tileset->getCategory(selected_category);
				std::vector<Brush*> brushes;
				if (category) {
					brushes = category->brushlist;
				}

				// Apply search filtering
				std::vector<Brush*> filtered_brushes;
				std::string q = search_query;
				std::transform(q.begin(), q.end(), q.begin(), ::tolower);

				for (Brush* brush : brushes) {
					std::string name = brush->getName();
					std::string name_lower = name;
					std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
					if (q.empty() || name_lower.find(q) != std::string::npos) {
						filtered_brushes.push_back(brush);
					}
				}

				if (filtered_brushes.empty()) {
					ImGui::Text("No brushes match search query.");
				} else {
					if (list_view_mode) {
						// LIST VIEW MODE
						if (ImGui::BeginChild("BrushListChild", ImVec2(0, 0), true)) {
							for (Brush* brush : filtered_brushes) {
								ImGui::PushID(brush);
								bool selected = (g_gui.GetCurrentBrush() == brush);
								
								Sprite* spr = g_gui.gfx.getSprite(brush->getLookID());
								GLuint texid = 0;
								if (spr) {
									GameSprite* gspr = dynamic_cast<GameSprite*>(spr);
									if (gspr) {
										texid = gspr->getHardwareID(0, 0, 0, -1, 0, 0, 0, 0);
									}
								}

								// Draw Row
								if (texid > 0) {
									ImGui::Image((ImTextureID)texid, ImVec2(16, 16));
									ImGui::SameLine();
								} else {
									ImGui::Dummy(ImVec2(16, 16));
									ImGui::SameLine();
								}

								if (ImGui::Selectable(brush->getName().c_str(), selected)) {
									g_gui.SelectBrush(brush);
								}
								ImGui::PopID();
							}
							ImGui::EndChild();
						}
					} else {
						// GRID VIEW MODE
						if (ImGui::BeginChild("BrushGridChild", ImVec2(0, 0), true)) {
							float width = ImGui::GetContentRegionAvail().x;
							int columns = std::max(1, (int)(width / 44.0f));
							if (ImGui::BeginTable("BrushGridTable", columns)) {
								for (Brush* brush : filtered_brushes) {
									ImGui::TableNextColumn();
									ImGui::PushID(brush);

									bool selected = (g_gui.GetCurrentBrush() == brush);
									
									Sprite* spr = g_gui.gfx.getSprite(brush->getLookID());
									GLuint texid = 0;
									if (spr) {
										GameSprite* gspr = dynamic_cast<GameSprite*>(spr);
										if (gspr) {
											texid = gspr->getHardwareID(0, 0, 0, -1, 0, 0, 0, 0);
										}
									}

									if (selected) {
										ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 0.9f, 0.6f));
									}

									if (texid > 0) {
										std::string btn_id = "brush_btn_" + brush->getName();
										if (ImGui::ImageButton(btn_id.c_str(), (ImTextureID)texid, ImVec2(32, 32))) {
											g_gui.SelectBrush(brush);
										}
									} else {
										std::string initials = brush->getName().substr(0, 3);
										if (ImGui::Button(initials.c_str(), ImVec2(36, 36))) {
											g_gui.SelectBrush(brush);
										}
									}

									if (selected) {
										ImGui::PopStyleColor();
									}

									if (ImGui::IsItemHovered()) {
										ImGui::SetTooltip("%s", brush->getName().c_str());
									}

									ImGui::PopID();
								}
								ImGui::EndTable();
							}
							ImGui::EndChild();
						}
					}
				}
			} else {
				ImGui::Text("No active tileset.");
			}
		}
	}
	ImGui::End();
}
