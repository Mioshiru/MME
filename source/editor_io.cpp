#include "main.h"
#include "editor.h"
#include "iomap_otbm.h"
#include "gui.h"

bool MapEditor::exportMiniMap(FileName filename, int floor, bool show_dialog) {
	return map.exportMinimap(filename, floor, show_dialog);
}

void MapEditor::clearInvalidHouseTiles(bool show_dialog) {
	map.cleanInvalidTiles(show_dialog);
	map.doChange();
}

void MapEditor::clearModifiedTileState(bool show_dialog) {
	for (auto it = map.begin(); it != map.end(); ++it) {
		if (Tile* t = (*it)->get()) t->unmodify();
	}
	if (show_dialog) g_gui.SetStatusText("Modified state cleared.");
}