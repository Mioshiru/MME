#include "map_diagnostic_window.h"
#include "editor.h"
#include "gui.h"
#include "map.h"
#include "tile.h"
#include "item.h"
#include "spawn.h"
#include "creature.h"
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <set>

#include "style_manager.h"

enum {
	DIAG_BTN_SCAN = wxID_HIGHEST + 300,
	DIAG_BTN_FIX,
	DIAG_LIST_CTRL
};

BEGIN_EVENT_TABLE(MapDiagnosticDialog, wxDialog)
EVT_BUTTON(DIAG_BTN_SCAN, MapDiagnosticDialog::OnClickScan)
EVT_BUTTON(DIAG_BTN_FIX, MapDiagnosticDialog::OnClickAutoFix)
EVT_LIST_ITEM_SELECTED(DIAG_LIST_CTRL, MapDiagnosticDialog::OnItemSelect)
EVT_BUTTON(wxID_CANCEL, MapDiagnosticDialog::OnClickClose)
END_EVENT_TABLE()

MapDiagnosticDialog::MapDiagnosticDialog(wxWindow* parent, Editor& editor) :
	wxDialog(parent, wxID_ANY, "Map Diagnostic & Health Scanner", wxDefaultPosition, wxSize(640, 480), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	editor(editor) {

	SetBackgroundColour(wxColour(16, 28, 48));
	SetForegroundColour(wxColour(240, 245, 255));

	wxSizer* topsizer = new wxBoxSizer(wxVERTICAL);

	// Title
	wxStaticText* header = new wxStaticText(this, wxID_ANY, "Map Analysis & Quality Check");
	wxFont headerFont = header->GetFont();
	headerFont.SetPointSize(12);
	headerFont.SetWeight(wxFONTWEIGHT_BOLD);
	header->SetFont(headerFont);
	header->SetForegroundColour(wxColor(240, 210, 120));
	topsizer->Add(header, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 10);

	issueListCtrl = new wxListCtrl(this, DIAG_LIST_CTRL, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
	issueListCtrl->InsertColumn(0, "Category", wxLIST_FORMAT_LEFT, 140);
	issueListCtrl->InsertColumn(1, "Position", wxLIST_FORMAT_LEFT, 100);
	issueListCtrl->InsertColumn(2, "Description", wxLIST_FORMAT_LEFT, 350);
	issueListCtrl->SetBackgroundColour(wxColour(10, 20, 35));
	issueListCtrl->SetForegroundColour(wxColour(240, 245, 255));

	topsizer->Add(issueListCtrl, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);

	wxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	buttonSizer->Add(new wxButton(this, DIAG_BTN_SCAN, "Start Scan"), 0, wxRIGHT, 10);
	buttonSizer->Add(new wxButton(this, DIAG_BTN_FIX, "Auto-Fix All"), 0, wxRIGHT, 10);
	buttonSizer->Add(new wxButton(this, wxID_CANCEL, "Close"), 0);

	topsizer->Add(buttonSizer, 0, wxALIGN_RIGHT | wxALL, 10);
	SetSizerAndFit(topsizer);

	RME::UI::StyleManager::ApplyThemeRecursively(this, RME::UI::StyleManager::GetTheme());
}

MapDiagnosticDialog::~MapDiagnosticDialog() {
}

void MapDiagnosticDialog::runDiagnostics() {
	issues.clear();
	issueListCtrl->DeleteAllItems();

	std::set<uint32_t> usedUids;

	Map& map = editor.map;
	for (int z = 0; z < 16; ++z) {
		for (int y = 0; y < map.getHeight(); ++y) {
			for (int x = 0; x < map.getWidth(); ++x) {
				Tile* tile = map.getTile(x, y, z);
				if (!tile) continue;

				Position pos = tile->getPosition();

				// Check 1: Orphan Spawn
				if (tile->spawn && !tile->hasGround()) {
					DiagnosticIssue issue;
					issue.pos = pos;
					issue.category = "Orphan Spawn";
					issue.description = "Spawn is located on a tile without walkable ground!";
					issues.push_back(issue);
				}

				// Check 2: Floating Walls without Ground & UniqueIDs
				bool hasWall = false;
				for (Item* item : tile->items) {
					if (item && item->isWall()) {
						hasWall = true;
					}
					if (item) {
						uint32_t uid = item->getUniqueID();
						if (uid != 0) {
							if (usedUids.find(uid) != usedUids.end()) {
								DiagnosticIssue issue;
								issue.pos = pos;
								issue.category = "Duplicate UID";
								issue.description = wxString::Format("UniqueID %d is assigned multiple times!", uid);
								issues.push_back(issue);
							} else {
								usedUids.insert(uid);
							}
						}
					}
				}

				if (hasWall && !tile->hasGround()) {
					DiagnosticIssue issue;
					issue.pos = pos;
					issue.category = "Floating Wall";
					issue.description = "Wall segment without underlying ground tile!";
					issues.push_back(issue);
				}
			}
		}
	}

	for (size_t i = 0; i < issues.size(); ++i) {
		long idx = issueListCtrl->InsertItem((long)i, issues[i].category);
		issueListCtrl->SetItem(idx, 1, wxString::Format("(%d, %d, %d)", issues[i].pos.x, issues[i].pos.y, issues[i].pos.z));
		issueListCtrl->SetItem(idx, 2, issues[i].description);
	}

	g_gui.SetStatusText(wxString::Format("Scan finished: %d potential issues found.", (int)issues.size()));
}

void MapDiagnosticDialog::OnClickScan(wxCommandEvent& WXUNUSED(event)) {
	runDiagnostics();
}

void MapDiagnosticDialog::autoFixAll() {
	Map& map = editor.map;
	int fixed_count = 0;

	for (const auto& issue : issues) {
		Tile* tile = map.getTile(issue.pos);
		if (!tile) continue;

		if (issue.category == "Orphan House Tile" && tile->getHouseID() != 0) {
			tile->setHouseID(0);
			fixed_count++;
		}
	}

	map.doChange();
	g_gui.SetStatusText(wxString::Format("Auto-Fix finished: %d issues automatically resolved.", fixed_count));
	runDiagnostics();
}

void MapDiagnosticDialog::OnClickAutoFix(wxCommandEvent& WXUNUSED(event)) {
	autoFixAll();
}

void MapDiagnosticDialog::OnItemSelect(wxListEvent& event) {
	long idx = event.GetIndex();
	if (idx >= 0 && idx < (long)issues.size()) {
		g_gui.SetScreenCenterPosition(issues[idx].pos);
	}
}

void MapDiagnosticDialog::OnClickClose(wxCommandEvent& WXUNUSED(event)) {
	EndModal(wxID_CANCEL);
}
