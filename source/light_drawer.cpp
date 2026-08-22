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
#include "light_drawer.h"
#include "settings.h"
#include "map.h"
#include "tile.h"
#include "item.h"
#include "items.h"
#include <algorithm>
#include <cmath>

LightDrawer::LightDrawer() {
	texture = 0;
	global_color = wxColor(5, 7, 14, 255);
}

LightDrawer::~LightDrawer() {
	unloadGLTexture();
	lights.clear();
}

void LightDrawer::draw(int map_x, int map_y, int end_x, int end_y, int scroll_x, int scroll_y, Map* map, int current_floor) {
	int w = (end_x - map_x) + 2;
	int h = (end_y - map_y) + 2;
	if (w <= 0 || h <= 0) return;

	const int draw_x = map_x * TileSize - scroll_x;
	const int draw_y = map_y * TileSize - scroll_y;
	int draw_width = w * TileSize;
	int draw_height = h * TileSize;

	glDisable(GL_TEXTURE_2D);
	glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
	
	// Draw the global ambient darkening (slightly deeper nighttime)
	float ambient_val = std::min(1.0f, std::max(0.0f, g_settings.getFloat(Config::LIGHT_AMBIENT)));
	uint8_t ambient_alpha = static_cast<uint8_t>(255 * (1.0f - (ambient_val * 0.85f)));
	glColor4ub(global_color.Red(), global_color.Green(), global_color.Blue(), ambient_alpha);
	glBegin(GL_QUADS);
	glVertex2f(draw_x, draw_y);
	glVertex2f(draw_x + draw_width, draw_y);
	glVertex2f(draw_x + draw_width, draw_y + draw_height);
	glVertex2f(draw_x, draw_y + draw_height);
	glEnd();

	if (lights.empty()) {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4ub(255, 255, 255, 255);
		return;
	}

	// [PERFORMANCE] Fast 2D Lightmap Accumulation Grid (Zero GPU Overdraw)
	size_t grid_size = static_cast<size_t>(w * h);
	std::vector<float> r_acc(grid_size, 0.0f);
	std::vector<float> g_acc(grid_size, 0.0f);
	std::vector<float> b_acc(grid_size, 0.0f);
	std::vector<float> a_acc(grid_size, 0.0f);

	float intensity_setting = g_settings.getFloat(Config::LIGHT_INTENSITY);

	for (const auto& light : lights) {
		int lx = light.map_x;
		int ly = light.map_y;
		int rad = light.intensity;
		if (rad <= 0) continue;

		wxColor light_color = colorFromEightBit(light.color);
		float lr = light_color.Red();
		float lg = light_color.Green();
		float lb = light_color.Blue();

		// Boost light intensity for radiant torches, lanterns, coal basins, lava
		float base_alpha = (0.35f + (std::min(15, rad) / 15.0f) * 0.85f) * intensity_setting;
		if (base_alpha > 0.98f) base_alpha = 0.98f;

		int min_x = std::max(map_x, lx - rad);
		int max_x = std::min(end_x + 1, lx + rad);
		int min_y = std::max(map_y, ly - rad);
		int max_y = std::min(end_y + 1, ly + rad);

		for (int ty = min_y; ty <= max_y; ++ty) {
			for (int tx = min_x; tx <= max_x; ++tx) {
				int dx = tx - lx;
				int dy = ty - ly;
				float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
				if (dist > rad) continue;

				// Robust Line-of-Sight Raycasting (Wall occlusion vs Window transparency)
				bool occluded = false;
				if (map && (dx != 0 || dy != 0)) {
					auto isWallOcclusion = [&](int check_x, int check_y) -> bool {
						Tile* t = map->getTile(check_x, check_y, current_floor);
						if (!t) return false;
						for (Item* item : t->items) {
							if (!item) continue;
							const ItemType& it = g_items[item->getID()];
							if (item->isWall() || it.isWall || (it.brush && it.brush->isWall())) {
								if (it.isOpen) return false;
								std::string lname = it.name;
								std::transform(lname.begin(), lname.end(), lname.begin(), ::tolower);
								if (lname.find("window") != std::string::npos || lname.find("hatch") != std::string::npos) {
									return false;
								}
								return true;
							}
						}
						return false;
					};

					// If light source is mounted on a wall, only project away from the wall
					Tile* origin_tile = map->getTile(lx, ly, current_floor);
					if (origin_tile) {
						for (Item* item : origin_tile->items) {
							if (!item) continue;
							const ItemType& it = g_items[item->getID()];
							if (item->isWall() || it.isWall || (it.brush && it.brush->isWall())) {
								if (it.hookSouth || origin_tile->hasProperty(HOOK_SOUTH)) {
									if (ty < ly) occluded = true;
								}
								if (it.hookEast || origin_tile->hasProperty(HOOK_EAST)) {
									if (tx < lx) occluded = true;
								}
							}
						}
					}

					if (!occluded) {
						int num_samples = std::max(std::abs(tx - lx), std::abs(ty - ly)) * 2 + 1;
						float fx0 = static_cast<float>(lx) + 0.5f;
						float fy0 = static_cast<float>(ly) + 0.5f;
						float fx1 = static_cast<float>(tx) + 0.5f;
						float fy1 = static_cast<float>(ty) + 0.5f;

						int last_cx = lx;
						int last_cy = ly;
						for (int s = 1; s < num_samples; ++s) {
							float t_param = static_cast<float>(s) / static_cast<float>(num_samples);
							int cx = static_cast<int>(std::floor(fx0 + (fx1 - fx0) * t_param));
							int cy = static_cast<int>(std::floor(fy0 + (fy1 - fy0) * t_param));

							if (cx == tx && cy == ty) {
								break;
							}
							if (cx == lx && cy == ly) {
								continue;
							}

							if (cx != last_cx || cy != last_cy) {
								last_cx = cx;
								last_cy = cy;

								if (isWallOcclusion(cx, cy)) {
									occluded = true;
									break;
								}
							}
						}
					}
				}

				if (occluded) continue;

				float norm_dist = dist / static_cast<float>(rad);
				float falloff = std::pow(0.5f * (1.0f + std::cos(norm_dist * 3.14159265358979323846f)), 0.85f);
				float a_val = falloff * base_alpha;

				int gx = tx - map_x;
				int gy = ty - map_y;
				if (gx >= 0 && gx < w && gy >= 0 && gy < h) {
					int idx = gy * w + gx;
					float cur_r = lr * a_val;
					float cur_g = lg * a_val;
					float cur_b = lb * a_val;
					float cur_a = 255.0f * a_val;

					r_acc[idx] = std::max(r_acc[idx], cur_r) + (cur_r * (1.0f - r_acc[idx] / 255.0f) * 0.15f);
					g_acc[idx] = std::max(g_acc[idx], cur_g) + (cur_g * (1.0f - g_acc[idx] / 255.0f) * 0.15f);
					b_acc[idx] = std::max(b_acc[idx], cur_b) + (cur_b * (1.0f - b_acc[idx] / 255.0f) * 0.15f);
					a_acc[idx] = std::max(a_acc[idx], cur_a);

					r_acc[idx] = std::min(255.0f, r_acc[idx]);
					g_acc[idx] = std::min(255.0f, g_acc[idx]);
					b_acc[idx] = std::min(255.0f, b_acc[idx]);
				}
			}
		}
	}

	lightmap_pixels.resize(grid_size * 4);
	for (size_t i = 0; i < grid_size; ++i) {
		lightmap_pixels[i * 4 + 0] = static_cast<uint8_t>(r_acc[i]);
		lightmap_pixels[i * 4 + 1] = static_cast<uint8_t>(g_acc[i]);
		lightmap_pixels[i * 4 + 2] = static_cast<uint8_t>(b_acc[i]);
		lightmap_pixels[i * 4 + 3] = static_cast<uint8_t>(a_acc[i]);
	}

	if (texture == 0) {
		glGenTextures(1, &texture);
	}
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F); // GL_CLAMP_TO_EDGE
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, lightmap_pixels.data());

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glColor4ub(255, 255, 255, 255);

	// Render 1 Single Quad for the entire lightmap (Zero Overdraw Bottleneck)
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(draw_x, draw_y);
	glTexCoord2f(1.0f, 0.0f); glVertex2f(draw_x + draw_width, draw_y);
	glTexCoord2f(1.0f, 1.0f); glVertex2f(draw_x + draw_width, draw_y + draw_height);
	glTexCoord2f(0.0f, 1.0f); glVertex2f(draw_x, draw_y + draw_height);
	glEnd();

	glDisable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void LightDrawer::setGlobalLightColor(uint8_t color) {
	global_color = colorFromEightBit(color);
}

void LightDrawer::addLight(int map_x, int map_y, int map_z, const SpriteLight& light) {
	if (map_z <= GROUND_LAYER) {
		map_x -= (GROUND_LAYER - map_z);
		map_y -= (GROUND_LAYER - map_z);
	}

	if (map_x <= 0 || map_x >= MAP_MAX_WIDTH || map_y <= 0 || map_y >= MAP_MAX_HEIGHT) {
		return;
	}

	uint8_t intensity = std::min(light.intensity, static_cast<uint8_t>(MaxLightIntensity));
	lights.push_back(Light { static_cast<uint16_t>(map_x), static_cast<uint16_t>(map_y), light.color, intensity });
}

void LightDrawer::clear() noexcept {
	lights.clear();
}

void LightDrawer::unloadGLTexture() {
	if (texture != 0) {
		glDeleteTextures(1, &texture);
		texture = 0;
	}
}
