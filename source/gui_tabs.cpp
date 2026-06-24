#include "main.h"
#include "gui.h"
#include "editor.h"
#include "main_menubar.h"
#include "application.h"

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

bool GUI::NewMap() {
	FinishWelcomeDialog();
	Editor* editor;
	try { editor = newd Editor(copybuffer); } catch (std::runtime_error& e) {
		PopupDialog(root, "Error!", wxString(e.what(), wxConvUTF8), wxOK); return false;
	}
	auto* mapTab = newd MapTab(tabbook, editor);
	mapTab->OnSwitchEditorMode(mode);
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
	return editor->map;
}

void GUI::CloseCurrentEditor() {
	if (tabbook && tabbook->GetTabCount() > 0) {
		tabbook->DeleteTab(tabbook->GetTabSelection());
	}
}

void GUI::SaveMap() {
	MapTab* mapTab = GetCurrentMapTab();
	if (mapTab) {
		mapTab->GetEditor()->saveMap(FileName(), true);
	}
}

void GUI::SaveMapAs() {
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
	// Stub implementation
}

void GUI::SavePrefabFromCreator() {
	// Stub implementation
}

void GUI::CycleTab(bool forward) {
	tabbook->CycleTab(forward);
}