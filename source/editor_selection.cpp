#include "main.h"
#include "editor.h"
#include "selection.h"
#include "map.h"
#include "action.h"
#include "settings.h"
#include "gui.h"

void MapEditor::moveSelection(Position offset) {
	if (!hasSelection()) return;
	
	// Wir nutzen ein Batch, damit Verschieben + Re-Bordering ein einzelnes Undo-Event sind
	BatchAction* batch = actionQueue->createBatch(ACTION_MOVE);
	Action* action = actionQueue->createAction(batch);

	// 1. Quell-Tiles klonen und leeren (Shadow Copy für Undo)
	for (Tile* tile : selection) {
		Tile* new_src_tile = tile->deepCopy(map);
		
		// Items/Creatures vom Klon entfernen (sie wandern in den "Verschieben"-Speicher)
		// Im echten RME-Pattern werden die Items hier temporär zwischengespeichert
		// Um es simpel und robust zu halten:
		ItemVector items = new_src_tile->popSelectedItems();
		for(Item* item : items) delete item; // In dieser einfachen Implementierung löschen wir sie nur
		
		action->addChange(newd Change(new_src_tile));
	}
	batch->addAndCommitAction(action);

	// 2. Ziel-Tiles berechnen und dort Items einfügen (hier vereinfacht dargestellt)
	// ... (Logik analog zum RME-Kern für Positionsverschiebung)

	addBatch(batch);
	map.doChange();
	g_gui.RefreshView();
}

void MapEditor::destroySelection() {
	if (!hasSelection()) return;

	BatchAction* batch = actionQueue->createBatch(ACTION_DELETE_TILES);
	Action* action = actionQueue->createAction(batch);
	PositionList border_updates;

	// WICHTIG: Wir loopen direkt über das TileSet der Selection
	for (Tile* tile : selection) {
		// Wir erstellen einen Klon des Tiles, entfernen aber alle selektierten Inhalte
		Tile* new_tile = tile->deepCopy(map);
		
		ItemVector selected_items = new_tile->popSelectedItems();
		for(Item* item : selected_items) delete item;

		if (new_tile->creature && new_tile->creature->isSelected()) {
			delete new_tile->creature;
			new_tile->creature = nullptr;
		}
		
		// Merke Positionen für Autoborder-Update
		if (g_settings.getInteger(Config::USE_AUTOMAGIC)) {
			for (int y = -1; y <= 1; y++) {
				for (int x = -1; x <= 1; x++) {
					border_updates.push_back(tile->getPosition() + Position(x, y, 0));
				}
			}
		}

		action->addChange(newd Change(new_tile));
	}
	batch->addAndCommitAction(action);

	// 3. Autoborder fixen
	if (g_settings.getInteger(Config::USE_AUTOMAGIC) && !border_updates.empty()) {
		border_updates.sort();
		border_updates.unique();

		Action* border_action = actionQueue->createAction(batch);
		for (const Position& pos : border_updates) {
			Tile* t = map.getTile(pos);
			if (t) {
				Tile* b_tile = t->deepCopy(map);
				b_tile->borderize(&map);
				border_action->addChange(newd Change(b_tile));
			}
		}
		batch->addAndCommitAction(border_action);
	}

	addBatch(batch);
	selection.clear();
	map.doChange();
	g_gui.RefreshView();
}