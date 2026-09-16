#ifndef RME_TFS_NPC_WIZARD_WINDOW_H_
#define RME_TFS_NPC_WIZARD_WINDOW_H_

#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/spinctrl.h>
#include <wx/listctrl.h>
#include <vector>
#include <string>

struct ShopOffer {
	int id;
	std::string name;
	bool is_buy;
	int buy_price;
	bool is_sell;
	int sell_price;
};

struct TravelRoute {
	std::string town;
	int x, y, z;
	int cost;
	std::string text;
};

struct DialogueEntry {
	std::string keyword;
	std::string reply;
};

struct QuestItemEntry {
	int id;
	std::string name;
	int count;
};

struct QuestDefinition {
	std::string title;
	int storage_key;
	int storage_val;
	int required_level;
	std::vector<QuestItemEntry> required_items;
	int reward_exp;
	int reward_gold;
	std::vector<QuestItemEntry> reward_items;
	std::string offer_text;
	std::string in_progress_text;
	std::string complete_text;
	std::string quest_log_text;
};

enum OutfitChannel {
	CHANNEL_HEAD = 0,
	CHANNEL_PRIMARY = 1,
	CHANNEL_SECONDARY = 2,
	CHANNEL_DETAIL = 3
};

class TibiaPalettePanel : public wxPanel {
public:
	TibiaPalettePanel(wxWindow* parent, wxWindowID id = wxID_ANY);
	virtual ~TibiaPalettePanel() {}

	void SetSelectedColorId(int colorId);
	int GetSelectedColorId() const { return selected_color_id; }

private:
	void OnPaint(wxPaintEvent& event);
	void OnMouseDown(wxMouseEvent& event);

	int selected_color_id;

	DECLARE_EVENT_TABLE()
};

class CreaturePreviewPanel : public wxPanel {
public:
	CreaturePreviewPanel(wxWindow* parent, wxWindowID id = wxID_ANY, int size = 150);
	virtual ~CreaturePreviewPanel() {}

	void SetOutfit(int type, int head, int body, int legs, int feet, int addons, int mount = 0, bool mounted = false, int dir = 2, int frame = 0);
	void SetDirection(int dir);
	int GetDirection() const { return current_direction; }
	void RotateDirection();
	void RotateLeft();
	void ToggleFrontBack();
	void StepFrame();

private:
	void OnPaint(const wxPaintEvent& event);
	void OnEraseBackground(wxEraseEvent&) {}

	int look_type;
	int look_head;
	int look_body;
	int look_legs;
	int look_feet;
	int look_addons;
	int mount_type;
	bool has_mount;
	int current_direction; // 0 = North, 1 = East, 2 = South, 3 = West
	int current_frame;
};

class NPCWizardDialog : public wxDialog {
public:
	NPCWizardDialog(wxWindow* parent);
	virtual ~NPCWizardDialog();

private:
	void CreateIdentityTab(wxNotebook* parent);
	void CreateAppearanceTab(wxNotebook* parent);
	void CreateTradeTab(wxNotebook* parent);
	void CreateQuestsTab(wxNotebook* parent);
	void CreateTravelTab(wxNotebook* parent);
	void CreateDialogueTab(wxNotebook* parent);
	void CreateScriptTab(wxNotebook* parent);

	// TLG / Appearance Actions
	void OnPresetPrev(wxCommandEvent& event);
	void OnPresetNext(wxCommandEvent& event);
	void OnPresetChoice(wxCommandEvent& event);
	void OnGenderFilterChanged(wxCommandEvent& event);
	void RepopulatePresets();
	void OnOutfitParamChanged(wxCommandEvent& event);
	void OnOutfitSpinChanged(wxSpinEvent& event);
	void OnChannelSelect(wxCommandEvent& event);
	void OnPaletteColorSelected(wxCommandEvent& event);
	void OnRandomizeOutfit(wxCommandEvent& event);
	void OnCopyLooktypeCode(wxCommandEvent& event);
	void OnLoadLooktypeCode(wxCommandEvent& event);
	void OnRotateLeft(wxCommandEvent& event);
	void OnRotateRight(wxCommandEvent& event);
	void OnToggleFrontBack(wxCommandEvent& event);
	void OnToggleAnimate(wxCommandEvent& event);
	void OnStepTimer(wxTimerEvent& event);
	void UpdateOutfitPreview();
	void UpdateChannelButtonStyles();
	void UpdateColorPreviews();
	void UpdateLooktypeCodeBox();

	// Trade / Shop Actions
	void OnSelectShopItem(wxCommandEvent& event);
	void OnAddShopOffer(wxCommandEvent& event);
	void OnEditShopOffer(wxCommandEvent& event);
	void OnRemoveShopOffer(wxCommandEvent& event);

	// Quest Actions
	void OnQuestSelect(wxListEvent& event);
	void OnNewQuest(wxCommandEvent& event);
	void OnSaveQuest(wxCommandEvent& event);
	void OnDeleteQuest(wxCommandEvent& event);
	void OnSelectQuestReqItem(wxCommandEvent& event);
	void OnAddQuestReqItem(wxCommandEvent& event);
	void OnRemoveQuestReqItem(wxCommandEvent& event);
	void OnSelectQuestRewardItem(wxCommandEvent& event);
	void OnAddQuestRewardItem(wxCommandEvent& event);
	void OnRemoveQuestRewardItem(wxCommandEvent& event);
	void RefreshQuestsList();
	void LoadQuestDetails(int index);

