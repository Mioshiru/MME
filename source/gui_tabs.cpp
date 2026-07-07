#include "main.h"
#include "gui.h"
#include "editor.h"
#include "main_menubar.h"
#include "application.h"
#include "common_windows.h"
#include "brush.h"
#include "palette_window.h"
#include "lua/lua_api.h"
#include <wx/msgdlg.h>
#include <wx/textdlg.h>

void GUI::SaveCurrentMap(FileName filename, bool showdialog) {
	MapTab* mapTab = GetCurrentMapTab();
	if (mapTab) {
		Editor* editor = mapTab->GetEditor();
		if (editor) {
			editor->saveMap(filename, showdialog);
			const std::string& fname = editor->map.getFilename();
			const Position& position = mapTab->GetScreenCenterPosition();
			std::ostringstream stream; stream << position;
			g_settings.setString(Config::RECENT_EDITED_MAP_PATH, fname);
			g_settings.setString(Config::RECENT_EDITED_MAP_POSITION, stream.str());
		}
	}
	UpdateTitle();
	root->UpdateMenubar(); root->Refresh();
}

bool GUI::NewMap(const wxString& target_dir) {
	ScopedAction action("GUI::NewMap");
	FinishWelcomeDialog();
	
	// Ensure the user actually has a valid version loaded, else trigger choice prompt
	if (loaded_version == CLIENT_VERSION_NONE) {
		wxCommandEvent trigger_event(WELCOME_DIALOG_ACTION);
		trigger_event.SetId(wxID_NEW);
		OnWelcomeDialogAction(trigger_event);
		return true;
	}

	// Memory safety: verify that the graphics/sprite data was actually loaded
	// before constructing the Editor. If loadSpriteMetadata failed or returned
	// an unknown DAT format, accessing sprite_space[id] will be a null pointer
	// dereference and produce a 0xc0000005 access violation.
	if (gfx.getDatFormat() == DAT_FORMAT_UNKNOWN) {
		ClientVersion* cv = getLoadedVersion();
		wxString nameStr = cv ? wxString::FromUTF8(cv->getName()) : wxString("Unknown");
		PopupDialog(root, "Asset Load Error",
			"Failed to load assets for version " + nameStr,
			wxOK | wxICON_ERROR);
		return false;
	}

	Editor* editor;
	try { editor = newd Editor(copybuffer, target_dir); } catch (std::runtime_error& e) {
		PopupDialog(root, "Error!", wxString(e.what(), wxConvUTF8), wxOK); return false;
	}
	auto* mapTab = newd MapTab(tabbook, editor);
	mapTab->OnSwitchEditorMode(mode);

	// Show map properties window once upon map creation
	MapPropertiesWindow* propWindow = newd MapPropertiesWindow(root, mapTab, *editor);
	propWindow->ShowModal();
	propWindow->Destroy();

	editor->map.clearChanges();
	UpdateTitle(); RefreshPalettes();
	root->UpdateMenubar(); root->Refresh();
	return true;
}

