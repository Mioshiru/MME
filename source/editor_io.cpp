#include "main.h"
#include "editor.h"
#include "iomap_otbm.h"
#include "gui.h"

bool MapEditor::saveMap(FileName filename, bool show_dialog) {
	if (filename.empty()) filename = map.getFilename();
	if (filename.empty()) return false;

	std::unique_ptr<ScopedLoadingBar> progress;
	if (show_dialog) progress = std::make_unique<ScopedLoadingBar>("Saving map...");

	IOMapOTBM saver(map.getVersion());
	if (saver.saveMap(map, filename)) {
		map.clearChanges();
		return true;
	} else {
		g_gui.PopupDialog("Save Error", saver.getError(), wxOK | wxICON_ERROR);
		return false;
	}
}

void MapEditor::exportMiniMap(FileName filename, int floor, bool show_dialog) {
	map.exportMinimap(filename, floor, show_dialog);
}

void MapEditor::clearInvalidHouseTiles(bool show_dialog) {
	map.cleanInvalidTiles(show_dialog);
	map.doChange();
}

void MapEditor::clearModifiedTileState(bool show_dialog) {
	for (auto it = map.begin(); it != map.end(); ++it) {
		if (Tile* t = (*it)->get()) t->setModified(false);
	}
	if (show_dialog) g_gui.SetStatusText("Modified state cleared.");
}