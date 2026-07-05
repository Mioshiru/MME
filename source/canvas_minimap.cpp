#include "main.h"
#include "map_display.h"
#include "gui.h"
#include "editor.h"
#include "map.h"

void MapCanvas::UpdateMinimapTexture() {
	if (!g_gui.IsEditorOpen()) return;

	uint32_t current_time = wxGetLocalTimeMillis().GetValue();
	if (wxGLContext::GetCurrentContext() != nullptr && minimap_tex_id != 0 && current_time - last_minimap_update_time < 200) return;
	if (wxGLContext::GetCurrentContext() != nullptr) {
		last_minimap_update_time = current_time;
	}

	if (wxGLContext::GetCurrentContext() != nullptr && minimap_tex_id == 0) {
		glGenTextures(1, &minimap_tex_id);
		glBindTexture(GL_TEXTURE_2D, minimap_tex_id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 180, 180, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	}

	int center_x, center_y;
	GetScreenCenter(&center_x, &center_y);

	int span_w = (int)(180.0f * minimap_zoom);
	int span_h = (int)(180.0f * minimap_zoom);
	minimap_span_w = span_w;
	minimap_span_h = span_h;

	int start_x = center_x - span_w / 2;
	int start_y = center_y - span_h / 2;
	minimap_start_x = start_x;
	minimap_start_y = start_y;

	memset(minimap_pixels, 0, sizeof(minimap_pixels));

	for (int y = 0; y < 180; ++y) {
		for (int x = 0; x < 180; ++x) {
			int map_x = start_x + (int)(x * ((float)span_w / 180.0f));
			int map_y = start_y + (int)(y * ((float)span_h / 180.0f));
			Tile* tile = editor.map.getTile(map_x, map_y, floor);
			if (tile) {
				uint8_t color_idx = tile->getMiniMapColor();
				if (color_idx) {
					int idx = (y * 180 + x) * 3;
					minimap_pixels[idx] = minimap_color[color_idx].red;
					minimap_pixels[idx+1] = minimap_color[color_idx].green;
					minimap_pixels[idx+2] = minimap_color[color_idx].blue;
				}
			}
		}
	}

	if (wxGLContext::GetCurrentContext() != nullptr && minimap_tex_id != 0) {
		glBindTexture(GL_TEXTURE_2D, minimap_tex_id);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 180, 180, GL_RGB, GL_UNSIGNED_BYTE, minimap_pixels);
	}
}