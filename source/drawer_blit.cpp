#include "main.h"
#include "map_drawer.h"
#include "graphics.h"
#include "gui.h"
#include "tile.h"
#include "item.h"
#include "creature.h"
#include "ground_brush.h"

extern std::vector<RME_Rendering::MapVertex> g_vbo_vertices;
extern std::vector<DrawBatch> g_vbo_batches;
extern bool g_vbo_building;
extern float g_vbo_current_shader_flag;
extern std::map<uint32_t, std::vector<DoodadInstance>> g_pending_instances;

void MapDrawer::DrawTile(TileLocation* location) {
	if (!location) return;
	Tile* tile = location->get();
	if (!tile || (options.show_only_modified && !tile->isModified())) return;

	int map_x = location->getX(), map_y = location->getY(), map_z = location->getZ();
	int offset = (map_z <= GROUND_LAYER) ? (GROUND_LAYER - map_z) * TileSize : TileSize * (floor - map_z);
	int draw_x = ((map_x * TileSize) - view_scroll_x) - offset;
	int draw_y = ((map_y * TileSize) - view_scroll_y) - offset;

	uint8_t r = 255, g = 255, b = 255;
	// ... (Filter-Logik für Tile-Farben)

	if (tile->ground) {
		BlitItem(draw_x, draw_y, tile, tile->ground, false, r, g, b);
	}

	if (zoom < 10.0 || !options.hide_items_when_zoomed) {
		for (Item* it : tile->items) {
			BlitItem(draw_x, draw_y, tile, it, false, 255, 255, 255);
		}
		if (tile->creature && options.show_creatures) BlitCreature(draw_x, draw_y, tile->creature);
	}
}

void MapDrawer::BlitItem(int& sx, int& sy, const Position& pos, Item* item, bool ephemeral, int r, int g, int b, int a, const Tile* tile) {
	ItemType& it = g_items[item->getID()];
	
	// Wasser/Lava Flag für Vertex-Shader Animation setzen
	g_vbo_current_shader_flag = 0.0f;
	if (it.isGroundTile()) {
		if (it.name.find("Water") != std::string::npos) g_vbo_current_shader_flag = 1.0f;
		else if (it.name.find("Lava") != std::string::npos) g_vbo_current_shader_flag = 3.0f;
	}
	if (it.sprite && it.sprite->animator) g_vbo_current_shader_flag = 2.0f;

	GameSprite* spr = it.sprite;
	if (it.isMetaItem() || spr == nullptr) return;

	// Batching-Optimierung für hohe Zoomstufen:
	// Kleine 1x1 Doodads werden für Hardware-Instancing registriert statt einzeln ins VBO geschrieben
	if (zoom > 2.0f && it.doodad_brush && spr->width == 1 && spr->height == 1) {
		int tex = spr->getHardwareID(0, 0, 0, -1, pos.x%spr->pattern_x, pos.y%spr->pattern_y, pos.z%spr->pattern_z, item->getFrame());
		g_pending_instances[tex].push_back({ (float)sx, (float)sy, (uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a, 1.0f });
		return;
	}

	int screenx = sx - spr->getDrawOffset().first;
	int screeny = sy - spr->getDrawOffset().second;
	sx -= spr->getDrawHeight(); sy -= spr->getDrawHeight();

	for (int cx = 0; cx != spr->width; cx++) {
		for (int cy = 0; cy != spr->height; cy++) {
			for (int cf = 0; cf != spr->layers; cf++) {
				int texnum = spr->getHardwareID(cx, cy, cf, -1, pos.x%spr->pattern_x, pos.y%spr->pattern_y, pos.z%spr->pattern_z, item->getFrame());
				glBlitTexture(screenx - cx * TileSize, screeny - cy * TileSize, texnum, r, g, b, a);
			}
		}
	}
}

void MapDrawer::glBlitTexture(int sx, int sy, int texture_number, int red, int green, int blue, int alpha) {
	if (texture_number == 0) return;
	if (g_vbo_building) {
		if (g_vbo_batches.empty() || g_vbo_batches.back().textureId != (GLuint)texture_number) {
			g_vbo_batches.push_back({ (GLuint)texture_number, (uint32_t)g_vbo_vertices.size(), 0 });
		}
		float ts = (float)TileSize;
		uint8_t r = (uint8_t)red, g = (uint8_t)green, b = (uint8_t)blue, a = (uint8_t)alpha;
		g_vbo_vertices.push_back({ (float)sx, (float)sy, 0.0f, 0.0f, r, g, b, a, g_vbo_current_shader_flag });
		g_vbo_vertices.push_back({ (float)sx + ts, (float)sy, 1.0f, 0.0f, r, g, b, a, g_vbo_current_shader_flag });
		g_vbo_vertices.push_back({ (float)sx + ts, (float)sy + ts, 1.0f, 1.0f, r, g, b, a, g_vbo_current_shader_flag });
		g_vbo_vertices.push_back({ (float)sx, (float)sy + ts, 0.0f, 1.0f, r, g, b, a, g_vbo_current_shader_flag });
		g_vbo_batches.back().count += 4;
		return;
	}
	bindTexture(texture_number);
	glColor4ub(red, green, blue, alpha);
	glBegin(GL_QUADS);
	glTexCoord2f(0,0); glVertex2f(sx, sy);
	glTexCoord2f(1,0); glVertex2f(sx+TileSize, sy);
	glTexCoord2f(1,1); glVertex2f(sx+TileSize, sy+TileSize);
	glTexCoord2f(0,1); glVertex2f(sx, sy+TileSize);
	glEnd();
}