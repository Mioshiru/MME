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

#ifndef RME_REALOTS_CONVERTER_DIALOG_H_
#define RME_REALOTS_CONVERTER_DIALOG_H_

#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/filepicker.h>
#include <wx/spinctrl.h>
#include <wx/gauge.h>
#include "iomap_sec.h"
#include "editor.h"

class RealOTSConverterDialog : public wxDialog {
public:
	RealOTSConverterDialog(wxWindow* parent, Editor* editor = nullptr);
	virtual ~RealOTSConverterDialog();

private:
	Editor* editor;
	IOMapSEC sec_io;

	wxNotebook* notebook;

	// Tab 1: Map Format Converter (OTBM <-> SEC)
	wxFilePickerCtrl* otbm_src_picker;
	wxDirPickerCtrl* sec_dst_picker;
	wxFilePickerCtrl* obj_srv_picker;
	wxButton* btn_convert_otbm_to_sec;

	wxDirPickerCtrl* sec_src_picker;
	wxFilePickerCtrl* otbm_dst_picker;
	wxButton* btn_convert_sec_to_otbm;

	wxGauge* map_progress_gauge;
	wxTextCtrl* map_log_ctrl;

	// Tab 2: Server <-> Client ID Translator & Inspector
	wxSpinCtrl* id_server_spin;
	wxSpinCtrl* id_client_spin;
	wxSpinCtrl* id_otb_spin;
	wxStaticText* id_item_name_text;
	wxStaticText* id_flags_text;
	wxStaticText* id_weight_text;
	wxTextCtrl* id_attributes_text;
	class ItemPreviewPanel* item_preview_panel;

	// Tab 3: Spawns Converter (monster.db <-> spawns.xml)
	wxFilePickerCtrl* monster_db_picker;
	wxFilePickerCtrl* spawns_xml_dst_picker;
	wxButton* btn_convert_spawns;

	void OnConvertOTBMToSEC(wxCommandEvent& event);
	void OnConvertSECToOTBM(wxCommandEvent& event);
	void OnServerIdChanged(wxSpinEvent& event);
	void OnClientIdChanged(wxSpinEvent& event);
	void OnOtbIdChanged(wxSpinEvent& event);
	void OnLoadObjectsSrv(wxFileDirPickerEvent& event);
	void OnConvertSpawns(wxCommandEvent& event);
	void OnClose(wxCommandEvent& event);

	void UpdateIdInspector(uint16_t server_id, uint16_t client_id, uint16_t otb_id, int source_field);
	void AppendLog(const wxString& text);

	DECLARE_EVENT_TABLE()
};

#endif // RME_REALOTS_CONVERTER_DIALOG_H_
