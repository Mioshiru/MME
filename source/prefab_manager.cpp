#include "prefab_manager.h"
#include "editor.h"
#include "gui.h"
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/textdlg.h>
#include <wx/msgdlg.h>

enum {
	PREFAB_BTN_SAVE_SELECTION = wxID_HIGHEST + 200,
	PREFAB_BTN_PASTE_SELECTED
};

BEGIN_EVENT_TABLE(PrefabLibraryDialog, wxDialog)
EVT_BUTTON(PREFAB_BTN_SAVE_SELECTION, PrefabLibraryDialog::OnClickSaveCurrentSelection)
EVT_BUTTON(PREFAB_BTN_PASTE_SELECTED, PrefabLibraryDialog::OnClickPastePrefab)
EVT_BUTTON(wxID_CANCEL, PrefabLibraryDialog::OnClickClose)
END_EVENT_TABLE()

PrefabManager& PrefabManager::getInstance() {
	static PrefabManager instance;
	return instance;
}

void PrefabManager::addPrefab(const wxString& name, CopyBuffer* buffer) {
	if (!buffer) return;
	prefabs[name] = buffer;
}

CopyBuffer* PrefabManager::getPrefab(const wxString& name) {
	auto it = prefabs.find(name);
	if (it != prefabs.end()) {
		return it->second;
	}
	return nullptr;
}

void PrefabManager::removePrefab(const wxString& name) {
	auto it = prefabs.find(name);
	if (it != prefabs.end()) {
		delete it->second;
		prefabs.erase(it);
	}
}

void PrefabManager::renamePrefab(const wxString& old_name, const wxString& new_name) {
	auto it = prefabs.find(old_name);
	if (it != prefabs.end()) {
		CopyBuffer* buf = it->second;
		prefabs.erase(it);
		prefabs[new_name] = buf;
	}
}


std::vector<wxString> PrefabManager::getPrefabNames() const {
	std::vector<wxString> names;
	for (const auto& pair : prefabs) {
		names.push_back(pair.first);
	}
	return names;
}

PrefabLibraryDialog::PrefabLibraryDialog(wxWindow* parent, Editor& editor) :
	wxDialog(parent, wxID_ANY, "Prefab & Template Library", wxDefaultPosition, wxSize(440, 380), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	editor(editor) {

	wxSizer* topsizer = new wxBoxSizer(wxVERTICAL);

	// Title
	wxStaticText* header = new wxStaticText(this, wxID_ANY, "Map Templates & Prefab Manager");
	wxFont headerFont = header->GetFont();
	headerFont.SetPointSize(12);
	headerFont.SetWeight(wxFONTWEIGHT_BOLD);
	header->SetFont(headerFont);
	header->SetForegroundColour(wxColor(180, 140, 50));
	topsizer->Add(header, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 10);

	wxArrayString names;
	for (const auto& name : PrefabManager::getInstance().getPrefabNames()) {
		names.Add(name);
	}

	prefabListBox = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, names, wxLB_SINGLE);
	topsizer->Add(prefabListBox, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);

	wxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	buttonSizer->Add(new wxButton(this, PREFAB_BTN_SAVE_SELECTION, "Save Selection as Prefab"), 0, wxRIGHT, 10);
	buttonSizer->Add(new wxButton(this, PREFAB_BTN_PASTE_SELECTED, "Stamp Prefab"), 0, wxRIGHT, 10);
	buttonSizer->Add(new wxButton(this, wxID_CANCEL, "Close"), 0);

	topsizer->Add(buttonSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 10);
	SetSizerAndFit(topsizer);
}

PrefabLibraryDialog::~PrefabLibraryDialog() {
}

#include "tile.h"
#include "item.h"
#include "creature.h"
#include "spawn.h"

void PrefabLibraryDialog::OnClickSaveCurrentSelection(wxCommandEvent& WXUNUSED(event)) {
	if (editor.selection.size() == 0) {
		wxMessageBox("Please select an area on the map first!", "No Selection", wxOK | wxICON_INFORMATION);
		return;
	}

	wxTextEntryDialog nameDialog(this, "Enter a name for the new prefab:", "Save Prefab");
	if (nameDialog.ShowModal() == wxID_OK) {
		wxString name = nameDialog.GetValue();
		if (!name.IsEmpty()) {
			CopyBuffer* buf = newd CopyBuffer();
			buf->copy(editor, g_gui.GetCurrentFloor());
			PrefabManager::getInstance().addPrefab(name, buf);

			prefabListBox->Append(name);
			g_gui.SetStatusText("Prefab '" + name + "' saved successfully.");
		}
	}
}

void PrefabLibraryDialog::OnClickPastePrefab(wxCommandEvent& WXUNUSED(event)) {
	int sel = prefabListBox->GetSelection();
	if (sel == wxNOT_FOUND) {
		wxMessageBox("Please select a prefab from the list!", "Notice", wxOK | wxICON_INFORMATION);
		return;
	}

	wxString name = prefabListBox->GetString(sel);
	CopyBuffer* buf = PrefabManager::getInstance().getPrefab(name);
	if (buf && buf->GetTileCount() > 0) {
		editor.copybuffer.clear();
		BaseMap& dest = editor.copybuffer.getBufferMap();
		for (MapIterator it = buf->getBufferMap().begin(); it != buf->getBufferMap().end(); ++it) {
			TileLocation* loc = *it;
			if (!loc) continue;
			Tile* srcTile = loc->get();
			if (!srcTile) continue;

			TileLocation* newloc = dest.createTileL(srcTile->getPosition());
			Tile* destTile = dest.allocator(newloc);
			if (srcTile->ground) {
				destTile->house_id = srcTile->house_id;
				destTile->setMapFlags(srcTile->getMapFlags());
				destTile->addItem(srcTile->ground->deepCopy());
			}
			for (Item* item : srcTile->items) {
				if (item) destTile->addItem(item->deepCopy());
			}
			if (srcTile->creature) destTile->creature = srcTile->creature->deepCopy();
			if (srcTile->spawn) destTile->spawn = srcTile->spawn->deepCopy();
		}

		g_gui.PreparePaste();
		g_gui.SetStatusText("Prefab '" + name + "' ready to stamp.");
		EndModal(wxID_OK);
	}
}


void PrefabLibraryDialog::OnClickClose(wxCommandEvent& WXUNUSED(event)) {
	EndModal(wxID_CANCEL);
}
