#include "map_diff_window.h"
#include "editor.h"
#include "gui.h"
#include "iomap_otbm.h"
#include "map.h"
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/msgdlg.h>

enum {
	DIFF_BTN_COMPARE = wxID_HIGHEST + 400,
	DIFF_BTN_CLEAR
};

BEGIN_EVENT_TABLE(MapDiffDialog, wxDialog)
EVT_BUTTON(DIFF_BTN_COMPARE, MapDiffDialog::OnClickCompare)
EVT_BUTTON(DIFF_BTN_CLEAR, MapDiffDialog::OnClickClear)
EVT_BUTTON(wxID_CANCEL, MapDiffDialog::OnClickClose)
END_EVENT_TABLE()

std::set<Position> MapDiffDialog::addedPositions;
std::set<Position> MapDiffDialog::removedPositions;
bool MapDiffDialog::isDiffModeActive = false;

MapDiffDialog::MapDiffDialog(wxWindow* parent, Editor& editor) :
	wxDialog(parent, wxID_ANY, "Map Diff & Versionsvergleich", wxDefaultPosition, wxSize(480, 240), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	editor(editor) {

	wxSizer* topsizer = new wxBoxSizer(wxVERTICAL);

	// Title
	wxStaticText* header = new wxStaticText(this, wxID_ANY, "Visueller Map-Versionsvergleich (OTBM Diff)");
	wxFont headerFont = header->GetFont();
	headerFont.SetPointSize(12);
	headerFont.SetWeight(wxFONTWEIGHT_BOLD);
	header->SetFont(headerFont);
	header->SetForegroundColour(wxColor(180, 140, 50));
	topsizer->Add(header, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 10);

	wxFlexGridSizer* grid = new wxFlexGridSizer(2, 10, 10);
	grid->AddGrowableCol(1);

	grid->Add(new wxStaticText(this, wxID_ANY, "Referenz Map Datei:"), 0, wxALIGN_CENTER_VERTICAL);
	filePicker = new wxFilePickerCtrl(this, wxID_ANY, "", "Zweite Map auswählen", "*.otbm", wxDefaultPosition, wxDefaultSize, wxFLP_OPEN | wxFLP_FILE_MUST_EXIST);
	grid->Add(filePicker, 1, wxEXPAND);

	topsizer->Add(grid, 1, wxEXPAND | wxALL, 15);

	wxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	buttonSizer->Add(new wxButton(this, DIFF_BTN_COMPARE, "Vergleichen"), 0, wxRIGHT, 10);
	buttonSizer->Add(new wxButton(this, DIFF_BTN_CLEAR, "Overlay löschen"), 0, wxRIGHT, 10);
	buttonSizer->Add(new wxButton(this, wxID_CANCEL, "Schließen"), 0);

	topsizer->Add(buttonSizer, 0, wxALIGN_RIGHT | wxALL, 10);
	SetSizerAndFit(topsizer);
}

MapDiffDialog::~MapDiffDialog() {
}

void MapDiffDialog::OnClickCompare(wxCommandEvent& WXUNUSED(event)) {
	wxFileName fn(filePicker->GetPath());
	if (!fn.FileExists()) {
		wxMessageBox("Bitte wählen Sie eine gültige OTBM-Map-Datei aus!", "Datei nicht gefunden", wxOK | wxICON_ERROR);
		return;
	}

	addedPositions.clear();
	removedPositions.clear();

	IOMapOTBM otbm;
	Map refMap;
	otbm.loadMap(refMap, fn.GetFullPath());

	Map& curMap = editor.map;
	for (int z = 0; z < 16; ++z) {
		for (int y = 0; y < std::max(curMap.getHeight(), refMap.getHeight()); y += 4) {
			for (int x = 0; x < std::max(curMap.getWidth(), refMap.getWidth()); x += 4) {
				QTreeNode* curLeaf = curMap.getLeaf(x, y);
				QTreeNode* refLeaf = refMap.getLeaf(x, y);

				Floor* curFloor = curLeaf ? curLeaf->getFloor(z) : nullptr;
				Floor* refFloor = refLeaf ? refLeaf->getFloor(z) : nullptr;

				for (int ty = 0; ty < 4; ++ty) {
					for (int tx = 0; tx < 4; ++tx) {
						Tile* curTile = curFloor ? curFloor->getTile(tx, ty) : nullptr;
						Tile* refTile = refFloor ? refFloor->getTile(tx, ty) : nullptr;

						Position pos(x + tx, y + ty, z);
						if (curTile && !refTile) {
							addedPositions.insert(pos);
						} else if (!curTile && refTile) {
							removedPositions.insert(pos);
						} else if (curTile && refTile) {
							if (curTile->size() != refTile->size()) {
								addedPositions.insert(pos);
							}
						}
					}
				}
			}
		}
	}

	isDiffModeActive = true;
	g_gui.RefreshView();
	g_gui.SetStatusText(wxString::Format("Map Diff beendet: %d Hinzufügungen, %d Entfernungen.", (int)addedPositions.size(), (int)removedPositions.size()));
	EndModal(wxID_OK);
}

void MapDiffDialog::OnClickClear(wxCommandEvent& WXUNUSED(event)) {
	addedPositions.clear();
	removedPositions.clear();
	isDiffModeActive = false;
	g_gui.RefreshView();
	g_gui.SetStatusText("Map Diff Overlay entfernt.");
	EndModal(wxID_CANCEL);
}

void MapDiffDialog::OnClickClose(wxCommandEvent& WXUNUSED(event)) {
	EndModal(wxID_CANCEL);
}
