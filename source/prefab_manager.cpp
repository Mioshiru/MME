#include "prefab_manager.h"
#include "style_manager.h"
#include "editor.h"
#include "style_manager.h"
#include "gui.h"
#include "style_manager.h"
#include <wx/stattext.h>
#include "style_manager.h"
#include <wx/button.h>
#include "style_manager.h"
#include <wx/sizer.h>
#include "style_manager.h"
#include <wx/textdlg.h>
#include "style_manager.h"
#include <wx/msgdlg.h>
#include "style_manager.h"
#include <wx/filename.h>
#include "style_manager.h"
#include <wx/stdpaths.h>
#include "style_manager.h"
#include "ext/pugixml.hpp"
#include "style_manager.h"
#include "tile.h"
#include "style_manager.h"
#include "item.h"
#include "style_manager.h"
#include "creature.h"
#include "style_manager.h"
#include "spawn.h"
#include "style_manager.h"

enum {
	PREFAB_BTN_SAVE_SELECTION = wxID_HIGHEST + 200,
	PREFAB_BTN_PASTE_SELECTED,
	PREFAB_BTN_DELETE_SELECTED,
	PREFAB_BTN_RENAME_SELECTED
};

BEGIN_EVENT_TABLE(PrefabLibraryDialog, wxDialog)
EVT_BUTTON(PREFAB_BTN_SAVE_SELECTION, PrefabLibraryDialog::OnClickSaveCurrentSelection)
EVT_BUTTON(PREFAB_BTN_PASTE_SELECTED, PrefabLibraryDialog::OnClickPastePrefab)
EVT_BUTTON(PREFAB_BTN_DELETE_SELECTED, PrefabLibraryDialog::OnClickDeletePrefab)
EVT_BUTTON(PREFAB_BTN_RENAME_SELECTED, PrefabLibraryDialog::OnClickRenamePrefab)
EVT_BUTTON(wxID_CANCEL, PrefabLibraryDialog::OnClickClose)
END_EVENT_TABLE()

static wxFileName GetPrefabsFilePath() {
	wxString base_path = g_gui.GetExecDirectory();
	if (base_path.IsEmpty()) {
		base_path = wxStandardPaths::Get().GetUserDataDir();
	}
	return wxFileName(base_path, "prefabs.xml");
}

PrefabManager::PrefabManager() {
	loadPrefabs();
}

PrefabManager::~PrefabManager() {
	savePrefabs();
	for (auto& pair : prefabs) {
		delete pair.second;
	}
	prefabs.clear();
}

PrefabManager& PrefabManager::getInstance() {
	static PrefabManager instance;
	return instance;
}

void PrefabManager::addPrefab(const wxString& name, CopyBuffer* buffer) {
	if (!buffer) return;
	auto it = prefabs.find(name);
	if (it != prefabs.end()) {
		delete it->second;
	}
	prefabs[name] = buffer;
	savePrefabs();
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
		savePrefabs();
	}
}

void PrefabManager::renamePrefab(const wxString& old_name, const wxString& new_name) {
	auto it = prefabs.find(old_name);
	if (it != prefabs.end()) {
		CopyBuffer* buf = it->second;
		prefabs.erase(it);
		prefabs[new_name] = buf;
		savePrefabs();
	}
}

std::vector<wxString> PrefabManager::getPrefabNames() const {
	std::vector<wxString> names;
	for (const auto& pair : prefabs) {
		names.push_back(pair.first);
	}
	return names;
}

