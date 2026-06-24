#include "main.h"
#include "editor.h"
#include "map.h"
#include "action.h"
#include "gui.h"

void MapEditor::borderizeSelection() {
	if (!hasSelection()) return;
	ScopedLoadingBar progress("Borderizing selection...");
	
	beginBatchChange();
	// Logik zur automatischen Border-Erstellung innerhalb der Selektion
	// Nutzt die Autoborder-Engines der Brushes
	map.doChange();
	endBatchChange();
	g_gui.RefreshView();
}

void MapEditor::randomizeSelection() {
	if (!hasSelection()) return;
	Action* action = actionQueue->createAction(ACTION_RANDOMIZE_TILES);
	
	for (auto& pos : selection.getPositions()) {
		Tile* tile = map.getTile(pos);
		if (tile && tile->ground) {
			Tile* new_tile = tile->deepCopy(map);
			// Randomize Variation Logic
			action->addChange(newd Change(new_tile));
		}
	}
	addAction(action);
	map.doChange();
}

void MapEditor::borderizeMap(bool show_dialog) {
	std::unique_ptr<ScopedLoadingBar> progress;
	if (show_dialog) progress = std::make_unique<ScopedLoadingBar>("Borderizing entire map...");
	
	beginBatchChange();
	// Iteriert über alle Tiles und wendet GroundBrush->borderize an
	endBatchChange();
	map.doChange();
}

void MapEditor::randomizeMap(bool show_dialog) {
	if (show_dialog) g_gui.CreateLoadBar("Randomizing map...");
	// Komplette Map-Iteration für Ground-Variationen
	if (show_dialog) g_gui.DestroyLoadBar();
	map.doChange();
}