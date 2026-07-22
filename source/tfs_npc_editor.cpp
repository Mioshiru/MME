#include "tfs_npc_editor.h"
#include "creature.h"
#include "action.h"

#include <wx/msgdlg.h>
#include <sstream>

BEGIN_EVENT_TABLE(TFSNPCDialog, wxDialog)
EVT_BUTTON(wxID_OK, TFSNPCDialog::OnClickGenerate)
EVT_BUTTON(wxID_CANCEL, TFSNPCDialog::OnClickCancel)
END_EVENT_TABLE()

TFSNPCDialog::TFSNPCDialog(wxWindow* parent, Editor& editor, Creature* target_creature) :
	wxDialog(parent, wxID_ANY, "TFS 1.6 NPC & Dialogue Creator", wxDefaultPosition, wxSize(560, 620), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	editor(editor),
	creature(target_creature) {

	SetBackgroundColour(wxColour(15, 23, 42));
	wxBoxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticText* header = newd wxStaticText(this, wxID_ANY, "Create TFS 1.6 NPC Definition & Script");
	header->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	header->SetForegroundColour(wxColour(241, 245, 249));
	topsizer->Add(header, 0, wxALL, 12);

	wxFlexGridSizer* grid = newd wxFlexGridSizer(2, 6, 12);
	grid->AddGrowableCol(1);

	auto addLabel = [this, grid](const wxString& labelText) {
		wxStaticText* label = newd wxStaticText(this, wxID_ANY, labelText);
		label->SetForegroundColour(wxColour(203, 213, 225));
		grid->Add(label, 0, wxALIGN_CENTER_VERTICAL);
	};

	addLabel("NPC Name:");
	npcNameCtrl = newd wxTextCtrl(this, wxID_ANY, "Merchant");
	grid->Add(npcNameCtrl, 1, wxEXPAND);

	addLabel("LookType (Outfit ID):");
	lookTypeSpin = newd wxSpinCtrl(this, wxID_ANY, "128", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 2000, 128);
	grid->Add(lookTypeSpin, 1, wxEXPAND);

	addLabel("Health:");
	healthSpin = newd wxSpinCtrl(this, wxID_ANY, "100", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100000, 100);
	grid->Add(healthSpin, 1, wxEXPAND);

	addLabel("Max Health:");
	maxHealthSpin = newd wxSpinCtrl(this, wxID_ANY, "100", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100000, 100);
	grid->Add(maxHealthSpin, 1, wxEXPAND);

	addLabel("Walk Interval (ms):");
	walkIntervalSpin = newd wxSpinCtrl(this, wxID_ANY, "2000", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 500, 10000, 2000);
	grid->Add(walkIntervalSpin, 1, wxEXPAND);

	topsizer->Add(grid, 0, wxEXPAND | wxALL, 12);

	// Dialogue Triggers section
	wxStaticText* dlgHeader = newd wxStaticText(this, wxID_ANY, "Dialogue Triggers (English)");
	dlgHeader->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	dlgHeader->SetForegroundColour(wxColour(148, 163, 184));
	topsizer->Add(dlgHeader, 0, wxLEFT | wxRIGHT | wxTOP, 12);

	wxFlexGridSizer* dlgGrid = newd wxFlexGridSizer(2, 6, 12);
	dlgGrid->AddGrowableCol(1);

	auto addDlgLabel = [this, dlgGrid](const wxString& labelText) {
		wxStaticText* label = newd wxStaticText(this, wxID_ANY, labelText);
		label->SetForegroundColour(wxColour(203, 213, 225));
		dlgGrid->Add(label, 0, wxALIGN_CENTER_VERTICAL);
	};

	addDlgLabel("Greeting ('hi'):");
	hiGreetingCtrl = newd wxTextCtrl(this, wxID_ANY, "Hello traveler! How can I help you?");
	dlgGrid->Add(hiGreetingCtrl, 1, wxEXPAND);

	addDlgLabel("Job Response ('job'):");
	jobResponseCtrl = newd wxTextCtrl(this, wxID_ANY, "I am a local merchant selling fine goods.");
	dlgGrid->Add(jobResponseCtrl, 1, wxEXPAND);

	addDlgLabel("Quest Response ('quest'):");
	questResponseCtrl = newd wxTextCtrl(this, wxID_ANY, "I need someone to retrieve my stolen ring.");
	dlgGrid->Add(questResponseCtrl, 1, wxEXPAND);

	addDlgLabel("Farewell ('bye'):");
	byeResponseCtrl = newd wxTextCtrl(this, wxID_ANY, "Farewell and safe travels!");
	dlgGrid->Add(byeResponseCtrl, 1, wxEXPAND);

	topsizer->Add(dlgGrid, 0, wxEXPAND | wxALL, 12);

	// Shop grid
	wxStaticText* shopHeader = newd wxStaticText(this, wxID_ANY, "Shop Items (Item ID | Item Name | Buy Price | Sell Price)");
	shopHeader->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	shopHeader->SetForegroundColour(wxColour(148, 163, 184));
	topsizer->Add(shopHeader, 0, wxLEFT | wxRIGHT | wxTOP, 12);

	shopGrid = newd wxGrid(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 100));
	shopGrid->CreateGrid(3, 4);
	shopGrid->SetColLabelValue(0, "Item ID");
	shopGrid->SetColLabelValue(1, "Item Name");
	shopGrid->SetColLabelValue(2, "Buy Price");
	shopGrid->SetColLabelValue(3, "Sell Price");

	shopGrid->SetCellValue(0, 0, "2160"); shopGrid->SetCellValue(0, 1, "crystal coin"); shopGrid->SetCellValue(0, 2, "10000"); shopGrid->SetCellValue(0, 3, "10000");
	shopGrid->SetCellValue(1, 0, "2152"); shopGrid->SetCellValue(1, 1, "platinum coin"); shopGrid->SetCellValue(1, 2, "100"); shopGrid->SetCellValue(1, 3, "100");
	shopGrid->SetCellValue(2, 0, "2148"); shopGrid->SetCellValue(2, 1, "gold coin"); shopGrid->SetCellValue(2, 2, "1"); shopGrid->SetCellValue(2, 3, "1");

	topsizer->Add(shopGrid, 1, wxEXPAND | wxALL, 12);

	wxBoxSizer* btnsizer = newd wxBoxSizer(wxHORIZONTAL);
	wxButton* okBtn = newd wxButton(this, wxID_OK, "Generate NPC Files");
	wxButton* cancelBtn = newd wxButton(this, wxID_CANCEL, "Cancel");
	btnsizer->Add(okBtn, 0, wxRIGHT, 8);
	btnsizer->Add(cancelBtn, 0);

	topsizer->Add(btnsizer, 0, wxALIGN_RIGHT | wxALL, 12);
	SetSizerAndFit(topsizer);

	if (creature) {
		npcNameCtrl->SetValue(wxstr(creature->getName()));
		lookTypeSpin->SetValue(creature->getLookType().lookType);
	}
}