	// Travel Actions
	void OnAddTravelRoute(wxCommandEvent& event);
	void OnRemoveTravelRoute(wxCommandEvent& event);

	// Dialogue Actions
	void OnAddDialogue(wxCommandEvent& event);
	void OnRemoveDialogue(wxCommandEvent& event);

	// Script & Export Actions
	void OnSaveFile(wxCommandEvent& event);
	void OnRegisterPalette(wxCommandEvent& event);
	void OnCopyLiveCode(wxCommandEvent& event);
	void OnRefreshScriptView(wxCommandEvent& event);
	void OnClose(wxCommandEvent& event);

	void PopulateItemChoices(wxChoice* choice);
	void PopulateTownChoices(wxChoice* choice);
	std::string BuildXml();
	std::string BuildLua();
	std::string BuildLiveCode();

	wxNotebook* notebook;

	// Identity Controls
	wxTextCtrl* npc_name_ctrl;
	wxTextCtrl* npc_title_ctrl;
	wxChoice* npc_role_choice;
	wxSpinCtrl* npc_walk_interval;
	wxSpinCtrl* npc_walk_range;
	wxSpinCtrl* npc_spawn_delay;
	wxTextCtrl* npc_greet_ctrl;
	wxTextCtrl* npc_farewell_ctrl;
	wxTextCtrl* npc_busy_ctrl;

	// TLG Controls (Sleqqus LookType Generator)
	CreaturePreviewPanel* tlg_preview_panel;
	wxChoice* tlg_gender_filter;
	wxChoice* tlg_preset_choice;
	wxSpinCtrl* tlg_looktype_spin;
	wxCheckBox* tlg_addon1_cb;
	wxCheckBox* tlg_addon2_cb;
	wxCheckBox* tlg_mount_cb;
	wxSpinCtrl* tlg_mount_spin;
	wxTextCtrl* tlg_code_ctrl;

	wxButton* tlg_btn_head;
	wxButton* tlg_btn_primary;
	wxButton* tlg_btn_secondary;
	wxButton* tlg_btn_detail;
	wxPanel* tlg_col_preview_head;
	wxPanel* tlg_col_preview_primary;
	wxPanel* tlg_col_preview_secondary;
	wxPanel* tlg_col_preview_detail;
	OutfitChannel active_channel;
	TibiaPalettePanel* tlg_palette;

	wxTimer* npc_step_timer;
	bool npc_is_stepping_loop;

	// Shopkeeper
	wxListView* shop_unified_list;
	wxStaticText* shop_item_label;
	int shop_selected_item_id;
	wxCheckBox* shop_chk_buy;
	wxSpinCtrl* shop_buy_price_spin;
	wxCheckBox* shop_chk_sell;
	wxSpinCtrl* shop_sell_price_spin;
	std::vector<ShopOffer> shop_offers;

	// Travel / Ship
	wxListView* ship_routes_list;
	wxChoice* ship_dest_choice;
	wxSpinCtrl* ship_pos_x_spin;
	wxSpinCtrl* ship_pos_y_spin;
	wxSpinCtrl* ship_pos_z_spin;
	wxSpinCtrl* ship_cost_spin;
	wxTextCtrl* ship_text_ctrl;
	std::vector<TravelRoute> travel_routes;

	// Quest Chains & Missions
	wxListView* quests_list;
	wxTextCtrl* quest_title_ctrl;
	wxSpinCtrl* quest_storage_spin;
	wxSpinCtrl* quest_val_spin;
	wxSpinCtrl* quest_req_lvl_spin;
	
	wxStaticText* quest_req_item_label;
	int quest_req_selected_item_id;
	wxSpinCtrl* quest_req_item_count_spin;
	wxListView* quest_req_items_list;
	std::vector<QuestItemEntry> cur_quest_req_items;

	wxSpinCtrl* quest_exp_spin;
	wxSpinCtrl* quest_gold_spin;
	wxStaticText* quest_reward_item_label;
	int quest_reward_selected_item_id;
	wxSpinCtrl* quest_reward_item_count_spin;
	wxListView* quest_rewards_list;
	std::vector<QuestItemEntry> cur_quest_reward_items;

	wxTextCtrl* quest_offer_text;
	wxTextCtrl* quest_prog_text;
	wxTextCtrl* quest_done_text;
	wxTextCtrl* quest_log_text;
	std::vector<QuestDefinition> quest_chains;
	int selected_quest_index;

	// Custom Dialogue
	wxListView* dialogue_list;
	wxTextCtrl* diag_kw_ctrl;
	wxTextCtrl* diag_reply_ctrl;
	std::vector<DialogueEntry> custom_dialogues;

	// Script & Preview
	wxTextCtrl* live_code_ctrl;

	int outfit_looktype;
	int outfit_head;
	int outfit_body;
	int outfit_legs;
	int outfit_feet;
	int outfit_addons;
	int outfit_mount;
	bool outfit_has_mount;

	DECLARE_EVENT_TABLE()
};

#endif // RME_TFS_NPC_WIZARD_WINDOW_H_
