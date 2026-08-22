#include "monster_editor_dialog.h"
#include "find_creature_dialog.h"
#include "find_item_window.h"
#include "tfs_npc_wizard_window.h"
#include "gui.h"
#include "editor.h"
#include "items.h"
#include "creatures.h"
#include "graphics.h"
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/dcclient.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/scrolwin.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <set>

enum {
	ID_MON_PRESET = wxID_HIGHEST + 700,
	ID_MON_PICK_PRESET_PALETTE,
	ID_MON_LOOKTYPE,
	ID_MON_LOOKTYPEEX,
	ID_MON_PALETTE,
	ID_MON_RANDOM_COLOR,
	ID_MON_ROTATE,
	ID_MON_TOGGLE_VIEW,
	ID_MON_STEP_FRAME,
	ID_MON_STEP_TIMER,
	ID_MON_BTN_HEAD,
	ID_MON_BTN_PRIMARY,
	ID_MON_BTN_SECONDARY,
	ID_MON_BTN_DETAIL,
	ID_MON_ATTACK_TYPE,
	ID_MON_ADD_ATTACK,
	ID_MON_REM_ATTACK,
	ID_MON_PICK_LOOT,
	ID_MON_ADD_LOOT,
	ID_MON_REM_LOOT,
	ID_MON_SAVE_XML,
	ID_MON_REGISTER_PALETTE
};

BEGIN_EVENT_TABLE(MonsterEditorDialog, wxDialog)
	EVT_CHOICE(ID_MON_PRESET, MonsterEditorDialog::OnLookTypePresetChanged)
	EVT_BUTTON(ID_MON_PICK_PRESET_PALETTE, MonsterEditorDialog::OnPickMonsterFromPalette)
	EVT_SPINCTRL(ID_MON_LOOKTYPE, MonsterEditorDialog::OnOutfitSpinChanged)
	EVT_SPINCTRL(ID_MON_LOOKTYPEEX, MonsterEditorDialog::OnOutfitSpinChanged)
	EVT_BUTTON(ID_MON_PALETTE, MonsterEditorDialog::OnPaletteSelected)
	EVT_BUTTON(ID_MON_RANDOM_COLOR, MonsterEditorDialog::OnRandomizeColors)
	EVT_BUTTON(ID_MON_ROTATE, MonsterEditorDialog::OnRotate)
	EVT_BUTTON(ID_MON_TOGGLE_VIEW, MonsterEditorDialog::OnToggleView)
	EVT_BUTTON(ID_MON_STEP_FRAME, MonsterEditorDialog::OnStepFrame)
	EVT_TIMER(ID_MON_STEP_TIMER, MonsterEditorDialog::OnStepTimer)
	EVT_BUTTON(ID_MON_ADD_ATTACK, MonsterEditorDialog::OnAddAttack)
	EVT_BUTTON(ID_MON_REM_ATTACK, MonsterEditorDialog::OnRemoveAttack)
	EVT_BUTTON(ID_MON_PICK_LOOT, MonsterEditorDialog::OnPickLootItemFromPalette)
	EVT_BUTTON(ID_MON_ADD_LOOT, MonsterEditorDialog::OnAddLoot)
	EVT_BUTTON(ID_MON_REM_LOOT, MonsterEditorDialog::OnRemoveLoot)
	EVT_BUTTON(ID_MON_SAVE_XML, MonsterEditorDialog::OnSaveXmlFile)
	EVT_BUTTON(ID_MON_REGISTER_PALETTE, MonsterEditorDialog::OnRegisterInPalette)
	EVT_BUTTON(wxID_CANCEL, MonsterEditorDialog::OnClose)
END_EVENT_TABLE()

