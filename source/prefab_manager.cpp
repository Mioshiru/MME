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

std::vector<wxString> PrefabManager::getPrefabNames() const {
	std::vector<wxString> names;
	for (const auto& pair : prefabs) {
		names.push_back(pair.first);
	}
	return names;
}

PrefabLibraryDialog::PrefabLibraryDialog(wxWindow* parent, Editor& editor) :
	wxDialog(parent, wxID_ANY, "Prefab & Template Bibliothek", wxDefaultPosition, wxSize(420, 380), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	editor(editor) {

	wxSizer* topsizer = new wxBoxSizer(wxVERTICAL);

	// Title
	wxStaticText* header = new wxStaticText(this, wxID_ANY, "Map Vorlagen & Prefab Manager");
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
	buttonSizer->Add(new wxButton(this, PREFAB_BTN_SAVE_SELECTION, "Auswahl als Prefab speichern"), 0, wxRIGHT, 10);
	buttonSizer->Add(new wxButton(this, PREFAB_BTN_PASTE_SELECTED, "Prefab stempeln"), 0, wxRIGHT, 10);
	buttonSizer->Add(new wxButton(this, wxID_CANCEL, "Schließen"), 0);

	topsizer->Add(buttonSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 10);
	SetSizerAndFit(topsizer);
}

PrefabLibraryDialog::~PrefabLibraryDialog() {
}

void PrefabLibraryDialog::OnClickSaveCurrentSelection(wxCommandEvent& WXUNUSED(event)) {
	if (editor.selection.size() == 0) {
		wxMessageBox("Bitte wählen Sie zuerst einen Bereich auf der Karte aus!", "Keine Auswahl", wxOK | wxICON_INFORMATION);
		return;
	}

	wxTextEntryDialog nameDialog(this, "Name für das neue Prefab eingeben:", "Prefab speichern");
	if (nameDialog.ShowModal() == wxID_OK) {
		wxString name = nameDialog.GetValue();
		if (!name.IsEmpty()) {
			CopyBuffer* buf = newd CopyBuffer();
			buf->copyFrom(editor, editor.selection);
			PrefabManager::getInstance().addPrefab(name, buf);

			prefabListBox->Append(name);
			g_gui.SetStatusText("Prefab '" + name + "' gespeichert.");
		}
	}
}

void PrefabLibraryDialog::OnClickPastePrefab(wxCommandEvent& WXUNUSED(event)) {
	int sel = prefabListBox->GetSelection();
	if (sel == wxNOT_FOUND) {
		wxMessageBox("Bitte wählen Sie ein Prefab aus der Liste aus!", "Hinweis", wxOK | wxICON_INFORMATION);
		return;
	}

	wxString name = prefabListBox->GetString(sel);
	CopyBuffer* buf = PrefabManager::getInstance().getPrefab(name);
	if (buf) {
		g_gui.copybuffer->copyFrom(*buf);
		g_gui.StartPasting();
		g_gui.SetStatusText("Prefab '" + name + "' bereit zum Platzieren.");
		EndModal(wxID_OK);
	}
}

void PrefabLibraryDialog::OnClickClose(wxCommandEvent& WXUNUSED(event)) {
	EndModal(wxID_CANCEL);
}
