#include "tfs_npc_wizard_window.h"
#include "style_manager.h"
#include "find_item_window.h"
#include "gui.h"
#include "editor.h"
#include "items.h"
#include "creatures.h"
#include "graphics.h"
#include "materials.h"
#include "creature_brush.h"
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/dcclient.h>
#include <wx/wfstream.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/textdlg.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>

// Classic Tibia Colors Map (RGB lookup)
static const wxColour& GetTibiaColour(int colorId) {
	static const wxColour colors[] = {
		wxColour(255, 255, 255), wxColour(255, 212, 212), wxColour(255, 170, 170), wxColour(255, 128, 128), wxColour(255, 85, 85), wxColour(255, 42, 42), wxColour(255, 0, 0),
		wxColour(212, 42, 42), wxColour(170, 85, 85), wxColour(128, 128, 128), wxColour(85, 85, 85), wxColour(42, 42, 42), wxColour(0, 0, 0),
		wxColour(255, 234, 212), wxColour(255, 212, 170), wxColour(255, 191, 128), wxColour(255, 170, 85), wxColour(255, 149, 42), wxColour(255, 128, 0),
		wxColour(212, 106, 0), wxColour(170, 85, 0), wxColour(128, 64, 0), wxColour(85, 42, 0), wxColour(42, 21, 0),
		wxColour(255, 255, 212), wxColour(255, 255, 170), wxColour(255, 255, 128), wxColour(255, 255, 85), wxColour(255, 255, 42), wxColour(255, 255, 0),
		wxColour(212, 212, 0), wxColour(170, 170, 0), wxColour(128, 128, 0), wxColour(85, 85, 0), wxColour(42, 42, 0),
		wxColour(234, 255, 212), wxColour(212, 255, 170), wxColour(191, 255, 128), wxColour(170, 255, 85), wxColour(149, 255, 42), wxColour(128, 255, 0),
		wxColour(106, 212, 0), wxColour(85, 170, 0), wxColour(64, 128, 0), wxColour(42, 85, 0), wxColour(21, 42, 0),
		wxColour(212, 255, 212), wxColour(170, 255, 170), wxColour(128, 255, 128), wxColour(85, 255, 85), wxColour(42, 255, 42), wxColour(0, 255, 0),
		wxColour(0, 212, 0), wxColour(0, 170, 0), wxColour(0, 128, 0), wxColour(0, 85, 0), wxColour(0, 42, 0),
		wxColour(212, 255, 234), wxColour(170, 255, 212), wxColour(128, 255, 191), wxColour(85, 255, 170), wxColour(42, 255, 149), wxColour(0, 255, 128),
		wxColour(0, 212, 106), wxColour(0, 170, 85), wxColour(0, 128, 64), wxColour(0, 85, 42), wxColour(0, 42, 21),
		wxColour(212, 255, 255), wxColour(170, 255, 255), wxColour(128, 255, 255), wxColour(85, 255, 255), wxColour(42, 255, 255), wxColour(0, 255, 255),
		wxColour(0, 212, 212), wxColour(0, 170, 170), wxColour(0, 128, 128), wxColour(0, 85, 85), wxColour(0, 42, 42),
		wxColour(212, 234, 255), wxColour(170, 212, 255), wxColour(128, 191, 255), wxColour(85, 170, 255), wxColour(42, 149, 255), wxColour(0, 128, 255),
		wxColour(0, 106, 212), wxColour(0, 85, 170), wxColour(0, 64, 128), wxColour(0, 42, 85), wxColour(0, 21, 42),
		wxColour(212, 212, 255), wxColour(170, 170, 255), wxColour(128, 128, 255), wxColour(85, 85, 255), wxColour(42, 42, 255), wxColour(0, 0, 255),
		wxColour(0, 0, 212), wxColour(0, 0, 170), wxColour(0, 0, 128), wxColour(0, 0, 85), wxColour(0, 0, 42),
		wxColour(234, 212, 255), wxColour(212, 170, 255), wxColour(191, 128, 255), wxColour(170, 85, 255), wxColour(149, 42, 255), wxColour(128, 0, 255),
		wxColour(106, 0, 212), wxColour(85, 0, 170), wxColour(64, 0, 128), wxColour(42, 0, 85), wxColour(21, 0, 42),
		wxColour(255, 212, 255), wxColour(255, 170, 255), wxColour(255, 128, 255), wxColour(255, 85, 255), wxColour(255, 42, 255), wxColour(255, 0, 255),
		wxColour(212, 0, 212), wxColour(170, 0, 170), wxColour(128, 0, 128), wxColour(85, 0, 85), wxColour(42, 0, 42),
		wxColour(255, 212, 234), wxColour(255, 170, 212), wxColour(255, 128, 191), wxColour(255, 85, 170), wxColour(255, 42, 149), wxColour(255, 0, 128),
		wxColour(212, 0, 106), wxColour(170, 0, 85), wxColour(128, 0, 64), wxColour(85, 0, 42), wxColour(42, 0, 21)
	};
	static const size_t count = sizeof(colors) / sizeof(colors[0]);
	if (colorId >= 0 && static_cast<size_t>(colorId) < count) return colors[colorId];
	static wxColour def(0, 0, 0);
	return def;
}

// ----------------------------------------------------------------------------
// TibiaPalettePanel Implementation
// ----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(TibiaPalettePanel, wxPanel)
	EVT_PAINT(TibiaPalettePanel::OnPaint)
	EVT_LEFT_DOWN(TibiaPalettePanel::OnMouseDown)
END_EVENT_TABLE()

TibiaPalettePanel::TibiaPalettePanel(wxWindow* parent, wxWindowID id) :
	wxPanel(parent, id, wxDefaultPosition, wxSize(19 * 16 + 4, 7 * 16 + 4), wxBORDER_NONE),
	selected_color_id(0)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
}

void TibiaPalettePanel::SetSelectedColorId(int colorId) {
	selected_color_id = colorId;
	Refresh();
}

void TibiaPalettePanel::OnPaint(wxPaintEvent& WXUNUSED(event)) {
	wxPaintDC dc(this);
	dc.SetBackground(wxBrush(wxColour(18, 22, 30)));
	dc.Clear();

	int cellSize = 16;
	for (int i = 0; i < 133; ++i) {
		int row = i / 19;
		int col = i % 19;
		int x = 2 + col * cellSize;
		int y = 2 + row * cellSize;

		dc.SetBrush(wxBrush(GetTibiaColour(i)));
		if (i == selected_color_id) {
			dc.SetPen(wxPen(wxColour(255, 215, 0), 2)); // Gold selection highlight
		} else {
			dc.SetPen(wxPen(wxColour(12, 14, 18), 1));
		}
		dc.DrawRectangle(x, y, cellSize - 1, cellSize - 1);
	}
}

void TibiaPalettePanel::OnMouseDown(wxMouseEvent& event) {
	int cellSize = 16;
	int col = (event.GetX() - 2) / cellSize;
	int row = (event.GetY() - 2) / cellSize;

	if (col >= 0 && col < 19 && row >= 0 && row < 7) {
		int idx = row * 19 + col;
		if (idx >= 0 && idx < 133) {
			selected_color_id = idx;
			Refresh();

			wxCommandEvent evt(wxEVT_BUTTON, GetId());
			evt.SetInt(selected_color_id);
			ProcessWindowEvent(evt);
		}
	}
}

// ----------------------------------------------------------------------------
// CreaturePreviewPanel Implementation
// ----------------------------------------------------------------------------
CreaturePreviewPanel::CreaturePreviewPanel(wxWindow* parent, wxWindowID id, int size) :
	wxPanel(parent, id, wxDefaultPosition, wxSize(size, size), wxBORDER_NONE),
	look_type(138), look_head(0), look_body(0), look_legs(0), look_feet(0), look_addons(0),
	mount_type(0), has_mount(false), current_direction(2), current_frame(0)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	Bind(wxEVT_PAINT, &CreaturePreviewPanel::OnPaint, this);
}

void CreaturePreviewPanel::SetOutfit(int type, int head, int body, int legs, int feet, int addons, int mount, bool mounted, int dir, int frame) {
	look_type = type;
	look_head = head;
	look_body = body;
	look_legs = legs;
	look_feet = feet;
	look_addons = addons;
	mount_type = mount;
	has_mount = mounted;
	current_direction = dir;
	current_frame = frame;
	Refresh();
}

void CreaturePreviewPanel::SetDirection(int dir) {
	current_direction = (dir % 4);
	Refresh();
}

void CreaturePreviewPanel::RotateDirection() {
	current_direction = (current_direction + 1) % 4;
	Refresh();
}

void CreaturePreviewPanel::RotateLeft() {
	current_direction = (current_direction + 3) % 4;
	Refresh();
}

void CreaturePreviewPanel::ToggleFrontBack() {
	current_direction = (current_direction == 2 ? 0 : 2);
	Refresh();
}

void CreaturePreviewPanel::StepFrame() {
	current_frame = (current_frame + 1) % 3;
	Refresh();
}

void CreaturePreviewPanel::OnPaint(const wxPaintEvent& WXUNUSED(event)) {
	wxPaintDC dc(this);
	wxRect rect = GetClientRect();

	dc.SetBrush(wxBrush(wxColour(14, 18, 26)));
	dc.SetPen(wxPen(wxColour(14, 18, 26)));
	dc.DrawRectangle(rect);

	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.SetPen(wxPen(wxColour(229, 193, 88), 2));
	dc.DrawRectangle(rect);

	dc.SetPen(wxPen(wxColour(160, 130, 45, 180), 1));
	dc.DrawRectangle(wxRect(3, 3, rect.width - 6, rect.height - 6));

	dc.SetPen(wxPen(wxColour(28, 38, 52), 1));
	dc.DrawLine(rect.width / 2, 4, rect.width / 2, rect.height - 4);
	dc.DrawLine(4, rect.height / 2, rect.width - 4, rect.height / 2);

	Outfit outfit;
	outfit.lookType = look_type;
	outfit.lookHead = look_head;
	outfit.lookBody = look_body;
	outfit.lookLegs = look_legs;
	outfit.lookFeet = look_feet;
	outfit.lookAddon = look_addons;
	if (has_mount && mount_type > 0) {
		outfit.lookMount = mount_type;
	}

	GameSprite* spr = g_gui.gfx.getCreatureSprite(look_type);
	if (spr) {
		int zoom = (rect.width >= 160) ? 3 : 2;
		int sprW = 32 * zoom;
		int sprH = 32 * zoom;
		int px = (rect.width - sprW) / 2;
		int py = (rect.height - sprH) / 2;
		spr->DrawOutfitTo(&dc, outfit, px, py, sprW, sprH, current_direction, look_addons, 0, current_frame);
		return;
	}

	dc.SetTextForeground(wxColour(160, 170, 185));
	wxString fallback = wxString::Format("Outfit\nType %d", look_type);
	dc.DrawLabel(fallback, rect, wxALIGN_CENTER);
}

// ----------------------------------------------------------------------------
// Preset Data
// ----------------------------------------------------------------------------
struct TLGPreset {
	std::string name;
	int male_id;
	int female_id;
};

static const std::vector<TLGPreset> g_tlgPresets = {
	{ "Citizen", 128, 136 },
	{ "Hunter", 129, 137 },
	{ "Mage", 130, 138 },
	{ "Knight", 131, 139 },
	{ "Nobleman", 132, 140 },
	{ "Summoner", 133, 141 },
	{ "Warrior", 134, 142 },
	{ "Barbarian", 143, 147 },
	{ "Druid", 144, 148 },
	{ "Wizard", 145, 149 },
	{ "Oriental", 146, 150 },
	{ "Pirate", 151, 155 },
	{ "Assassin", 152, 156 },
	{ "Beggar", 153, 157 },
	{ "Shaman", 154, 158 },
	{ "Norseman", 251, 252 },
	{ "Nightmare", 268, 269 },
	{ "Jester", 270, 273 },
	{ "Brotherhood", 278, 279 },
	{ "Demon Hunter", 288, 289 },
	{ "Yalaharian", 324, 325 },
	{ "Warmaster", 335, 336 },
	{ "Wayfarer", 366, 367 },
	{ "Afflicted", 430, 431 },
	{ "Elementalist", 432, 433 }
};

