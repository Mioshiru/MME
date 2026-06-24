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

#ifndef RME_EDITOR_H
#define RME_EDITOR_H

#include "main.h"
#include "map.h"
#include "selection.h"
#include "editor_interfaces.h"

class MapEditor : public IEditorIO, public IEditorActions, public IEditorSelection {
public:
	MapEditor(CopyBuffer& copybuffer, LiveClient* client);
	MapEditor(CopyBuffer& copybuffer, const FileName& fn);
	MapEditor(CopyBuffer& copybuffer);
	~MapEditor();

protected:
	// Live Server
	LiveServer* live_server;
	LiveClient* live_client;

public:
	// Public members
	ActionQueue* actionQueue;
	Selection selection;
	CopyBuffer& copybuffer;
	GroundBrush* replace_brush;
	Map map; // The map that is being edited

public: // Functions
	// Live Server handling
	LiveClient* GetLiveClient() const;
	LiveServer* GetLiveServer() const;
	LiveSocket& GetLive() const;
	Map* getMap() {
		return &map;
	}
	bool CanEdit() const {
		return true;
	}
	bool IsLocal() const;
	bool IsLive() const;
	bool IsLiveServer() const;
	bool IsLiveClient() const;

	// Server side
	LiveServer* StartLiveServer();
	void CloseLiveServer();
	void BroadcastNodes(DirtyList& dirty_list);

	// Client side
	void QueryNode(int ndx, int ndy, bool underground);
	void SendNodeRequests();

	// Map handling
	bool saveMap(FileName filename, bool show_dialog) override; // "" means default filename

	uint16_t getMapWidth() const {
		return map.width;
	}
	uint16_t getMapHeight() const {
		return map.height;
	}

	wxString getLoaderError() const {
		return map.getError();
	}
	bool importMap(FileName filename, int import_x_offset, int import_y_offset, ImportType house_import_type, ImportType spawn_import_type);
	bool importMiniMap(FileName filename, int import, int import_x_offset, int import_y_offset, int import_z_offset);
	bool exportMiniMap(FileName filename, int floor /*= GROUND_LAYER*/, bool displaydialog);
	bool exportSelectionAsMiniMap(FileName directory, wxString fileName);

	// Adds an action to the action queue (this allows the user to undo the action)
	// Invalidates the action pointer
	void addBatch(BatchAction* action, int stacking_delay = 0);
	void addAction(Action* action, int stacking_delay = 0);

	// Selection
	bool hasSelection() const {
		return selection.size() != 0;
	}
	// Some simple actions that work on the map (these will work through the undo queue)
	// Moves the selected area by the offset
	void moveSelection(Position offset);
	// Deletes all selected items
	void destroySelection();
	// Borderizes the selected region
	void borderizeSelection();
	// Randomizes the ground in the selected region
	void randomizeSelection();

	// Same as above although it applies to the entire map
	// action queue is flushed when these functions are called
	// showdialog is whether a progress bar should be shown
	void borderizeMap(bool showdialog);
	void randomizeMap(bool showdialog);
	void clearInvalidHouseTiles(bool showdialog);
	void clearModifiedTileState(bool showdialog);

	// Border optimization during drawing
	void setDeferBorders(bool defer);
	void commitBorders();

	// Draw using the current brush to the target position
	// alt is whether the ALT key is pressed
	void draw(const Position& offset, bool alt);
	void undraw(const Position& offset, bool alt);
	void draw(const PositionVector& posvec, bool alt);
	void draw(const PositionVector& todraw, PositionVector& toborder, bool alt);
	void undraw(const PositionVector& posvec, bool alt);
	void undraw(const PositionVector& todraw, PositionVector& toborder, bool alt);

protected:
	void drawInternal(const Position offset, bool alt, bool dodraw);
	void drawInternal(const PositionVector& posvec, bool alt, bool dodraw);
	void drawInternal(const PositionVector& todraw, PositionVector& toborder, bool alt, bool dodraw);

	bool defer_borders;
	PositionVector pending_borders;

	MapEditor(const MapEditor&);
	MapEditor& operator=(const MapEditor&);
};

inline void MapEditor::draw(const Position& offset, bool alt) {
	drawInternal(offset, alt, true);
}
inline void MapEditor::undraw(const Position& offset, bool alt) {
	drawInternal(offset, alt, false);
}
inline void MapEditor::draw(const PositionVector& posvec, bool alt) {
	drawInternal(posvec, alt, true);
}
inline void MapEditor::draw(const PositionVector& todraw, PositionVector& toborder, bool alt) {
	drawInternal(todraw, toborder, alt, true);
}
inline void MapEditor::undraw(const PositionVector& posvec, bool alt) {
	drawInternal(posvec, alt, false);
}
inline void MapEditor::undraw(const PositionVector& todraw, PositionVector& toborder, bool alt) {
	drawInternal(todraw, toborder, alt, false);
}

typedef MapEditor Editor;

#endif
