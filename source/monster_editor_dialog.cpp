#include "monster_editor_dialog.h"
#include "style_manager.h"
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
#include <wx/textdlg.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <set>

enum {
	ID_MON_VIEW_MODE = wxID_HIGHEST + 699,
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
	ID_MON_REGISTER_PALETTE,
	ID_MON_SIMPLE_PRESET,
	ID_MON_SIMPLE_PICK_LOOT,
	ID_MON_SIMPLE_ADD_LOOT,
	ID_MON_SIMPLE_REM_LOOT
};

BEGIN_EVENT_TABLE(MonsterEditorDialog, wxDialog)
	EVT_CHOICE(ID_MON_VIEW_MODE, MonsterEditorDialog::OnViewModeChanged)
	EVT_CHOICE(ID_MON_PRESET, MonsterEditorDialog::OnLookTypePresetChanged)
	EVT_CHOICE(ID_MON_SIMPLE_PRESET, MonsterEditorDialog::OnLookTypePresetChanged)
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
	EVT_BUTTON(ID_MON_SIMPLE_PICK_LOOT, MonsterEditorDialog::OnPickLootItemFromPalette)
	EVT_BUTTON(ID_MON_SIMPLE_ADD_LOOT, MonsterEditorDialog::OnAddLoot)
	EVT_BUTTON(ID_MON_SIMPLE_REM_LOOT, MonsterEditorDialog::OnRemoveLoot)
	EVT_BUTTON(ID_MON_SAVE_XML, MonsterEditorDialog::OnSaveXmlFile)
	EVT_BUTTON(ID_MON_REGISTER_PALETTE, MonsterEditorDialog::OnRegisterInPalette)
	EVT_BUTTON(wxID_CANCEL, MonsterEditorDialog::OnClose)
END_EVENT_TABLE()

