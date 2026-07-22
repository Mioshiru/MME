#include "tfs_exporter.h"
#include "iomap_otbm.h"

#include <wx/msgdlg.h>
#include <wx/ffile.h>
#include <wx/filename.h>

BEGIN_EVENT_TABLE(TFSExportDialog, wxDialog)
EVT_BUTTON(wxID_OK, TFSExportDialog::OnClickExport)
EVT_BUTTON(wxID_CANCEL, TFSExportDialog::OnClickCancel)
END_EVENT_TABLE()

TFSExportDialog::TFSExportDialog(wxWindow* parent, Editor& editor) :
	wxDialog(parent, wxID_ANY, "Export to TFS 1.6 Server Data", wxDefaultPosition, wxSize(480, 260), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	editor(editor) {

	SetBackgroundColour(wxColour(15, 23, 42));
	wxBoxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticText* header = newd wxStaticText(this, wxID_ANY, "Export Server Files (TFS 1.6)");
	header->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	header->SetForegroundColour(wxColour(241, 245, 249));
	topsizer->Add(header, 0, wxALL, 12);

	wxStaticText* info = newd wxStaticText(this, wxID_ANY, "Select the root 'data' folder of your TFS 1.6 server.\nThe editor will export the map (.otbm) into data/world/ and all scripts/NPCs into their respective directories.");
	info->SetForegroundColour(wxColour(203, 213, 225));
	info->Wrap(440);
	topsizer->Add(info, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

	wxStaticText* dirLabel = newd wxStaticText(this, wxID_ANY, "TFS 'data' Directory:");
	dirLabel->SetForegroundColour(wxColour(203, 213, 225));
	topsizer->Add(dirLabel, 0, wxLEFT | wxRIGHT, 12);

	dirPicker = newd wxDirPickerCtrl(this, wxID_ANY, "", "Select TFS data directory");
	topsizer->Add(dirPicker, 0, wxEXPAND | wxALL, 12);

	wxBoxSizer* btnsizer = newd wxBoxSizer(wxHORIZONTAL);
	wxButton* okBtn = newd wxButton(this, wxID_OK, "Export All Files");
	wxButton* cancelBtn = newd wxButton(this, wxID_CANCEL, "Cancel");
	btnsizer->Add(okBtn, 0, wxRIGHT, 8);
	btnsizer->Add(cancelBtn, 0);

	topsizer->Add(btnsizer, 0, wxALIGN_RIGHT | wxALL, 12);
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