enum NPCWizardCtrlIDs {
	ID_NPC_NOTEBOOK = 12000,
	ID_NPC_PRESET_PREV,
	ID_NPC_PRESET_NEXT,
	ID_NPC_PRESET_CHOICE,
	ID_NPC_GENDER_FILTER,
	ID_NPC_LOOKTYPE_SPIN,
	ID_NPC_ADDON1_CB,
	ID_NPC_ADDON2_CB,
	ID_NPC_MOUNT_CB,
	ID_NPC_MOUNT_SPIN,
	ID_NPC_CODE_CTRL,
	ID_NPC_COPY_CODE,
	ID_NPC_LOAD_CODE,
	ID_NPC_RANDOM_OUTFIT,
	ID_NPC_ROT_LEFT,
	ID_NPC_ROT_RIGHT,
	ID_NPC_FRONT_BACK,
	ID_NPC_ANIMATE,
	ID_NPC_BTN_HEAD,
	ID_NPC_BTN_PRIMARY,
	ID_NPC_BTN_SECONDARY,
	ID_NPC_BTN_DETAIL,
	ID_NPC_PALETTE,
	ID_NPC_TIMER_ANIM,

	// Trade
	ID_NPC_SHOP_SELECT_ITEM,
	ID_NPC_SHOP_ADD_OFFER,
	ID_NPC_SHOP_EDIT_OFFER,
	ID_NPC_SHOP_REMOVE_OFFER,

	// Quests
	ID_NPC_QUESTS_LIST,
	ID_NPC_QUEST_NEW,
	ID_NPC_QUEST_SAVE,
	ID_NPC_QUEST_DELETE,
	ID_NPC_QUEST_SELECT_REQ_ITEM,
	ID_NPC_QUEST_ADD_REQ_ITEM,
	ID_NPC_QUEST_REMOVE_REQ_ITEM,
	ID_NPC_QUEST_SELECT_REW_ITEM,
	ID_NPC_QUEST_ADD_REW_ITEM,
	ID_NPC_QUEST_REMOVE_REW_ITEM,

	// Travel
	ID_NPC_TRAVEL_ADD,
	ID_NPC_TRAVEL_REMOVE,

	// Dialogue
	ID_NPC_DIAG_ADD,
	ID_NPC_DIAG_REMOVE,

	// Export
	ID_NPC_SAVE_FILE,
	ID_NPC_REGISTER_PALETTE,
	ID_NPC_COPY_LIVE_CODE,
	ID_NPC_REFRESH_SCRIPT
};

BEGIN_EVENT_TABLE(NPCWizardDialog, wxDialog)
	EVT_BUTTON(ID_NPC_PRESET_PREV, NPCWizardDialog::OnPresetPrev)
	EVT_BUTTON(ID_NPC_PRESET_NEXT, NPCWizardDialog::OnPresetNext)
	EVT_CHOICE(ID_NPC_PRESET_CHOICE, NPCWizardDialog::OnPresetChoice)
	EVT_CHOICE(ID_NPC_GENDER_FILTER, NPCWizardDialog::OnGenderFilterChanged)
	EVT_SPINCTRL(ID_NPC_LOOKTYPE_SPIN, NPCWizardDialog::OnOutfitSpinChanged)
	EVT_CHECKBOX(ID_NPC_ADDON1_CB, NPCWizardDialog::OnOutfitParamChanged)
	EVT_CHECKBOX(ID_NPC_ADDON2_CB, NPCWizardDialog::OnOutfitParamChanged)
	EVT_CHECKBOX(ID_NPC_MOUNT_CB, NPCWizardDialog::OnOutfitParamChanged)
	EVT_SPINCTRL(ID_NPC_MOUNT_SPIN, NPCWizardDialog::OnOutfitSpinChanged)
	EVT_BUTTON(ID_NPC_COPY_CODE, NPCWizardDialog::OnCopyLooktypeCode)
	EVT_BUTTON(ID_NPC_LOAD_CODE, NPCWizardDialog::OnLoadLooktypeCode)
	EVT_TEXT_ENTER(ID_NPC_CODE_CTRL, NPCWizardDialog::OnLoadLooktypeCode)
	EVT_BUTTON(ID_NPC_RANDOM_OUTFIT, NPCWizardDialog::OnRandomizeOutfit)

	EVT_BUTTON(ID_NPC_ROT_LEFT, NPCWizardDialog::OnRotateLeft)
	EVT_BUTTON(ID_NPC_ROT_RIGHT, NPCWizardDialog::OnRotateRight)
	EVT_BUTTON(ID_NPC_FRONT_BACK, NPCWizardDialog::OnToggleFrontBack)
	EVT_BUTTON(ID_NPC_ANIMATE, NPCWizardDialog::OnToggleAnimate)
	EVT_TIMER(ID_NPC_TIMER_ANIM, NPCWizardDialog::OnStepTimer)

	EVT_BUTTON(ID_NPC_BTN_HEAD, NPCWizardDialog::OnChannelSelect)
	EVT_BUTTON(ID_NPC_BTN_PRIMARY, NPCWizardDialog::OnChannelSelect)
	EVT_BUTTON(ID_NPC_BTN_SECONDARY, NPCWizardDialog::OnChannelSelect)
	EVT_BUTTON(ID_NPC_BTN_DETAIL, NPCWizardDialog::OnChannelSelect)
	EVT_BUTTON(ID_NPC_PALETTE, NPCWizardDialog::OnPaletteColorSelected)

	// Trade
	EVT_BUTTON(ID_NPC_SHOP_SELECT_ITEM, NPCWizardDialog::OnSelectShopItem)
	EVT_BUTTON(ID_NPC_SHOP_ADD_OFFER, NPCWizardDialog::OnAddShopOffer)
	EVT_BUTTON(ID_NPC_SHOP_EDIT_OFFER, NPCWizardDialog::OnEditShopOffer)
	EVT_BUTTON(ID_NPC_SHOP_REMOVE_OFFER, NPCWizardDialog::OnRemoveShopOffer)

	// Quests
	EVT_LIST_ITEM_SELECTED(ID_NPC_QUESTS_LIST, NPCWizardDialog::OnQuestSelect)
	EVT_BUTTON(ID_NPC_QUEST_NEW, NPCWizardDialog::OnNewQuest)
	EVT_BUTTON(ID_NPC_QUEST_SAVE, NPCWizardDialog::OnSaveQuest)
	EVT_BUTTON(ID_NPC_QUEST_DELETE, NPCWizardDialog::OnDeleteQuest)
	EVT_BUTTON(ID_NPC_QUEST_SELECT_REQ_ITEM, NPCWizardDialog::OnSelectQuestReqItem)
	EVT_BUTTON(ID_NPC_QUEST_ADD_REQ_ITEM, NPCWizardDialog::OnAddQuestReqItem)
	EVT_BUTTON(ID_NPC_QUEST_REMOVE_REQ_ITEM, NPCWizardDialog::OnRemoveQuestReqItem)
	EVT_BUTTON(ID_NPC_QUEST_SELECT_REW_ITEM, NPCWizardDialog::OnSelectQuestRewardItem)
	EVT_BUTTON(ID_NPC_QUEST_ADD_REW_ITEM, NPCWizardDialog::OnAddQuestRewardItem)
	EVT_BUTTON(ID_NPC_QUEST_REMOVE_REW_ITEM, NPCWizardDialog::OnRemoveQuestRewardItem)

	// Travel
	EVT_BUTTON(ID_NPC_TRAVEL_ADD, NPCWizardDialog::OnAddTravelRoute)
	EVT_BUTTON(ID_NPC_TRAVEL_REMOVE, NPCWizardDialog::OnRemoveTravelRoute)

	// Dialogue
	EVT_BUTTON(ID_NPC_DIAG_ADD, NPCWizardDialog::OnAddDialogue)
	EVT_BUTTON(ID_NPC_DIAG_REMOVE, NPCWizardDialog::OnRemoveDialogue)

	// Script
	EVT_BUTTON(ID_NPC_SAVE_FILE, NPCWizardDialog::OnSaveFile)
	EVT_BUTTON(ID_NPC_REGISTER_PALETTE, NPCWizardDialog::OnRegisterPalette)
	EVT_BUTTON(ID_NPC_COPY_LIVE_CODE, NPCWizardDialog::OnCopyLiveCode)
	EVT_BUTTON(ID_NPC_REFRESH_SCRIPT, NPCWizardDialog::OnRefreshScriptView)
	EVT_BUTTON(wxID_CANCEL, NPCWizardDialog::OnClose)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// NPCWizardDialog Constructor
