#include "main.h"
#include "command_palette_dialog.h"
#include "gui_ids.h"
#include "style_manager.h"
#include <algorithm>
#include <cctype>

enum {
	CMD_PALETTE_SEARCH = wxID_HIGHEST + 450,
	CMD_PALETTE_LIST
};

BEGIN_EVENT_TABLE(CommandPaletteDialog, wxDialog)
EVT_TEXT(CMD_PALETTE_SEARCH, CommandPaletteDialog::OnSearchText)
EVT_LISTBOX_DCLICK(CMD_PALETTE_LIST, CommandPaletteDialog::OnListDClick)
EVT_BUTTON(wxID_OK, CommandPaletteDialog::OnClickOK)
END_EVENT_TABLE()

CommandPaletteDialog::CommandPaletteDialog(wxWindow* parent) :
	wxDialog(parent, wxID_ANY, "Command Palette (Schnellsuche)", wxDefaultPosition, wxSize(500, 360), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	selected_action_id(-1) {

	SetBackgroundColour(wxColour(16, 28, 48));
	SetForegroundColour(wxColour(240, 245, 255));

	wxBoxSizer* main_sizer = newd wxBoxSizer(wxVERTICAL);

	search_field = newd wxTextCtrl(this, CMD_PALETTE_SEARCH, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	search_field->SetBackgroundColour(wxColour(10, 20, 35));
	search_field->SetForegroundColour(wxColour(240, 245, 255));
	search_field->SetHint("Tippe einen Befehl oder Werkzeugnamen...");
	main_sizer->Add(search_field, 0, wxEXPAND | wxALL, 8);

	results_list = newd wxListBox(this, CMD_PALETTE_LIST, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE);
	results_list->SetBackgroundColour(wxColour(10, 20, 35));
	results_list->SetForegroundColour(wxColour(240, 245, 255));
	main_sizer->Add(results_list, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

	SetSizer(main_sizer);
	PopulateCommands();
	FilterCommands();

	RME::UI::StyleManager::ApplyThemeRecursively(this, RME::UI::StyleManager::GetTheme());

	search_field->SetFocus();
}

CommandPaletteDialog::~CommandPaletteDialog() {
}

void CommandPaletteDialog::PopulateCommands() {
	all_commands = {
		{ "Map Diagnostic & Health Scanner", "Analyse", TOOLS_MAP_DIAGNOSTIC },
		{ "Procedural Terrain Generator", "Tools", TOOLS_PROCEDURAL_GENERATOR },
		{ "House Wizard (Haus erstellen/bearbeiten)", "Häuser", PALETTE_HOUSE_ADD_HOUSE },
		{ "Map Diff Tool", "Analyse", TOOLS_MAP_DIFF },
		{ "Prefab Library", "Tools", TOOLS_PREFAB_LIBRARY },
		{ "Optional Border Tool", "Modus", PALETTE_TERRAIN_OPTIONAL_BORDER_TOOL },
		{ "Eraser Tool (Radiergummi)", "Werkzeug", PALETTE_TERRAIN_ERASER },
		{ "Protection Zone Tool (PZ)", "Zonen", PALETTE_TERRAIN_PZ_TOOL },
		{ "No PVP Zone Tool", "Zonen", PALETTE_TERRAIN_NOPVP_TOOL },
		{ "No Logout Zone Tool", "Zonen", PALETTE_TERRAIN_NOLOGOUT_TOOL }
	};
}

void CommandPaletteDialog::FilterCommands() {
	results_list->Clear();
	filtered_commands.clear();

	std::string query = search_field->GetValue().ToStdString();
	std::transform(query.begin(), query.end(), query.begin(), ::tolower);

	for (const auto& cmd : all_commands) {
		std::string name_lower = cmd.name;
		std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);

		if (query.empty() || name_lower.find(query) != std::string::npos) {
			filtered_commands.push_back(cmd);
			results_list->Append(wxstr("[" + cmd.category + "] " + cmd.name));
		}
	}

	if (results_list->GetCount() > 0) {
		results_list->SetSelection(0);
	}
}

void CommandPaletteDialog::OnSearchText(wxCommandEvent& evt) {
	FilterCommands();
}

void CommandPaletteDialog::OnListDClick(wxCommandEvent& evt) {
	OnClickOK(evt);
}

void CommandPaletteDialog::OnClickOK(wxCommandEvent& evt) {
	int sel = results_list->GetSelection();
	if (sel >= 0 && static_cast<size_t>(sel) < filtered_commands.size()) {
		selected_action_id = filtered_commands[sel].action_id;
		EndModal(wxID_OK);
	} else {
		EndModal(wxID_CANCEL);
	}
}