void GUI::OpenMap() {
	wxString wildcard = g_settings.getInteger(Config::USE_OTGZ) != 0 ? MAP_LOAD_FILE_WILDCARD_OTGZ : MAP_LOAD_FILE_WILDCARD;
	wxFileDialog dialog(root, "Open map file", wxEmptyString, wxEmptyString, wildcard, wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (dialog.ShowModal() == wxID_OK) LoadMap(dialog.GetPath());
}

bool GUI::LoadMap(const FileName& fileName) {
	FinishWelcomeDialog();
	if (GetCurrentEditor() && !GetCurrentMap().hasChanged() && !GetCurrentMap().hasFile()) CloseCurrentEditor();
	Editor* editor;
	try { editor = newd Editor(copybuffer, fileName); } catch (std::runtime_error& e) {
		PopupDialog(root, "Error!", wxString(e.what(), wxConvUTF8), wxOK); return false;
	}

	// Memory safety: verify that the graphics/sprite data was actually loaded
	// after constructing the Editor (which determines the version and loads it).
	if (gfx.getDatFormat() == DAT_FORMAT_UNKNOWN) {
		ClientVersion* cv = getLoadedVersion();
		wxString nameStr = cv ? wxString::FromUTF8(cv->getName()) : wxString("Unknown");
		PopupDialog(root, "Asset Load Error",
			"Failed to load assets for version " + nameStr,
			wxOK | wxICON_ERROR);
		delete editor;
		return false;
	}
	auto* mapTab = newd MapTab(tabbook, editor);
	mapTab->OnSwitchEditorMode(mode);
	root->AddRecentFile(fileName);
	mapTab->GetView()->FitToMap();
	UpdateTitle();
	ListDialog("Map loader errors", mapTab->GetMap()->getWarnings());
	root->DoQueryImportCreatures();
	FitViewToMap(mapTab);
	root->UpdateMenubar();
	return true;
}

Editor* GUI::GetCurrentEditor() {
	MapTab* mapTab = GetCurrentMapTab();
	return mapTab ? mapTab->GetEditor() : nullptr;
}

MapTab* GUI::GetCurrentMapTab() const {
	if (tabbook && tabbook->GetTabCount() > 0) {
		return dynamic_cast<MapTab*>(tabbook->GetCurrentTab());
	}
	return nullptr;
}

EditorTab* GUI::GetCurrentTab() {
	return tabbook ? tabbook->GetCurrentTab() : nullptr;
}

bool GUI::IsEditorOpen() const {
	return tabbook && tabbook->GetTabCount() > 0;
}

EditorTab* GUI::GetTab(int idx) {
	return tabbook ? tabbook->GetTab(idx) : nullptr;
}

int GUI::GetTabCount() const {
	return tabbook ? tabbook->GetTabCount() : 0;
}

bool GUI::IsAnyEditorOpen() const {
	return IsEditorOpen();
}

Map& GUI::GetCurrentMap() {
	Editor* editor = GetCurrentEditor();
	ASSERT(editor);
	if (!editor) {
		throw std::runtime_error("GetCurrentMap() called with no active editor");
	}
	return editor->map;
}

void GUI::CloseCurrentEditor() {
	if (tabbook && tabbook->GetTabCount() > 0) {
		tabbook->DeleteTab(tabbook->GetSelection());
	}
}

void GUI::SaveMap() {
	MapTab* mapTab = GetCurrentMapTab();
	if (mapTab) {
		mapTab->GetEditor()->saveMap(FileName(), true);
	}
}



int GUI::GetOpenMapCount() const {
	if (!tabbook) return 0;
	int count = 0;
	for (int i = 0; i < tabbook->GetTabCount(); ++i) {
		if (dynamic_cast<MapTab*>(tabbook->GetTab(i))) {
			++count;
		}
	}
	return count;
}

bool GUI::ShouldSave() {
	MapTab* mapTab = GetCurrentMapTab();
	if (mapTab) {
		Editor* editor = mapTab->GetEditor();
		return editor && editor->map.hasChanged();
	}
	return false;
}

wxGLContext* GUI::GetGLContext(wxGLCanvas* win) {
	if (!OGLContext) {
		OGLContext = new wxGLContext(win);
	}
	return OGLContext;
}

double GUI::GetCurrentZoom() {
	MapTab* mapTab = GetCurrentMapTab();
	if (mapTab && mapTab->GetCanvas()) {
		return mapTab->GetCanvas()->GetZoom();
	}
	return 1.0;
}

void GUI::SetCurrentZoom(double zoom) {
	MapTab* mapTab = GetCurrentMapTab();
	if (mapTab && mapTab->GetCanvas()) {
		mapTab->GetCanvas()->SetZoom(zoom);
	}
}

void GUI::RegisterVirtualBrush(const std::string& name, const std::string& data, const std::string& iconName) {
	PrefabBrush* brush = newd PrefabBrush(name, data, iconName);
	g_brushes.addBrush(brush);
	PaletteWindow* palette = GetPalette();
	if (palette) {
		palette->InvalidatePrefabPalette();
	}
}

void GUI::SavePrefabFromCreator() {
	if (!prefab_creator_brush) return;
	const auto& selected = prefab_creator_brush->getSelectedTiles();
	if (selected.empty()) {
		wxMessageBox("No tiles selected in Prefab Creator.", "Error", wxOK | wxICON_ERROR);
		return;
	}

	// Calculate bounds
	int min_x = 0x7FFFFFFF, min_y = 0x7FFFFFFF;
	int max_x = -0x7FFFFFFF, max_y = -0x7FFFFFFF;
	for (const auto& pos : selected) {
		if (pos.x < min_x) min_x = pos.x;
		if (pos.y < min_y) min_y = pos.y;
		if (pos.x > max_x) max_x = pos.x;
		if (pos.y > max_y) max_y = pos.y;
	}

	int width = max_x - min_x + 1;
	int height = max_y - min_y + 1;

	// Prompt for name
	wxString name = wxGetTextFromUser("Enter a name for the new prefab:", "Save Prefab");
	if (name.empty()) return;

	// Create snapshot
	Map& map = GetCurrentMap();
	std::vector<uint8_t> snapshot = map.createRegionSnapshot(min_x, min_y, width, height);
	std::string base64Data = LuaAPI::base64_encode(snapshot.data(), snapshot.size());

	// Register the brush
	RegisterVirtualBrush(std::string(name.utf8_str()), base64Data, "icons/prefab.png");

	// Clear creator selection
	prefab_creator_brush->clear();

	// Select the newly created prefab brush
	Brush* new_brush = g_brushes.getBrush(std::string(name.utf8_str()));
	if (new_brush) {
		SelectBrush(new_brush);
	}
}

void GUI::CycleTab(bool forward) {
	tabbook->CycleTab(forward);
}