void PrefabManager::savePrefabs() {
	pugi::xml_document doc;
	pugi::xml_node root = doc.append_child("prefabs");

	for (const auto& pair : prefabs) {
		const wxString& name = pair.first;
		CopyBuffer* buf = pair.second;
		if (!buf || buf->GetTileCount() == 0) continue;

		pugi::xml_node pNode = root.append_child("prefab");
		pNode.append_attribute("name") = name.ToUTF8();

		Position basePos = buf->getPosition();
		pNode.append_attribute("base_x") = basePos.x;
		pNode.append_attribute("base_y") = basePos.y;
		pNode.append_attribute("base_z") = basePos.z;

		BaseMap& bmap = buf->getBufferMap();
		for (MapIterator it = bmap.begin(); it != bmap.end(); ++it) {
			TileLocation* loc = *it;
			if (!loc || !loc->get()) continue;
			Tile* t = loc->get();

			pugi::xml_node tNode = pNode.append_child("tile");
			tNode.append_attribute("x") = t->getX() - basePos.x;
			tNode.append_attribute("y") = t->getY() - basePos.y;
			tNode.append_attribute("z") = t->getZ() - basePos.z;

			if (t->ground) {
				pugi::xml_node gNode = tNode.append_child("ground");
				gNode.append_attribute("id") = t->ground->getID();
			}
			for (Item* item : t->items) {
				if (item) {
					pugi::xml_node iNode = tNode.append_child("item");
					iNode.append_attribute("id") = item->getID();
					if (item->getSubtype() > 0) {
						iNode.append_attribute("count") = item->getSubtype();
					}
					if (item->getActionID() > 0) {
						iNode.append_attribute("action_id") = item->getActionID();
					}
					if (item->getUniqueID() > 0) {
						iNode.append_attribute("unique_id") = item->getUniqueID();
					}
				}
			}
		}
	}

	wxFileName fn = GetPrefabsFilePath();
	if (!wxDirExists(fn.GetPath())) {
		wxFileName::Mkdir(fn.GetPath(), 511, wxPATH_MKDIR_FULL);
	}
	doc.save_file(fn.GetFullPath().mb_str());
}

void PrefabManager::loadPrefabs() {
	if (loaded) return;
	loaded = true;

	wxFileName fn = GetPrefabsFilePath();
	if (!fn.FileExists()) return;

	pugi::xml_document doc;
	pugi::xml_parse_result res = doc.load_file(fn.GetFullPath().mb_str());
	if (!res) return;

	pugi::xml_node root = doc.child("prefabs");
	if (!root) return;

	for (pugi::xml_node pNode = root.child("prefab"); pNode; pNode = pNode.next_sibling("prefab")) {
		const char* nameStr = pNode.attribute("name").as_string("");
		if (!nameStr || strlen(nameStr) == 0) continue;

		wxString name = wxString::FromUTF8(nameStr);
		CopyBuffer* buf = newd CopyBuffer();
		BaseMap& dest = buf->getBufferMap();

		int base_x = 0x8000;
		int base_y = 0x8000;
		int base_z = 7;

		for (pugi::xml_node tNode = pNode.child("tile"); tNode; tNode = tNode.next_sibling("tile")) {
			int rel_x = tNode.attribute("x").as_int(0);
			int rel_y = tNode.attribute("y").as_int(0);
			int rel_z = tNode.attribute("z").as_int(0);

			int tx = base_x + rel_x;
			int ty = base_y + rel_y;
			int tz = base_z + rel_z;

			TileLocation* newloc = dest.createTileL(tx, ty, tz);
			Tile* destTile = dest.allocator(newloc);

			pugi::xml_node gNode = tNode.child("ground");
			if (gNode) {
				int gid = gNode.attribute("id").as_int(0);
				if (gid > 0) {
					Item* gr = Item::Create(gid);
					if (gr) destTile->addItem(gr);
				}
			}

			for (pugi::xml_node iNode = tNode.child("item"); iNode; iNode = iNode.next_sibling("item")) {
				int iid = iNode.attribute("id").as_int(0);
				if (iid > 0) {
					Item* itm = Item::Create(iid);
					if (itm) {
						int cnt = iNode.attribute("count").as_int(0);
						if (cnt > 0) itm->setSubtype(cnt);
						int aid = iNode.attribute("action_id").as_int(0);
						if (aid > 0) itm->setActionID(aid);
						int uid = iNode.attribute("unique_id").as_int(0);
						if (uid > 0) itm->setUniqueID(uid);
						destTile->addItem(itm);
					}
				}
			}
			dest.setTile(destTile);
		}

		if (buf->GetTileCount() > 0) {
			prefabs[name] = buf;
		} else {
			delete buf;
		}
	}
}