MonsterEditorDialog::MonsterEditorDialog(wxWindow* parent) :
	wxDialog(parent, wxID_ANY, "Monster & Creature Maker", wxDefaultPosition, wxSize(880, 720), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	current_view_mode(VIEW_MODE_SIMPLE),
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
	simple_loot_selected_id(2160),
	is_stepping_loop(false)
{
	step_timer = new wxTimer(this, ID_MON_STEP_TIMER);

	wxBoxSizer* rootSizer = new wxBoxSizer(wxVERTICAL);

	// Header Panel (Corporate Dark Obsidian with Gold Accent)
	wxPanel* headerPanel = new wxPanel(this, wxID_ANY);
	headerPanel->SetBackgroundColour(wxColour(16, 20, 30));
	wxBoxSizer* headerSizer = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer* headerTopRow = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* title = new wxStaticText(headerPanel, wxID_ANY, "Monster & Creature Creator");
	wxFont tFont = title->GetFont();
	tFont.SetPointSize(12);
	tFont.SetWeight(wxFONTWEIGHT_BOLD);
	title->SetFont(tFont);
	title->SetForegroundColour(wxColour(255, 215, 0));
	headerTopRow->Add(title, 1, wxALL | wxALIGN_CENTER_VERTICAL, 8);

	// View Mode Choice Switcher (Simple / Extended)
	wxStaticText* modeLbl = new wxStaticText(headerPanel, wxID_ANY, "View Mode:");
	modeLbl->SetForegroundColour(wxColour(220, 225, 235));
	headerTopRow->Add(modeLbl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);

	wxArrayString viewModes;
	viewModes.Add("Simple (Core Stats, Visual, Loot, Damage)");
	viewModes.Add("Extended (Full Attributes, Spells, Immunities, Scripting)");
	view_mode_choice = new wxChoice(headerPanel, ID_MON_VIEW_MODE, wxDefaultPosition, wxSize(290, -1), viewModes);
	view_mode_choice->SetSelection(0);
	headerTopRow->Add(view_mode_choice, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);

	headerSizer->Add(headerTopRow, 0, wxEXPAND);

	wxStaticText* sub = new wxStaticText(headerPanel, wxID_ANY, "Configure monsters and creatures with full visual preview, combat balance, and drop tables.");
	sub->SetForegroundColour(wxColour(190, 195, 205));
	headerSizer->Add(sub, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

	headerPanel->SetSizer(headerSizer);
	rootSizer->Add(headerPanel, 0, wxEXPAND);

	// =========================================================================
	// 1. SIMPLE VIEW CONTAINER
	// =========================================================================
	simple_container = new wxPanel(this, wxID_ANY);
	wxBoxSizer* simpleMainSizer = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer* simpleTopRow = new wxBoxSizer(wxHORIZONTAL);

	// Left: Visual Appearance (Sprite Preview & Model Picker)
	wxStaticBoxSizer* sVisualBox = new wxStaticBoxSizer(wxVERTICAL, simple_container, "Visual Appearance");
	simple_preview_panel = new CreaturePreviewPanel(simple_container, wxID_ANY, 140);
	sVisualBox->Add(simple_preview_panel, 0, wxALIGN_CENTER | wxALL, 6);

	wxBoxSizer* sVisBtnRow = new wxBoxSizer(wxHORIZONTAL);
	wxButton* sRotBtn = new wxButton(simple_container, wxID_ANY, "Rotate", wxDefaultPosition, wxSize(65, 24));
	sRotBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
		simple_preview_panel->RotateDirection();
		preview_panel->RotateDirection();
	});
	wxButton* sViewBtn = new wxButton(simple_container, wxID_ANY, "Front/Back", wxDefaultPosition, wxSize(80, 24));
	sViewBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
		simple_preview_panel->ToggleFrontBack();
		preview_panel->ToggleFrontBack();
	});
	sVisBtnRow->Add(sRotBtn, 0, wxRIGHT, 4);
	sVisBtnRow->Add(sViewBtn, 0);
	sVisualBox->Add(sVisBtnRow, 0, wxALIGN_CENTER | wxBOTTOM, 6);

	wxBoxSizer* sPresetRow = new wxBoxSizer(wxHORIZONTAL);
	sPresetRow->Add(new wxStaticText(simple_container, wxID_ANY, "Model:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	simple_preset_choice = new wxChoice(simple_container, ID_MON_SIMPLE_PRESET);
	sPresetRow->Add(simple_preset_choice, 1, wxEXPAND | wxRIGHT, 4);
	wxButton* sPickBtn = new wxButton(simple_container, wxID_ANY, "Select..", wxDefaultPosition, wxSize(65, -1));
	sPickBtn->Bind(wxEVT_BUTTON, &MonsterEditorDialog::OnPickMonsterFromPalette, this);
	sPresetRow->Add(sPickBtn, 0);
	sVisualBox->Add(sPresetRow, 0, wxEXPAND | wxALL, 4);

	simpleTopRow->Add(sVisualBox, 0, wxEXPAND | wxALL, 6);

	// Right: Core Stats & Damage (Simple View)
	wxStaticBoxSizer* sStatsBox = new wxStaticBoxSizer(wxVERTICAL, simple_container, "Core Stats & Combat Damage");
	wxFlexGridSizer* sStatsGrid = new wxFlexGridSizer(4, 2, 8, 12);
	sStatsGrid->AddGrowableCol(1, 1);

	sStatsGrid->Add(new wxStaticText(simple_container, wxID_ANY, "Monster Name:"), 0, wxALIGN_CENTER_VERTICAL);
	simple_name_ctrl = new wxTextCtrl(simple_container, wxID_ANY, "Demon Lord");
	sStatsGrid->Add(simple_name_ctrl, 1, wxEXPAND);

	sStatsGrid->Add(new wxStaticText(simple_container, wxID_ANY, "Health (HP):"), 0, wxALIGN_CENTER_VERTICAL);
	simple_health_ctrl = new wxSpinCtrl(simple_container, wxID_ANY, "8200", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 9999999, 8200);
	sStatsGrid->Add(simple_health_ctrl, 1, wxEXPAND);

	sStatsGrid->Add(new wxStaticText(simple_container, wxID_ANY, "Experience (EXP):"), 0, wxALIGN_CENTER_VERTICAL);
	simple_exp_ctrl = new wxSpinCtrl(simple_container, wxID_ANY, "6000", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 9999999, 6000);
	sStatsGrid->Add(simple_exp_ctrl, 1, wxEXPAND);

	sStatsGrid->Add(new wxStaticText(simple_container, wxID_ANY, "Speed:"), 0, wxALIGN_CENTER_VERTICAL);
	simple_speed_ctrl = new wxSpinCtrl(simple_container, wxID_ANY, "280", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 5000, 280);
	sStatsGrid->Add(simple_speed_ctrl, 1, wxEXPAND);

	sStatsGrid->Add(new wxStaticText(simple_container, wxID_ANY, "Damage (Min / Max):"), 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* sDmgRow = new wxBoxSizer(wxHORIZONTAL);
	simple_min_dmg = new wxSpinCtrl(simple_container, wxID_ANY, "150", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 99999, 150);
	simple_max_dmg = new wxSpinCtrl(simple_container, wxID_ANY, "450", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 99999, 450);
	sDmgRow->Add(simple_min_dmg, 1, wxEXPAND | wxRIGHT, 4);
	sDmgRow->Add(simple_max_dmg, 1, wxEXPAND);
	sStatsGrid->Add(sDmgRow, 1, wxEXPAND);

	sStatsBox->Add(sStatsGrid, 1, wxEXPAND | wxALL, 6);
	simpleTopRow->Add(sStatsBox, 1, wxEXPAND | wxALL, 6);

	simpleMainSizer->Add(simpleTopRow, 0, wxEXPAND);

	// Bottom: Loot Table (Simple View)
	wxStaticBoxSizer* sLootBox = new wxStaticBoxSizer(wxVERTICAL, simple_container, "Loot & Drop Table");
	simple_loot_list = new wxListView(simple_container, wxID_ANY, wxDefaultPosition, wxSize(-1, 140), wxLC_REPORT | wxLC_SINGLE_SEL);
	simple_loot_list->InsertColumn(0, "Item ID", wxLIST_FORMAT_RIGHT, 75);
	simple_loot_list->InsertColumn(1, "Item Name", wxLIST_FORMAT_LEFT, 200);
	simple_loot_list->InsertColumn(2, "Max Count", wxLIST_FORMAT_RIGHT, 90);
	simple_loot_list->InsertColumn(3, "Drop Chance (%)", wxLIST_FORMAT_RIGHT, 110);
	sLootBox->Add(simple_loot_list, 1, wxEXPAND | wxALL, 4);

	wxBoxSizer* sLootCtrlRow = new wxBoxSizer(wxHORIZONTAL);
	wxButton* sPickLootBtn = new wxButton(simple_container, ID_MON_SIMPLE_PICK_LOOT, "Select..");
	sPickLootBtn->SetBackgroundColour(wxColour(40, 70, 120));
	sPickLootBtn->SetForegroundColour(*wxWHITE);
	sLootCtrlRow->Add(sPickLootBtn, 0, wxRIGHT, 6);

	simple_loot_item_text = new wxStaticText(simple_container, wxID_ANY, "Item: crystal coin (ID 2160)");
	sLootCtrlRow->Add(simple_loot_item_text, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);

	sLootCtrlRow->Add(new wxStaticText(simple_container, wxID_ANY, "Count:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
	simple_loot_count = new wxSpinCtrl(simple_container, wxID_ANY, "5", wxDefaultPosition, wxSize(55, -1), wxSP_ARROW_KEYS, 1, 100, 5);
	sLootCtrlRow->Add(simple_loot_count, 0, wxRIGHT, 6);

	sLootCtrlRow->Add(new wxStaticText(simple_container, wxID_ANY, "Chance (%):"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
	simple_loot_chance = new wxSpinCtrl(simple_container, wxID_ANY, "25", wxDefaultPosition, wxSize(55, -1), wxSP_ARROW_KEYS, 1, 100, 25);
	sLootCtrlRow->Add(simple_loot_chance, 0, wxRIGHT, 6);

	wxButton* sAddLootBtn = new wxButton(simple_container, ID_MON_SIMPLE_ADD_LOOT, "+ Add Loot");
	sAddLootBtn->SetBackgroundColour(wxColour(40, 120, 60));
	sAddLootBtn->SetForegroundColour(*wxWHITE);
	sLootCtrlRow->Add(sAddLootBtn, 0, wxRIGHT, 4);

	wxButton* sRemLootBtn = new wxButton(simple_container, ID_MON_SIMPLE_REM_LOOT, "- Remove");
	sLootCtrlRow->Add(sRemLootBtn, 0);

	sLootBox->Add(sLootCtrlRow, 0, wxEXPAND | wxALL, 4);
	simpleMainSizer->Add(sLootBox, 1, wxEXPAND | wxALL, 6);

	simple_container->SetSizer(simpleMainSizer);
	rootSizer->Add(simple_container, 1, wxEXPAND | wxALL, 8);

	// =========================================================================
	// 2. EXTENDED VIEW NOTEBOOK
	// =========================================================================
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
	RME::UI::StyleManager::ApplyThemeRecursively(this, RME::UI::StyleManager::GetTheme());
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

		long sIdx = simple_loot_list->InsertItem(simple_loot_list->GetItemCount(), std::to_string(l.item_id));
		simple_loot_list->SetItem(sIdx, 1, l.name);
		simple_loot_list->SetItem(sIdx, 2, std::to_string(l.countmax));
		simple_loot_list->SetItem(sIdx, 3, wxString::Format("%.1f%%", l.chance / 1000.0f));
	}

	SetViewMode(VIEW_MODE_SIMPLE);
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

void MonsterEditorDialog::SetViewMode(EditorViewMode mode) {
	current_view_mode = mode;
	if (mode == VIEW_MODE_SIMPLE) {
		SyncExtendedToSimple();
		if (notebook) notebook->Hide();
		if (simple_container) simple_container->Show();
		if (view_mode_choice) view_mode_choice->SetSelection(0);
	} else {
		SyncSimpleToExtended();
		if (simple_container) simple_container->Hide();
		if (notebook) notebook->Show();
		if (view_mode_choice) view_mode_choice->SetSelection(1);
	}
	Layout();
}

void MonsterEditorDialog::OnViewModeChanged(wxCommandEvent& event) {
	int sel = event.GetSelection();
	SetViewMode(sel == 0 ? VIEW_MODE_SIMPLE : VIEW_MODE_EXTENDED);
}

void MonsterEditorDialog::SyncSimpleToExtended() {
	if (!simple_name_ctrl || !mon_name_ctrl) return;
	mon_name_ctrl->SetValue(simple_name_ctrl->GetValue());
	mon_description_ctrl->SetValue("a " + simple_name_ctrl->GetValue());
	mon_health_ctrl->SetValue(simple_health_ctrl->GetValue());
	mon_maxhealth_ctrl->SetValue(simple_health_ctrl->GetValue());
	mon_exp_ctrl->SetValue(simple_exp_ctrl->GetValue());
	mon_speed_ctrl->SetValue(simple_speed_ctrl->GetValue());

	if (!attacks.empty()) {
		attacks[0].min_damage = simple_min_dmg->GetValue();
		attacks[0].max_damage = simple_max_dmg->GetValue();
		if (attacks_list && attacks_list->GetItemCount() > 0) {
			attacks_list->SetItem(0, 1, std::to_string(attacks[0].min_damage));
			attacks_list->SetItem(0, 2, std::to_string(attacks[0].max_damage));
		}
	}
	UpdateOutfitPreview();
	UpdateLiveCode();
}

void MonsterEditorDialog::SyncExtendedToSimple() {
	if (!simple_name_ctrl || !mon_name_ctrl) return;
	simple_name_ctrl->SetValue(mon_name_ctrl->GetValue());
	simple_health_ctrl->SetValue(mon_health_ctrl->GetValue());
	simple_exp_ctrl->SetValue(mon_exp_ctrl->GetValue());
	simple_speed_ctrl->SetValue(mon_speed_ctrl->GetValue());

	if (!attacks.empty()) {
		simple_min_dmg->SetValue(attacks[0].min_damage);
		simple_max_dmg->SetValue(attacks[0].max_damage);
	}

	if (simple_preset_choice && outfit_preset_choice) {
		simple_preset_choice->SetSelection(outfit_preset_choice->GetSelection());
	}

	if (simple_loot_list && loot_list) {
		simple_loot_list->DeleteAllItems();
		for (const auto& l : loot_items) {
			long sIdx = simple_loot_list->InsertItem(simple_loot_list->GetItemCount(), std::to_string(l.item_id));
			simple_loot_list->SetItem(sIdx, 1, l.name);
			simple_loot_list->SetItem(sIdx, 2, std::to_string(l.countmax));
			simple_loot_list->SetItem(sIdx, 3, wxString::Format("%.1f%%", l.chance / 1000.0f));
		}
	}
	UpdateOutfitPreview();
}

void MonsterEditorDialog::PopulateMonsterPresets() {
	if (outfit_preset_choice) outfit_preset_choice->Clear();
	if (simple_preset_choice) simple_preset_choice->Clear();
	int demonIdx = 0;

	for (auto iter = g_creatures.begin(); iter != g_creatures.end(); ++iter) {
		CreatureType* ct = iter->second;
		if (!ct) continue;

		int itemIdx = -1;
		if (outfit_preset_choice) {
			itemIdx = outfit_preset_choice->Append(ct->name, (void*)ct);
		}
		if (simple_preset_choice) {
			simple_preset_choice->Append(ct->name, (void*)ct);
		}
		if (ct->name == "Demon" && itemIdx != -1) {
			demonIdx = itemIdx;
		}
	}

	if (outfit_preset_choice && outfit_preset_choice->GetCount() > 0) {
		outfit_preset_choice->SetSelection(demonIdx);
	}
	if (simple_preset_choice && simple_preset_choice->GetCount() > 0) {
		simple_preset_choice->SetSelection(demonIdx);
	}
}

void MonsterEditorDialog::LoadCreatureType(CreatureType* ct) {
	if (!ct) return;

	mon_name_ctrl->SetValue(ct->name);
	mon_description_ctrl->SetValue("a " + ct->name);
	if (simple_name_ctrl) simple_name_ctrl->SetValue(ct->name);

	look_type = ct->outfit.lookType;
	look_head = ct->outfit.lookHead;
	look_body = ct->outfit.lookBody;
	look_legs = ct->outfit.lookLegs;
	look_feet = ct->outfit.lookFeet;
	look_addons = ct->outfit.lookAddon;

	outfit_looktype_ctrl->SetValue(look_type);

	// Select in choice dropdown if present
	if (outfit_preset_choice) {
		for (unsigned int i = 0; i < outfit_preset_choice->GetCount(); ++i) {
			if (outfit_preset_choice->GetString(i) == ct->name) {
				outfit_preset_choice->SetSelection(i);
				if (simple_preset_choice) simple_preset_choice->SetSelection(i);
				break;
			}
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

void MonsterEditorDialog::OnLookTypePresetChanged(wxCommandEvent& event) {
	wxChoice* src = dynamic_cast<wxChoice*>(event.GetEventObject());
	int sel = src ? src->GetSelection() : wxNOT_FOUND;
	if (sel == wxNOT_FOUND) return;

	if (src == simple_preset_choice && outfit_preset_choice) {
		outfit_preset_choice->SetSelection(sel);
	} else if (src == outfit_preset_choice && simple_preset_choice) {
		simple_preset_choice->SetSelection(sel);
	}

	CreatureType* ct = (CreatureType*)src->GetClientData(sel);
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

	int col = 0;
	if (ch == CHANNEL_HEAD) col = look_head;
	else if (ch == CHANNEL_PRIMARY) col = look_body;
	else if (ch == CHANNEL_SECONDARY) col = look_legs;
	else if (ch == CHANNEL_DETAIL) col = look_feet;

	outfit_palette->SetSelectedColorId(col);
	Refresh();
}

void MonsterEditorDialog::OnPaletteSelected(wxCommandEvent& WXUNUSED(event)) {
	int col = outfit_palette->GetSelectedColorId();
	if (active_channel == CHANNEL_HEAD) look_head = col;
	else if (active_channel == CHANNEL_PRIMARY) look_body = col;
	else if (active_channel == CHANNEL_SECONDARY) look_legs = col;
	else if (active_channel == CHANNEL_DETAIL) look_feet = col;

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
	if (preview_panel) preview_panel->RotateDirection();
	if (simple_preview_panel) simple_preview_panel->RotateDirection();
}

void MonsterEditorDialog::OnToggleView(wxCommandEvent& WXUNUSED(event)) {
	if (preview_panel) preview_panel->ToggleFrontBack();
	if (simple_preview_panel) simple_preview_panel->ToggleFrontBack();
}

void MonsterEditorDialog::OnStepFrame(wxCommandEvent& WXUNUSED(event)) {
	if (preview_panel) preview_panel->StepFrame();
	if (simple_preview_panel) simple_preview_panel->StepFrame();
}

void MonsterEditorDialog::OnStepTimer(wxTimerEvent& WXUNUSED(event)) {
	if (preview_panel) preview_panel->StepFrame();
	if (simple_preview_panel) simple_preview_panel->StepFrame();
}

void MonsterEditorDialog::UpdateOutfitPreview() {
	if (preview_panel) {
		preview_panel->SetOutfit(look_type, look_head, look_body, look_legs, look_feet, look_addons, mount_type, false, current_direction, current_frame);
	}
	if (simple_preview_panel) {
		simple_preview_panel->SetOutfit(look_type, look_head, look_body, look_legs, look_feet, look_addons, mount_type, false, current_direction, current_frame);
	}
}

void MonsterEditorDialog::UpdateLiveCode() {
	if (!live_look_code_ctrl) return;
	std::ostringstream ss;
	ss << "<look type=\"" << look_type << "\" head=\"" << look_head << "\" body=\"" << look_body << "\" legs=\"" << look_legs << "\" feet=\"" << look_feet << "\" addons=\"" << look_addons << "\"/>";
	live_look_code_ctrl->SetValue(ss.str());
}

void MonsterEditorDialog::OnAddAttack(wxCommandEvent& WXUNUSED(event)) {
	MonsterAttackEntry atk;
	std::string t = attack_type_choice->GetStringSelection().ToStdString();
	if (t == "Custom") {
		atk.name = attack_custom_name->GetValue().ToStdString();
		atk.type = "combat";
	} else {
		atk.name = t;
		atk.type = t;
	}

	atk.min_damage = attack_min_dmg->GetValue();
	atk.max_damage = attack_max_dmg->GetValue();
	atk.interval = attack_interval->GetValue();
	atk.chance = attack_chance->GetValue();
	attacks.push_back(atk);

	long idx = attacks_list->InsertItem(attacks_list->GetItemCount(), atk.name);
	attacks_list->SetItem(idx, 1, std::to_string(atk.min_damage));
	attacks_list->SetItem(idx, 2, std::to_string(atk.max_damage));
	attacks_list->SetItem(idx, 3, std::to_string(atk.interval));
	attacks_list->SetItem(idx, 4, std::to_string(atk.chance));
}

void MonsterEditorDialog::OnRemoveAttack(wxCommandEvent& WXUNUSED(event)) {
	long sel = attacks_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (sel != -1 && sel < (long)attacks.size()) {
		attacks.erase(attacks.begin() + sel);
		attacks_list->DeleteItem(sel);
	}
}

void MonsterEditorDialog::OnPickLootItemFromPalette(wxCommandEvent& event) {
	FindItemDialog dlg(this, "Select Loot Item from Palette", true);
	if (dlg.ShowModal() == wxID_OK) {
		uint16_t id = dlg.getResultID();
		if (id > 0) {
			loot_selected_item_id = id;
			simple_loot_selected_id = id;
			ItemType& it = g_items[id];
			if (loot_selected_item_text) {
				loot_selected_item_text->SetLabel(wxString::Format("Item: %s (ID %d)", it.name, id));
			}
			if (simple_loot_item_text) {
				simple_loot_item_text->SetLabel(wxString::Format("Item: %s (ID %d)", it.name, id));
			}
		}
	}
}

void MonsterEditorDialog::OnAddLoot(wxCommandEvent& event) {
	bool fromSimple = (current_view_mode == VIEW_MODE_SIMPLE);
	int itemId = fromSimple ? simple_loot_selected_id : loot_selected_item_id;
	int count = fromSimple ? simple_loot_count->GetValue() : loot_countmax->GetValue();
	int chance = fromSimple ? (simple_loot_chance->GetValue() * 1000) : (loot_chance_percent->GetValue() * 1000);

	if (itemId <= 0) {
		wxMessageBox("Please select a valid item first.", "No Item Selected", wxOK | wxICON_WARNING, this);
		return;
	}

	ItemType& it = g_items[itemId];
	std::string name = it.name.empty() ? ("Item #" + std::to_string(itemId)) : it.name;

	MonsterLootEntry l{itemId, name, count, chance};
	loot_items.push_back(l);

	long idx = loot_list->InsertItem(loot_list->GetItemCount(), std::to_string(l.item_id));
	loot_list->SetItem(idx, 1, l.name);
	loot_list->SetItem(idx, 2, std::to_string(l.countmax));
	loot_list->SetItem(idx, 3, wxString::Format("%.1f%%", l.chance / 1000.0f));

	long sIdx = simple_loot_list->InsertItem(simple_loot_list->GetItemCount(), std::to_string(l.item_id));
	simple_loot_list->SetItem(sIdx, 1, l.name);
	simple_loot_list->SetItem(sIdx, 2, std::to_string(l.countmax));
	simple_loot_list->SetItem(sIdx, 3, wxString::Format("%.1f%%", l.chance / 1000.0f));
}

void MonsterEditorDialog::OnEditLoot(wxCommandEvent& WXUNUSED(event)) {
	long sel = loot_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (sel == -1 || sel >= (long)loot_items.size()) return;

	MonsterLootEntry& l = loot_items[sel];
	wxTextEntryDialog countDlg(this, "Enter Max Drop Count:", "Edit Loot Count", std::to_string(l.countmax));
	if (countDlg.ShowModal() != wxID_OK) return;
	long newCount = 1;
	countDlg.GetValue().ToLong(&newCount);

	wxTextEntryDialog chanceDlg(this, "Enter Drop Chance in Percent (1 - 100):", "Edit Loot Chance", std::to_string(l.chance / 1000));
	if (chanceDlg.ShowModal() != wxID_OK) return;
	long newChance = 10;
	chanceDlg.GetValue().ToLong(&newChance);

	l.countmax = std::max(1L, newCount);
	l.chance = std::clamp((int)(newChance * 1000), 1, 100000);

	loot_list->SetItem(sel, 2, std::to_string(l.countmax));
	loot_list->SetItem(sel, 3, wxString::Format("%.1f%%", l.chance / 1000.0f));
	SyncExtendedToSimple();
}

void MonsterEditorDialog::OnRemoveLoot(wxCommandEvent& WXUNUSED(event)) {
	wxListView* targetList = (current_view_mode == VIEW_MODE_SIMPLE) ? simple_loot_list : loot_list;
	long sel = targetList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (sel != -1 && sel < (long)loot_items.size()) {
		loot_items.erase(loot_items.begin() + sel);
		loot_list->DeleteItem(sel);
		simple_loot_list->DeleteItem(sel);
	}
}

std::string MonsterEditorDialog::GenerateTFSXml() const {
	std::string name = (current_view_mode == VIEW_MODE_SIMPLE && simple_name_ctrl) ? simple_name_ctrl->GetValue().ToStdString() : mon_name_ctrl->GetValue().ToStdString();
	std::string desc = (current_view_mode == VIEW_MODE_SIMPLE && simple_name_ctrl) ? ("a " + simple_name_ctrl->GetValue().ToStdString()) : mon_description_ctrl->GetValue().ToStdString();
	int hp = (current_view_mode == VIEW_MODE_SIMPLE && simple_health_ctrl) ? simple_health_ctrl->GetValue() : mon_health_ctrl->GetValue();
	int maxhp = (current_view_mode == VIEW_MODE_SIMPLE && simple_health_ctrl) ? simple_health_ctrl->GetValue() : mon_maxhealth_ctrl->GetValue();
	int exp = (current_view_mode == VIEW_MODE_SIMPLE && simple_exp_ctrl) ? simple_exp_ctrl->GetValue() : mon_exp_ctrl->GetValue();
	int spd = (current_view_mode == VIEW_MODE_SIMPLE && simple_speed_ctrl) ? simple_speed_ctrl->GetValue() : mon_speed_ctrl->GetValue();

	std::ostringstream ss;
	ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	ss << "<monster name=\"" << name << "\" nameDescription=\"" << desc << "\" race=\"" << mon_race_choice->GetStringSelection().ToStdString() << "\" experience=\"" << exp << "\" speed=\"" << spd << "\">\n";
	ss << "\t<health now=\"" << hp << "\" max=\"" << maxhp << "\"/>\n";

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
	if (current_view_mode == VIEW_MODE_SIMPLE) {
		SyncSimpleToExtended();
	}
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
	if (current_view_mode == VIEW_MODE_SIMPLE) {
		SyncSimpleToExtended();
	}
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
