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

struct QuestRewardItem {
	int id;
	std::string name;
	int count;
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
	CreaturePreviewPanel(wxWindow* parent, wxWindowID id = wxID_ANY, int size = 140);
	virtual ~CreaturePreviewPanel() {}

	void SetOutfit(int type, int head, int body, int legs, int feet, int addons, int mount = 0, bool mounted = false, int dir = 2, int frame = 0);
	void SetDirection(int dir);
	int GetDirection() const { return current_direction; }
	void RotateDirection();
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
	int current_direction; // 0 = North (Back), 1 = East, 2 = South (Front), 3 = West
	int current_frame;
};

class NPCWizardDialog : public wxDialog {
public:
	NPCWizardDialog(wxWindow* parent);
	virtual ~NPCWizardDialog();

private:
	void OnRoleChanged(wxCommandEvent& event);
	void OnPresetPrev(wxCommandEvent& event);
	void OnPresetNext(wxCommandEvent& event);
	void OnPresetChoice(wxCommandEvent& event);
	void OnOutfitParamChanged(wxCommandEvent& event);
	void OnChannelSelect(wxCommandEvent& event);
	void OnRandomizeColors(wxCommandEvent& event);
	void OnRotate(wxCommandEvent& event);
	void OnToggleView(wxCommandEvent& event);
	void OnAnimate(wxCommandEvent& event);
	void OnStepTimer(wxTimerEvent& event);
	void UpdateOutfitPreview();
	void UpdateChannelButtonStyles();
	void UpdateColorPreviews();

	// Gender filter
	void OnGenderFilterChanged(wxCommandEvent& event);
	void RepopulatePresets();

	// Shopkeeper actions (Unified table)
	void OnSelectShopItem(wxCommandEvent& event);
	void OnAddShopOffer(wxCommandEvent& event);
	void OnEditShopOffer(wxCommandEvent& event);
	void OnRemoveShopOffer(wxCommandEvent& event);

	// Ship Captain actions
	void OnAddTravelRoute(wxCommandEvent& event);
	void OnRemoveTravelRoute(wxCommandEvent& event);

	// Dialogue actions
	void OnAddDialogue(wxCommandEvent& event);
	void OnRemoveDialogue(wxCommandEvent& event);

	// Quest actions
	void OnSelectRewardItem(wxCommandEvent& event);
	void OnAddQuestRewardItem(wxCommandEvent& event);
	void OnRemoveQuestRewardItem(wxCommandEvent& event);

	// Generation & Save
	void OnSaveFile(wxCommandEvent& event);
	void OnRegisterPalette(wxCommandEvent& event);
	void OnCopyLiveCode(wxCommandEvent& event);
	void OnClose(wxCommandEvent& event);

	void PopulateItemChoices(wxChoice* choice);
	void PopulateTownChoices(wxChoice* choice);
	std::string BuildXml();
	std::string BuildLua();
	std::string BuildSrv();
	std::string BuildLiveCode();

	wxNotebook* notebook;

	// Identity Controls
	wxTextCtrl* npc_name_ctrl;
	wxTextCtrl* npc_title_ctrl;
	wxChoice* npc_role_choice;

	// TLG Controls (Sleqqus Style)
	CreaturePreviewPanel* tlg_preview_panel;
	wxButton* tlg_prev_btn;
	wxChoice* tlg_preset_choice;
	wxButton* tlg_next_btn;

	wxButton* tlg_btn_rotate;
	wxButton* tlg_btn_view;
	wxButton* tlg_btn_animate;
	wxButton* tlg_btn_random;

	// Channel Buttons (Head, Primary, Secondary, Detail)
	OutfitChannel active_channel;
	wxButton* tlg_btn_head;
	wxButton* tlg_btn_primary;
	wxButton* tlg_btn_secondary;
	wxButton* tlg_btn_detail;
	wxPanel* tlg_col_preview_head;
	wxPanel* tlg_col_preview_primary;
	wxPanel* tlg_col_preview_secondary;
	wxPanel* tlg_col_preview_detail;

	TibiaPalettePanel* tlg_palette;
	wxTextCtrl* live_code_ctrl;
	wxTimer* npc_step_timer;
	bool npc_is_stepping_loop;

	// Gender filter
	wxChoice* tlg_gender_filter;

	// Dynamic Role Panels
	wxPanel* role_container;
	wxBoxSizer* role_container_sizer;

	// Shopkeeper (Unified table)
	wxPanel* shop_panel;
	wxListView* shop_unified_list;
	wxStaticText* shop_item_label;
	int shop_selected_item_id;
	wxCheckBox* shop_chk_buy;
	wxSpinCtrl* shop_buy_price_spin;
	wxCheckBox* shop_chk_sell;
	wxSpinCtrl* shop_sell_price_spin;
	std::vector<ShopOffer> shop_offers;

	// Ship Captain
	wxPanel* ship_panel;
	wxListView* ship_routes_list;
	wxChoice* ship_dest_choice;
	wxSpinCtrl* ship_pos_x_spin;
	wxSpinCtrl* ship_pos_y_spin;
	wxSpinCtrl* ship_pos_z_spin;
	wxSpinCtrl* ship_cost_spin;
	std::vector<TravelRoute> travel_routes;

	// Temple Healer
	wxPanel* heal_panel;
	wxSpinCtrl* heal_percent_spin;
	wxSpinCtrl* heal_cost_spin;
	wxCheckBox* heal_cure_cb;
	wxTextCtrl* heal_msg_ctrl;

	// Town Resident
	wxPanel* resident_panel;
	wxChoice* resident_town_choice;
	wxSpinCtrl* resident_cost_spin;
	wxTextCtrl* resident_msg_ctrl;

	// Quest Giver
	wxPanel* quest_panel;
	wxSpinCtrl* quest_storage_spin;
	wxSpinCtrl* quest_val_spin;
	wxSpinCtrl* quest_exp_spin;
	wxSpinCtrl* quest_gold_spin;
	wxStaticText* quest_reward_item_label;
	int quest_reward_selected_item_id;
	wxSpinCtrl* quest_item_count_spin;
	wxListView* quest_rewards_list;
	wxTextCtrl* quest_offer_text;
	wxTextCtrl* quest_prog_text;
	wxTextCtrl* quest_done_text;
	std::vector<QuestRewardItem> quest_reward_items;

	// Custom Dialogue
	wxPanel* dialogue_panel;
	wxListView* dialogue_list;
	wxTextCtrl* diag_kw_ctrl;
	wxTextCtrl* diag_reply_ctrl;
	std::vector<DialogueEntry> custom_dialogues;

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
