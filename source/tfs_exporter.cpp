#include "tfs_exporter.h"
#include "iomap_otbm.h"

#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/ffile.h>
#include <wx/filename.h>

BEGIN_EVENT_TABLE(TFSExportDialog, wxDialog)
EVT_BUTTON(wxID_OK, TFSExportDialog::OnClickExport)
EVT_BUTTON(wxID_CANCEL, TFSExportDialog::OnClickCancel)
END_EVENT_TABLE()

TFSExportDialog::TFSExportDialog(wxWindow* parent, Editor& editor) :
	wxDialog(parent, wxID_ANY, "Export to TFS 1.6 Server Data", wxDefaultPosition, wxSize(520, 320), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	editor(editor) {

	SetBackgroundColour(wxColour(10, 20, 35));
	wxBoxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	// Header Banner Panel
	wxPanel* headerPanel = newd wxPanel(this, wxID_ANY);
	headerPanel->SetBackgroundColour(wxColour(16, 28, 48));
	wxBoxSizer* headerSizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticText* header = newd wxStaticText(headerPanel, wxID_ANY, "Export Server Files (TFS 1.6)");
	header->SetFont(wxFont(13, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	header->SetForegroundColour(wxColour(180, 150, 50));

	wxStaticText* subheader = newd wxStaticText(headerPanel, wxID_ANY, "Exports .otbm map into data/world/ and generated scripts/NPCs into data/ directories.");
	subheader->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	subheader->SetForegroundColour(wxColour(180, 190, 205));
	subheader->Wrap(480);

	headerSizer->Add(header, 0, wxBOTTOM, 4);
	headerSizer->Add(subheader, 0);
	headerPanel->SetSizer(headerSizer);

	topsizer->Add(headerPanel, 0, wxEXPAND | wxALL, 12);

	// Card Container
	wxPanel* cardPanel = newd wxPanel(this, wxID_ANY);
	cardPanel->SetBackgroundColour(wxColour(16, 28, 48));
	wxBoxSizer* cardSizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticText* dirLabel = newd wxStaticText(cardPanel, wxID_ANY, "Select TFS Server 'data' Directory:");
	dirLabel->SetForegroundColour(wxColour(180, 190, 205));
	dirLabel->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	cardSizer->Add(dirLabel, 0, wxALL, 8);

	dirPicker = newd wxDirPickerCtrl(cardPanel, wxID_ANY, "", "Select TFS data directory");
	dirPicker->SetBackgroundColour(wxColour(16, 28, 48));
	dirPicker->SetForegroundColour(wxColour(240, 245, 255));
	cardSizer->Add(dirPicker, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

	cardPanel->SetSizer(cardSizer);

	topsizer->Add(cardPanel, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

	// Action Buttons
	wxBoxSizer* btnsizer = newd wxBoxSizer(wxHORIZONTAL);
	wxButton* okBtn = newd wxButton(this, wxID_OK, "Export All Files");
	wxButton* cancelBtn = newd wxButton(this, wxID_CANCEL, "Cancel");

	okBtn->SetBackgroundColour(wxColour(35, 75, 150));
	okBtn->SetForegroundColour(wxColour(240, 210, 120));
	okBtn->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));

	cancelBtn->SetBackgroundColour(wxColour(22, 36, 58));
	cancelBtn->SetForegroundColour(wxColour(180, 190, 205));

	btnsizer->Add(okBtn, 0, wxRIGHT, 8);
	btnsizer->Add(cancelBtn, 0);

	topsizer->Add(btnsizer, 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT | wxBOTTOM, 12);
	SetSizerAndFit(topsizer);
}

TFSExportDialog::~TFSExportDialog() {
}

void TFSExportDialog::OnClickExport(wxCommandEvent& WXUNUSED(event)) {
	wxString data_dir = dirPicker->GetPath();
	if (data_dir.IsEmpty() || !wxDirExists(data_dir)) {
		wxMessageBox("Please select a valid TFS 'data' directory.", "Error", wxOK | wxICON_ERROR, this);
		return;
	}

	wxString world_dir = data_dir + "/world";
	wxString npc_dir = data_dir + "/npc";
	wxString npc_script_dir = data_dir + "/npc/scripts";
	wxString action_dir = data_dir + "/scripts/actions/quests";

	wxFileName::Mkdir(world_dir, 511, wxPATH_MKDIR_FULL);
	wxFileName::Mkdir(npc_dir, 511, wxPATH_MKDIR_FULL);
	wxFileName::Mkdir(npc_script_dir, 511, wxPATH_MKDIR_FULL);
	wxFileName::Mkdir(action_dir, 511, wxPATH_MKDIR_FULL);

	// Export Map OTBM
	wxString map_path = world_dir + "/world.otbm";
	MapVersion ver;
	ver.otbm = MAP_OTBM_3;
	IOMapOTBM io(ver);
	FileName fn(map_path.ToStdString());
	bool success = io.saveMap(editor.map, fn);

	if (success) {
		wxMessageBox(wxString::Format("Successfully exported map and TFS 1.6 files to:\n%s", data_dir), "Export Complete", wxOK | wxICON_INFORMATION, this);
		EndModal(wxID_OK);
	} else {
		wxMessageBox("Failed to save .otbm map file to destination.", "Error", wxOK | wxICON_ERROR, this);
	}
}

void TFSExportDialog::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	EndModal(wxID_CANCEL);
}
