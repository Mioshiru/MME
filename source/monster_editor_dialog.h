#ifndef RME_MONSTER_EDITOR_DIALOG_H_
#define RME_MONSTER_EDITOR_DIALOG_H_

#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/spinctrl.h>
#include <wx/listctrl.h>
#include <vector>
#include <string>

struct MonsterAttackEntry {
	std::string name;
	std::string type;
	int min_damage;
	int max_damage;
	int interval;
	int chance;
};

#include "tfs_npc_wizard_window.h"

enum EditorViewMode {
	VIEW_MODE_SIMPLE = 0,
	VIEW_MODE_EXTENDED = 1
};

class CreatureType;

struct MonsterLootEntry {
	int item_id;
	std::string name;
	int countmax;
	int chance; // Out of 100000 (100%)
};

class MonsterEditorDialog : public wxDialog {
public:
	MonsterEditorDialog(wxWindow* parent);
	virtual ~MonsterEditorDialog();

	void SetViewMode(EditorViewMode mode);
	EditorViewMode GetViewMode() const { return current_view_mode; }

private:
	EditorViewMode current_view_mode;

	// View Mode Switcher
	wxChoice* view_mode_choice;

	// Main containers
	wxPanel* simple_container;
	wxNotebook* notebook;

	// Simple View Controls
	wxTextCtrl* simple_name_ctrl;
	wxSpinCtrl* simple_health_ctrl;
	wxSpinCtrl* simple_exp_ctrl;
	wxSpinCtrl* simple_speed_ctrl;
	wxChoice* simple_preset_choice;
	wxSpinCtrl* simple_min_dmg;
	wxSpinCtrl* simple_max_dmg;
	class CreaturePreviewPanel* simple_preview_panel;
	wxListView* simple_loot_list;
	wxStaticText* simple_loot_item_text;
	int simple_loot_selected_id;
	wxSpinCtrl* simple_loot_count;
	wxSpinCtrl* simple_loot_chance;
	std::vector<MonsterLootEntry> simple_loot_items;

	// 1. General Tab Controls (Extended)
	wxTextCtrl* mon_name_ctrl;
	wxTextCtrl* mon_description_ctrl;
	wxSpinCtrl* mon_health_ctrl;
	wxSpinCtrl* mon_maxhealth_ctrl;
	wxSpinCtrl* mon_exp_ctrl;
	wxSpinCtrl* mon_speed_ctrl;
	wxSpinCtrl* mon_corpse_ctrl;
	wxSpinCtrl* mon_armor_ctrl;
	wxSpinCtrl* mon_defense_ctrl;
	wxChoice* mon_race_choice;
	wxSpinCtrl* mon_target_distance_ctrl;

	// 2. Look / Outfit Controls (Sleqqus TLG Style)
	wxChoice* outfit_preset_choice;
	wxSpinCtrl* outfit_looktype_ctrl;
	wxSpinCtrl* outfit_looktypeex_ctrl;

	wxButton* btn_channel_head;
	wxButton* btn_channel_primary;
	wxButton* btn_channel_secondary;
	wxButton* btn_channel_detail;
	OutfitChannel active_channel;

	class TibiaPalettePanel* outfit_palette;
	class CreaturePreviewPanel* preview_panel;
	wxTextCtrl* live_look_code_ctrl;
	wxButton* btn_step;
	wxTimer* step_timer;
	bool is_stepping_loop;

	int look_type;
	int look_head;
	int look_body;
	int look_legs;
	int look_feet;
	int look_addons;
	int mount_type;
	int current_direction;
	int current_frame;

	// 3. Attacks & Spells Controls (Extended)
	wxListView* attacks_list;
	wxChoice* attack_type_choice;
	wxTextCtrl* attack_custom_name;
	wxSpinCtrl* attack_min_dmg;
	wxSpinCtrl* attack_max_dmg;
	wxSpinCtrl* attack_interval;
	wxSpinCtrl* attack_chance;
	std::vector<MonsterAttackEntry> attacks;

	// 4. Defenses & Immunities Controls (Extended)
	wxSpinCtrl* def_heal_min;
	wxSpinCtrl* def_heal_max;
	wxSpinCtrl* def_heal_interval;
	wxSpinCtrl* def_heal_chance;
	wxCheckBox* def_immune_invisible;
	wxCheckBox* def_immune_paralyze;
	wxSpinCtrl* imm_fire;
	wxSpinCtrl* imm_earth;
	wxSpinCtrl* imm_energy;
	wxSpinCtrl* imm_ice;
	wxSpinCtrl* imm_holy;
	wxSpinCtrl* imm_death;
	wxSpinCtrl* imm_physical;

	// 5. Loot Table Controls (Extended)
	wxListView* loot_list;
	wxStaticText* loot_selected_item_text;
	int loot_selected_item_id;
	wxSpinCtrl* loot_countmax;
	wxSpinCtrl* loot_chance_percent;
	std::vector<MonsterLootEntry> loot_items;

	void SyncSimpleToExtended();
	void SyncExtendedToSimple();
	void OnViewModeChanged(wxCommandEvent& event);

	void PopulateMonsterPresets();
	void LoadCreatureType(CreatureType* ct);
	void OnPickMonsterFromPalette(wxCommandEvent& event);
	void OnLookTypePresetChanged(wxCommandEvent& event);
	void OnOutfitParamChanged(wxCommandEvent& event);
	void OnOutfitSpinChanged(wxSpinEvent& event);
	void OnChannelButtonClicked(OutfitChannel ch);
	void OnPaletteSelected(wxCommandEvent& event);
	void OnRandomizeColors(wxCommandEvent& event);
	void OnRotate(wxCommandEvent& event);
	void OnToggleView(wxCommandEvent& event);
	void OnStepFrame(wxCommandEvent& event);
	void OnStepTimer(wxTimerEvent& event);
	void UpdateOutfitPreview();
	void UpdateLiveCode();

	void OnAddAttack(wxCommandEvent& event);
	void OnRemoveAttack(wxCommandEvent& event);
	void OnPickLootItemFromPalette(wxCommandEvent& event);
	void OnAddLoot(wxCommandEvent& event);
	void OnEditLoot(wxCommandEvent& event);
	void OnRemoveLoot(wxCommandEvent& event);

	void OnSaveXmlFile(wxCommandEvent& event);
	void OnRegisterInPalette(wxCommandEvent& event);
	void OnClose(wxCommandEvent& event);

	std::string GenerateTFSXml() const;

	DECLARE_EVENT_TABLE()
};

#endif // RME_MONSTER_EDITOR_DIALOG_H_