MonsterEditorDialog::MonsterEditorDialog(wxWindow* parent) :
	wxDialog(parent, wxID_ANY, "Monster Editor", wxDefaultPosition, wxSize(880, 740), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	active_channel(CHANNEL_HEAD),
	look_type(35), // Demon
	look_head(0),
	look_body(0),
	look_legs(0),
	look_feet(0),
	look_addons(0),
	mount_type(0),
	current_direction(2), // South
	current_frame(0),
	loot_selected_item_id(2160),
	is_stepping_loop(false)
{
	step_timer = new wxTimer(this, ID_MON_STEP_TIMER);

	wxBoxSizer* rootSizer = new wxBoxSizer(wxVERTICAL);

	// Header Panel (Corporate Dark Obsidian with Gold Accent)
	wxPanel* headerPanel = new wxPanel(this, wxID_ANY);
	headerPanel->SetBackgroundColour(wxColour(16, 20, 30));
	wxBoxSizer* headerSizer = new wxBoxSizer(wxVERTICAL);

	wxStaticText* title = new wxStaticText(headerPanel, wxID_ANY, "Monster Editor");
	wxFont tFont = title->GetFont();
	tFont.SetPointSize(12);
	tFont.SetWeight(wxFONTWEIGHT_BOLD);
	title->SetFont(tFont);
	title->SetForegroundColour(wxColour(255, 215, 0));
	headerSizer->Add(title, 0, wxALL, 8);

	wxStaticText* sub = new wxStaticText(headerPanel, wxID_ANY, "Design custom monsters with full outfit styling, attacks, defenses, resistances, and loot tables.");
	sub->SetForegroundColour(wxColour(190, 195, 205));
	headerSizer->Add(sub, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

	headerPanel->SetSizer(headerSizer);
	rootSizer->Add(headerPanel, 0, wxEXPAND);

	notebook = new wxNotebook(this, wxID_ANY);

	// =========================================================================
	// TAB 1: Main (General Attributes + Attacks & Spells + Defenses & Resistances)
	// =========================================================================
	wxScrolledWindow* tabMain = new wxScrolledWindow(notebook, wxID_ANY);
	tabMain->SetScrollRate(5, 10);
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	// 1.1 Core Attributes Box
	wxStaticBoxSizer* baseBox = new wxStaticBoxSizer(wxVERTICAL, tabMain, "Core Attributes");
	wxFlexGridSizer* baseGrid = new wxFlexGridSizer(2, 4, 6, 10);
	baseGrid->AddGrowableCol(1, 1);
	baseGrid->AddGrowableCol(3, 1);

	baseGrid->Add(new wxStaticText(tabMain, wxID_ANY, "Monster Name:"), 0, wxALIGN_CENTER_VERTICAL);
	mon_name_ctrl = new wxTextCtrl(tabMain, wxID_ANY, "Demon Lord");
	baseGrid->Add(mon_name_ctrl, 1, wxEXPAND);

	baseGrid->Add(new wxStaticText(tabMain, wxID_ANY, "Description / Title:"), 0, wxALIGN_CENTER_VERTICAL);
	mon_description_ctrl = new wxTextCtrl(tabMain, wxID_ANY, "a demon lord");
	baseGrid->Add(mon_description_ctrl, 1, wxEXPAND);

	baseGrid->Add(new wxStaticText(tabMain, wxID_ANY, "Health / Max Health:"), 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* hpSizer = new wxBoxSizer(wxHORIZONTAL);
	mon_health_ctrl = new wxSpinCtrl(tabMain, wxID_ANY, "8200", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 9999999, 8200);
	mon_maxhealth_ctrl = new wxSpinCtrl(tabMain, wxID_ANY, "8200", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 9999999, 8200);
	hpSizer->Add(mon_health_ctrl, 1, wxEXPAND | wxRIGHT, 4);
	hpSizer->Add(mon_maxhealth_ctrl, 1, wxEXPAND);
	baseGrid->Add(hpSizer, 1, wxEXPAND);

	baseGrid->Add(new wxStaticText(tabMain, wxID_ANY, "Experience (EXP):"), 0, wxALIGN_CENTER_VERTICAL);
	mon_exp_ctrl = new wxSpinCtrl(tabMain, wxID_ANY, "6000", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 9999999, 6000);
	baseGrid->Add(mon_exp_ctrl, 1, wxEXPAND);

	baseGrid->Add(new wxStaticText(tabMain, wxID_ANY, "Speed:"), 0, wxALIGN_CENTER_VERTICAL);
	mon_speed_ctrl = new wxSpinCtrl(tabMain, wxID_ANY, "280", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 5000, 280);
	baseGrid->Add(mon_speed_ctrl, 1, wxEXPAND);

	baseGrid->Add(new wxStaticText(tabMain, wxID_ANY, "Creature Race:"), 0, wxALIGN_CENTER_VERTICAL);
	wxArrayString races;
	races.Add("blood");
	races.Add("undead");
	races.Add("venom");
	races.Add("energy");
	races.Add("fire");
	races.Add("holy");
	races.Add("ice");
	races.Add("earth");
	races.Add("death");
	races.Add("ghost");
	races.Add("construct");
	races.Add("amphibic");
	races.Add("aquatic");
	races.Add("plant");

	mon_race_choice = new wxChoice(tabMain, wxID_ANY, wxDefaultPosition, wxDefaultSize, races);
	mon_race_choice->SetSelection(4); // "fire"
	baseGrid->Add(mon_race_choice, 1, wxEXPAND);

	baseGrid->Add(new wxStaticText(tabMain, wxID_ANY, "Armor / Defense:"), 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* armSizer = new wxBoxSizer(wxHORIZONTAL);
	mon_armor_ctrl = new wxSpinCtrl(tabMain, wxID_ANY, "40", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 1000, 40);
	mon_defense_ctrl = new wxSpinCtrl(tabMain, wxID_ANY, "40", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 1000, 40);
	armSizer->Add(mon_armor_ctrl, 1, wxEXPAND | wxRIGHT, 4);
	armSizer->Add(mon_defense_ctrl, 1, wxEXPAND);
	baseGrid->Add(armSizer, 1, wxEXPAND);

	baseGrid->Add(new wxStaticText(tabMain, wxID_ANY, "Corpse ItemID:"), 0, wxALIGN_CENTER_VERTICAL);
	mon_corpse_ctrl = new wxSpinCtrl(tabMain, wxID_ANY, "5995", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 65535, 5995);
	baseGrid->Add(mon_corpse_ctrl, 1, wxEXPAND);

	baseGrid->Add(new wxStaticText(tabMain, wxID_ANY, "Target Distance:"), 0, wxALIGN_CENTER_VERTICAL);
	mon_target_distance_ctrl = new wxSpinCtrl(tabMain, wxID_ANY, "1", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 10, 1);
	baseGrid->Add(mon_target_distance_ctrl, 1, wxEXPAND);

	baseBox->Add(baseGrid, 1, wxEXPAND | wxALL, 6);
	mainSizer->Add(baseBox, 0, wxEXPAND | wxALL, 6);

	// 1.2 Attacks & Spells Box
	wxStaticBoxSizer* atkBox = new wxStaticBoxSizer(wxVERTICAL, tabMain, "Attacks & Combat Spells");
	attacks_list = new wxListView(tabMain, wxID_ANY, wxDefaultPosition, wxSize(-1, 110), wxLC_REPORT | wxLC_SINGLE_SEL);
	attacks_list->InsertColumn(0, "Attack Type / Name", wxLIST_FORMAT_LEFT, 160);
	attacks_list->InsertColumn(1, "Min Dmg", wxLIST_FORMAT_RIGHT, 75);
	attacks_list->InsertColumn(2, "Max Dmg", wxLIST_FORMAT_RIGHT, 75);
	attacks_list->InsertColumn(3, "Interval (ms)", wxLIST_FORMAT_RIGHT, 85);
	attacks_list->InsertColumn(4, "Chance (%)", wxLIST_FORMAT_RIGHT, 75);
	atkBox->Add(attacks_list, 0, wxEXPAND | wxALL, 4);

	wxFlexGridSizer* atkGrid = new wxFlexGridSizer(2, 4, 4, 8);
	atkGrid->AddGrowableCol(1, 1);
	atkGrid->AddGrowableCol(3, 1);

	atkGrid->Add(new wxStaticText(tabMain, wxID_ANY, "Attack Type:"), 0, wxALIGN_CENTER_VERTICAL);
	wxArrayString atks;
	atks.Add("melee");
	atks.Add("combat (physical)");
	atks.Add("combat (fire)");
	atks.Add("combat (energy)");
	atks.Add("combat (earth)");
	atks.Add("combat (ice)");
	atks.Add("combat (holy)");
	atks.Add("combat (death)");
	atks.Add("combat (lifedrain)");
	atks.Add("combat (manadrain)");
	atks.Add("combat (speed)");
	atks.Add("combat (drunk)");
	atks.Add("combat (drown)");
	atks.Add("Custom");
	attack_type_choice = new wxChoice(tabMain, ID_MON_ATTACK_TYPE, wxDefaultPosition, wxDefaultSize, atks);
	attack_type_choice->SetSelection(0);
	atkGrid->Add(attack_type_choice, 1, wxEXPAND);

	atkGrid->Add(new wxStaticText(tabMain, wxID_ANY, "Custom Attack Name:"), 0, wxALIGN_CENTER_VERTICAL);
	attack_custom_name = new wxTextCtrl(tabMain, wxID_ANY, "great fireball");
	attack_custom_name->Enable(false);
	atkGrid->Add(attack_custom_name, 1, wxEXPAND);

	attack_type_choice->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
		bool isCustom = (attack_type_choice->GetStringSelection() == "Custom");
		attack_custom_name->Enable(isCustom);
	});

	atkGrid->Add(new wxStaticText(tabMain, wxID_ANY, "Damage (Min / Max):"), 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* dmgSizer = new wxBoxSizer(wxHORIZONTAL);
	attack_min_dmg = new wxSpinCtrl(tabMain, wxID_ANY, "100", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 99999, 100);
	attack_max_dmg = new wxSpinCtrl(tabMain, wxID_ANY, "300", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 99999, 300);
	dmgSizer->Add(attack_min_dmg, 1, wxEXPAND | wxRIGHT, 4);
	dmgSizer->Add(attack_max_dmg, 1, wxEXPAND);
	atkGrid->Add(dmgSizer, 1, wxEXPAND);

	atkGrid->Add(new wxStaticText(tabMain, wxID_ANY, "Interval / Chance (%):"), 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* atICh = new wxBoxSizer(wxHORIZONTAL);
	attack_interval = new wxSpinCtrl(tabMain, wxID_ANY, "2000", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 100, 60000, 2000);
	attack_chance = new wxSpinCtrl(tabMain, wxID_ANY, "15", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 15);
	atICh->Add(attack_interval, 1, wxEXPAND | wxRIGHT, 4);
	atICh->Add(attack_chance, 1, wxEXPAND);
	atkGrid->Add(atICh, 1, wxEXPAND);

	atkBox->Add(atkGrid, 0, wxEXPAND | wxALL, 4);

	wxBoxSizer* atkBtnRow = new wxBoxSizer(wxHORIZONTAL);
	wxButton* addAtkBtn = new wxButton(tabMain, ID_MON_ADD_ATTACK, "+ Add Attack");
	addAtkBtn->SetBackgroundColour(wxColour(40, 120, 60));
	addAtkBtn->SetForegroundColour(*wxWHITE);
	atkBtnRow->Add(addAtkBtn, 0, wxRIGHT, 6);
	atkBtnRow->Add(new wxButton(tabMain, ID_MON_REM_ATTACK, "- Remove Selected"), 0);
	atkBox->Add(atkBtnRow, 0, wxALL, 4);

	mainSizer->Add(atkBox, 0, wxEXPAND | wxALL, 6);

	// 1.3 Defenses & Elemental Resistances Box
	wxStaticBoxSizer* defBox = new wxStaticBoxSizer(wxVERTICAL, tabMain, "Defenses & Elemental Resistances");
	wxFlexGridSizer* healGrid = new wxFlexGridSizer(2, 4, 4, 8);
	healGrid->AddGrowableCol(1, 1);
	healGrid->AddGrowableCol(3, 1);

	healGrid->Add(new wxStaticText(tabMain, wxID_ANY, "Heal Spell (Min/Max):"), 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* hDmgSizer = new wxBoxSizer(wxHORIZONTAL);
	def_heal_min = new wxSpinCtrl(tabMain, wxID_ANY, "150", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 99999, 150);
	def_heal_max = new wxSpinCtrl(tabMain, wxID_ANY, "350", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 99999, 350);
	hDmgSizer->Add(def_heal_min, 1, wxEXPAND | wxRIGHT, 4);
	hDmgSizer->Add(def_heal_max, 1, wxEXPAND);
	healGrid->Add(hDmgSizer, 1, wxEXPAND);

	healGrid->Add(new wxStaticText(tabMain, wxID_ANY, "Heal Interval / Chance:"), 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* hChSizer = new wxBoxSizer(wxHORIZONTAL);
	def_heal_interval = new wxSpinCtrl(tabMain, wxID_ANY, "2000", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 100, 60000, 2000);
	def_heal_chance = new wxSpinCtrl(tabMain, wxID_ANY, "15", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 15);
	hChSizer->Add(def_heal_interval, 1, wxEXPAND | wxRIGHT, 4);
	hChSizer->Add(def_heal_chance, 1, wxEXPAND);
	healGrid->Add(hChSizer, 1, wxEXPAND);

	defBox->Add(healGrid, 0, wxEXPAND | wxALL, 4);

	wxGridSizer* immGrid = new wxGridSizer(2, 4, 4, 8);
	immGrid->Add(new wxStaticText(tabMain, wxID_ANY, "Physical %:"), 0, wxALIGN_CENTER_VERTICAL);
	imm_physical = new wxSpinCtrl(tabMain, wxID_ANY, "100", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -100, 500, 100);
	immGrid->Add(imm_physical, 1, wxEXPAND);

	immGrid->Add(new wxStaticText(tabMain, wxID_ANY, "Fire %:"), 0, wxALIGN_CENTER_VERTICAL);
	imm_fire = new wxSpinCtrl(tabMain, wxID_ANY, "-100", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -100, 500, -100);
	immGrid->Add(imm_fire, 1, wxEXPAND);

	immGrid->Add(new wxStaticText(tabMain, wxID_ANY, "Earth %:"), 0, wxALIGN_CENTER_VERTICAL);
	imm_earth = new wxSpinCtrl(tabMain, wxID_ANY, "-100", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -100, 500, -100);
	immGrid->Add(imm_earth, 1, wxEXPAND);

	immGrid->Add(new wxStaticText(tabMain, wxID_ANY, "Energy %:"), 0, wxALIGN_CENTER_VERTICAL);
	imm_energy = new wxSpinCtrl(tabMain, wxID_ANY, "100", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -100, 500, 100);
	immGrid->Add(imm_energy, 1, wxEXPAND);

	immGrid->Add(new wxStaticText(tabMain, wxID_ANY, "Ice %:"), 0, wxALIGN_CENTER_VERTICAL);
	imm_ice = new wxSpinCtrl(tabMain, wxID_ANY, "100", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -100, 500, 100);
	immGrid->Add(imm_ice, 1, wxEXPAND);

	immGrid->Add(new wxStaticText(tabMain, wxID_ANY, "Holy %:"), 0, wxALIGN_CENTER_VERTICAL);
	imm_holy = new wxSpinCtrl(tabMain, wxID_ANY, "100", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -100, 500, 100);
	immGrid->Add(imm_holy, 1, wxEXPAND);

	immGrid->Add(new wxStaticText(tabMain, wxID_ANY, "Death %:"), 0, wxALIGN_CENTER_VERTICAL);
	imm_death = new wxSpinCtrl(tabMain, wxID_ANY, "100", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -100, 500, 100);
	immGrid->Add(imm_death, 1, wxEXPAND);

	defBox->Add(immGrid, 0, wxEXPAND | wxALL, 4);
	mainSizer->Add(defBox, 0, wxEXPAND | wxALL, 6);

	tabMain->SetSizer(mainSizer);
	notebook->AddPage(tabMain, "Main");

	// =========================================================================
	// TAB 2: Look
	// =========================================================================
	wxPanel* tabLook = new wxPanel(notebook, wxID_ANY);
	wxBoxSizer* lookMainSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* topLookRow = new wxBoxSizer(wxHORIZONTAL);

	// Left: Preview Card + Rotation / Animation Controls
	wxBoxSizer* previewCol = new wxBoxSizer(wxVERTICAL);
	preview_panel = new CreaturePreviewPanel(tabLook, wxID_ANY, 150);
	previewCol->Add(preview_panel, 0, wxALIGN_CENTER | wxBOTTOM, 6);

	wxBoxSizer* actBtnRow = new wxBoxSizer(wxHORIZONTAL);
	wxButton* rotBtn = new wxButton(tabLook, ID_MON_ROTATE, "Rotate", wxDefaultPosition, wxSize(65, 26));
	wxButton* viewBtn = new wxButton(tabLook, ID_MON_TOGGLE_VIEW, "Back/Front", wxDefaultPosition, wxSize(80, 26));
	btn_step = new wxButton(tabLook, ID_MON_STEP_FRAME, "Step", wxDefaultPosition, wxSize(65, 26));
	wxButton* randBtn = new wxButton(tabLook, ID_MON_RANDOM_COLOR, "Randomize", wxDefaultPosition, wxSize(80, 26));
	actBtnRow->Add(rotBtn, 0, wxRIGHT, 4);
	actBtnRow->Add(viewBtn, 0, wxRIGHT, 4);
	actBtnRow->Add(btn_step, 0, wxRIGHT, 4);
	actBtnRow->Add(randBtn, 0);
	previewCol->Add(actBtnRow, 0, wxALIGN_CENTER);

	topLookRow->Add(previewCol, 0, wxALL, 8);

	// Right: Model Selection & TLG Channel Selector
	wxBoxSizer* ctrlCol = new wxBoxSizer(wxVERTICAL);
	wxFlexGridSizer* modelGrid = new wxFlexGridSizer(2, 2, 6, 8);
	modelGrid->AddGrowableCol(1, 1);

	modelGrid->Add(new wxStaticText(tabLook, wxID_ANY, "Monster Preset:"), 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* presetRow = new wxBoxSizer(wxHORIZONTAL);
	outfit_preset_choice = new wxChoice(tabLook, ID_MON_PRESET);
	PopulateMonsterPresets();
	wxButton* pickMonsterBtn = new wxButton(tabLook, ID_MON_PICK_PRESET_PALETTE, "Select..", wxDefaultPosition, wxSize(90, -1));
	pickMonsterBtn->SetBackgroundColour(wxColour(40, 70, 120));
	pickMonsterBtn->SetForegroundColour(*wxWHITE);
	presetRow->Add(outfit_preset_choice, 1, wxRIGHT, 4);
	presetRow->Add(pickMonsterBtn, 0);
	modelGrid->Add(presetRow, 1, wxEXPAND);

	modelGrid->Add(new wxStaticText(tabLook, wxID_ANY, "LookType / LookTypeEx:"), 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* ltSizer = new wxBoxSizer(wxHORIZONTAL);
	outfit_looktype_ctrl = new wxSpinCtrl(tabLook, ID_MON_LOOKTYPE, "35", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 65535, 35);
	outfit_looktypeex_ctrl = new wxSpinCtrl(tabLook, ID_MON_LOOKTYPEEX, "0", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 65535, 0);
	ltSizer->Add(outfit_looktype_ctrl, 1, wxEXPAND | wxRIGHT, 4);
	ltSizer->Add(outfit_looktypeex_ctrl, 1, wxEXPAND);
	modelGrid->Add(ltSizer, 1, wxEXPAND);

	ctrlCol->Add(modelGrid, 0, wxEXPAND | wxBOTTOM, 6);

	// 4 Channel Selection Buttons (Head, Primary, Secondary, Detail)
	wxBoxSizer* palRow = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* chCol = new wxBoxSizer(wxVERTICAL);

	btn_channel_head = new wxButton(tabLook, ID_MON_BTN_HEAD, "Head", wxDefaultPosition, wxSize(82, 28));
	btn_channel_primary = new wxButton(tabLook, ID_MON_BTN_PRIMARY, "Primary", wxDefaultPosition, wxSize(82, 28));
	btn_channel_secondary = new wxButton(tabLook, ID_MON_BTN_SECONDARY, "Secondary", wxDefaultPosition, wxSize(82, 28));
	btn_channel_detail = new wxButton(tabLook, ID_MON_BTN_DETAIL, "Detail", wxDefaultPosition, wxSize(82, 28));

	btn_channel_head->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnChannelButtonClicked(CHANNEL_HEAD); });
	btn_channel_primary->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnChannelButtonClicked(CHANNEL_PRIMARY); });
	btn_channel_secondary->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnChannelButtonClicked(CHANNEL_SECONDARY); });
	btn_channel_detail->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnChannelButtonClicked(CHANNEL_DETAIL); });

	chCol->Add(btn_channel_head, 0, wxBOTTOM, 4);
	chCol->Add(btn_channel_primary, 0, wxBOTTOM, 4);
	chCol->Add(btn_channel_secondary, 0, wxBOTTOM, 4);
	chCol->Add(btn_channel_detail, 0);
	palRow->Add(chCol, 0, wxRIGHT, 8);

	outfit_palette = new TibiaPalettePanel(tabLook, ID_MON_PALETTE);
	palRow->Add(outfit_palette, 0, wxALIGN_CENTER_VERTICAL);
	ctrlCol->Add(palRow, 0, wxEXPAND);

	topLookRow->Add(ctrlCol, 1, wxALL | wxEXPAND, 8);
	lookMainSizer->Add(topLookRow, 0, wxEXPAND | wxALL, 4);

	// Live Code Bar
	wxBoxSizer* codeRow = new wxBoxSizer(wxHORIZONTAL);
	codeRow->Add(new wxStaticText(tabLook, wxID_ANY, "Look Tag:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
	live_look_code_ctrl = new wxTextCtrl(tabLook, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	codeRow->Add(live_look_code_ctrl, 1, wxEXPAND);
	lookMainSizer->Add(codeRow, 0, wxEXPAND | wxALL, 8);

	tabLook->SetSizer(lookMainSizer);
	notebook->AddPage(tabLook, "Look");

	// =========================================================================
	// TAB 3: Loot
	// =========================================================================
	wxPanel* tabLoot = new wxPanel(notebook, wxID_ANY);
	wxBoxSizer* lootSizer = new wxBoxSizer(wxVERTICAL);

	loot_list = new wxListView(tabLoot, wxID_ANY, wxDefaultPosition, wxSize(-1, 200), wxLC_REPORT | wxLC_SINGLE_SEL);
	loot_list->InsertColumn(0, "Item ID", wxLIST_FORMAT_RIGHT, 80);
	loot_list->InsertColumn(1, "Item Name", wxLIST_FORMAT_LEFT, 200);
	loot_list->InsertColumn(2, "Max Count", wxLIST_FORMAT_RIGHT, 100);
	loot_list->InsertColumn(3, "Drop Chance (%)", wxLIST_FORMAT_RIGHT, 120);

	loot_list->Bind(wxEVT_CONTEXT_MENU, [this](wxContextMenuEvent&) {
		long sel = loot_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		wxMenu menu;
		if (sel != -1) {
			menu.Append(1001, "Edit Loot (Count & Chance)...");
			menu.Append(1002, "Delete Selected Loot");
			menu.Bind(wxEVT_MENU, [this, sel](wxCommandEvent& e) {
				if (e.GetId() == 1001) {
					OnEditLoot(e);
				} else if (e.GetId() == 1002) {
					wxCommandEvent dummy;
					OnRemoveLoot(dummy);
				}
			});
		} else {
			menu.Append(1003, "Select..");
			menu.Bind(wxEVT_MENU, [this](wxCommandEvent&) {
				wxCommandEvent dummy;
				OnPickLootItemFromPalette(dummy);
			});
		}
		loot_list->PopupMenu(&menu);
	});

	loot_list->Bind(wxEVT_LIST_ITEM_SELECTED, [this](wxListEvent& event) {
		long idx = event.GetIndex();
		if (idx >= 0 && idx < (long)loot_items.size()) {
			const auto& l = loot_items[idx];
			loot_selected_item_id = l.item_id;
			loot_selected_item_text->SetLabel(wxString::Format("Item: %s (ID %d)", l.name, l.item_id));
			loot_countmax->SetValue(l.countmax);
			loot_chance_percent->SetValue(l.chance / 1000);
		}
	});

	lootSizer->Add(loot_list, 1, wxEXPAND | wxALL, 8);

	wxStaticBoxSizer* addLootBox = new wxStaticBoxSizer(wxHORIZONTAL, tabLoot, "Add Item Drop");
	wxBoxSizer* lootCtrlRow = new wxBoxSizer(wxHORIZONTAL);

	wxButton* pickLootBtn = new wxButton(tabLoot, ID_MON_PICK_LOOT, "Select..");
	pickLootBtn->SetBackgroundColour(wxColour(40, 70, 120));
	pickLootBtn->SetForegroundColour(*wxWHITE);
	lootCtrlRow->Add(pickLootBtn, 0, wxRIGHT, 8);

	loot_selected_item_text = new wxStaticText(tabLoot, wxID_ANY, "Item: crystal coin (ID 2160)");
	lootCtrlRow->Add(loot_selected_item_text, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	lootCtrlRow->Add(new wxStaticText(tabLoot, wxID_ANY, "CountMax:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	loot_countmax = new wxSpinCtrl(tabLoot, wxID_ANY, "1", wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS, 1, 100, 1);
	lootCtrlRow->Add(loot_countmax, 0, wxRIGHT, 8);

	lootCtrlRow->Add(new wxStaticText(tabLoot, wxID_ANY, "Chance (%):"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	loot_chance_percent = new wxSpinCtrl(tabLoot, wxID_ANY, "10", wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS, 1, 100, 10);
	lootCtrlRow->Add(loot_chance_percent, 0, wxRIGHT, 8);

	wxButton* addLootBtn = new wxButton(tabLoot, ID_MON_ADD_LOOT, "+ Add Loot");
	addLootBtn->SetBackgroundColour(wxColour(40, 120, 60));
	addLootBtn->SetForegroundColour(*wxWHITE);
	lootCtrlRow->Add(addLootBtn, 0, wxRIGHT, 4);

	wxButton* remLootBtn = new wxButton(tabLoot, ID_MON_REM_LOOT, "- Remove Selected");
	lootCtrlRow->Add(remLootBtn, 0);

	addLootBox->Add(lootCtrlRow, 1, wxEXPAND | wxALL, 6);
	lootSizer->Add(addLootBox, 0, wxEXPAND | wxALL, 8);

	tabLoot->SetSizer(lootSizer);
	notebook->AddPage(tabLoot, "Loot");

	rootSizer->Add(notebook, 1, wxALL | wxEXPAND, 8);

	// Bottom Bar
	wxBoxSizer* bottomSizer = new wxBoxSizer(wxHORIZONTAL);
	wxButton* saveXmlBtn = new wxButton(this, ID_MON_SAVE_XML, "Export TFS XML...");
	saveXmlBtn->SetBackgroundColour(wxColour(40, 70, 120));
	saveXmlBtn->SetForegroundColour(*wxWHITE);

	wxButton* regPalBtn = new wxButton(this, ID_MON_REGISTER_PALETTE, "Add to Creature Palette");
	regPalBtn->SetBackgroundColour(wxColour(200, 140, 30));
	regPalBtn->SetForegroundColour(*wxWHITE);

	wxButton* closeBtn = new wxButton(this, wxID_CANCEL, "Close");

	bottomSizer->Add(saveXmlBtn, 0, wxRIGHT, 8);
	bottomSizer->Add(regPalBtn, 0, wxRIGHT, 8);
	bottomSizer->AddStretchSpacer();
	bottomSizer->Add(closeBtn, 0);

	rootSizer->Add(bottomSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

	SetSizer(rootSizer);
	Layout();
	CenterOnParent();

	// Default Demo Attacks
	MonsterAttackEntry atk1{"melee", "melee", 150, 450, 2000, 100};
	MonsterAttackEntry atk2{"firefield", "combat", 200, 350, 3000, 20};
	attacks.push_back(atk1);
	attacks.push_back(atk2);
	for (const auto& a : attacks) {
		long idx = attacks_list->InsertItem(attacks_list->GetItemCount(), a.name);
		attacks_list->SetItem(idx, 1, std::to_string(a.min_damage));
		attacks_list->SetItem(idx, 2, std::to_string(a.max_damage));
		attacks_list->SetItem(idx, 3, std::to_string(a.interval));
		attacks_list->SetItem(idx, 4, std::to_string(a.chance));
	}

	// Default Demo Loot
	MonsterLootEntry l1{2160, "crystal coin", 5, 25000};
	MonsterLootEntry l2{2152, "platinum coin", 20, 80000};
	loot_items.push_back(l1);
	loot_items.push_back(l2);
	for (const auto& l : loot_items) {
		long idx = loot_list->InsertItem(loot_list->GetItemCount(), std::to_string(l.item_id));
		loot_list->SetItem(idx, 1, l.name);
		loot_list->SetItem(idx, 2, std::to_string(l.countmax));
		loot_list->SetItem(idx, 3, wxString::Format("%.1f%%", l.chance / 1000.0f));
	}

	UpdateOutfitPreview();
	UpdateLiveCode();
}

MonsterEditorDialog::~MonsterEditorDialog() {
	if (step_timer) {
		step_timer->Stop();
		delete step_timer;
		step_timer = nullptr;
	}
}

void MonsterEditorDialog::PopulateMonsterPresets() {
	outfit_preset_choice->Clear();
	int demonIdx = 0;

	for (auto iter = g_creatures.begin(); iter != g_creatures.end(); ++iter) {
		CreatureType* ct = iter->second;
		if (!ct) continue;

		int itemIdx = outfit_preset_choice->Append(ct->name, (void*)ct);
		if (ct->name == "Demon") {
			demonIdx = itemIdx;
		}
	}

	if (outfit_preset_choice->GetCount() > 0) {
		outfit_preset_choice->SetSelection(demonIdx);
	}
}

void MonsterEditorDialog::LoadCreatureType(CreatureType* ct) {
	if (!ct) return;

	mon_name_ctrl->SetValue(ct->name);
	mon_description_ctrl->SetValue("a " + ct->name);
	look_type = ct->outfit.lookType;
	look_head = ct->outfit.lookHead;
	look_body = ct->outfit.lookBody;
	look_legs = ct->outfit.lookLegs;
	look_feet = ct->outfit.lookFeet;
	look_addons = ct->outfit.lookAddon;

	outfit_looktype_ctrl->SetValue(look_type);

	// Select in choice dropdown if present
	for (unsigned int i = 0; i < outfit_preset_choice->GetCount(); ++i) {
		if (outfit_preset_choice->GetString(i) == ct->name) {
			outfit_preset_choice->SetSelection(i);
			break;
		}
	}

	UpdateOutfitPreview();
	UpdateLiveCode();
}

void MonsterEditorDialog::OnPickMonsterFromPalette(wxCommandEvent& WXUNUSED(event)) {
	FindCreatureDialog dlg(this, "Select Monster from Palette");
	if (dlg.ShowModal() == wxID_OK) {
		CreatureType* ct = dlg.GetSelectedCreature();
		if (ct) {
			LoadCreatureType(ct);
		}
	}
}

void MonsterEditorDialog::OnLookTypePresetChanged(wxCommandEvent& WXUNUSED(event)) {
	int sel = outfit_preset_choice->GetSelection();
	if (sel == wxNOT_FOUND) return;

	CreatureType* ct = (CreatureType*)outfit_preset_choice->GetClientData(sel);
	if (ct) {
		LoadCreatureType(ct);
	}
}

void MonsterEditorDialog::OnOutfitParamChanged(wxCommandEvent& WXUNUSED(event)) {
	UpdateOutfitPreview();
	UpdateLiveCode();
}

void MonsterEditorDialog::OnOutfitSpinChanged(wxSpinEvent& WXUNUSED(event)) {
	look_type = outfit_looktype_ctrl->GetValue();
	UpdateOutfitPreview();
	UpdateLiveCode();
}

void MonsterEditorDialog::OnChannelButtonClicked(OutfitChannel ch) {
	active_channel = ch;

	btn_channel_head->SetBackgroundColour(ch == CHANNEL_HEAD ? wxColour(200, 140, 30) : wxColour(45, 55, 75));
	btn_channel_primary->SetBackgroundColour(ch == CHANNEL_PRIMARY ? wxColour(200, 140, 30) : wxColour(45, 55, 75));
	btn_channel_secondary->SetBackgroundColour(ch == CHANNEL_SECONDARY ? wxColour(200, 140, 30) : wxColour(45, 55, 75));
	btn_channel_detail->SetBackgroundColour(ch == CHANNEL_DETAIL ? wxColour(200, 140, 30) : wxColour(45, 55, 75));

	btn_channel_head->SetForegroundColour(*wxWHITE);
	btn_channel_primary->SetForegroundColour(*wxWHITE);
	btn_channel_secondary->SetForegroundColour(*wxWHITE);
	btn_channel_detail->SetForegroundColour(*wxWHITE);

	btn_channel_head->Refresh();
	btn_channel_primary->Refresh();
	btn_channel_secondary->Refresh();
	btn_channel_detail->Refresh();
}

void MonsterEditorDialog::OnPaletteSelected(wxCommandEvent& event) {
	int colorIndex = event.GetInt();
	switch (active_channel) {
		case CHANNEL_HEAD: look_head = colorIndex; break;
		case CHANNEL_PRIMARY: look_body = colorIndex; break;
		case CHANNEL_SECONDARY: look_legs = colorIndex; break;
		case CHANNEL_DETAIL: look_feet = colorIndex; break;
	}
	UpdateOutfitPreview();
	UpdateLiveCode();
}

void MonsterEditorDialog::OnRandomizeColors(wxCommandEvent& WXUNUSED(event)) {
	look_head = rand() % 133;
	look_body = rand() % 133;
	look_legs = rand() % 133;
	look_feet = rand() % 133;
	UpdateOutfitPreview();
	UpdateLiveCode();
}

void MonsterEditorDialog::OnRotate(wxCommandEvent& WXUNUSED(event)) {
	if (preview_panel) {
		preview_panel->RotateDirection();
		current_direction = preview_panel->GetDirection();
	}
}

void MonsterEditorDialog::OnToggleView(wxCommandEvent& WXUNUSED(event)) {
	if (preview_panel) {
		preview_panel->ToggleFrontBack();
		current_direction = preview_panel->GetDirection();
	}
}

void MonsterEditorDialog::OnStepFrame(wxCommandEvent& WXUNUSED(event)) {
	is_stepping_loop = !is_stepping_loop;
	if (is_stepping_loop) {
		if (step_timer) step_timer->Start(200);
		if (btn_step) {
			btn_step->SetLabel("Stop");
			btn_step->SetBackgroundColour(wxColour(180, 50, 50));
			btn_step->SetForegroundColour(*wxWHITE);
		}
	} else {
		if (step_timer) step_timer->Stop();
		if (btn_step) {
			btn_step->SetLabel("Step");
			btn_step->SetBackgroundColour(wxNullColour);
			btn_step->SetForegroundColour(wxNullColour);
		}
	}
}

void MonsterEditorDialog::OnStepTimer(wxTimerEvent& WXUNUSED(event)) {
	if (preview_panel) {
		preview_panel->StepFrame();
	}
}

void MonsterEditorDialog::UpdateOutfitPreview() {
	if (preview_panel) {
		preview_panel->SetOutfit(look_type, look_head, look_body, look_legs, look_feet, look_addons, 0, false, current_direction, current_frame);
	}
}

void MonsterEditorDialog::UpdateLiveCode() {
	std::ostringstream ss;
	ss << "<look type=\"" << look_type << "\"";
	if (look_head > 0) ss << " head=\"" << look_head << "\"";
	if (look_body > 0) ss << " body=\"" << look_body << "\"";
	if (look_legs > 0) ss << " legs=\"" << look_legs << "\"";
	if (look_feet > 0) ss << " feet=\"" << look_feet << "\"";
	if (look_addons > 0) ss << " addons=\"" << look_addons << "\"";
	if (mon_corpse_ctrl && mon_corpse_ctrl->GetValue() > 0) ss << " corpse=\"" << mon_corpse_ctrl->GetValue() << "\"";
	ss << "/>";
	live_look_code_ctrl->SetValue(ss.str());
}

void MonsterEditorDialog::OnAddAttack(wxCommandEvent& WXUNUSED(event)) {
	MonsterAttackEntry atk;
	std::string selType = attack_type_choice->GetStringSelection().ToStdString();
	if (selType == "Custom") {
		atk.name = attack_custom_name->GetValue().ToStdString();
		if (atk.name.empty()) atk.name = "custom_spell";
	} else {
		atk.name = selType;
	}
	atk.type = (atk.name == "melee") ? "melee" : "combat";
	atk.min_damage = attack_min_dmg->GetValue();
	atk.max_damage = attack_max_dmg->GetValue();
	atk.interval = attack_interval->GetValue();
	atk.chance = attack_chance->GetValue();

	attacks.push_back(atk);
	long idx = attacks_list->InsertItem(attacks.size() - 1, atk.name);
	attacks_list->SetItem(idx, 1, std::to_string(atk.min_damage));
	attacks_list->SetItem(idx, 2, std::to_string(atk.max_damage));
	attacks_list->SetItem(idx, 3, std::to_string(atk.interval));
	attacks_list->SetItem(idx, 4, std::to_string(atk.chance));
}

void MonsterEditorDialog::OnRemoveAttack(wxCommandEvent& WXUNUSED(event)) {
	long item = attacks_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (item != -1 && item < (long)attacks.size()) {
		attacks.erase(attacks.begin() + item);
		attacks_list->DeleteItem(item);
	}
}

void MonsterEditorDialog::OnPickLootItemFromPalette(wxCommandEvent& WXUNUSED(event)) {
	FindItemDialog dlg(this, "Select Item", true);
	if (dlg.ShowModal() == wxID_OK) {
		uint16_t id = dlg.getResultID();
		if (id > 0) {
			loot_selected_item_id = id;
			ItemType& it = g_items[id];
			loot_selected_item_text->SetLabel(wxString::Format("Item: %s (ID %d)", it.name, id));

			// Automatically add to loot table
			wxCommandEvent dummy;
			OnAddLoot(dummy);
		}
	}
}

void MonsterEditorDialog::OnAddLoot(wxCommandEvent& WXUNUSED(event)) {
	ItemType& it = g_items[loot_selected_item_id];
	MonsterLootEntry l;
	l.item_id = loot_selected_item_id;
	l.name = it.name.empty() ? "Item #" + std::to_string(l.item_id) : it.name;
	l.countmax = loot_countmax->GetValue();
	l.chance = loot_chance_percent->GetValue() * 1000;

	loot_items.push_back(l);
	long idx = loot_list->InsertItem(loot_items.size() - 1, std::to_string(l.item_id));
	loot_list->SetItem(idx, 1, l.name);
	loot_list->SetItem(idx, 2, std::to_string(l.countmax));
	loot_list->SetItem(idx, 3, wxString::Format("%.1f%%", l.chance / 1000.0f));
}

void MonsterEditorDialog::OnEditLoot(wxCommandEvent& WXUNUSED(event)) {
	long item = loot_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (item == -1 || item >= (long)loot_items.size()) return;

	MonsterLootEntry& l = loot_items[item];

	wxDialog editDlg(this, wxID_ANY, "Edit Loot Drop", wxDefaultPosition, wxSize(320, 220));
	editDlg.SetBackgroundColour(wxColour(16, 28, 48));

	wxBoxSizer* mainS = new wxBoxSizer(wxVERTICAL);
	wxStaticText* title = new wxStaticText(&editDlg, wxID_ANY, wxString::Format("Edit: %s (ID %d)", l.name, l.item_id));
	title->SetForegroundColour(wxColour(255, 215, 0));
	title->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	mainS->Add(title, 0, wxALL, 10);

	wxFlexGridSizer* fgrid = new wxFlexGridSizer(2, 6, 8);
	fgrid->AddGrowableCol(1);

	fgrid->Add(new wxStaticText(&editDlg, wxID_ANY, "Max Count:"), 0, wxALIGN_CENTER_VERTICAL);
	wxSpinCtrl* cntCtrl = new wxSpinCtrl(&editDlg, wxID_ANY, std::to_string(l.countmax), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, l.countmax);
	fgrid->Add(cntCtrl, 1, wxEXPAND);

	fgrid->Add(new wxStaticText(&editDlg, wxID_ANY, "Drop Chance (%):"), 0, wxALIGN_CENTER_VERTICAL);
	wxSpinCtrl* chnCtrl = new wxSpinCtrl(&editDlg, wxID_ANY, std::to_string(l.chance / 1000), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, l.chance / 1000);
	fgrid->Add(chnCtrl, 1, wxEXPAND);

	mainS->Add(fgrid, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	wxBoxSizer* btnS = new wxBoxSizer(wxHORIZONTAL);
	wxButton* okBtn = new wxButton(&editDlg, wxID_OK, "Save");
	okBtn->SetBackgroundColour(wxColour(40, 120, 60));
	okBtn->SetForegroundColour(*wxWHITE);
	wxButton* cancelBtn = new wxButton(&editDlg, wxID_CANCEL, "Cancel");

	btnS->Add(okBtn, 0, wxRIGHT, 6);
	btnS->Add(cancelBtn, 0);
	mainS->Add(btnS, 0, wxALIGN_CENTER | wxBOTTOM, 10);

	editDlg.SetSizer(mainS);
	editDlg.CenterOnParent();

	if (editDlg.ShowModal() == wxID_OK) {
		l.countmax = cntCtrl->GetValue();
		l.chance = chnCtrl->GetValue() * 1000;
		loot_list->SetItem(item, 2, std::to_string(l.countmax));
		loot_list->SetItem(item, 3, wxString::Format("%.1f%%", l.chance / 1000.0f));
	}
}

void MonsterEditorDialog::OnRemoveLoot(wxCommandEvent& WXUNUSED(event)) {
	long item = loot_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (item != -1 && item < (long)loot_items.size()) {
		loot_items.erase(loot_items.begin() + item);
		loot_list->DeleteItem(item);
	}
}

std::string MonsterEditorDialog::GenerateTFSXml() const {
	std::ostringstream ss;
	std::string name = mon_name_ctrl->GetValue().ToStdString();
	std::string desc = mon_description_ctrl->GetValue().ToStdString();
	std::string race = mon_race_choice->GetStringSelection().ToStdString();

	ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	ss << "<monster name=\"" << name << "\" nameDescription=\"" << desc << "\" race=\"" << race << "\" experience=\"" << mon_exp_ctrl->GetValue() << "\" speed=\"" << mon_speed_ctrl->GetValue() << "\">\n";
	ss << "\t<health now=\"" << mon_health_ctrl->GetValue() << "\" max=\"" << mon_maxhealth_ctrl->GetValue() << "\"/>\n";

	ss << "\t<look type=\"" << look_type << "\"";
	if (outfit_looktypeex_ctrl->GetValue() > 0) ss << " looktypeex=\"" << outfit_looktypeex_ctrl->GetValue() << "\"";
	if (look_head > 0) ss << " head=\"" << look_head << "\"";
	if (look_body > 0) ss << " body=\"" << look_body << "\"";
	if (look_legs > 0) ss << " legs=\"" << look_legs << "\"";
	if (look_feet > 0) ss << " feet=\"" << look_feet << "\"";
	if (look_addons > 0) ss << " addons=\"" << look_addons << "\"";
	if (mon_corpse_ctrl->GetValue() > 0) ss << " corpse=\"" << mon_corpse_ctrl->GetValue() << "\"";
	ss << "/>\n";

	ss << "\t<targetchange interval=\"4000\" chance=\"10\"/>\n";
	ss << "\t<flags>\n";
	ss << "\t\t<flag attackable=\"1\"/>\n";
	ss << "\t\t<flag hostile=\"1\"/>\n";
	ss << "\t\t<flag illusionable=\"0\"/>\n";
	ss << "\t\t<flag convinceable=\"0\"/>\n";
	ss << "\t\t<flag pushable=\"0\"/>\n";
	ss << "\t\t<flag canpushitems=\"1\"/>\n";
	ss << "\t\t<flag canpushcreatures=\"1\"/>\n";
	ss << "\t\t<flag targetdistance=\"" << mon_target_distance_ctrl->GetValue() << "\"/>\n";
	ss << "\t\t<flag staticattack=\"90\"/>\n";
	ss << "\t\t<flag runonhealth=\"0\"/>\n";
	ss << "\t</flags>\n";

	// Attacks
	ss << "\t<attacks>\n";
	for (const auto& atk : attacks) {
		if (atk.name == "melee") {
			ss << "\t\t<attack name=\"melee\" interval=\"" << atk.interval << "\" min=\"" << atk.min_damage << "\" max=\"" << atk.max_damage << "\"/>\n";
		} else {
			ss << "\t\t<attack name=\"" << atk.name << "\" interval=\"" << atk.interval << "\" chance=\"" << atk.chance << "\" min=\"" << atk.min_damage << "\" max=\"" << atk.max_damage << "\"/>\n";
		}
	}
	ss << "\t</attacks>\n";

	// Defenses
	ss << "\t<defenses armor=\"" << mon_armor_ctrl->GetValue() << "\" defense=\"" << mon_defense_ctrl->GetValue() << "\">\n";
	if (def_heal_max->GetValue() > 0) {
		ss << "\t\t<defense name=\"healing\" interval=\"" << def_heal_interval->GetValue() << "\" chance=\"" << def_heal_chance->GetValue() << "\" min=\"" << def_heal_min->GetValue() << "\" max=\"" << def_heal_max->GetValue() << "\"/>\n";
	}
	ss << "\t</defenses>\n";

	// Elements
	ss << "\t<elements>\n";
	if (imm_physical->GetValue() != 100) ss << "\t\t<element physicalPercent=\"" << (100 - imm_physical->GetValue()) << "\"/>\n";
	if (imm_fire->GetValue() != 100) ss << "\t\t<element firePercent=\"" << (100 - imm_fire->GetValue()) << "\"/>\n";
	if (imm_earth->GetValue() != 100) ss << "\t\t<element earthPercent=\"" << (100 - imm_earth->GetValue()) << "\"/>\n";
	if (imm_energy->GetValue() != 100) ss << "\t\t<element energyPercent=\"" << (100 - imm_energy->GetValue()) << "\"/>\n";
	if (imm_ice->GetValue() != 100) ss << "\t\t<element icePercent=\"" << (100 - imm_ice->GetValue()) << "\"/>\n";
	if (imm_holy->GetValue() != 100) ss << "\t\t<element holyPercent=\"" << (100 - imm_holy->GetValue()) << "\"/>\n";
	if (imm_death->GetValue() != 100) ss << "\t\t<element deathPercent=\"" << (100 - imm_death->GetValue()) << "\"/>\n";
	ss << "\t</elements>\n";

	// Loot
	if (!loot_items.empty()) {
		ss << "\t<loot>\n";
		for (const auto& l : loot_items) {
			ss << "\t\t<item id=\"" << l.item_id << "\" countmax=\"" << l.countmax << "\" chance=\"" << l.chance << "\"/> <!-- " << l.name << " -->\n";
		}
		ss << "\t</loot>\n";
	}

	ss << "</monster>\n";
	return ss.str();
}

void MonsterEditorDialog::OnSaveXmlFile(wxCommandEvent& WXUNUSED(event)) {
	std::string xml = GenerateTFSXml();
	std::string monName = mon_name_ctrl->GetValue().ToStdString();
	std::replace(monName.begin(), monName.end(), ' ', '_');
	std::transform(monName.begin(), monName.end(), monName.begin(), ::tolower);

	wxFileDialog saveFileDialog(this, "Save TFS Monster XML", "", monName + ".xml", "XML files (*.xml)|*.xml", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (saveFileDialog.ShowModal() == wxID_CANCEL) return;

	wxString path = saveFileDialog.GetPath();
	wxFile file(path, wxFile::write);
	if (file.IsOpened()) {
		file.Write(xml);
		file.Close();
		g_gui.SetStatusText(wxString::Format("Monster XML saved: %s", path));
		wxMessageBox("Monster XML saved successfully!", "Saved", wxOK | wxICON_INFORMATION, this);
	}
}

void MonsterEditorDialog::OnRegisterInPalette(wxCommandEvent& WXUNUSED(event)) {
	std::string monName = mon_name_ctrl->GetValue().ToStdString();
	if (monName.empty()) {
		wxMessageBox("Please specify a valid monster name.", "Error", wxOK | wxICON_ERROR, this);
		return;
	}

	Outfit outfit;
	outfit.lookType = look_type;
	outfit.lookHead = look_head;
	outfit.lookBody = look_body;
	outfit.lookLegs = look_legs;
	outfit.lookFeet = look_feet;
	outfit.lookAddon = look_addons;

	CreatureType* ct = g_creatures[monName];
	if (!ct) {
		ct = g_creatures.addCreatureType(monName, false, outfit);
	} else {
		ct->outfit = outfit;
	}

	if (ct) {
		g_gui.SetStatusText(wxString::Format("Monster '%s' registered into Creature Palette!", monName));
		wxMessageBox(wxString::Format("Monster '%s' has been successfully registered into the active Creature Palette!\nYou can now select and paint it directly on the map.", monName), "Registered in Palette", wxOK | wxICON_INFORMATION, this);
	}
}

void MonsterEditorDialog::OnClose(wxCommandEvent& WXUNUSED(event)) {
	if (step_timer && step_timer->IsRunning()) {
		step_timer->Stop();
	}
	EndModal(wxID_CANCEL);
}
