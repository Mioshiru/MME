#include "tfs_npc_editor.h"
#include "creature.h"
#include "action.h"

#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <sstream>

BEGIN_EVENT_TABLE(TFSNPCDialog, wxDialog)
EVT_BUTTON(wxID_OK, TFSNPCDialog::OnClickGenerate)
EVT_BUTTON(wxID_CANCEL, TFSNPCDialog::OnClickCancel)
END_EVENT_TABLE()

TFSNPCDialog::TFSNPCDialog(wxWindow* parent, Editor& editor, Creature* target_creature) :
	wxDialog(parent, wxID_ANY, "TFS 1.6 NPC & Dialogue Creator", wxDefaultPosition, wxSize(640, 680), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	editor(editor),
	creature(target_creature) {

	SetBackgroundColour(wxColour(10, 20, 35));
	wxBoxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	// Header Banner Panel
	wxPanel* headerPanel = newd wxPanel(this, wxID_ANY);
	headerPanel->SetBackgroundColour(wxColour(16, 28, 48));
	wxBoxSizer* headerSizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticText* header = newd wxStaticText(headerPanel, wxID_ANY, "TFS 1.6 NPC & Dialogue Creator");
	header->SetFont(wxFont(13, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	header->SetForegroundColour(wxColour(180, 150, 50));

	wxStaticText* subheader = newd wxStaticText(headerPanel, wxID_ANY, "Build TFS 1.6 XML NPC definitions, English dialogue keywords, and shop lists.");
	subheader->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	subheader->SetForegroundColour(wxColour(180, 190, 205));

	headerSizer->Add(header, 0, wxBOTTOM, 4);
	headerSizer->Add(subheader, 0);
	headerPanel->SetSizer(headerSizer);

	topsizer->Add(headerPanel, 0, wxEXPAND | wxALL, 12);

	// Two-Column Main Content Card
	wxBoxSizer* contentSizer = newd wxBoxSizer(wxHORIZONTAL);

	auto styleTextCtrl = [](wxTextCtrl* ctrl) {
		ctrl->SetBackgroundColour(wxColour(16, 28, 48));
		ctrl->SetForegroundColour(wxColour(240, 245, 255));
	};

	auto styleSpinCtrl = [](wxSpinCtrl* ctrl) {
		ctrl->SetBackgroundColour(wxColour(16, 28, 48));
		ctrl->SetForegroundColour(wxColour(240, 245, 255));
	};

	// Column 1: NPC Attributes Panel
	wxPanel* attrPanel = newd wxPanel(this, wxID_ANY);
	attrPanel->SetBackgroundColour(wxColour(16, 28, 48));
	wxBoxSizer* attrSizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticText* attrHeader = newd wxStaticText(attrPanel, wxID_ANY, "NPC Attributes");
	attrHeader->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	attrHeader->SetForegroundColour(wxColour(129, 140, 248)); // Indigo accent
	attrSizer->Add(attrHeader, 0, wxALL, 8);

	wxFlexGridSizer* attrGrid = newd wxFlexGridSizer(2, 6, 10);
	attrGrid->AddGrowableCol(1);

	auto addAttrLabel = [attrPanel, attrGrid](const wxString& labelText) {
		wxStaticText* label = newd wxStaticText(attrPanel, wxID_ANY, labelText);
		label->SetForegroundColour(wxColour(203, 213, 225));
		label->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
		attrGrid->Add(label, 0, wxALIGN_CENTER_VERTICAL);
	};

	addAttrLabel("NPC Name:");
	npcNameCtrl = newd wxTextCtrl(attrPanel, wxID_ANY, "Merchant");
	styleTextCtrl(npcNameCtrl);
	attrGrid->Add(npcNameCtrl, 1, wxEXPAND);

	addAttrLabel("LookType ID:");
	lookTypeSpin = newd wxSpinCtrl(attrPanel, wxID_ANY, "128", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 2000, 128);
	styleSpinCtrl(lookTypeSpin);
	attrGrid->Add(lookTypeSpin, 1, wxEXPAND);

	addAttrLabel("Health:");
	healthSpin = newd wxSpinCtrl(attrPanel, wxID_ANY, "100", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100000, 100);
	styleSpinCtrl(healthSpin);
	attrGrid->Add(healthSpin, 1, wxEXPAND);

	addAttrLabel("Max Health:");
	maxHealthSpin = newd wxSpinCtrl(attrPanel, wxID_ANY, "100", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100000, 100);
	styleSpinCtrl(maxHealthSpin);
	attrGrid->Add(maxHealthSpin, 1, wxEXPAND);

	addAttrLabel("Walk Speed:");
	walkIntervalSpin = newd wxSpinCtrl(attrPanel, wxID_ANY, "2000", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 500, 10000, 2000);
	styleSpinCtrl(walkIntervalSpin);
	attrGrid->Add(walkIntervalSpin, 1, wxEXPAND);

	attrSizer->Add(attrGrid, 1, wxEXPAND | wxALL, 8);
	attrPanel->SetSizer(attrSizer);

	contentSizer->Add(attrPanel, 1, wxEXPAND | wxRIGHT, 6);

	// Column 2: Dialogue Triggers Panel
	wxPanel* dlgPanel = newd wxPanel(this, wxID_ANY);
	dlgPanel->SetBackgroundColour(wxColour(16, 28, 48));
	wxBoxSizer* dlgSizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticText* dlgHeader = newd wxStaticText(dlgPanel, wxID_ANY, "Dialogue Triggers (English)");
	dlgHeader->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	dlgHeader->SetForegroundColour(wxColour(180, 150, 50));
	dlgSizer->Add(dlgHeader, 0, wxALL, 8);

	wxFlexGridSizer* dlgGrid = newd wxFlexGridSizer(2, 6, 10);
	dlgGrid->AddGrowableCol(1);

	auto addDlgLabel = [dlgPanel, dlgGrid](const wxString& labelText) {
		wxStaticText* label = newd wxStaticText(dlgPanel, wxID_ANY, labelText);
		label->SetForegroundColour(wxColour(180, 190, 205));
		label->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
		dlgGrid->Add(label, 0, wxALIGN_CENTER_VERTICAL);
	};

	addDlgLabel("Greeting:");
	hiGreetingCtrl = newd wxTextCtrl(dlgPanel, wxID_ANY, "Hello traveler! How can I help you?");
	styleTextCtrl(hiGreetingCtrl);
	dlgGrid->Add(hiGreetingCtrl, 1, wxEXPAND);

	addDlgLabel("Job:");
	jobResponseCtrl = newd wxTextCtrl(dlgPanel, wxID_ANY, "I am a local merchant selling fine goods.");
	styleTextCtrl(jobResponseCtrl);
	dlgGrid->Add(jobResponseCtrl, 1, wxEXPAND);

	addDlgLabel("Quest:");
	questResponseCtrl = newd wxTextCtrl(dlgPanel, wxID_ANY, "I need someone to retrieve my stolen ring.");
	styleTextCtrl(questResponseCtrl);
	dlgGrid->Add(questResponseCtrl, 1, wxEXPAND);

	addDlgLabel("Farewell:");
	byeResponseCtrl = newd wxTextCtrl(dlgPanel, wxID_ANY, "Farewell and safe travels!");
	styleTextCtrl(byeResponseCtrl);
	dlgGrid->Add(byeResponseCtrl, 1, wxEXPAND);

	dlgSizer->Add(dlgGrid, 1, wxEXPAND | wxALL, 8);
	dlgPanel->SetSizer(dlgSizer);

	contentSizer->Add(dlgPanel, 1, wxEXPAND | wxLEFT, 6);

	topsizer->Add(contentSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

	// Shop Table Card
	wxPanel* shopPanel = newd wxPanel(this, wxID_ANY);
	shopPanel->SetBackgroundColour(wxColour(16, 28, 48));
	wxBoxSizer* shopSizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticText* shopHeader = newd wxStaticText(shopPanel, wxID_ANY, "Shop Items (Item ID | Item Name | Buy Price | Sell Price)");
	shopHeader->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	shopHeader->SetForegroundColour(wxColour(180, 150, 50));
	shopSizer->Add(shopHeader, 0, wxALL, 8);

	shopGrid = newd wxGrid(shopPanel, wxID_ANY, wxDefaultPosition, wxSize(-1, 120));
	shopGrid->CreateGrid(3, 4);
	shopGrid->SetColLabelValue(0, "Item ID");
	shopGrid->SetColLabelValue(1, "Item Name");
	shopGrid->SetColLabelValue(2, "Buy Price");
	shopGrid->SetColLabelValue(3, "Sell Price");

	shopGrid->SetGridLineColour(wxColour(180, 150, 50));
	shopGrid->SetDefaultCellBackgroundColour(wxColour(16, 28, 48));
	shopGrid->SetDefaultCellTextColour(wxColour(240, 245, 255));
	shopGrid->SetLabelBackgroundColour(wxColour(10, 20, 35));
	shopGrid->SetLabelTextColour(wxColour(180, 190, 205));

	shopGrid->SetCellValue(0, 0, "2160"); shopGrid->SetCellValue(0, 1, "crystal coin"); shopGrid->SetCellValue(0, 2, "10000"); shopGrid->SetCellValue(0, 3, "10000");
	shopGrid->SetCellValue(1, 0, "2152"); shopGrid->SetCellValue(1, 1, "platinum coin"); shopGrid->SetCellValue(1, 2, "100"); shopGrid->SetCellValue(1, 3, "100");
	shopGrid->SetCellValue(2, 0, "2148"); shopGrid->SetCellValue(2, 1, "gold coin"); shopGrid->SetCellValue(2, 2, "1"); shopGrid->SetCellValue(2, 3, "1");

	shopSizer->Add(shopGrid, 1, wxEXPAND | wxALL, 8);
	shopPanel->SetSizer(shopSizer);

	topsizer->Add(shopPanel, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

	// Action Buttons
	wxBoxSizer* btnsizer = newd wxBoxSizer(wxHORIZONTAL);
	wxButton* okBtn = newd wxButton(this, wxID_OK, "Generate NPC Files");
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
