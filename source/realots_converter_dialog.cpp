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

#include "main.h"
#include "realots_converter_dialog.h"
#include "editor.h"
#include "map.h"
#include "items.h"
#include "iomap_otbm.h"
#include "gui.h"

#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/msgdlg.h>
#include <fstream>
#include <sstream>

enum {
	ID_CONV_OTBM_TO_SEC = 20500,
	ID_CONV_SEC_TO_OTBM,
	ID_CONV_SERVER_ID,
	ID_CONV_CLIENT_ID,
	ID_CONV_OTB_ID,
	ID_CONV_OBJ_SRV,
	ID_CONV_SPAWNS
};

BEGIN_EVENT_TABLE(RealOTSConverterDialog, wxDialog)
	EVT_BUTTON(ID_CONV_OTBM_TO_SEC, RealOTSConverterDialog::OnConvertOTBMToSEC)
	EVT_BUTTON(ID_CONV_SEC_TO_OTBM, RealOTSConverterDialog::OnConvertSECToOTBM)
	EVT_SPINCTRL(ID_CONV_SERVER_ID, RealOTSConverterDialog::OnServerIdChanged)
	EVT_SPINCTRL(ID_CONV_CLIENT_ID, RealOTSConverterDialog::OnClientIdChanged)
	EVT_SPINCTRL(ID_CONV_OTB_ID, RealOTSConverterDialog::OnOtbIdChanged)
	EVT_FILEPICKER_CHANGED(ID_CONV_OBJ_SRV, RealOTSConverterDialog::OnLoadObjectsSrv)
	EVT_BUTTON(ID_CONV_SPAWNS, RealOTSConverterDialog::OnConvertSpawns)
	EVT_BUTTON(wxID_CANCEL, RealOTSConverterDialog::OnClose)
END_EVENT_TABLE()

class ItemPreviewPanel : public wxPanel {
public:
	ItemPreviewPanel(wxWindow* parent, wxWindowID id = wxID_ANY) :
		wxPanel(parent, id, wxDefaultPosition, wxSize(80, 80)),
		client_id(100) {
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		Bind(wxEVT_PAINT, &ItemPreviewPanel::OnPaint, this);
		Bind(wxEVT_ERASE_BACKGROUND, &ItemPreviewPanel::OnEraseBackground, this);
	}

	void SetItem(int cid) {
		client_id = cid;
		Refresh();
	}

private:
	int client_id;

	void OnEraseBackground(wxEraseEvent&) {}

	void OnPaint(const wxPaintEvent&) {
		wxPaintDC dc(this);
		wxRect rect = GetClientRect();

		// Obsidian background
		dc.SetBrush(wxBrush(wxColour(16, 20, 30)));
		dc.SetPen(wxPen(wxColour(16, 20, 30)));
		dc.DrawRectangle(rect);

		// Outer Corporate Gold Border (#FFD700)
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.SetPen(wxPen(wxColour(255, 215, 0), 2));
		dc.DrawRectangle(rect);

		// Inner Accent Gold Border (#D4AF37)
		dc.SetPen(wxPen(wxColour(180, 140, 50, 180), 1));
		dc.DrawRectangle(wxRect(3, 3, rect.width - 6, rect.height - 6));

		ItemType& it = g_items.getItemIdByClientID(client_id);
		if (it.sprite) {
			it.sprite->DrawTo(&dc, SPRITE_SIZE_32x32, rect.width / 2 - 16, rect.height / 2 - 16, 32, 32);
		} else {
			dc.SetTextForeground(wxColour(240, 210, 120));
			wxFont f = GetFont();
			f.SetPointSize(8);
			dc.SetFont(f);
			wxString str = wxString::Format("#%d", client_id);
			wxSize sz = dc.GetTextExtent(str);
			dc.DrawText(str, rect.width / 2 - sz.x / 2, rect.height / 2 - sz.y / 2);
		}
	}
};

