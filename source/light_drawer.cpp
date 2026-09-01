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
	// Match OTClient: slightly blue-tinted shadow instead of pure black
	// This matches the Tibia client's night-time tint color
	global_color = wxColor(8, 8, 20, 255);
}

LightDrawer::~LightDrawer() {
	unloadGLTexture();
	lights.clear();
}

void LightDrawer::draw(int map_x, int map_y, int end_x, int end_y, int scroll_x, int scroll_y, Map* map, int current_floor, float shader_time) {
	int w = (end_x - map_x) + 2;
	int h = (end_y - map_y) + 2;
	if (w <= 0 || h <= 0) return;

	const int draw_x = map_x * TileSize - scroll_x;
	const int draw_y = map_y * TileSize - scroll_y;
	int draw_width = w * TileSize;
	int draw_height = h * TileSize;

	glDisable(GL_TEXTURE_2D);
	glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);

	float ambient_val = std::min(1.0f, std::max(0.0f, g_settings.getFloat(Config::LIGHT_AMBIENT)));
	
	// Automatic Dungeon Depth Dimming:
	// Surface (floor <= 7) has standard ambient daylight.
	// Underground floors (8..15) gradually become darker and moodier to feel like real caves/dungeons!
	if (current_floor > 7) {
		int depth = current_floor - 7; // 1..8
		float depth_factor = 1.0f - (float(depth) * 0.045f);
		ambient_val = std::clamp(ambient_val * depth_factor, 0.25f, 1.0f);
	}

	uint8_t ambient_alpha = static_cast<uint8_t>(255 * (1.0f - ambient_val));
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
		float lr = static_cast<float>(light_color.Red());
		float lg = static_cast<float>(light_color.Green());
		float lb = static_cast<float>(light_color.Blue());

		// Match OTClient: softer light contribution curve
		// Client uses squared falloff and caps individual light brightness
		float rad_factor = std::min(15, rad) / 15.0f;
		float base_alpha = (0.35f + rad_factor * 0.40f) * intensity_setting;
		base_alpha = std::min(base_alpha, 0.85f); // Never fully overpower ambient

		int min_x = std::max(map_x, lx - rad);
		int max_x = std::min(end_x + 1, lx + rad);
		int min_y = std::max(map_y, ly - rad);
		int max_y = std::min(end_y + 1, ly + rad);

		for (int y = min_y; y <= max_y; ++y) {
			int gy = y - map_y;
			if (gy < 0 || gy >= h) continue;

			for (int x = min_x; x <= max_x; ++x) {
				int gx = x - map_x;
				if (gx < 0 || gx >= w) continue;

				float dx = static_cast<float>(x - lx);
				float dy = static_cast<float>(y - ly);
				float dist = std::sqrt(dx * dx + dy * dy);

				if (dist <= static_cast<float>(rad)) {
					// OTClient uses quadratic falloff for smoother light edges
					float linear_falloff = (1.0f - (dist / static_cast<float>(rad)));
					if (linear_falloff <= 0.0f) continue;
					float falloff = linear_falloff * linear_falloff; // squared = softer edges

					size_t idx = gy * w + gx;
					float cur_r = lr * falloff;
					float cur_g = lg * falloff;
					float cur_b = lb * falloff;
					float cur_a = 255.0f * (falloff * base_alpha);

					r_acc[idx] = std::min(255.0f, r_acc[idx] + cur_r);
					g_acc[idx] = std::min(255.0f, g_acc[idx] + cur_g);
					b_acc[idx] = std::min(255.0f, b_acc[idx] + cur_b);
					a_acc[idx] = std::max(a_acc[idx], cur_a);
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
