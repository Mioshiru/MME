#include "main.h"
#include "map_display.h"
#include "gui.h"
#include "editor.h"
#include "map.h"

void MapCanvas::UpdateMinimapTexture() {
	if (!g_gui.IsEditorOpen()) return;

	uint32_t current_time = wxGetLocalTimeMillis().GetValue();
	if (minimap_tex_id != 0 && current_time - last_minimap_update_time < 200) return;
	last_minimap_update_time = current_time;

	if (minimap_tex_id == 0) {
		glGenTextures(1, &minimap_tex_id);
		glBindTexture(GL_TEXTURE_2D, minimap_tex_id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 180, 180, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	}

	int cx, cy; GetScreenCenter(&center_x, &center_y);
	int start_x = std::max(0, center_x - 90);
	int start_y = std::max(0, center_y - 90);
	minimap_start_x = start_x; minimap_start_y = start_y;

	static uint8_t tex_data[180 * 180 * 3];
	memset(tex_data, 0, sizeof(tex_data));

	if (g_gui.IsRenderingEnabled()) {
		for (int y = 0; y < 180; ++y) {
			for (int x = 0; x < 180; ++x) {
				Tile* tile = editor.map.getTile(start_x + x, start_y + y, floor);
				if (tile) {
					uint8_t color_idx = tile->getMiniMapColor();
					if (color_idx) {
						int idx = (y * 180 + x) * 3;
						tex_data[idx] = minimap_color[color_idx].red;
						tex_data[idx+1] = minimap_color[color_idx].green;
						tex_data[idx+2] = minimap_color[color_idx].blue;
					}
				}
			}
		}
	}
	glBindTexture(GL_TEXTURE_2D, minimap_tex_id);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 180, 180, GL_RGB, GL_UNSIGNED_BYTE, tex_data);
}