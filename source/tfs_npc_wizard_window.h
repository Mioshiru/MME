#ifndef RME_TFS_NPC_WIZARD_WINDOW_H_
#define RME_TFS_NPC_WIZARD_WINDOW_H_

#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/spinctrl.h>
#include <wx/treectrl.h>
#include <wx/listctrl.h>
#include <vector>


struct ShopItemEntry {
	int id;
	std::string name;
	int buy_price;
	int sell_price;
};

class NPCWizardDialog : public wxDialog {
public:
	NPCWizardDialog(wxWindow* parent);
	virtual ~NPCWizardDialog() {}

private:
	void OnInteractTypeChanged(wxCommandEvent& event);
	void OnAddShopItem(wxCommandEvent& event);
	void OnRemoveShopItem(wxCommandEvent& event);
	void OnAddDialogueNode(wxCommandEvent& event);
	void OnRemoveDialogueNode(wxCommandEvent& event);
	void OnGenerate(wxCommandEvent& event);
	void OnClose(wxCommandEvent& event);

	void PopulateItemChoices(wxChoice* choice);
	void PopulateTownChoices(wxChoice* choice);

	wxNotebook* notebook;

	// 1. Quest NPC Controls
	wxTextCtrl* quest_npc_name;
	wxSpinCtrl* quest_id_ctrl;
	wxSpinCtrl* quest_value_ctrl;
	wxSpinCtrl* quest_reward_exp;
	wxSpinCtrl* quest_reward_gold;
	wxChoice* quest_reward_item_choice;
	wxSpinCtrl* quest_reward_count;
	wxTreeCtrl* quest_dialogue_tree;

	// 2. Interaction NPC Controls
	wxTextCtrl* interact_npc_name;
	wxChoice* interact_type_choice; // Ship, Shop, Healing, Temple
	wxPanel* ship_panel;
	wxPanel* shop_panel;
	wxPanel* heal_panel;
	wxPanel* temple_panel;

	// Ship controls
	wxChoice* ship_town_choice;
	wxSpinCtrl* ship_cost_ctrl;

	// Shop controls
	wxChoice* shop_item_choice;
	wxSpinCtrl* shop_buy_price;
	wxSpinCtrl* shop_sell_price;
	wxListView* shop_items_list;
	std::vector<ShopItemEntry> shop_items;

	// Heal controls
	wxSpinCtrl* heal_cost_ctrl;

	// Temple controls
	wxChoice* temple_town_choice;

	// 3. Information NPC Controls
	wxTextCtrl* info_npc_name;
	wxChoice* info_town_choice;
	wxTreeCtrl* info_dialogue_tree;

	DECLARE_EVENT_TABLE()
};

#endif // RME_TFS_NPC_WIZARD_WINDOW_H_

