#ifndef RME_IMGUI_PALETTE_H_
#define RME_IMGUI_PALETTE_H_

#include "main.h"
#include "tileset.h"

class ImGuiPalette {
public:
	static void DrawToolsWindow();
	static void DrawBrushSizeWindow();
	static void DrawTilesetWindow();

	static TilesetCategoryType selected_category;
};

#endif // RME_IMGUI_PALETTE_H_