// ----------------------------------------------------------------------------
NPCWizardDialog::NPCWizardDialog(wxWindow* parent) :
	wxDialog(parent, wxID_ANY, "TFS NPC & Dialogue Studio (Classic Full View)", wxDefaultPosition, wxSize(980, 720), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	active_channel(CHANNEL_HEAD),
	npc_is_stepping_loop(false),
	shop_selected_item_id(2160),
	quest_req_selected_item_id(2120),
	quest_reward_selected_item_id(2160),
	selected_quest_index(-1),
	outfit_looktype(137), // Classic default (Hunter Female: #137:78:69:58:76:0:0)
	outfit_head(78),
	outfit_body(69),
	outfit_legs(58),
	outfit_feet(76),
	outfit_addons(0),
	outfit_mount(0),
	outfit_has_mount(false)
{
	npc_step_timer = new wxTimer(this, ID_NPC_TIMER_ANIM);

	wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);

	// Header banner with Gold Border
	wxPanel* header_panel = new wxPanel(this, wxID_ANY);
	wxBoxSizer* header_sizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* header_title = new wxStaticText(header_panel, wxID_ANY, "TFS NPC & Dialogue Studio");
	header_title->SetFont(wxFont(13, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, "Arial"));
	header_title->SetForegroundColour(wxColour(229, 193, 88));
	header_sizer->Add(header_title, 0, wxALIGN_CENTER_VERTICAL | wxALL, 6);

	wxStaticText* header_subtitle = new wxStaticText(header_panel, wxID_ANY, "- Design NPCs, Outfits (TLG), Dialogues, Shops, and Quest Chains");
	header_subtitle->SetForegroundColour(wxColour(170, 180, 195));
	header_sizer->Add(header_subtitle, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
	header_panel->SetSizer(header_sizer);
	main_sizer->Add(header_panel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);

	// Notebook with all classic feature tabs
	notebook = new wxNotebook(this, ID_NPC_NOTEBOOK);
	CreateIdentityTab(notebook);
	CreateAppearanceTab(notebook);
	CreateTradeTab(notebook);
	CreateQuestsTab(notebook);
	CreateTravelTab(notebook);
	CreateDialogueTab(notebook);
	CreateScriptTab(notebook);
	main_sizer->Add(notebook, 1, wxEXPAND | wxALL, 8);

	// Bottom Action Bar
	wxBoxSizer* bottom_sizer = new wxBoxSizer(wxHORIZONTAL);

	wxButton* btn_palette = new wxButton(this, ID_NPC_REGISTER_PALETTE, "Add to Creature Palette (Spawn on Map)");
	btn_palette->SetBackgroundColour(wxColour(180, 140, 40));
	btn_palette->SetForegroundColour(wxColour(255, 255, 255));
	btn_palette->SetMinSize(wxSize(260, 32));
	bottom_sizer->Add(btn_palette, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	wxButton* btn_save = new wxButton(this, ID_NPC_SAVE_FILE, "Export Script / XML...");
	btn_save->SetMinSize(wxSize(160, 32));
	bottom_sizer->Add(btn_save, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	bottom_sizer->AddStretchSpacer(1);

	wxButton* btn_close = new wxButton(this, wxID_CANCEL, "Close");
	btn_close->SetMinSize(wxSize(100, 32));
	bottom_sizer->Add(btn_close, 0, wxALIGN_CENTER_VERTICAL);

	main_sizer->Add(bottom_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
	SetSizer(main_sizer);

	// Initial state setup
	UpdateOutfitPreview();
	UpdateChannelButtonStyles();
	UpdateColorPreviews();
	UpdateLooktypeCodeBox();

	// Load first quest demo data
	QuestDefinition q1;
	q1.title = "The Lost Shovel";
	q1.storage_key = 10050;
	q1.storage_val = 1;
	q1.required_level = 8;
	q1.required_items.push_back({ 2554, "shovel", 1 });
	q1.reward_exp = 500;
	q1.reward_gold = 100;
	q1.reward_items.push_back({ 2120, "rope", 1 });
	q1.offer_text = "Greetings! I lost my sturdy shovel in the rat caves. Could you retrieve it for me?";
	q1.in_progress_text = "Have you found my shovel yet? Search the deep caverns!";
	q1.complete_text = "Thank you so much! Here is your reward.";
	q1.quest_log_text = "Help find the lost shovel in the caverns.";
	quest_chains.push_back(q1);
	RefreshQuestsList();
	LoadQuestDetails(0);

	// Default shop data
	shop_offers.push_back({ 2120, "rope", true, 50, false, 0 });
	shop_offers.push_back({ 2554, "shovel", true, 50, false, 0 });
	shop_offers.push_back({ 2050, "torch", true, 8, false, 0 });
	shop_offers.push_back({ 2148, "gold coin", false, 0, true, 1 });
	shop_offers.push_back({ 2152, "platinum coin", true, 100, true, 100 });

	for (const auto& off : shop_offers) {
		long idx = shop_unified_list->InsertItem(shop_unified_list->GetItemCount(), std::to_string(off.id));
		shop_unified_list->SetItem(idx, 1, off.name);
		std::string modeStr = (off.is_buy && off.is_sell) ? "Buy & Sell" : (off.is_buy ? "Buy Only" : "Sell Only");
		shop_unified_list->SetItem(idx, 2, modeStr);
		shop_unified_list->SetItem(idx, 3, off.is_buy ? std::to_string(off.buy_price) : "-");
		shop_unified_list->SetItem(idx, 4, off.is_sell ? std::to_string(off.sell_price) : "-");
	}

	RME::UI::StyleManager::ApplyThemeRecursively(this, RME::UI::StyleManager::GetTheme());
	CentreOnParent();
}

NPCWizardDialog::~NPCWizardDialog() {
	if (npc_step_timer) {
		npc_step_timer->Stop();
		delete npc_step_timer;
	}
}

// ----------------------------------------------------------------------------
// Tab 1: Identity & General
// ----------------------------------------------------------------------------
void NPCWizardDialog::CreateIdentityTab(wxNotebook* parent) {
	wxPanel* panel = new wxPanel(parent, wxID_ANY);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	// Name and Title
	wxStaticBoxSizer* id_box = new wxStaticBoxSizer(wxVERTICAL, panel, "NPC Identity");
	wxFlexGridSizer* fg = new wxFlexGridSizer(2, 5, 10);
	fg->AddGrowableCol(1, 1);

	fg->Add(new wxStaticText(panel, wxID_ANY, "NPC Name:"), 0, wxALIGN_CENTER_VERTICAL);
	npc_name_ctrl = new wxTextCtrl(panel, wxID_ANY, "Captain Jack");
	fg->Add(npc_name_ctrl, 1, wxEXPAND);

	fg->Add(new wxStaticText(panel, wxID_ANY, "Title / Job:"), 0, wxALIGN_CENTER_VERTICAL);
	npc_title_ctrl = new wxTextCtrl(panel, wxID_ANY, "Ship Captain & Explorer");
	fg->Add(npc_title_ctrl, 1, wxEXPAND);

	fg->Add(new wxStaticText(panel, wxID_ANY, "Role Template:"), 0, wxALIGN_CENTER_VERTICAL);
	wxArrayString roles;
	roles.Add("Shopkeeper (Buy / Sell items)");
	roles.Add("Ship Captain (Travel destinations)");
	roles.Add("Temple Healer (Heal & Cure conditions)");
	roles.Add("Quest Giver (Multi-step Quest Chains)");
	roles.Add("Banker (Deposit & Withdraw)");
	roles.Add("Citizen / Custom Interactive Dialogue");
	npc_role_choice = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, roles);
	npc_role_choice->SetSelection(1);
	fg->Add(npc_role_choice, 1, wxEXPAND);

	id_box->Add(fg, 1, wxEXPAND | wxALL, 6);
	sizer->Add(id_box, 0, wxEXPAND | wxALL, 6);

	// Movement & Spawning
	wxStaticBoxSizer* move_box = new wxStaticBoxSizer(wxVERTICAL, panel, "Spawning & Movement Behavior");
	wxFlexGridSizer* mfg = new wxFlexGridSizer(2, 5, 10);
	mfg->AddGrowableCol(1, 1);

	mfg->Add(new wxStaticText(panel, wxID_ANY, "Walk Interval (ms):"), 0, wxALIGN_CENTER_VERTICAL);
	npc_walk_interval = new wxSpinCtrl(panel, wxID_ANY, "2000", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 500, 30000, 2000);
	mfg->Add(npc_walk_interval, 0, wxEXPAND);

	mfg->Add(new wxStaticText(panel, wxID_ANY, "Walk Radius / Range (sqm):"), 0, wxALIGN_CENTER_VERTICAL);
	npc_walk_range = new wxSpinCtrl(panel, wxID_ANY, "2", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 15, 2);
	mfg->Add(npc_walk_range, 0, wxEXPAND);

	mfg->Add(new wxStaticText(panel, wxID_ANY, "Spawn Delay (sec):"), 0, wxALIGN_CENTER_VERTICAL);
	npc_spawn_delay = new wxSpinCtrl(panel, wxID_ANY, "60", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 5, 3600, 60);
	mfg->Add(npc_spawn_delay, 0, wxEXPAND);

	move_box->Add(mfg, 1, wxEXPAND | wxALL, 6);
	sizer->Add(move_box, 0, wxEXPAND | wxALL, 6);

	// Default Speech
	wxStaticBoxSizer* speech_box = new wxStaticBoxSizer(wxVERTICAL, panel, "Standard Greetings & Dialogue");
	wxFlexGridSizer* sfg = new wxFlexGridSizer(2, 5, 10);
	sfg->AddGrowableCol(1, 1);

	sfg->Add(new wxStaticText(panel, wxID_ANY, "Greeting (Hi / Hello):"), 0, wxALIGN_CENTER_VERTICAL);
	npc_greet_ctrl = new wxTextCtrl(panel, wxID_ANY, "Ahoy, |PLAYERNAME|! Welcome aboard my ship.");
	sfg->Add(npc_greet_ctrl, 1, wxEXPAND);

	sfg->Add(new wxStaticText(panel, wxID_ANY, "Farewell (Bye):"), 0, wxALIGN_CENTER_VERTICAL);
	npc_farewell_ctrl = new wxTextCtrl(panel, wxID_ANY, "Fair winds and following seas, |PLAYERNAME|!");
	sfg->Add(npc_farewell_ctrl, 1, wxEXPAND);

	sfg->Add(new wxStaticText(panel, wxID_ANY, "Busy Message:"), 0, wxALIGN_CENTER_VERTICAL);
	npc_busy_ctrl = new wxTextCtrl(panel, wxID_ANY, "Hold on, |PLAYERNAME|! I'm speaking with someone else.");
	sfg->Add(npc_busy_ctrl, 1, wxEXPAND);

	speech_box->Add(sfg, 1, wxEXPAND | wxALL, 6);
	sizer->Add(speech_box, 1, wxEXPAND | wxALL, 6);

	panel->SetSizer(sizer);
	parent->AddPage(panel, "Identity & General");
}

// ----------------------------------------------------------------------------
// Tab 2: Appearance & Outfit (Full TLG LookType Generator)
// ----------------------------------------------------------------------------
void NPCWizardDialog::CreateAppearanceTab(wxNotebook* parent) {
	wxPanel* panel = new wxPanel(parent, wxID_ANY);
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);

	// Left: Live Preview Canvas & Rotation / Animation Controls
	wxBoxSizer* left_sizer = new wxBoxSizer(wxVERTICAL);
	wxStaticBoxSizer* prev_box = new wxStaticBoxSizer(wxVERTICAL, panel, "Visual Preview");

	tlg_preview_panel = new CreaturePreviewPanel(panel, wxID_ANY, 160);
	prev_box->Add(tlg_preview_panel, 0, wxALIGN_CENTER | wxALL, 6);

	// Rotation & Frame Controls
	wxGridSizer* rot_grid = new wxGridSizer(2, 2, 4, 4);
	wxButton* b_rot_l = new wxButton(panel, ID_NPC_ROT_LEFT, "Rotate Left");
	wxButton* b_rot_r = new wxButton(panel, ID_NPC_ROT_RIGHT, "Rotate Right");
	wxButton* b_front_back = new wxButton(panel, ID_NPC_FRONT_BACK, "Front / Back");
	wxButton* b_anim = new wxButton(panel, ID_NPC_ANIMATE, "Animate Walk");
	rot_grid->Add(b_rot_l, 1, wxEXPAND);
	rot_grid->Add(b_rot_r, 1, wxEXPAND);
	rot_grid->Add(b_front_back, 1, wxEXPAND);
	rot_grid->Add(b_anim, 1, wxEXPAND);
	prev_box->Add(rot_grid, 0, wxEXPAND | wxALL, 4);

	left_sizer->Add(prev_box, 1, wxEXPAND | wxALL, 6);
	sizer->Add(left_sizer, 0, wxEXPAND);

	// Right: Presets, LookType Code, 4 Channel Selectors, and 133-color Palette
	wxBoxSizer* right_sizer = new wxBoxSizer(wxVERTICAL);

	// LookType Code Box (e.g. #137:78:69:58:76:0:0)
	wxStaticBoxSizer* code_box = new wxStaticBoxSizer(wxHORIZONTAL, panel, "LookType Code (TLG Format: #type:head:body:legs:feet:addons:mount)");
	tlg_code_ctrl = new wxTextCtrl(panel, ID_NPC_CODE_CTRL, "#137:78:69:58:76:0:0", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	code_box->Add(tlg_code_ctrl, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

	wxButton* btn_copy_code = new wxButton(panel, ID_NPC_COPY_CODE, "Copy");
	wxButton* btn_load_code = new wxButton(panel, ID_NPC_LOAD_CODE, "Load");
	wxButton* btn_rand = new wxButton(panel, ID_NPC_RANDOM_OUTFIT, "Randomize");
	code_box->Add(btn_copy_code, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	code_box->Add(btn_load_code, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	code_box->Add(btn_rand, 0, wxALIGN_CENTER_VERTICAL);
	right_sizer->Add(code_box, 0, wxEXPAND | wxALL, 4);

	// Presets & Options
	wxStaticBoxSizer* preset_box = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Outfit Presets & LookType");
	wxArrayString genders;
	genders.Add("All Outfits");
	genders.Add("Male");
	genders.Add("Female");
	tlg_gender_filter = new wxChoice(panel, ID_NPC_GENDER_FILTER, wxDefaultPosition, wxDefaultSize, genders);
	tlg_gender_filter->SetSelection(0);
	preset_box->Add(tlg_gender_filter, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

	wxButton* btn_prev = new wxButton(panel, ID_NPC_PRESET_PREV, "<", wxDefaultPosition, wxSize(28, 26));
	preset_box->Add(btn_prev, 0, wxALIGN_CENTER_VERTICAL);

	wxArrayString presetList;
	for (const auto& p : g_tlgPresets) {
		presetList.Add(p.name);
	}
	tlg_preset_choice = new wxChoice(panel, ID_NPC_PRESET_CHOICE, wxDefaultPosition, wxDefaultSize, presetList);
	tlg_preset_choice->SetSelection(1); // Hunter
	preset_box->Add(tlg_preset_choice, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 4);

	wxButton* btn_next = new wxButton(panel, ID_NPC_PRESET_NEXT, ">", wxDefaultPosition, wxSize(28, 26));
	preset_box->Add(btn_next, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	preset_box->Add(new wxStaticText(panel, wxID_ANY, "LookType:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	tlg_looktype_spin = new wxSpinCtrl(panel, ID_NPC_LOOKTYPE_SPIN, "137", wxDefaultPosition, wxSize(70, -1), wxSP_ARROW_KEYS, 1, 2000, 137);
	preset_box->Add(tlg_looktype_spin, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	tlg_addon1_cb = new wxCheckBox(panel, ID_NPC_ADDON1_CB, "Addon 1");
	tlg_addon2_cb = new wxCheckBox(panel, ID_NPC_ADDON2_CB, "Addon 2");
	preset_box->Add(tlg_addon1_cb, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
	preset_box->Add(tlg_addon2_cb, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	tlg_mount_cb = new wxCheckBox(panel, ID_NPC_MOUNT_CB, "Mount:");
	tlg_mount_spin = new wxSpinCtrl(panel, ID_NPC_MOUNT_SPIN, "0", wxDefaultPosition, wxSize(65, -1), wxSP_ARROW_KEYS, 0, 2000, 0);
	preset_box->Add(tlg_mount_cb, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	preset_box->Add(tlg_mount_spin, 0, wxALIGN_CENTER_VERTICAL);

	right_sizer->Add(preset_box, 0, wxEXPAND | wxALL, 4);

	// Color Channels (Head, Primary, Secondary, Detail) and 133-color Tibia Palette
	wxStaticBoxSizer* col_box = new wxStaticBoxSizer(wxHORIZONTAL, panel, "133 Tibia Color Palette & Channel Selection");
	
	// Left: 4 Channel Buttons with live color swatches
	wxBoxSizer* chan_col_sizer = new wxBoxSizer(wxVERTICAL);

	auto makeChanRow = [&](const wxString& label, int btnId, wxPanel*& swatchPtr) -> wxBoxSizer* {
		wxBoxSizer* r = new wxBoxSizer(wxHORIZONTAL);
		wxButton* b = new wxButton(panel, btnId, label, wxDefaultPosition, wxSize(95, 26));
		swatchPtr = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxSize(26, 26), wxBORDER_SIMPLE);
		r->Add(b, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
		r->Add(swatchPtr, 0, wxALIGN_CENTER_VERTICAL);
		if (btnId == ID_NPC_BTN_HEAD) tlg_btn_head = b;
		else if (btnId == ID_NPC_BTN_PRIMARY) tlg_btn_primary = b;
		else if (btnId == ID_NPC_BTN_SECONDARY) tlg_btn_secondary = b;
		else if (btnId == ID_NPC_BTN_DETAIL) tlg_btn_detail = b;
		return r;
	};

	chan_col_sizer->Add(makeChanRow("Head", ID_NPC_BTN_HEAD, tlg_col_preview_head), 0, wxBOTTOM, 4);
	chan_col_sizer->Add(makeChanRow("Primary", ID_NPC_BTN_PRIMARY, tlg_col_preview_primary), 0, wxBOTTOM, 4);
	chan_col_sizer->Add(makeChanRow("Secondary", ID_NPC_BTN_SECONDARY, tlg_col_preview_secondary), 0, wxBOTTOM, 4);
	chan_col_sizer->Add(makeChanRow("Detail", ID_NPC_BTN_DETAIL, tlg_col_preview_detail), 0);

	col_box->Add(chan_col_sizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 6);

	// Right: Interactive 133-color Palette Matrix (19 cols x 7 rows)
	tlg_palette = new TibiaPalettePanel(panel, ID_NPC_PALETTE);
	tlg_palette->SetSelectedColorId(outfit_head);
	col_box->Add(tlg_palette, 0, wxALIGN_CENTER_VERTICAL | wxALL, 6);

	right_sizer->Add(col_box, 1, wxEXPAND | wxALL, 4);
	sizer->Add(right_sizer, 1, wxEXPAND);

	panel->SetSizer(sizer);
	parent->AddPage(panel, "Appearance & Outfit");
}

// ----------------------------------------------------------------------------
// Tab 3: Trade & Shop
// ----------------------------------------------------------------------------
void NPCWizardDialog::CreateTradeTab(wxNotebook* parent) {
	wxPanel* panel = new wxPanel(parent, wxID_ANY);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	// Offer list table
	shop_unified_list = new wxListView(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
	shop_unified_list->InsertColumn(0, "Item ID", wxLIST_FORMAT_LEFT, 70);
	shop_unified_list->InsertColumn(1, "Item Name", wxLIST_FORMAT_LEFT, 200);
	shop_unified_list->InsertColumn(2, "Mode", wxLIST_FORMAT_LEFT, 110);
	shop_unified_list->InsertColumn(3, "Buy Price (gp)", wxLIST_FORMAT_LEFT, 110);
	shop_unified_list->InsertColumn(4, "Sell Price (gp)", wxLIST_FORMAT_LEFT, 110);
	sizer->Add(shop_unified_list, 1, wxEXPAND | wxALL, 6);

	// Add/Edit Offer controls
	wxStaticBoxSizer* edit_box = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Add / Configure Shop Offer");

	wxButton* btn_pick = new wxButton(panel, ID_NPC_SHOP_SELECT_ITEM, "Select Item...");
	btn_pick->SetMinSize(wxSize(110, 28));
	edit_box->Add(btn_pick, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	shop_item_label = new wxStaticText(panel, wxID_ANY, "Selected: crystal coin (ID 2160)");
	shop_item_label->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, "Arial"));
	edit_box->Add(shop_item_label, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

	shop_chk_buy = new wxCheckBox(panel, wxID_ANY, "Buy (Player buys):");
	shop_chk_buy->SetValue(true);
	edit_box->Add(shop_chk_buy, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

	shop_buy_price_spin = new wxSpinCtrl(panel, wxID_ANY, "10000", wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, 0, 10000000, 10000);
	edit_box->Add(shop_buy_price_spin, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);

	shop_chk_sell = new wxCheckBox(panel, wxID_ANY, "Sell (Player sells):");
	shop_chk_sell->SetValue(true);
	edit_box->Add(shop_chk_sell, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

	shop_sell_price_spin = new wxSpinCtrl(panel, wxID_ANY, "10000", wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, 0, 10000000, 10000);
	edit_box->Add(shop_sell_price_spin, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

	wxButton* btn_add = new wxButton(panel, ID_NPC_SHOP_ADD_OFFER, "+ Add Offer");
	btn_add->SetBackgroundColour(wxColour(30, 90, 45));
	btn_add->SetForegroundColour(wxColour(255, 255, 255));
	btn_add->SetMinSize(wxSize(100, 28));
	edit_box->Add(btn_add, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);

	wxButton* btn_rem = new wxButton(panel, ID_NPC_SHOP_REMOVE_OFFER, "- Remove");
	btn_rem->SetMinSize(wxSize(85, 28));
	edit_box->Add(btn_rem, 0, wxALIGN_CENTER_VERTICAL);

	sizer->Add(edit_box, 0, wxEXPAND | wxALL, 6);
	panel->SetSizer(sizer);
	parent->AddPage(panel, "Trade & Shop");
}

// ----------------------------------------------------------------------------
// Tab 4: Quests & Missions (Complex Quest Chains & Multi-Rewards)
// ----------------------------------------------------------------------------
void NPCWizardDialog::CreateQuestsTab(wxNotebook* parent) {
	wxPanel* panel = new wxPanel(parent, wxID_ANY);
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);

	// Left: Quest Chain Overview
	wxBoxSizer* left_sizer = new wxBoxSizer(wxVERTICAL);
	wxStaticBoxSizer* qlist_box = new wxStaticBoxSizer(wxVERTICAL, panel, "Quest Chains / Missions");

	quests_list = new wxListView(panel, ID_NPC_QUESTS_LIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
	quests_list->InsertColumn(0, "#", wxLIST_FORMAT_LEFT, 30);
	quests_list->InsertColumn(1, "Quest Title", wxLIST_FORMAT_LEFT, 150);
	quests_list->InsertColumn(2, "Storage", wxLIST_FORMAT_LEFT, 65);
	quests_list->InsertColumn(3, "Req Lvl", wxLIST_FORMAT_LEFT, 60);
	qlist_box->Add(quests_list, 1, wxEXPAND | wxALL, 4);

	wxBoxSizer* q_btn_row = new wxBoxSizer(wxHORIZONTAL);
	wxButton* btn_new_q = new wxButton(panel, ID_NPC_QUEST_NEW, "+ New Quest");
	wxButton* btn_del_q = new wxButton(panel, ID_NPC_QUEST_DELETE, "- Delete Quest");
	q_btn_row->Add(btn_new_q, 1, wxEXPAND | wxRIGHT, 4);
	q_btn_row->Add(btn_del_q, 1, wxEXPAND);
	qlist_box->Add(q_btn_row, 0, wxEXPAND | wxALL, 4);

	left_sizer->Add(qlist_box, 1, wxEXPAND);
	sizer->Add(left_sizer, 0, wxEXPAND | wxALL, 4);

	// Right: Selected Quest Configuration
	wxBoxSizer* right_sizer = new wxBoxSizer(wxVERTICAL);
	wxStaticBoxSizer* detail_box = new wxStaticBoxSizer(wxVERTICAL, panel, "Mission Details, Requirements & Rewards");

	// Row 1: Quest Title, Storage Key, Value, Level
	wxFlexGridSizer* r1 = new wxFlexGridSizer(4, 5, 8);
	r1->AddGrowableCol(1, 1);

	r1->Add(new wxStaticText(panel, wxID_ANY, "Quest Title:"), 0, wxALIGN_CENTER_VERTICAL);
	quest_title_ctrl = new wxTextCtrl(panel, wxID_ANY, "The Lost Shovel");
	r1->Add(quest_title_ctrl, 1, wxEXPAND);

	r1->Add(new wxStaticText(panel, wxID_ANY, "Storage Key:"), 0, wxALIGN_CENTER_VERTICAL);
	quest_storage_spin = new wxSpinCtrl(panel, wxID_ANY, "10050", wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, 1, 999999, 10050);
	r1->Add(quest_storage_spin, 0, wxEXPAND);

	r1->Add(new wxStaticText(panel, wxID_ANY, "Storage Val:"), 0, wxALIGN_CENTER_VERTICAL);
	quest_val_spin = new wxSpinCtrl(panel, wxID_ANY, "1", wxDefaultPosition, wxSize(65, -1), wxSP_ARROW_KEYS, 0, 999, 1);
	r1->Add(quest_val_spin, 0, wxEXPAND);

	r1->Add(new wxStaticText(panel, wxID_ANY, "Required Lvl:"), 0, wxALIGN_CENTER_VERTICAL);
	quest_req_lvl_spin = new wxSpinCtrl(panel, wxID_ANY, "8", wxDefaultPosition, wxSize(65, -1), wxSP_ARROW_KEYS, 1, 1000, 8);
	r1->Add(quest_req_lvl_spin, 0, wxEXPAND);

	detail_box->Add(r1, 0, wxEXPAND | wxALL, 4);

	// Row 2: Two Lists (Required Items & Reward Items)
	wxBoxSizer* lists_sizer = new wxBoxSizer(wxHORIZONTAL);

	// Required Items
	wxStaticBoxSizer* req_box = new wxStaticBoxSizer(wxVERTICAL, panel, "Required Items (Player must bring)");
	quest_req_items_list = new wxListView(panel, wxID_ANY, wxDefaultPosition, wxSize(200, 90), wxLC_REPORT | wxLC_SINGLE_SEL);
	quest_req_items_list->InsertColumn(0, "ID", wxLIST_FORMAT_LEFT, 50);
	quest_req_items_list->InsertColumn(1, "Item Name", wxLIST_FORMAT_LEFT, 110);
	quest_req_items_list->InsertColumn(2, "Count", wxLIST_FORMAT_LEFT, 45);
	req_box->Add(quest_req_items_list, 1, wxEXPAND | wxALL, 2);

	wxBoxSizer* req_edit = new wxBoxSizer(wxHORIZONTAL);
	wxButton* b_pick_req = new wxButton(panel, ID_NPC_QUEST_SELECT_REQ_ITEM, "Pick...");
	quest_req_item_label = new wxStaticText(panel, wxID_ANY, "shovel (2554)");
	quest_req_item_count_spin = new wxSpinCtrl(panel, wxID_ANY, "1", wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, 1, 1000, 1);
	wxButton* b_add_req = new wxButton(panel, ID_NPC_QUEST_ADD_REQ_ITEM, "+");
	wxButton* b_rem_req = new wxButton(panel, ID_NPC_QUEST_REMOVE_REQ_ITEM, "-");
	req_edit->Add(b_pick_req, 0, wxRIGHT, 2);
	req_edit->Add(quest_req_item_label, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	req_edit->Add(quest_req_item_count_spin, 0, wxRIGHT, 2);
	req_edit->Add(b_add_req, 0, wxRIGHT, 2);
	req_edit->Add(b_rem_req, 0);
	req_box->Add(req_edit, 0, wxEXPAND | wxALL, 2);
	lists_sizer->Add(req_box, 1, wxEXPAND | wxRIGHT, 4);

	// Reward Items & Stats
	wxStaticBoxSizer* rew_box = new wxStaticBoxSizer(wxVERTICAL, panel, "Rewards (EXP, Gold & Items)");
	wxBoxSizer* rew_stats = new wxBoxSizer(wxHORIZONTAL);
	rew_stats->Add(new wxStaticText(panel, wxID_ANY, "EXP:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	quest_exp_spin = new wxSpinCtrl(panel, wxID_ANY, "500", wxDefaultPosition, wxSize(75, -1), wxSP_ARROW_KEYS, 0, 10000000, 500);
	rew_stats->Add(quest_exp_spin, 1, wxEXPAND | wxRIGHT, 8);
	rew_stats->Add(new wxStaticText(panel, wxID_ANY, "Gold:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	quest_gold_spin = new wxSpinCtrl(panel, wxID_ANY, "100", wxDefaultPosition, wxSize(75, -1), wxSP_ARROW_KEYS, 0, 10000000, 100);
	rew_stats->Add(quest_gold_spin, 1, wxEXPAND);
	rew_box->Add(rew_stats, 0, wxEXPAND | wxALL, 2);

	quest_rewards_list = new wxListView(panel, wxID_ANY, wxDefaultPosition, wxSize(200, 70), wxLC_REPORT | wxLC_SINGLE_SEL);
	quest_rewards_list->InsertColumn(0, "ID", wxLIST_FORMAT_LEFT, 50);
	quest_rewards_list->InsertColumn(1, "Item Name", wxLIST_FORMAT_LEFT, 110);
	quest_rewards_list->InsertColumn(2, "Count", wxLIST_FORMAT_LEFT, 45);
	rew_box->Add(quest_rewards_list, 1, wxEXPAND | wxALL, 2);

	wxBoxSizer* rew_edit = new wxBoxSizer(wxHORIZONTAL);
	wxButton* b_pick_rew = new wxButton(panel, ID_NPC_QUEST_SELECT_REW_ITEM, "Pick...");
	quest_reward_item_label = new wxStaticText(panel, wxID_ANY, "rope (2120)");
	quest_reward_item_count_spin = new wxSpinCtrl(panel, wxID_ANY, "1", wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, 1, 1000, 1);
	wxButton* b_add_rew = new wxButton(panel, ID_NPC_QUEST_ADD_REW_ITEM, "+");
	wxButton* b_rem_rew = new wxButton(panel, ID_NPC_QUEST_REMOVE_REW_ITEM, "-");
	rew_edit->Add(b_pick_rew, 0, wxRIGHT, 2);
	rew_edit->Add(quest_reward_item_label, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	rew_edit->Add(quest_reward_item_count_spin, 0, wxRIGHT, 2);
	rew_edit->Add(b_add_rew, 0, wxRIGHT, 2);
	rew_edit->Add(b_rem_rew, 0);
	rew_box->Add(rew_edit, 0, wxEXPAND | wxALL, 2);
	lists_sizer->Add(rew_box, 1, wxEXPAND);

	detail_box->Add(lists_sizer, 1, wxEXPAND | wxALL, 4);

	// Dialogue Texts
	wxFlexGridSizer* dfg = new wxFlexGridSizer(2, 4, 6);
	dfg->AddGrowableCol(1, 1);

	dfg->Add(new wxStaticText(panel, wxID_ANY, "Offer Speech:"), 0, wxALIGN_CENTER_VERTICAL);
	quest_offer_text = new wxTextCtrl(panel, wxID_ANY, "I lost my shovel in the caverns! Retrieve it for a handsome reward.");
	dfg->Add(quest_offer_text, 1, wxEXPAND);

	dfg->Add(new wxStaticText(panel, wxID_ANY, "In-Progress:"), 0, wxALIGN_CENTER_VERTICAL);
	quest_prog_text = new wxTextCtrl(panel, wxID_ANY, "Have you found my shovel yet?");
	dfg->Add(quest_prog_text, 1, wxEXPAND);

	dfg->Add(new wxStaticText(panel, wxID_ANY, "Completed:"), 0, wxALIGN_CENTER_VERTICAL);
	quest_done_text = new wxTextCtrl(panel, wxID_ANY, "Splendid! Here is your reward, well earned!");
	dfg->Add(quest_done_text, 1, wxEXPAND);

	dfg->Add(new wxStaticText(panel, wxID_ANY, "Quest Log:"), 0, wxALIGN_CENTER_VERTICAL);
	quest_log_text = new wxTextCtrl(panel, wxID_ANY, "Bring the lost shovel back to Captain Jack.");
	dfg->Add(quest_log_text, 1, wxEXPAND);

	detail_box->Add(dfg, 0, wxEXPAND | wxALL, 4);

	wxButton* btn_save_q = new wxButton(panel, ID_NPC_QUEST_SAVE, "Save / Update Quest in Chain");
	btn_save_q->SetBackgroundColour(wxColour(30, 90, 45));
	btn_save_q->SetForegroundColour(wxColour(255, 255, 255));
	btn_save_q->SetMinSize(wxSize(-1, 30));
	detail_box->Add(btn_save_q, 0, wxEXPAND | wxALL, 4);

	right_sizer->Add(detail_box, 1, wxEXPAND);
	sizer->Add(right_sizer, 1, wxEXPAND | wxALL, 4);

	panel->SetSizer(sizer);
	parent->AddPage(panel, "Quests & Missions");
}

// ----------------------------------------------------------------------------
// Tab 5: Travel & Routes
// ----------------------------------------------------------------------------
void NPCWizardDialog::CreateTravelTab(wxNotebook* parent) {
	wxPanel* panel = new wxPanel(parent, wxID_ANY);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	ship_routes_list = new wxListView(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
	ship_routes_list->InsertColumn(0, "Destination Town", wxLIST_FORMAT_LEFT, 150);
	ship_routes_list->InsertColumn(1, "X", wxLIST_FORMAT_LEFT, 60);
	ship_routes_list->InsertColumn(2, "Y", wxLIST_FORMAT_LEFT, 60);
	ship_routes_list->InsertColumn(3, "Z", wxLIST_FORMAT_LEFT, 45);
	ship_routes_list->InsertColumn(4, "Cost (gp)", wxLIST_FORMAT_LEFT, 80);
	ship_routes_list->InsertColumn(5, "Speech / Confirmation", wxLIST_FORMAT_LEFT, 260);
	sizer->Add(ship_routes_list, 1, wxEXPAND | wxALL, 6);

	wxStaticBoxSizer* add_box = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Add Travel Destination");
	ship_dest_choice = new wxChoice(panel, wxID_ANY);
	PopulateTownChoices(ship_dest_choice);
	add_box->Add(ship_dest_choice, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);

	add_box->Add(new wxStaticText(panel, wxID_ANY, "X:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
	ship_pos_x_spin = new wxSpinCtrl(panel, wxID_ANY, "1000", wxDefaultPosition, wxSize(65, -1), wxSP_ARROW_KEYS, 0, 65535, 1000);
	add_box->Add(ship_pos_x_spin, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);

	add_box->Add(new wxStaticText(panel, wxID_ANY, "Y:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
	ship_pos_y_spin = new wxSpinCtrl(panel, wxID_ANY, "1000", wxDefaultPosition, wxSize(65, -1), wxSP_ARROW_KEYS, 0, 65535, 1000);
	add_box->Add(ship_pos_y_spin, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);

	add_box->Add(new wxStaticText(panel, wxID_ANY, "Z:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
	ship_pos_z_spin = new wxSpinCtrl(panel, wxID_ANY, "7", wxDefaultPosition, wxSize(45, -1), wxSP_ARROW_KEYS, 0, 15, 7);
	add_box->Add(ship_pos_z_spin, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	add_box->Add(new wxStaticText(panel, wxID_ANY, "Cost:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
	ship_cost_spin = new wxSpinCtrl(panel, wxID_ANY, "110", wxDefaultPosition, wxSize(65, -1), wxSP_ARROW_KEYS, 0, 100000, 110);
	add_box->Add(ship_cost_spin, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	ship_text_ctrl = new wxTextCtrl(panel, wxID_ANY, "Set sail for Thais! Hold on tight!");
	add_box->Add(ship_text_ctrl, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);

	wxButton* btn_add = new wxButton(panel, ID_NPC_TRAVEL_ADD, "+ Add Route");
	btn_add->SetBackgroundColour(wxColour(30, 90, 45));
	btn_add->SetForegroundColour(wxColour(255, 255, 255));
	add_box->Add(btn_add, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

	wxButton* btn_rem = new wxButton(panel, ID_NPC_TRAVEL_REMOVE, "- Remove");
	add_box->Add(btn_rem, 0, wxALIGN_CENTER_VERTICAL);

	sizer->Add(add_box, 0, wxEXPAND | wxALL, 6);

	// Default routes
	travel_routes.push_back({ "Thais", 32369, 32241, 7, 110, "Set sail for Thais! Hold on tight!" });
	travel_routes.push_back({ "Carlin", 32387, 31820, 7, 110, "Welcome aboard to Carlin!" });
	travel_routes.push_back({ "Venore", 32954, 32022, 7, 130, "Travelling to Venore, watch your pockets!" });

	for (const auto& r : travel_routes) {
		long idx = ship_routes_list->InsertItem(ship_routes_list->GetItemCount(), r.town);
		ship_routes_list->SetItem(idx, 1, std::to_string(r.x));
		ship_routes_list->SetItem(idx, 2, std::to_string(r.y));
		ship_routes_list->SetItem(idx, 3, std::to_string(r.z));
		ship_routes_list->SetItem(idx, 4, std::to_string(r.cost));
		ship_routes_list->SetItem(idx, 5, r.text);
	}

	panel->SetSizer(sizer);
	parent->AddPage(panel, "Travel & Routes");
}

// ----------------------------------------------------------------------------
// Tab 6: Custom Dialogues
// ----------------------------------------------------------------------------
void NPCWizardDialog::CreateDialogueTab(wxNotebook* parent) {
	wxPanel* panel = new wxPanel(parent, wxID_ANY);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	dialogue_list = new wxListView(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
	dialogue_list->InsertColumn(0, "Keyword(s)", wxLIST_FORMAT_LEFT, 160);
	dialogue_list->InsertColumn(1, "NPC Reply / Answer", wxLIST_FORMAT_LEFT, 460);
	sizer->Add(dialogue_list, 1, wxEXPAND | wxALL, 6);

	wxStaticBoxSizer* add_box = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Add Custom Dialogue Reply");
	add_box->Add(new wxStaticText(panel, wxID_ANY, "Keyword:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	diag_kw_ctrl = new wxTextCtrl(panel, wxID_ANY, "job");
	diag_kw_ctrl->SetMinSize(wxSize(130, -1));
	add_box->Add(diag_kw_ctrl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	add_box->Add(new wxStaticText(panel, wxID_ANY, "Reply:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	diag_reply_ctrl = new wxTextCtrl(panel, wxID_ANY, "I am the captain of this ship. I sail between various lands.");
	add_box->Add(diag_reply_ctrl, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	wxButton* btn_add = new wxButton(panel, ID_NPC_DIAG_ADD, "+ Add Dialogue");
	btn_add->SetBackgroundColour(wxColour(30, 90, 45));
	btn_add->SetForegroundColour(wxColour(255, 255, 255));
	add_box->Add(btn_add, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

	wxButton* btn_rem = new wxButton(panel, ID_NPC_DIAG_REMOVE, "- Remove");
	add_box->Add(btn_rem, 0, wxALIGN_CENTER_VERTICAL);

	sizer->Add(add_box, 0, wxEXPAND | wxALL, 6);

	// Default dialogues
	custom_dialogues.push_back({ "job", "I am the captain of this vessel. I transport adventurers across the realm." });
	custom_dialogues.push_back({ "name", "My name is Jack, master of the high seas." });
	custom_dialogues.push_back({ "rumor", "They say fierce sea serpents lurk south of the ice isles." });

	for (const auto& d : custom_dialogues) {
		long idx = dialogue_list->InsertItem(dialogue_list->GetItemCount(), d.keyword);
		dialogue_list->SetItem(idx, 1, d.reply);
	}

	panel->SetSizer(sizer);
	parent->AddPage(panel, "Custom Dialogues");
}

// ----------------------------------------------------------------------------
// Tab 7: Script & XML Preview
// ----------------------------------------------------------------------------
void NPCWizardDialog::CreateScriptTab(wxNotebook* parent) {
	wxPanel* panel = new wxPanel(parent, wxID_ANY);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	live_code_ctrl = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxHSCROLL);
	live_code_ctrl->SetFont(wxFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	sizer->Add(live_code_ctrl, 1, wxEXPAND | wxALL, 6);

	wxBoxSizer* btn_row = new wxBoxSizer(wxHORIZONTAL);
	wxButton* btn_refresh = new wxButton(panel, ID_NPC_REFRESH_SCRIPT, "Refresh Code Preview");
	wxButton* btn_copy = new wxButton(panel, ID_NPC_COPY_LIVE_CODE, "Copy Code to Clipboard");
	btn_row->Add(btn_refresh, 0, wxRIGHT, 6);
	btn_row->Add(btn_copy, 0);
	sizer->Add(btn_row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

	panel->SetSizer(sizer);
	parent->AddPage(panel, "Script & XML Preview");
}

// ----------------------------------------------------------------------------
// TLG / LookType Generator Handlers
// ----------------------------------------------------------------------------
void NPCWizardDialog::OnPresetChoice(wxCommandEvent& WXUNUSED(event)) {
	int sel = tlg_preset_choice->GetSelection();
	if (sel >= 0 && sel < (int)g_tlgPresets.size()) {
		int gSel = tlg_gender_filter->GetSelection();
		bool female = (gSel == 2);
		outfit_looktype = female ? g_tlgPresets[sel].female_id : g_tlgPresets[sel].male_id;
		tlg_looktype_spin->SetValue(outfit_looktype);
		UpdateOutfitPreview();
		UpdateLooktypeCodeBox();
	}
}

void NPCWizardDialog::OnPresetPrev(wxCommandEvent& WXUNUSED(event)) {
	int sel = tlg_preset_choice->GetSelection();
	int count = tlg_preset_choice->GetCount();
	if (count > 0) {
		sel = (sel - 1 + count) % count;
		tlg_preset_choice->SetSelection(sel);
		wxCommandEvent evt;
		OnPresetChoice(evt);
	}
}

void NPCWizardDialog::OnPresetNext(wxCommandEvent& WXUNUSED(event)) {
	int sel = tlg_preset_choice->GetSelection();
	int count = tlg_preset_choice->GetCount();
	if (count > 0) {
		sel = (sel + 1) % count;
		tlg_preset_choice->SetSelection(sel);
		wxCommandEvent evt;
		OnPresetChoice(evt);
	}
}

void NPCWizardDialog::OnGenderFilterChanged(wxCommandEvent& WXUNUSED(event)) {
	RepopulatePresets();
}

void NPCWizardDialog::RepopulatePresets() {
	int sel = tlg_preset_choice->GetSelection();
	if (sel < 0) sel = 0;
	int gSel = tlg_gender_filter->GetSelection();
	bool female = (gSel == 2);
	if (sel < (int)g_tlgPresets.size()) {
		outfit_looktype = female ? g_tlgPresets[sel].female_id : g_tlgPresets[sel].male_id;
		tlg_looktype_spin->SetValue(outfit_looktype);
	}
	UpdateOutfitPreview();
	UpdateLooktypeCodeBox();
}

void NPCWizardDialog::OnOutfitParamChanged(wxCommandEvent& WXUNUSED(event)) {
	outfit_looktype = tlg_looktype_spin->GetValue();
	outfit_addons = (tlg_addon1_cb->IsChecked() ? 1 : 0) | (tlg_addon2_cb->IsChecked() ? 2 : 0);
	outfit_has_mount = tlg_mount_cb->IsChecked();
	outfit_mount = tlg_mount_spin->GetValue();

	UpdateOutfitPreview();
	UpdateLooktypeCodeBox();
}

void NPCWizardDialog::OnOutfitSpinChanged(wxSpinEvent& WXUNUSED(event)) {
	wxCommandEvent e;
	OnOutfitParamChanged(e);
}

void NPCWizardDialog::OnChannelSelect(wxCommandEvent& event) {
	int id = event.GetId();
	if (id == ID_NPC_BTN_HEAD) active_channel = CHANNEL_HEAD;
	else if (id == ID_NPC_BTN_PRIMARY) active_channel = CHANNEL_PRIMARY;
	else if (id == ID_NPC_BTN_SECONDARY) active_channel = CHANNEL_SECONDARY;
	else if (id == ID_NPC_BTN_DETAIL) active_channel = CHANNEL_DETAIL;

	UpdateChannelButtonStyles();
	int curCol = (active_channel == CHANNEL_HEAD) ? outfit_head :
		(active_channel == CHANNEL_PRIMARY) ? outfit_body :
		(active_channel == CHANNEL_SECONDARY) ? outfit_legs : outfit_feet;
	tlg_palette->SetSelectedColorId(curCol);
}

void NPCWizardDialog::OnPaletteColorSelected(wxCommandEvent& event) {
	int colorId = event.GetInt();
	if (active_channel == CHANNEL_HEAD) outfit_head = colorId;
	else if (active_channel == CHANNEL_PRIMARY) outfit_body = colorId;
	else if (active_channel == CHANNEL_SECONDARY) outfit_legs = colorId;
	else if (active_channel == CHANNEL_DETAIL) outfit_feet = colorId;

	UpdateColorPreviews();
	UpdateOutfitPreview();
	UpdateLooktypeCodeBox();
}

void NPCWizardDialog::UpdateChannelButtonStyles() {
	wxColour activeBorder(229, 193, 88);
	wxColour defaultBorder(60, 70, 85);

	tlg_btn_head->SetForegroundColour(active_channel == CHANNEL_HEAD ? activeBorder : *wxWHITE);
	tlg_btn_primary->SetForegroundColour(active_channel == CHANNEL_PRIMARY ? activeBorder : *wxWHITE);
	tlg_btn_secondary->SetForegroundColour(active_channel == CHANNEL_SECONDARY ? activeBorder : *wxWHITE);
	tlg_btn_detail->SetForegroundColour(active_channel == CHANNEL_DETAIL ? activeBorder : *wxWHITE);

	tlg_btn_head->Refresh();
	tlg_btn_primary->Refresh();
	tlg_btn_secondary->Refresh();
	tlg_btn_detail->Refresh();
}

void NPCWizardDialog::UpdateColorPreviews() {
	tlg_col_preview_head->SetBackgroundColour(GetTibiaColour(outfit_head));
	tlg_col_preview_primary->SetBackgroundColour(GetTibiaColour(outfit_body));
	tlg_col_preview_secondary->SetBackgroundColour(GetTibiaColour(outfit_legs));
	tlg_col_preview_detail->SetBackgroundColour(GetTibiaColour(outfit_feet));

	tlg_col_preview_head->Refresh();
	tlg_col_preview_primary->Refresh();
	tlg_col_preview_secondary->Refresh();
	tlg_col_preview_detail->Refresh();
}

void NPCWizardDialog::UpdateOutfitPreview() {
	if (tlg_preview_panel) {
		tlg_preview_panel->SetOutfit(outfit_looktype, outfit_head, outfit_body, outfit_legs, outfit_feet, outfit_addons, outfit_mount, outfit_has_mount);
	}
}

void NPCWizardDialog::UpdateLooktypeCodeBox() {
	if (tlg_code_ctrl) {
		wxString code = wxString::Format("#%d:%d:%d:%d:%d:%d:%d",
			outfit_looktype, outfit_head, outfit_body, outfit_legs, outfit_feet, outfit_addons, (outfit_has_mount ? outfit_mount : 0));
		tlg_code_ctrl->ChangeValue(code);
	}
}

void NPCWizardDialog::OnCopyLooktypeCode(wxCommandEvent& WXUNUSED(event)) {
	if (wxTheClipboard->Open()) {
		wxString code = tlg_code_ctrl->GetValue();
		wxTheClipboard->SetData(new wxTextDataObject(code));
		wxTheClipboard->Close();
		g_gui.SetStatusText("Copied LookType code to clipboard: " + code);
	}
}

void NPCWizardDialog::OnLoadLooktypeCode(wxCommandEvent& WXUNUSED(event)) {
	wxString code = tlg_code_ctrl->GetValue().Trim().Trim(false);
	if (code.StartsWith("#")) code = code.Mid(1);

	wxArrayString parts = wxSplit(code, ':');
	if (parts.GetCount() >= 5) {
		long type = 0, head = 0, body = 0, legs = 0, feet = 0, addons = 0, mount = 0;
		parts[0].ToLong(&type);
		parts[1].ToLong(&head);
		parts[2].ToLong(&body);
		parts[3].ToLong(&legs);
		parts[4].ToLong(&feet);
		if (parts.GetCount() > 5) parts[5].ToLong(&addons);
		if (parts.GetCount() > 6) parts[6].ToLong(&mount);

		outfit_looktype = type;
		outfit_head = std::clamp((int)head, 0, 132);
		outfit_body = std::clamp((int)body, 0, 132);
		outfit_legs = std::clamp((int)legs, 0, 132);
		outfit_feet = std::clamp((int)feet, 0, 132);
		outfit_addons = (int)addons;
		outfit_mount = (int)mount;
		outfit_has_mount = (mount > 0);

		tlg_looktype_spin->SetValue(outfit_looktype);
		tlg_addon1_cb->SetValue((outfit_addons & 1) != 0);
		tlg_addon2_cb->SetValue((outfit_addons & 2) != 0);
		tlg_mount_cb->SetValue(outfit_has_mount);
		tlg_mount_spin->SetValue(outfit_mount);

		UpdateColorPreviews();
		UpdateOutfitPreview();
		UpdateLooktypeCodeBox();
		g_gui.SetStatusText("LookType code loaded successfully.");
	} else {
		wxMessageBox("Invalid LookType code format.\nExpected format: #<type>:<head>:<body>:<legs>:<feet>:<addons>:<mount>", "Format Error", wxICON_WARNING, this);
	}
}

void NPCWizardDialog::OnRandomizeOutfit(wxCommandEvent& WXUNUSED(event)) {
	static std::mt19937 rng(1337);
	std::uniform_int_distribution<int> pDist(0, (int)g_tlgPresets.size() - 1);
	std::uniform_int_distribution<int> cDist(0, 132);
	std::uniform_int_distribution<int> aDist(0, 3);
	std::uniform_int_distribution<int> gDist(0, 1);

	int pIdx = pDist(rng);
	bool female = (gDist(rng) == 1);
	outfit_looktype = female ? g_tlgPresets[pIdx].female_id : g_tlgPresets[pIdx].male_id;
	outfit_head = cDist(rng);
	outfit_body = cDist(rng);
	outfit_legs = cDist(rng);
	outfit_feet = cDist(rng);
	outfit_addons = aDist(rng);

	tlg_preset_choice->SetSelection(pIdx);
	tlg_gender_filter->SetSelection(female ? 2 : 1);
	tlg_looktype_spin->SetValue(outfit_looktype);
	tlg_addon1_cb->SetValue((outfit_addons & 1) != 0);
	tlg_addon2_cb->SetValue((outfit_addons & 2) != 0);

	UpdateColorPreviews();
	UpdateOutfitPreview();
	UpdateLooktypeCodeBox();
}

void NPCWizardDialog::OnRotateLeft(wxCommandEvent& WXUNUSED(event)) {
	if (tlg_preview_panel) tlg_preview_panel->RotateLeft();
}

void NPCWizardDialog::OnRotateRight(wxCommandEvent& WXUNUSED(event)) {
	if (tlg_preview_panel) tlg_preview_panel->RotateDirection();
}

void NPCWizardDialog::OnToggleFrontBack(wxCommandEvent& WXUNUSED(event)) {
	if (tlg_preview_panel) tlg_preview_panel->ToggleFrontBack();
}

void NPCWizardDialog::OnToggleAnimate(wxCommandEvent& WXUNUSED(event)) {
	if (npc_is_stepping_loop) {
		npc_step_timer->Stop();
		npc_is_stepping_loop = false;
	} else {
		npc_step_timer->Start(350);
		npc_is_stepping_loop = true;
	}
}

void NPCWizardDialog::OnStepTimer(wxTimerEvent& WXUNUSED(event)) {
	if (tlg_preview_panel) tlg_preview_panel->StepFrame();
}

// ----------------------------------------------------------------------------
// Trade / Shop Handlers
// ----------------------------------------------------------------------------
void NPCWizardDialog::OnSelectShopItem(wxCommandEvent& WXUNUSED(event)) {
	FindItemDialog dlg(this, "Select Shop Item", true);
	if (dlg.ShowModal() == wxID_OK) {
		uint16_t id = dlg.getResultID();
		if (id > 0) {
			shop_selected_item_id = id;
			ItemType& it = g_items[id];
			shop_item_label->SetLabel(wxString::Format("Selected: %s (ID %d)", it.name.empty() ? ("Item #" + std::to_string(id)) : it.name, id));
		}
	}
}

void NPCWizardDialog::OnAddShopOffer(wxCommandEvent& WXUNUSED(event)) {
	if (shop_selected_item_id <= 0) return;
	bool isBuy = shop_chk_buy->IsChecked();
	bool isSell = shop_chk_sell->IsChecked();
	if (!isBuy && !isSell) {
		wxMessageBox("Please enable at least 'Buy' or 'Sell' for this offer.", "Invalid Offer", wxICON_WARNING, this);
		return;
	}

	ItemType& it = g_items[shop_selected_item_id];
	std::string name = it.name.empty() ? ("Item #" + std::to_string(shop_selected_item_id)) : it.name;
	int buyPrice = isBuy ? shop_buy_price_spin->GetValue() : 0;
	int sellPrice = isSell ? shop_sell_price_spin->GetValue() : 0;

	shop_offers.push_back({ shop_selected_item_id, name, isBuy, buyPrice, isSell, sellPrice });

	long idx = shop_unified_list->InsertItem(shop_unified_list->GetItemCount(), std::to_string(shop_selected_item_id));
	shop_unified_list->SetItem(idx, 1, name);
	std::string modeStr = (isBuy && isSell) ? "Buy & Sell" : (isBuy ? "Buy Only" : "Sell Only");
	shop_unified_list->SetItem(idx, 2, modeStr);
	shop_unified_list->SetItem(idx, 3, isBuy ? std::to_string(buyPrice) : "-");
	shop_unified_list->SetItem(idx, 4, isSell ? std::to_string(sellPrice) : "-");
}

void NPCWizardDialog::OnEditShopOffer(wxCommandEvent& WXUNUSED(event)) {
	long sel = shop_unified_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (sel >= 0 && sel < (long)shop_offers.size()) {
		bool isBuy = shop_chk_buy->IsChecked();
		bool isSell = shop_chk_sell->IsChecked();
		shop_offers[sel].is_buy = isBuy;
		shop_offers[sel].buy_price = isBuy ? shop_buy_price_spin->GetValue() : 0;
		shop_offers[sel].is_sell = isSell;
		shop_offers[sel].sell_price = isSell ? shop_sell_price_spin->GetValue() : 0;

		std::string modeStr = (isBuy && isSell) ? "Buy & Sell" : (isBuy ? "Buy Only" : "Sell Only");
		shop_unified_list->SetItem(sel, 2, modeStr);
		shop_unified_list->SetItem(sel, 3, isBuy ? std::to_string(shop_offers[sel].buy_price) : "-");
		shop_unified_list->SetItem(sel, 4, isSell ? std::to_string(shop_offers[sel].sell_price) : "-");
	}
}

void NPCWizardDialog::OnRemoveShopOffer(wxCommandEvent& WXUNUSED(event)) {
	long sel = shop_unified_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (sel >= 0 && sel < (long)shop_offers.size()) {
		shop_offers.erase(shop_offers.begin() + sel);
		shop_unified_list->DeleteItem(sel);
	}
}

// ----------------------------------------------------------------------------
// Quests & Missions Handlers
// ----------------------------------------------------------------------------
void NPCWizardDialog::RefreshQuestsList() {
	quests_list->DeleteAllItems();
	for (size_t i = 0; i < quest_chains.size(); ++i) {
		long idx = quests_list->InsertItem(quests_list->GetItemCount(), std::to_string(i + 1));
		quests_list->SetItem(idx, 1, quest_chains[i].title);
		quests_list->SetItem(idx, 2, std::to_string(quest_chains[i].storage_key));
		quests_list->SetItem(idx, 3, std::to_string(quest_chains[i].required_level));
	}
}

void NPCWizardDialog::LoadQuestDetails(int index) {
	if (index < 0 || index >= (int)quest_chains.size()) return;
	selected_quest_index = index;
	const auto& q = quest_chains[index];

	quest_title_ctrl->SetValue(q.title);
	quest_storage_spin->SetValue(q.storage_key);
	quest_val_spin->SetValue(q.storage_val);
	quest_req_lvl_spin->SetValue(q.required_level);
	quest_exp_spin->SetValue(q.reward_exp);
	quest_gold_spin->SetValue(q.reward_gold);
	quest_offer_text->SetValue(q.offer_text);
	quest_prog_text->SetValue(q.in_progress_text);
	quest_done_text->SetValue(q.complete_text);
	quest_log_text->SetValue(q.quest_log_text);

	cur_quest_req_items = q.required_items;
	cur_quest_reward_items = q.reward_items;

	quest_req_items_list->DeleteAllItems();
	for (const auto& it : cur_quest_req_items) {
		long idx = quest_req_items_list->InsertItem(quest_req_items_list->GetItemCount(), std::to_string(it.id));
		quest_req_items_list->SetItem(idx, 1, it.name);
		quest_req_items_list->SetItem(idx, 2, std::to_string(it.count));
	}

	quest_rewards_list->DeleteAllItems();
	for (const auto& it : cur_quest_reward_items) {
		long idx = quest_rewards_list->InsertItem(quest_rewards_list->GetItemCount(), std::to_string(it.id));
		quest_rewards_list->SetItem(idx, 1, it.name);
		quest_rewards_list->SetItem(idx, 2, std::to_string(it.count));
	}
}

void NPCWizardDialog::OnQuestSelect(wxListEvent& event) {
	LoadQuestDetails(event.GetIndex());
}

void NPCWizardDialog::OnNewQuest(wxCommandEvent& WXUNUSED(event)) {
	QuestDefinition q;
	q.title = "New Mission " + std::to_string(quest_chains.size() + 1);
	q.storage_key = 10050 + (int)quest_chains.size();
	q.storage_val = 1;
	q.required_level = 10;
	q.reward_exp = 1000;
	q.reward_gold = 250;
	q.offer_text = "I have a task for an experienced adventurer. Are you interested?";
	q.in_progress_text = "Have you completed your mission yet?";
	q.complete_text = "Great job! Take your reward.";
	q.quest_log_text = "Complete the task.";
	quest_chains.push_back(q);

	RefreshQuestsList();
	LoadQuestDetails((int)quest_chains.size() - 1);
}

void NPCWizardDialog::OnSaveQuest(wxCommandEvent& event) {
	if (selected_quest_index < 0 || selected_quest_index >= (int)quest_chains.size()) {
		OnNewQuest(event);
		return;
	}

	auto& q = quest_chains[selected_quest_index];
	q.title = quest_title_ctrl->GetValue().ToStdString();
	q.storage_key = quest_storage_spin->GetValue();
	q.storage_val = quest_val_spin->GetValue();
	q.required_level = quest_req_lvl_spin->GetValue();
	q.reward_exp = quest_exp_spin->GetValue();
	q.reward_gold = quest_gold_spin->GetValue();
	q.offer_text = quest_offer_text->GetValue().ToStdString();
	q.in_progress_text = quest_prog_text->GetValue().ToStdString();
	q.complete_text = quest_done_text->GetValue().ToStdString();
	q.quest_log_text = quest_log_text->GetValue().ToStdString();
	q.required_items = cur_quest_req_items;
	q.reward_items = cur_quest_reward_items;

	RefreshQuestsList();
	g_gui.SetStatusText("Quest '" + q.title + "' updated successfully.");
}

void NPCWizardDialog::OnDeleteQuest(wxCommandEvent& WXUNUSED(event)) {
	if (selected_quest_index >= 0 && selected_quest_index < (int)quest_chains.size()) {
		quest_chains.erase(quest_chains.begin() + selected_quest_index);
		RefreshQuestsList();
		if (!quest_chains.empty()) {
			LoadQuestDetails(0);
		} else {
			selected_quest_index = -1;
		}
	}
}

void NPCWizardDialog::OnSelectQuestReqItem(wxCommandEvent& WXUNUSED(event)) {
	FindItemDialog dlg(this, "Select Required Quest Item", true);
	if (dlg.ShowModal() == wxID_OK) {
		uint16_t id = dlg.getResultID();
		if (id > 0) {
			quest_req_selected_item_id = id;
			ItemType& it = g_items[id];
			quest_req_item_label->SetLabel(wxString::Format("%s (%d)", it.name.empty() ? ("Item #" + std::to_string(id)) : it.name, id));
		}
	}
}

void NPCWizardDialog::OnAddQuestReqItem(wxCommandEvent& WXUNUSED(event)) {
	if (quest_req_selected_item_id <= 0) return;
	ItemType& it = g_items[quest_req_selected_item_id];
	std::string name = it.name.empty() ? ("Item #" + std::to_string(quest_req_selected_item_id)) : it.name;
	int count = quest_req_item_count_spin->GetValue();

	cur_quest_req_items.push_back({ quest_req_selected_item_id, name, count });
	long idx = quest_req_items_list->InsertItem(quest_req_items_list->GetItemCount(), std::to_string(quest_req_selected_item_id));
	quest_req_items_list->SetItem(idx, 1, name);
	quest_req_items_list->SetItem(idx, 2, std::to_string(count));
}

void NPCWizardDialog::OnRemoveQuestReqItem(wxCommandEvent& WXUNUSED(event)) {
	long sel = quest_req_items_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (sel >= 0 && sel < (long)cur_quest_req_items.size()) {
		cur_quest_req_items.erase(cur_quest_req_items.begin() + sel);
		quest_req_items_list->DeleteItem(sel);
	}
}

void NPCWizardDialog::OnSelectQuestRewardItem(wxCommandEvent& WXUNUSED(event)) {
	FindItemDialog dlg(this, "Select Reward Item", true);
	if (dlg.ShowModal() == wxID_OK) {
		uint16_t id = dlg.getResultID();
		if (id > 0) {
			quest_reward_selected_item_id = id;
			ItemType& it = g_items[id];
			quest_reward_item_label->SetLabel(wxString::Format("%s (%d)", it.name.empty() ? ("Item #" + std::to_string(id)) : it.name, id));
		}
	}
}

void NPCWizardDialog::OnAddQuestRewardItem(wxCommandEvent& WXUNUSED(event)) {
	if (quest_reward_selected_item_id <= 0) return;
	ItemType& it = g_items[quest_reward_selected_item_id];
	std::string name = it.name.empty() ? ("Item #" + std::to_string(quest_reward_selected_item_id)) : it.name;
	int count = quest_reward_item_count_spin->GetValue();

	cur_quest_reward_items.push_back({ quest_reward_selected_item_id, name, count });
	long idx = quest_rewards_list->InsertItem(quest_rewards_list->GetItemCount(), std::to_string(quest_reward_selected_item_id));
	quest_rewards_list->SetItem(idx, 1, name);
	quest_rewards_list->SetItem(idx, 2, std::to_string(count));
}

void NPCWizardDialog::OnRemoveQuestRewardItem(wxCommandEvent& WXUNUSED(event)) {
	long sel = quest_rewards_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (sel >= 0 && sel < (long)cur_quest_reward_items.size()) {
		cur_quest_reward_items.erase(cur_quest_reward_items.begin() + sel);
		quest_rewards_list->DeleteItem(sel);
	}
}

// ----------------------------------------------------------------------------
// Travel Handlers
// ----------------------------------------------------------------------------
void NPCWizardDialog::OnAddTravelRoute(wxCommandEvent& WXUNUSED(event)) {
	std::string town = ship_dest_choice->GetStringSelection().ToStdString();
	if (town.empty()) town = "Thais";
	int x = ship_pos_x_spin->GetValue();
	int y = ship_pos_y_spin->GetValue();
	int z = ship_pos_z_spin->GetValue();
	int cost = ship_cost_spin->GetValue();
	std::string text = ship_text_ctrl->GetValue().ToStdString();

	travel_routes.push_back({ town, x, y, z, cost, text });
	long idx = ship_routes_list->InsertItem(ship_routes_list->GetItemCount(), town);
	ship_routes_list->SetItem(idx, 1, std::to_string(x));
	ship_routes_list->SetItem(idx, 2, std::to_string(y));
	ship_routes_list->SetItem(idx, 3, std::to_string(z));
	ship_routes_list->SetItem(idx, 4, std::to_string(cost));
	ship_routes_list->SetItem(idx, 5, text);
}

void NPCWizardDialog::OnRemoveTravelRoute(wxCommandEvent& WXUNUSED(event)) {
	long sel = ship_routes_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (sel >= 0 && sel < (long)travel_routes.size()) {
		travel_routes.erase(travel_routes.begin() + sel);
		ship_routes_list->DeleteItem(sel);
	}
}

// ----------------------------------------------------------------------------
// Dialogue Handlers
// ----------------------------------------------------------------------------
void NPCWizardDialog::OnAddDialogue(wxCommandEvent& WXUNUSED(event)) {
	std::string kw = diag_kw_ctrl->GetValue().ToStdString();
	std::string rep = diag_reply_ctrl->GetValue().ToStdString();
	if (kw.empty()) return;

	custom_dialogues.push_back({ kw, rep });
	long idx = dialogue_list->InsertItem(dialogue_list->GetItemCount(), kw);
	dialogue_list->SetItem(idx, 1, rep);
}

void NPCWizardDialog::OnRemoveDialogue(wxCommandEvent& WXUNUSED(event)) {
	long sel = dialogue_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (sel >= 0 && sel < (long)custom_dialogues.size()) {
		custom_dialogues.erase(custom_dialogues.begin() + sel);
		dialogue_list->DeleteItem(sel);
	}
}

// ----------------------------------------------------------------------------
// Code Generation & Exporters
// ----------------------------------------------------------------------------
void NPCWizardDialog::PopulateTownChoices(wxChoice* choice) {
	choice->Clear();
	if (g_gui.GetCurrentEditor()) {
		for (const auto& pair : g_gui.GetCurrentEditor()->map.towns) {
			choice->Append(pair.second->getName());
		}
	}
	if (choice->IsEmpty()) {
		choice->Append("Thais");
		choice->Append("Carlin");
		choice->Append("Kazordoon");
		choice->Append("Venore");
		choice->Append("Ab'Dendriel");
		choice->Append("Edron");
		choice->Append("Darashia");
		choice->Append("Ankrahmun");
		choice->Append("Liberty Bay");
		choice->Append("Port Hope");
		choice->Append("Svargrond");
		choice->Append("Yalahar");
		choice->Append("Rathleton");
		choice->Append("Roshamuul");
	}
	choice->SetSelection(0);
}

std::string NPCWizardDialog::BuildXml() {
	std::ostringstream ss;
	std::string name = npc_name_ctrl ? npc_name_ctrl->GetValue().ToStdString() : "Captain Jack";
	int walkInterval = npc_walk_interval ? npc_walk_interval->GetValue() : 2000;
	int walkRadius = npc_walk_range ? npc_walk_range->GetValue() : 2;
	int spawnDelay = npc_spawn_delay ? npc_spawn_delay->GetValue() : 60;

	ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	ss << "<npc name=\"" << name << "\" script=\"" << name << ".lua\" walkinterval=\"" << walkInterval << "\" floorchange=\"0\" radius=\"" << walkRadius << "\" spawndelay=\"" << spawnDelay << "\">\n";
	ss << "\t<health now=\"100\" max=\"100\"/>\n";
	ss << "\t<look type=\"" << outfit_looktype << "\" head=\"" << outfit_head << "\" body=\"" << outfit_body << "\" legs=\"" << outfit_legs << "\" feet=\"" << outfit_feet << "\" addons=\"" << outfit_addons << "\"";
	if (outfit_has_mount && outfit_mount > 0) {
		ss << " mount=\"" << outfit_mount << "\"";
	}
	ss << "/>\n";

	// Shop parameters in XML if applicable
	if (!shop_offers.empty()) {
		ss << "\t<parameters>\n";
		ss << "\t\t<parameter key=\"module_shop\" value=\"1\"/>\n";
		std::string buyItems, sellItems;
		for (const auto& off : shop_offers) {
			if (off.is_buy) {
				if (!buyItems.empty()) buyItems += ";";
				buyItems += off.name + "," + std::to_string(off.id) + "," + std::to_string(off.buy_price);
			}
			if (off.is_sell) {
				if (!sellItems.empty()) sellItems += ";";
				sellItems += off.name + "," + std::to_string(off.id) + "," + std::to_string(off.sell_price);
			}
		}
		if (!buyItems.empty()) ss << "\t\t<parameter key=\"shop_buyable\" value=\"" << buyItems << "\"/>\n";
		if (!sellItems.empty()) ss << "\t\t<parameter key=\"shop_sellable\" value=\"" << sellItems << "\"/>\n";
		ss << "\t</parameters>\n";
	}

	ss << "</npc>\n";
	return ss.str();
}

std::string NPCWizardDialog::BuildLua() {
	std::ostringstream ss;
	std::string name = npc_name_ctrl ? npc_name_ctrl->GetValue().ToStdString() : "Captain Jack";
	std::string greet = npc_greet_ctrl ? npc_greet_ctrl->GetValue().ToStdString() : "Ahoy, |PLAYERNAME|!";
	std::string farewell = npc_farewell_ctrl ? npc_farewell_ctrl->GetValue().ToStdString() : "Farewell, adventurer!";

	ss << "-- TFS 1.x / Revscriptsys NPC Handler: " << name << "\n";
	ss << "local keywordHandler = KeywordHandler:new()\n";
	ss << "local npcHandler = NpcHandler:new(keywordHandler)\n";
	ss << "NpcSystem.parseParameters(npcHandler)\n\n";

	ss << "function onCreatureAppear(cid) npcHandler:onCreatureAppear(cid) end\n";
	ss << "function onCreatureDisappear(cid) npcHandler:onCreatureDisappear(cid) end\n";
	ss << "function onCreatureSay(cid, type, msg) npcHandler:onCreatureSay(cid, type, msg) end\n";
	ss << "function onThink() npcHandler:onThink() end\n\n";

	// Travel node handlers
	if (!travel_routes.empty()) {
		for (const auto& r : travel_routes) {
			ss << "-- Destination: " << r.town << "\n";
			ss << "local travelNode_" << r.town << " = keywordHandler:addKeyword({'travel', '" << r.town << "'}, StdModule.say, {npcHandler = npcHandler, text = 'Do you want to travel to " << r.town << " for " << r.cost << " gold?'})\n";
			ss << "\ttravelNode_" << r.town << ":addChildKeyword({'yes'}, StdModule.travel, {npcHandler = npcHandler, premium = false, cost = " << r.cost << ", destination = Position(" << r.x << ", " << r.y << ", " << r.z << "), text = '" << r.text << "'})\n";
			ss << "\ttravelNode_" << r.town << ":addChildKeyword({'no'}, StdModule.say, {npcHandler = npcHandler, reset = true, text = 'Maybe next time then.'})\n\n";
		}
	}

	// Quests Handlers
	if (!quest_chains.empty()) {
		ss << "-- Quest Chain Handlers\n";
		for (size_t i = 0; i < quest_chains.size(); ++i) {
			const auto& q = quest_chains[i];
			ss << "local function questCallback_" << i << "(cid, type, msg)\n";
			ss << "\tlocal player = Player(cid)\n";
			ss << "\tif not player then return false end\n";
			ss << "\tif player:getStorageValue(" << q.storage_key << ") < " << q.storage_val << " then\n";
			ss << "\t\tif player:getLevel() < " << q.required_level << " then\n";
			ss << "\t\t\tnpcHandler:say('You need at least level " << q.required_level << " to undertake this mission.', cid)\n";
			ss << "\t\t\treturn true\n";
			ss << "\t\tend\n";
			if (!q.required_items.empty()) {
				for (const auto& req : q.required_items) {
					ss << "\t\tif player:getItemCount(" << req.id << ") < " << req.count << " then\n";
					ss << "\t\t\tnpcHandler:say('" << q.in_progress_text << "', cid)\n";
					ss << "\t\t\treturn true\n";
					ss << "\t\tend\n";
				}
				for (const auto& req : q.required_items) {
					ss << "\t\tplayer:removeItem(" << req.id << ", " << req.count << ")\n";
				}
			}
			if (q.reward_exp > 0) ss << "\t\tplayer:addExperience(" << q.reward_exp << ")\n";
			if (q.reward_gold > 0) ss << "\t\tplayer:addMoney(" << q.reward_gold << ")\n";
			for (const auto& rew : q.reward_items) {
				ss << "\t\tplayer:addItem(" << rew.id << ", " << rew.count << ")\n";
			}
			ss << "\t\tplayer:setStorageValue(" << q.storage_key << ", " << q.storage_val << ")\n";
			ss << "\t\tnpcHandler:say('" << q.complete_text << "', cid)\n";
			ss << "\telse\n";
			ss << "\t\tnpcHandler:say('You have already completed this quest!', cid)\n";
			ss << "\tend\n";
			ss << "\treturn true\n";
			ss << "end\n";
			ss << "keywordHandler:addKeyword({'quest', 'mission', '" << q.title << "'}, questCallback_" << i << ", {npcHandler = npcHandler})\n\n";
		}
	}

	// Custom dialogue keywords
	for (const auto& d : custom_dialogues) {
		ss << "keywordHandler:addKeyword({'" << d.keyword << "'}, StdModule.say, {npcHandler = npcHandler, text = '" << d.reply << "'})\n";
	}

	ss << "npcHandler:setMessage(MESSAGE_GREET, '" << greet << "')\n";
	ss << "npcHandler:setMessage(MESSAGE_FAREWELL, '" << farewell << "')\n";
	ss << "npcHandler:addModule(FocusModule:new())\n";

	return ss.str();
}

std::string NPCWizardDialog::BuildLiveCode() {
	std::ostringstream ss;
	ss << "=== XML NPC DEFINITION ===\n";
	ss << BuildXml();
	ss << "\n=== TFS 1.X / REVSCRIPTSYS LUA SCRIPT ===\n";
	ss << BuildLua();
	return ss.str();
}

void NPCWizardDialog::OnRefreshScriptView(wxCommandEvent& WXUNUSED(event)) {
	live_code_ctrl->SetValue(BuildLiveCode());
}

void NPCWizardDialog::OnCopyLiveCode(wxCommandEvent& WXUNUSED(event)) {
	if (wxTheClipboard->Open()) {
		std::string code = BuildLiveCode();
		wxTheClipboard->SetData(new wxTextDataObject(code));
		wxTheClipboard->Close();
		g_gui.SetStatusText("Copied generated NPC script & XML to clipboard!");
	}
}

void NPCWizardDialog::OnSaveFile(wxCommandEvent& WXUNUSED(event)) {
	std::string name = npc_name_ctrl ? npc_name_ctrl->GetValue().ToStdString() : "npc";
	wxFileDialog dlg(this, "Save NPC XML and Lua Definition", "", name + ".xml", "XML Files (*.xml)|*.xml|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (dlg.ShowModal() == wxID_OK) {
		wxString xmlPath = dlg.GetPath();
		wxFileOutputStream outXml(xmlPath);
		if (outXml.IsOk()) {
			std::string xml = BuildXml();
			outXml.Write(xml.c_str(), xml.size());
		}

		wxFileName fn(xmlPath);
		fn.SetExt("lua");
		wxFileOutputStream outLua(fn.GetFullPath());
		if (outLua.IsOk()) {
			std::string lua = BuildLua();
			outLua.Write(lua.c_str(), lua.size());
		}

		wxMessageBox("Saved NPC definition:\n" + xmlPath + "\n" + fn.GetFullPath(), "Export Complete", wxICON_INFORMATION, this);
	}
}

void NPCWizardDialog::OnRegisterPalette(wxCommandEvent& WXUNUSED(event)) {
	std::string name = npc_name_ctrl ? npc_name_ctrl->GetValue().ToStdString() : "Jack";
	if (name.empty()) name = "Jack";

	Outfit outfit;
	outfit.lookType = outfit_looktype;
	outfit.lookHead = outfit_head;
	outfit.lookBody = outfit_body;
	outfit.lookLegs = outfit_legs;
	outfit.lookFeet = outfit_feet;
	outfit.lookAddon = outfit_addons;
	if (outfit_has_mount && outfit_mount > 0) {
		outfit.lookMount = outfit_mount;
	}

	CreatureType* registered = g_creatures.addCreatureType(name, true, outfit);
	if (registered) {
		CreatureBrush* cb = new CreatureBrush(registered);
		g_gui.SelectBrush(cb);
		g_gui.SetStatusText("Created & selected NPC creature brush: " + name);
		wxMessageBox("NPC '" + name + "' successfully created and selected as active Creature Brush in the palette!\nYou can now click on the map to spawn this NPC.", "NPC Registered", wxICON_INFORMATION, this);
		EndModal(wxID_OK);
	} else {
		wxMessageBox("Could not register NPC in creature database (already exists or invalid).", "Error", wxICON_ERROR, this);
	}
}

void NPCWizardDialog::OnClose(wxCommandEvent& WXUNUSED(event)) {
	EndModal(wxID_CANCEL);
}