TFSNPCDialog::~TFSNPCDialog() {
}

void TFSNPCDialog::OnClickGenerate(wxCommandEvent& WXUNUSED(event)) {
	npc_name = nstr(npcNameCtrl->GetValue());
	if (npc_name.empty()) {
		wxMessageBox("Please enter a valid NPC name.", "Error", wxOK | wxICON_ERROR, this);
		return;
	}

	int look_type = lookTypeSpin->GetValue();
	int health = healthSpin->GetValue();
	int max_health = maxHealthSpin->GetValue();
	int walk_interval = walkIntervalSpin->GetValue();

	std::string hi_msg = nstr(hiGreetingCtrl->GetValue());
	std::string job_msg = nstr(jobResponseCtrl->GetValue());
	std::string quest_msg = nstr(questResponseCtrl->GetValue());
	std::string bye_msg = nstr(byeResponseCtrl->GetValue());

	std::string script_filename = npc_name;
	for (char& c : script_filename) {
		if (c == ' ') c = '_';
		else c = static_cast<char>(tolower(c));
	}

	// 1. Generate XML
	std::ostringstream xml;
	xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	xml << "<npc name=\"" << npc_name << "\" script=\"" << script_filename << ".lua\" walkinterval=\"" << walk_interval << "\" floorchange=\"0\">\n";
	xml << "\t<health now=\"" << health << "\" max=\"" << max_health << "\"/>\n";
	xml << "\t<look type=\"" << look_type << "\" head=\"0\" body=\"0\" legs=\"0\" feet=\"0\" addons=\"0\"/>\n";
	xml << "</npc>\n";

	generated_xml = xml.str();

	// 2. Generate LUA (TFS 1.6 NpcHandler)
	std::ostringstream lua;
	lua << "-- Generated by Mios Map Editor for TFS 1.6 NpcSystem\n";
	lua << "local keywordHandler = KeywordHandler:new()\n";
	lua << "local npcHandler = NpcHandler:new(keywordHandler)\n";
	lua << "NpcSystem.parseNpcParameter(npcHandler)\n\n";
	lua << "function onCreatureAppear(cid) npcHandler:onCreatureAppear(cid) end\n";
	lua << "function onCreatureDisappear(cid) npcHandler:onCreatureDisappear(cid) end\n";
	lua << "function onCreatureSay(cid, type, msg) npcHandler:onCreatureSay(cid, type, msg) end\n";
	lua << "function onThink() npcHandler:onThink() end\n\n";
	lua << "npcHandler:setMessage(MESSAGE_GREET, \"" << hi_msg << "\")\n";
	lua << "npcHandler:setMessage(MESSAGE_FAREWELL, \"" << bye_msg << "\")\n\n";
	lua << "keywordHandler:addKeyword({'job'}, StdModule.say, {npcHandler = npcHandler, text = \"" << job_msg << "\"})\n";
	lua << "keywordHandler:addKeyword({'quest'}, StdModule.say, {npcHandler = npcHandler, text = \"" << quest_msg << "\"})\n\n";
	lua << "npcHandler:addModule(FocusModule:new())\n";

	generated_lua = lua.str();

	EndModal(wxID_OK);
}

void TFSNPCDialog::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	EndModal(wxID_CANCEL);
}