RealOTSConverterDialog::RealOTSConverterDialog(wxWindow* parent, Editor* editor) :
	wxDialog(parent, wxID_ANY, "RealOTS / Nostalrius & CipSoft Format Converter", wxDefaultPosition, wxSize(640, 560), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	editor(editor), item_preview_panel(nullptr)
{
	wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
	notebook = new wxNotebook(this, wxID_ANY);

	// ==========================================
	// TAB 1: Map Conversion (OTBM <-> SEC)
	// ==========================================
	wxPanel* tab1 = new wxPanel(notebook, wxID_ANY);
	wxBoxSizer* t1_sizer = new wxBoxSizer(wxVERTICAL);

	// Section A: OTBM -> SEC
	wxStaticBoxSizer* sizer_a = new wxStaticBoxSizer(wxVERTICAL, tab1, "Convert OpenTibia Map (.otbm) -> CipSoft Sectors (.sec)");
	wxFlexGridSizer* grid_a = new wxFlexGridSizer(3, 2, 8, 10);
	grid_a->AddGrowableCol(1, 1);

	grid_a->Add(new wxStaticText(tab1, wxID_ANY, "Source .otbm File:"), 0, wxALIGN_CENTER_VERTICAL);
	otbm_src_picker = new wxFilePickerCtrl(tab1, wxID_ANY, wxEmptyString, "Select OTBM Map", "OTBM files (*.otbm)|*.otbm", wxDefaultPosition, wxDefaultSize, wxFLP_OPEN | wxFLP_FILE_MUST_EXIST);
	grid_a->Add(otbm_src_picker, 1, wxEXPAND);

	grid_a->Add(new wxStaticText(tab1, wxID_ANY, "Destination .sec Folder:"), 0, wxALIGN_CENTER_VERTICAL);
	sec_dst_picker = new wxDirPickerCtrl(tab1, wxID_ANY, wxEmptyString, "Select Destination Folder for .sec Files", wxDefaultPosition, wxDefaultSize, wxDIRP_DIR_MUST_EXIST);
	grid_a->Add(sec_dst_picker, 1, wxEXPAND);

	grid_a->Add(new wxStaticText(tab1, wxID_ANY, "Optional objects.srv:"), 0, wxALIGN_CENTER_VERTICAL);
	obj_srv_picker = new wxFilePickerCtrl(tab1, ID_CONV_OBJ_SRV, wxEmptyString, "Select objects.srv", "SRV files (*.srv;*.txt)|*.srv;*.txt", wxDefaultPosition, wxDefaultSize, wxFLP_OPEN | wxFLP_FILE_MUST_EXIST);
	grid_a->Add(obj_srv_picker, 1, wxEXPAND);

	sizer_a->Add(grid_a, 0, wxEXPAND | wxALL, 6);
	btn_convert_otbm_to_sec = new wxButton(tab1, ID_CONV_OTBM_TO_SEC, "Convert OTBM -> SEC Sectors", wxDefaultPosition, wxSize(-1, 32));
	btn_convert_otbm_to_sec->SetBackgroundColour(wxColour(40, 120, 200));
	btn_convert_otbm_to_sec->SetForegroundColour(*wxWHITE);
	sizer_a->Add(btn_convert_otbm_to_sec, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

	t1_sizer->Add(sizer_a, 0, wxEXPAND | wxALL, 6);

	// Section B: SEC -> OTBM
	wxStaticBoxSizer* sizer_b = new wxStaticBoxSizer(wxVERTICAL, tab1, "Convert CipSoft Sectors (.sec) -> OpenTibia Map (.otbm)");
	wxFlexGridSizer* grid_b = new wxFlexGridSizer(2, 2, 8, 10);
	grid_b->AddGrowableCol(1, 1);

	grid_b->Add(new wxStaticText(tab1, wxID_ANY, "Source .sec Folder:"), 0, wxALIGN_CENTER_VERTICAL);
	sec_src_picker = new wxDirPickerCtrl(tab1, wxID_ANY, wxEmptyString, "Select Folder Containing .sec Files", wxDefaultPosition, wxDefaultSize, wxDIRP_DIR_MUST_EXIST);
	grid_b->Add(sec_src_picker, 1, wxEXPAND);

	grid_b->Add(new wxStaticText(tab1, wxID_ANY, "Destination .otbm:"), 0, wxALIGN_CENTER_VERTICAL);
	otbm_dst_picker = new wxFilePickerCtrl(tab1, wxID_ANY, wxEmptyString, "Save As OTBM Map", "OTBM files (*.otbm)|*.otbm", wxDefaultPosition, wxDefaultSize, wxFLP_SAVE | wxFLP_OVERWRITE_PROMPT);
	grid_b->Add(otbm_dst_picker, 1, wxEXPAND);

	sizer_b->Add(grid_b, 0, wxEXPAND | wxALL, 6);
	btn_convert_sec_to_otbm = new wxButton(tab1, ID_CONV_SEC_TO_OTBM, "Convert SEC Sectors -> OTBM", wxDefaultPosition, wxSize(-1, 32));
	btn_convert_sec_to_otbm->SetBackgroundColour(wxColour(40, 160, 90));
	btn_convert_sec_to_otbm->SetForegroundColour(*wxWHITE);
	sizer_b->Add(btn_convert_sec_to_otbm, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

	t1_sizer->Add(sizer_b, 0, wxEXPAND | wxALL, 6);

	map_progress_gauge = new wxGauge(tab1, wxID_ANY, 100);
	t1_sizer->Add(map_progress_gauge, 0, wxEXPAND | wxALL, 6);

	map_log_ctrl = new wxTextCtrl(tab1, wxID_ANY, "Ready to convert maps and server files.\n", wxDefaultPosition, wxSize(-1, 80), wxTE_MULTILINE | wxTE_READONLY);
	t1_sizer->Add(map_log_ctrl, 1, wxEXPAND | wxALL, 6);

	tab1->SetSizer(t1_sizer);
	notebook->AddPage(tab1, "Map Converter (.otbm <-> .sec)");

	// ==========================================
	// TAB 2: Server <-> Client ID Translator
	// ==========================================
	wxPanel* tab2 = new wxPanel(notebook, wxID_ANY);
	wxBoxSizer* t2_sizer = new wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer* id_box = new wxStaticBoxSizer(wxVERTICAL, tab2, "Live Item ID Translation & Property Inspector");
	wxBoxSizer* top_id_box = new wxBoxSizer(wxHORIZONTAL);
	wxFlexGridSizer* id_grid = new wxFlexGridSizer(3, 2, 8, 12);
	id_grid->AddGrowableCol(1, 1);

	id_grid->Add(new wxStaticText(tab2, wxID_ANY, "CipSoft / RealOTS Server ID:"), 0, wxALIGN_CENTER_VERTICAL);
	id_server_spin = new wxSpinCtrl(tab2, ID_CONV_SERVER_ID, "100", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 65535, 100);
	id_grid->Add(id_server_spin, 1, wxEXPAND);

	id_grid->Add(new wxStaticText(tab2, wxID_ANY, "Client Sprite / DAT ID:"), 0, wxALIGN_CENTER_VERTICAL);
	id_client_spin = new wxSpinCtrl(tab2, ID_CONV_CLIENT_ID, "100", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 65535, 100);
	id_grid->Add(id_client_spin, 1, wxEXPAND);

	id_grid->Add(new wxStaticText(tab2, wxID_ANY, "OpenTibia OTB Item ID:"), 0, wxALIGN_CENTER_VERTICAL);
	id_otb_spin = new wxSpinCtrl(tab2, ID_CONV_OTB_ID, "100", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 65535, 100);
	id_grid->Add(id_otb_spin, 1, wxEXPAND);

	top_id_box->Add(id_grid, 1, wxEXPAND | wxRIGHT, 12);
	item_preview_panel = new ItemPreviewPanel(tab2, wxID_ANY);
	top_id_box->Add(item_preview_panel, 0, wxALIGN_CENTER_VERTICAL);

	id_box->Add(top_id_box, 0, wxEXPAND | wxALL, 8);

	wxStaticBoxSizer* info_box = new wxStaticBoxSizer(wxVERTICAL, tab2, "Item Properties & Metadata");
	id_item_name_text = new wxStaticText(tab2, wxID_ANY, "Item Name: (default item)");
	id_flags_text = new wxStaticText(tab2, wxID_ANY, "Flags: None");
	id_weight_text = new wxStaticText(tab2, wxID_ANY, "Weight / Stats: -");
	id_attributes_text = new wxTextCtrl(tab2, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 120), wxTE_MULTILINE | wxTE_READONLY);

	info_box->Add(id_item_name_text, 0, wxALL, 4);
	info_box->Add(id_flags_text, 0, wxALL, 4);
	info_box->Add(id_weight_text, 0, wxALL, 4);
	info_box->Add(new wxStaticText(tab2, wxID_ANY, "Parsed Attributes from objects.srv:"), 0, wxALL, 4);
	info_box->Add(id_attributes_text, 1, wxEXPAND | wxALL, 4);

	t2_sizer->Add(id_box, 0, wxEXPAND | wxALL, 8);
	t2_sizer->Add(info_box, 1, wxEXPAND | wxALL, 8);

	tab2->SetSizer(t2_sizer);
	notebook->AddPage(tab2, "ID Translator (Server <-> Client)");

	// ==========================================
	// TAB 3: Spawn & DB Converter
	// ==========================================
	wxPanel* tab3 = new wxPanel(notebook, wxID_ANY);
	wxBoxSizer* t3_sizer = new wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer* sp_box = new wxStaticBoxSizer(wxVERTICAL, tab3, "Monster Spawns Converter (monster.db <-> XML)");
	wxFlexGridSizer* sp_grid = new wxFlexGridSizer(2, 2, 8, 12);
	sp_grid->AddGrowableCol(1, 1);

	sp_grid->Add(new wxStaticText(tab3, wxID_ANY, "Source monster.db File:"), 0, wxALIGN_CENTER_VERTICAL);
	monster_db_picker = new wxFilePickerCtrl(tab3, wxID_ANY, wxEmptyString, "Select monster.db", "DB / Text files (*.db;*.txt)|*.db;*.txt", wxDefaultPosition, wxDefaultSize, wxFLP_OPEN | wxFLP_FILE_MUST_EXIST);
	sp_grid->Add(monster_db_picker, 1, wxEXPAND);

	sp_grid->Add(new wxStaticText(tab3, wxID_ANY, "Target spawns.xml:"), 0, wxALIGN_CENTER_VERTICAL);
	spawns_xml_dst_picker = new wxFilePickerCtrl(tab3, wxID_ANY, wxEmptyString, "Save as spawns.xml", "XML files (*.xml)|*.xml", wxDefaultPosition, wxDefaultSize, wxFLP_SAVE | wxFLP_OVERWRITE_PROMPT);
	sp_grid->Add(spawns_xml_dst_picker, 1, wxEXPAND);

	sp_box->Add(sp_grid, 0, wxEXPAND | wxALL, 8);
	btn_convert_spawns = new wxButton(tab3, ID_CONV_SPAWNS, "Convert monster.db -> spawns.xml", wxDefaultPosition, wxSize(-1, 32));
	sp_box->Add(btn_convert_spawns, 0, wxEXPAND | wxALL, 8);

	t3_sizer->Add(sp_box, 0, wxEXPAND | wxALL, 8);
	tab3->SetSizer(t3_sizer);
	notebook->AddPage(tab3, "Spawns (monster.db <-> XML)");

	main_sizer->Add(notebook, 1, wxEXPAND | wxALL, 8);

	wxBoxSizer* bottom_sizer = new wxBoxSizer(wxHORIZONTAL);
	bottom_sizer->AddStretchSpacer();
	wxButton* btn_close = new wxButton(this, wxID_CANCEL, "Close");
	bottom_sizer->Add(btn_close, 0, wxRIGHT | wxBOTTOM, 8);
	main_sizer->Add(bottom_sizer, 0, wxEXPAND);

	SetSizer(main_sizer);
	Centre();

	UpdateIdInspector(100, 100, 100, 0);
}

RealOTSConverterDialog::~RealOTSConverterDialog() {
}

void RealOTSConverterDialog::AppendLog(const wxString& text) {
	if (map_log_ctrl) {
		map_log_ctrl->AppendText(text + "\n");
	}
}

void RealOTSConverterDialog::OnLoadObjectsSrv(wxFileDirPickerEvent& event) {
	wxString path = event.GetPath();
	if (sec_io.loadObjectsSrv(nstr(path))) {
		AppendLog(wxString::Format("Successfully loaded objects.srv: %zu item definitions.", sec_io.GetObjects().size()));
		UpdateIdInspector(id_server_spin->GetValue(), 0, 0, 0);
	} else {
		AppendLog("Failed to parse objects.srv file.");
	}
}

void RealOTSConverterDialog::UpdateIdInspector(uint16_t server_id, uint16_t client_id, uint16_t otb_id, int source_field) {
	if (source_field == 0) { // server id changed
		client_id = sec_io.ServerToClientId(server_id);
		ItemType& it = g_items.getItemIdByClientID(client_id);
		otb_id = it.id > 0 ? it.id : client_id;
		id_client_spin->SetValue(client_id);
		id_otb_spin->SetValue(otb_id);
	} else if (source_field == 1) { // client id changed
		server_id = sec_io.ClientToServerId(client_id);
		ItemType& it = g_items.getItemIdByClientID(client_id);
		otb_id = it.id > 0 ? it.id : client_id;
		id_server_spin->SetValue(server_id);
		id_otb_spin->SetValue(otb_id);
	} else if (source_field == 2) { // otb id changed
		ItemType& it = g_items.getItemType(otb_id);
		client_id = it.clientID > 0 ? it.clientID : otb_id;
		server_id = sec_io.ClientToServerId(client_id);
		id_server_spin->SetValue(server_id);
		id_client_spin->SetValue(client_id);
	}

	ItemType& it = g_items.getItemType(otb_id);
	const CipObjectDef* def = sec_io.GetObjectDef(server_id);

	wxString name = def && !def->name.empty() ? wxstr(def->name) : (it.name.empty() ? "Item ID #" + std::to_string(server_id) : wxstr(it.name));
	id_item_name_text->SetLabel("Item Name: " + name);

	wxString flags = "Flags: ";
	if (def) {
		if (def->is_ground) flags += "[Ground/Bank] ";
		if (def->is_blocking) flags += "[Blocking/Clip] ";
		if (def->is_container) flags += "[Container] ";
		if (def->is_stackable) flags += "[Cumulative/Stackable] ";
		if (def->is_pickupable) flags += "[Pickupable/Take] ";
		if (def->is_unpassable) flags += "[Unpassable] ";
	} else {
		if (it.isGroundTile()) flags += "[Ground] ";
		if (it.unpassable) flags += "[Unpassable] ";
		if (it.stackable) flags += "[Stackable] ";
		if (it.pickupable) flags += "[Pickupable] ";
	}
	id_flags_text->SetLabel(flags);

	if (def) {
		id_weight_text->SetLabel(wxString::Format("Weight: %u oz | ClientID: %u | ServerID: %u", def->weight, def->client_id, def->server_id));
		wxString attrs;
		for (const auto& kv : def->attributes) {
			attrs += wxstr(kv.first) + " = " + wxstr(kv.second) + "\n";
		}
		id_attributes_text->SetValue(attrs);
	} else {
		id_weight_text->SetLabel(wxString::Format("Weight: %.2f oz | ClientID: %u | OTB ID: %u", it.weight, it.clientID, it.id));
		id_attributes_text->SetValue("No custom CipSoft attributes (Standard OpenTibia item)");
	}

	if (item_preview_panel) {
		item_preview_panel->SetItem(client_id);
	}
}

void RealOTSConverterDialog::OnServerIdChanged(wxSpinEvent& event) {
	UpdateIdInspector(event.GetValue(), 0, 0, 0);
}

void RealOTSConverterDialog::OnClientIdChanged(wxSpinEvent& event) {
	UpdateIdInspector(0, event.GetValue(), 0, 1);
}

void RealOTSConverterDialog::OnOtbIdChanged(wxSpinEvent& event) {
	UpdateIdInspector(0, 0, event.GetValue(), 2);
}

void RealOTSConverterDialog::OnConvertOTBMToSEC(wxCommandEvent& WXUNUSED(event)) {
	wxString src_otbm = otbm_src_picker->GetPath();
	wxString dst_sec = sec_dst_picker->GetPath();

	if (src_otbm.empty() || !wxFileExists(src_otbm)) {
		wxMessageBox("Please select a valid source .otbm file.", "Missing File", wxICON_WARNING);
		return;
	}
	if (dst_sec.empty() || !wxDirExists(dst_sec)) {
		wxMessageBox("Please select a destination folder for the .sec files.", "Missing Folder", wxICON_WARNING);
		return;
	}

	map_progress_gauge->SetValue(20);
	AppendLog("Loading OTBM map: " + src_otbm + "...");

	Map temp_map;
	MapVersion ver;
	if (!IOMapOTBM::getVersionInfo(FileName(src_otbm), ver)) {
		ver = MapVersion(MAP_OTBM_2, CLIENT_VERSION_860);
	}
	IOMapOTBM otbm_io(ver);
	if (!otbm_io.loadMap(temp_map, FileName(src_otbm))) {
		AppendLog("Error: Failed to load OTBM map!");
		map_progress_gauge->SetValue(0);
		return;
	}

	map_progress_gauge->SetValue(60);
	AppendLog(wxString::Format("OTBM loaded successfully (%u tiles). Exporting sectors to: %s...", temp_map.size(), dst_sec));

	IOMapSEC sec_exporter;
	wxString obj_srv = obj_srv_picker->GetPath();
	if (!obj_srv.empty() && wxFileExists(obj_srv)) {
		sec_exporter.loadObjectsSrv(nstr(obj_srv));
	}

	if (!sec_exporter.saveMap(temp_map, FileName(dst_sec))) {
		AppendLog("Error: Failed to save .sec sector files!");
		map_progress_gauge->SetValue(0);
		return;
	}

	map_progress_gauge->SetValue(100);
	AppendLog("Conversion COMPLETE! All .sec files and monster.db were created successfully.");
	wxMessageBox("OTBM Map converted to CipSoft .sec sector files successfully!", "Success", wxICON_INFORMATION);
}

void RealOTSConverterDialog::OnConvertSECToOTBM(wxCommandEvent& WXUNUSED(event)) {
	wxString src_sec = sec_src_picker->GetPath();
	wxString dst_otbm = otbm_dst_picker->GetPath();

	if (src_sec.empty() || !wxDirExists(src_sec)) {
		wxMessageBox("Please select a valid folder containing .sec files.", "Missing Folder", wxICON_WARNING);
		return;
	}
	if (dst_otbm.empty()) {
		wxMessageBox("Please select a destination path for the .otbm map.", "Missing File", wxICON_WARNING);
		return;
	}

	map_progress_gauge->SetValue(20);
	AppendLog("Loading CipSoft sectors from: " + src_sec + "...");

	Map temp_map;
	IOMapSEC sec_loader;
	if (!sec_loader.loadMap(temp_map, FileName(src_sec))) {
		AppendLog("Error: Failed to load .sec sectors!");
		map_progress_gauge->SetValue(0);
		return;
	}

	map_progress_gauge->SetValue(60);
	AppendLog(wxString::Format("Sectors loaded successfully (%u tiles). Saving OTBM to: %s...", temp_map.size(), dst_otbm));

	IOMapOTBM otbm_exporter(temp_map.getVersion());
	if (!otbm_exporter.saveMap(temp_map, FileName(dst_otbm))) {
		AppendLog("Error: Failed to save OTBM map!");
		map_progress_gauge->SetValue(0);
		return;
	}

	map_progress_gauge->SetValue(100);
	AppendLog("Conversion COMPLETE! OTBM map saved successfully.");
	wxMessageBox("CipSoft .sec sectors converted to OTBM Map successfully!", "Success", wxICON_INFORMATION);
}

void RealOTSConverterDialog::OnConvertSpawns(wxCommandEvent& WXUNUSED(event)) {
	wxString src_db = monster_db_picker->GetPath();
	wxString dst_xml = spawns_xml_dst_picker->GetPath();

	if (src_db.empty() || !wxFileExists(src_db)) {
		wxMessageBox("Please select a valid source monster.db file.", "Missing File", wxICON_WARNING);
		return;
	}
	if (dst_xml.empty()) {
		wxMessageBox("Please select a target spawns.xml destination.", "Missing File", wxICON_WARNING);
		return;
	}

	std::ifstream file(nstr(src_db));
	if (!file.is_open()) {
		wxMessageBox("Could not open monster.db file.", "Error", wxICON_ERROR);
		return;
	}

	std::ofstream out_file(nstr(dst_xml));
	if (!out_file.is_open()) {
		wxMessageBox("Could not create spawns.xml file.", "Error", wxICON_ERROR);
		return;
	}

	out_file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	out_file << "<spawns>\n";

	std::string line;
	int count = 0;
	while (std::getline(file, line)) {
		line.erase(0, line.find_first_not_of(" \t\r\n"));
		line.erase(line.find_last_not_of(" \t\r\n") + 1);
		if (line.empty() || line[0] == '#') continue;

		std::istringstream ss(line);
		std::string name;
		char first;
		ss >> std::ws;
		if (ss.peek() == '"') {
			ss >> first;
			std::getline(ss, name, '"');
		} else {
			ss >> name;
		}

		int x = 0, y = 0, z = 0, mcount = 1, respawn = 60, radius = 1;
		if (ss >> x >> y >> z) {
			if (!(ss >> mcount)) mcount = 1;
			if (!(ss >> respawn)) respawn = 60;
			if (!(ss >> radius)) radius = 1;

			out_file << "\t<spawn centerx=\"" << x << "\" centery=\"" << y << "\" centerz=\"" << z << "\" radius=\"" << radius << "\">\n";
			out_file << "\t\t<monster name=\"" << name << "\" x=\"0\" y=\"0\" z=\"0\" spawntime=\"" << respawn << "\"/>\n";
			out_file << "\t</spawn>\n";
			count++;
		}
	}

	out_file << "</spawns>\n";
	wxMessageBox(wxString::Format("Converted %d monster spawns from monster.db to spawns.xml successfully!", count), "Success", wxICON_INFORMATION);
}

void RealOTSConverterDialog::OnClose(wxCommandEvent& WXUNUSED(event)) {
	EndModal(wxID_CANCEL);
}