PrefabLibraryDialog::PrefabLibraryDialog(wxWindow* parent, Editor& editor) :
	wxDialog(parent, wxID_ANY, "Prefab & Template Library", wxDefaultPosition, wxSize(480, 420), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
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
	buttonSizer->Add(new wxButton(this, PREFAB_BTN_SAVE_SELECTION, "Save Selection"), 0, wxRIGHT, 6);
	buttonSizer->Add(new wxButton(this, PREFAB_BTN_PASTE_SELECTED, "Stamp / Paste"), 0, wxRIGHT, 6);
	buttonSizer->Add(new wxButton(this, PREFAB_BTN_RENAME_SELECTED, "Rename"), 0, wxRIGHT, 6);
	buttonSizer->Add(new wxButton(this, PREFAB_BTN_DELETE_SELECTED, "Delete"), 0, wxRIGHT, 6);
	buttonSizer->Add(new wxButton(this, wxID_CANCEL, "Close"), 0);

	topsizer->Add(buttonSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 10);
	SetSizerAndFit(topsizer);
	RME::UI::StyleManager::ApplyThemeRecursively(this, RME::UI::StyleManager::GetTheme());
}

PrefabLibraryDialog::~PrefabLibraryDialog() {
}

void PrefabLibraryDialog::OnClickSaveCurrentSelection(wxCommandEvent& WXUNUSED(event)) {
	if (editor.selection.size() == 0) {
		wxMessageBox("Please select an area on the map first!", "No Selection", wxOK | wxICON_INFORMATION);
		return;
	}

	wxTextEntryDialog nameDialog(this, "Enter a name for the new prefab:", "Save Prefab");
	if (nameDialog.ShowModal() == wxID_OK) {
		wxString name = nameDialog.GetValue().Trim().Trim(false);
		if (!name.IsEmpty()) {
			CopyBuffer* buf = newd CopyBuffer();
			buf->copy(editor, g_gui.GetCurrentFloor());
			PrefabManager::getInstance().addPrefab(name, buf);

			if (prefabListBox->FindString(name) == wxNOT_FOUND) {
				prefabListBox->Append(name);
			}
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
		g_gui.SetStatusText("Prefab '" + name + "' ready to stamp (Press 'Z' / 'R' to rotate).");
		EndModal(wxID_OK);
	}
}

void PrefabLibraryDialog::OnClickDeletePrefab(wxCommandEvent& WXUNUSED(event)) {
	int sel = prefabListBox->GetSelection();
	if (sel == wxNOT_FOUND) {
		wxMessageBox("Please select a prefab to delete!", "Notice", wxOK | wxICON_INFORMATION);
		return;
	}

	wxString name = prefabListBox->GetString(sel);
	if (wxMessageBox("Are you sure you want to delete prefab '" + name + "'?", "Confirm Delete", wxYES_NO | wxICON_QUESTION) == wxYES) {
		PrefabManager::getInstance().removePrefab(name);
		prefabListBox->Delete(sel);
		g_gui.SetStatusText("Prefab '" + name + "' deleted.");
	}
}

void PrefabLibraryDialog::OnClickRenamePrefab(wxCommandEvent& WXUNUSED(event)) {
	int sel = prefabListBox->GetSelection();
	if (sel == wxNOT_FOUND) {
		wxMessageBox("Please select a prefab to rename!", "Notice", wxOK | wxICON_INFORMATION);
		return;
	}

	wxString old_name = prefabListBox->GetString(sel);
	wxTextEntryDialog nameDialog(this, "Enter a new name for the prefab:", "Rename Prefab", old_name);
	if (nameDialog.ShowModal() == wxID_OK) {
		wxString new_name = nameDialog.GetValue().Trim().Trim(false);
		if (!new_name.IsEmpty() && new_name != old_name) {
			PrefabManager::getInstance().renamePrefab(old_name, new_name);
			prefabListBox->SetString(sel, new_name);
			g_gui.SetStatusText("Prefab renamed to '" + new_name + "'.");
		}
	}
}

void PrefabLibraryDialog::OnClickClose(wxCommandEvent& WXUNUSED(event)) {
	EndModal(wxID_CANCEL);
}
