#include "tfs_npc_wizard_window.h"
#include "style_manager.h"
#include "find_item_window.h"
#include "style_manager.h"
#include "gui.h"
#include "style_manager.h"
#include "editor.h"
#include "style_manager.h"
#include "items.h"
#include "style_manager.h"
#include "creatures.h"
#include "style_manager.h"
#include "graphics.h"
#include "style_manager.h"
#include "materials.h"
#include "style_manager.h"
#include "creature_brush.h"
#include "style_manager.h"
#include <wx/stattext.h>
#include "style_manager.h"
#include <wx/button.h>
#include "style_manager.h"
#include <wx/sizer.h>
#include "style_manager.h"
#include <wx/msgdlg.h>
#include "style_manager.h"
#include <wx/filedlg.h>
#include "style_manager.h"
#include <wx/dcclient.h>
#include "style_manager.h"
#include <wx/wfstream.h>
#include "style_manager.h"
#include <wx/clipbrd.h>
#include "style_manager.h"
#include <wx/dataobj.h>
#include "style_manager.h"
#include <sstream>
#include "style_manager.h"
#include <iomanip>
#include "style_manager.h"
#include <algorithm>
#include "style_manager.h"

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
	wxPanel(parent, id, wxDefaultPosition, wxSize(19 * 14 + 4, 7 * 14 + 4), wxBORDER_NONE),
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
	dc.SetBackground(wxBrush(wxColour(25, 30, 40)));
	dc.Clear();

	int cellSize = 14;
	for (int i = 0; i < 133; ++i) {
		int row = i / 19;
		int col = i % 19;
		int x = 2 + col * cellSize;
		int y = 2 + row * cellSize;

		dc.SetBrush(wxBrush(GetTibiaColour(i)));
		if (i == selected_color_id) {
			dc.SetPen(wxPen(wxColour(255, 215, 0), 2)); // Corporate Gold selection border
		} else {
			dc.SetPen(wxPen(wxColour(15, 15, 20), 1));
		}
		dc.DrawRectangle(x, y, cellSize - 1, cellSize - 1);
	}
}

void TibiaPalettePanel::OnMouseDown(wxMouseEvent& event) {
	int cellSize = 14;
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

	// Background fill with subtle dark obsidian
	dc.SetBrush(wxBrush(wxColour(16, 20, 30)));
	dc.SetPen(wxPen(wxColour(16, 20, 30)));
	dc.DrawRectangle(rect);

	// Outer Corporate Gold Border (#FFD700)
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.SetPen(wxPen(wxColour(255, 215, 0), 2));
	dc.DrawRectangle(rect);

	// Inner Gold Border (#D4AF37)
	dc.SetPen(wxPen(wxColour(180, 140, 50, 180), 1));
	dc.DrawRectangle(wxRect(3, 3, rect.width - 6, rect.height - 6));

	// Draw Tile floor grid lines for depth
	dc.SetPen(wxPen(wxColour(30, 40, 55), 1));
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
		spr->DrawOutfitTo(&dc, outfit, 6, 6, rect.width - 12, rect.height - 12, current_direction, look_addons, 0, current_frame);
	}
}

// ----------------------------------------------------------------------------
// NPCWizardDialog Implementation
// ----------------------------------------------------------------------------
enum {
	ID_NPC_ROLE_CHOICE = wxID_HIGHEST + 500,
	ID_NPC_TLG_PREV,
	ID_NPC_TLG_PRESET,
	ID_NPC_TLG_NEXT,
	ID_NPC_TLG_BTN_ROTATE,
	ID_NPC_TLG_BTN_VIEW,
	ID_NPC_TLG_BTN_ANIMATE,
	ID_NPC_TLG_BTN_RANDOM,
	ID_NPC_TLG_BTN_HEAD,
	ID_NPC_TLG_BTN_PRIMARY,
	ID_NPC_TLG_BTN_SECONDARY,
	ID_NPC_TLG_BTN_DETAIL,
	ID_NPC_TLG_PALETTE,
	ID_NPC_TLG_GENDER_FILTER,
	ID_NPC_STEP_TIMER,

	ID_NPC_ADD_BUY_OFFER,
	ID_NPC_REMOVE_BUY_OFFER,
	ID_NPC_ADD_SELL_OFFER,
	ID_NPC_REMOVE_SELL_OFFER,

	ID_NPC_ADD_ROUTE,
	ID_NPC_REMOVE_ROUTE,

	ID_NPC_ADD_DIAG,
	ID_NPC_REMOVE_DIAG,

	ID_NPC_ADD_REWARD_ITEM,
	ID_NPC_REMOVE_REWARD_ITEM,

	ID_NPC_BTN_SAVE_FILE,
	ID_NPC_BTN_REGISTER_PALETTE,
	ID_NPC_BTN_COPY_CODE
};

BEGIN_EVENT_TABLE(NPCWizardDialog, wxDialog)
	EVT_CHOICE(ID_NPC_ROLE_CHOICE, NPCWizardDialog::OnRoleChanged)
	EVT_BUTTON(ID_NPC_TLG_PREV, NPCWizardDialog::OnPresetPrev)
	EVT_CHOICE(ID_NPC_TLG_PRESET, NPCWizardDialog::OnPresetChoice)
	EVT_BUTTON(ID_NPC_TLG_NEXT, NPCWizardDialog::OnPresetNext)
	EVT_BUTTON(ID_NPC_TLG_BTN_ROTATE, NPCWizardDialog::OnRotate)
	EVT_BUTTON(ID_NPC_TLG_BTN_VIEW, NPCWizardDialog::OnToggleView)
	EVT_BUTTON(ID_NPC_TLG_BTN_ANIMATE, NPCWizardDialog::OnAnimate)
	EVT_TIMER(ID_NPC_STEP_TIMER, NPCWizardDialog::OnStepTimer)
	EVT_BUTTON(ID_NPC_TLG_BTN_RANDOM, NPCWizardDialog::OnRandomizeColors)

	EVT_BUTTON(ID_NPC_TLG_BTN_HEAD, NPCWizardDialog::OnChannelSelect)
	EVT_BUTTON(ID_NPC_TLG_BTN_PRIMARY, NPCWizardDialog::OnChannelSelect)
	EVT_BUTTON(ID_NPC_TLG_BTN_SECONDARY, NPCWizardDialog::OnChannelSelect)
	EVT_BUTTON(ID_NPC_TLG_BTN_DETAIL, NPCWizardDialog::OnChannelSelect)
	EVT_BUTTON(ID_NPC_TLG_PALETTE, NPCWizardDialog::OnOutfitParamChanged)

	EVT_CHOICE(ID_NPC_TLG_GENDER_FILTER, NPCWizardDialog::OnGenderFilterChanged)

	EVT_BUTTON(ID_NPC_ADD_ROUTE, NPCWizardDialog::OnAddTravelRoute)
	EVT_BUTTON(ID_NPC_REMOVE_ROUTE, NPCWizardDialog::OnRemoveTravelRoute)

	EVT_BUTTON(ID_NPC_ADD_DIAG, NPCWizardDialog::OnAddDialogue)
	EVT_BUTTON(ID_NPC_REMOVE_DIAG, NPCWizardDialog::OnRemoveDialogue)

	EVT_BUTTON(ID_NPC_ADD_REWARD_ITEM, NPCWizardDialog::OnAddQuestRewardItem)
	EVT_BUTTON(ID_NPC_REMOVE_REWARD_ITEM, NPCWizardDialog::OnRemoveQuestRewardItem)

	EVT_BUTTON(ID_NPC_BTN_SAVE_FILE, NPCWizardDialog::OnSaveFile)
	EVT_BUTTON(ID_NPC_BTN_REGISTER_PALETTE, NPCWizardDialog::OnRegisterPalette)
	EVT_BUTTON(ID_NPC_BTN_COPY_CODE, NPCWizardDialog::OnCopyLiveCode)
	EVT_BUTTON(wxID_CANCEL, NPCWizardDialog::OnClose)
END_EVENT_TABLE()

NPCWizardDialog::NPCWizardDialog(wxWindow* parent) :
	wxDialog(parent, wxID_ANY, "NPC Editor", wxDefaultPosition, wxSize(800, 700), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	active_channel(CHANNEL_HEAD),
	outfit_looktype(138),
	outfit_head(101),
	outfit_body(42),
	outfit_legs(114),
	outfit_feet(76),
	outfit_addons(0),
	outfit_mount(0),
	outfit_has_mount(false),
	npc_is_stepping_loop(false),
	shop_selected_item_id(2120),
	quest_reward_selected_item_id(2160)
{
	npc_step_timer = new wxTimer(this, ID_NPC_STEP_TIMER);

	wxBoxSizer* rootSizer = new wxBoxSizer(wxVERTICAL);

	// Header Panel (Obsidian Dark with Gold Title)
	wxPanel* headerPanel = new wxPanel(this, wxID_ANY);
	headerPanel->SetBackgroundColour(wxColour(16, 20, 30));
	wxBoxSizer* headerSizer = new wxBoxSizer(wxVERTICAL);

	wxStaticText* title = new wxStaticText(headerPanel, wxID_ANY, "NPC Editor");
	wxFont tFont = title->GetFont();
	tFont.SetPointSize(12);
	tFont.SetWeight(wxFONTWEIGHT_BOLD);
	title->SetFont(tFont);
	title->SetForegroundColour(wxColour(255, 215, 0));
	headerSizer->Add(title, 0, wxALL, 8);

	wxStaticText* sub = new wxStaticText(headerPanel, wxID_ANY, "Design custom NPCs, configure outfits with live Tibia LookType generator, and define interactive roles.");
	sub->SetForegroundColour(wxColour(190, 195, 205));
	headerSizer->Add(sub, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

	headerPanel->SetSizer(headerSizer);
	rootSizer->Add(headerPanel, 0, wxEXPAND);

	notebook = new wxNotebook(this, wxID_ANY);

	// =========================================================================
	// TAB 1: Look (Appearance & Outfit)
	// =========================================================================
	wxPanel* tab1 = new wxPanel(notebook, wxID_ANY);
	wxBoxSizer* t1Sizer = new wxBoxSizer(wxVERTICAL);

	// Top: Base Identity Bar
	wxStaticBoxSizer* idBox = new wxStaticBoxSizer(wxHORIZONTAL, tab1, "NPC Identity");
	wxFlexGridSizer* idGrid = new wxFlexGridSizer(1, 6, 6, 10);
	idGrid->AddGrowableCol(1, 1);
	idGrid->AddGrowableCol(3, 1);
	idGrid->AddGrowableCol(5, 1);

	idGrid->Add(new wxStaticText(tab1, wxID_ANY, "Name:"), 0, wxALIGN_CENTER_VERTICAL);
	npc_name_ctrl = new wxTextCtrl(tab1, wxID_ANY, "Captain Jack");
	idGrid->Add(npc_name_ctrl, 1, wxEXPAND);

	idGrid->Add(new wxStaticText(tab1, wxID_ANY, "Title:"), 0, wxALIGN_CENTER_VERTICAL);
	npc_title_ctrl = new wxTextCtrl(tab1, wxID_ANY, "Ship Captain");
	idGrid->Add(npc_title_ctrl, 1, wxEXPAND);

	idGrid->Add(new wxStaticText(tab1, wxID_ANY, "Role:"), 0, wxALIGN_CENTER_VERTICAL);
	wxArrayString roles;
	roles.Add("Shopkeeper (Buy & Sell)");
	roles.Add("Ship / Travel Captain");
	roles.Add("Temple Healer");
	roles.Add("Town Resident (Citizenship Registrar)");
	roles.Add("Quest Giver & Task Master");
	roles.Add("Custom Dialogue & Lore NPC");
	npc_role_choice = new wxChoice(tab1, ID_NPC_ROLE_CHOICE, wxDefaultPosition, wxDefaultSize, roles);
	npc_role_choice->SetSelection(0);
	idGrid->Add(npc_role_choice, 1, wxEXPAND);

	idBox->Add(idGrid, 1, wxEXPAND | wxALL, 4);
	t1Sizer->Add(idBox, 0, wxEXPAND | wxALL, 6);

	// Main TLG Layout (Left: Preview & Controls, Right: Channel Buttons & Palette)
	wxStaticBoxSizer* tlgBox = new wxStaticBoxSizer(wxHORIZONTAL, tab1, "Tibia LookType Generator");

	// --- Left Side of TLG ---
	wxBoxSizer* leftTLGSizer = new wxBoxSizer(wxVERTICAL);

	// Big Centered Preview Card
	tlg_preview_panel = new CreaturePreviewPanel(tab1, wxID_ANY, 150);
	leftTLGSizer->Add(tlg_preview_panel, 0, wxALIGN_CENTER | wxBOTTOM, 6);

	// Gender Filter
	wxBoxSizer* genderSizer = new wxBoxSizer(wxHORIZONTAL);
	genderSizer->Add(new wxStaticText(tab1, wxID_ANY, "Filter:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	wxArrayString genderOptions;
	genderOptions.Add("All");
	genderOptions.Add("Male");
	genderOptions.Add("Female");
	tlg_gender_filter = new wxChoice(tab1, ID_NPC_TLG_GENDER_FILTER, wxDefaultPosition, wxDefaultSize, genderOptions);
	tlg_gender_filter->SetSelection(0);
	genderSizer->Add(tlg_gender_filter, 1, wxEXPAND);
	leftTLGSizer->Add(genderSizer, 0, wxEXPAND | wxBOTTOM, 6);

	// Preset Choice & Arrows
	wxBoxSizer* presetSizer = new wxBoxSizer(wxHORIZONTAL);
	tlg_prev_btn = new wxButton(tab1, ID_NPC_TLG_PREV, "<", wxDefaultPosition, wxSize(28, 26));
	tlg_preset_choice = new wxChoice(tab1, ID_NPC_TLG_PRESET);

	struct LookPreset { std::string name; int id; };
	static const LookPreset presets[] = {
		{ "Citizen (Male)", 128 }, { "Citizen (Female)", 136 },
		{ "Hunter (Male)", 129 }, { "Hunter (Female)", 137 },
		{ "Mage (Male)", 130 }, { "Mage (Female)", 138 },
		{ "Knight (Male)", 131 }, { "Knight (Female)", 139 },
		{ "Nobleman (Male)", 132 }, { "Noblewoman (Female)", 140 },
		{ "Summoner (Male)", 133 }, { "Summoner (Female)", 141 },
		{ "Warrior (Male)", 134 }, { "Warrior (Female)", 142 },
		{ "Barbarian (Male)", 143 }, { "Barbarian (Female)", 147 },
		{ "Druid (Male)", 144 }, { "Druid (Female)", 148 },
		{ "Wizard (Male)", 145 }, { "Wizard (Female)", 149 },
		{ "Oriental (Male)", 146 }, { "Oriental (Female)", 150 },
		{ "Pirate (Male)", 151 }, { "Pirate (Female)", 155 },
		{ "Assassin (Male)", 152 }, { "Assassin (Female)", 156 },
		{ "Beggar (Male)", 153 }, { "Beggar (Female)", 157 },
		{ "Shaman (Male)", 154 }, { "Shaman (Female)", 158 },
		{ "Norseman (Male)", 251 }, { "Norsewoman (Female)", 252 },
		{ "Nightmare (Male)", 268 }, { "Nightmare (Female)", 269 },
		{ "Jester (Male)", 273 }, { "Jester (Female)", 270 },
		{ "Brotherhood (Male)", 278 }, { "Brotherhood (Female)", 279 },
		{ "Demon Hunter (Male)", 289 }, { "Demon Hunter (Female)", 288 },
		{ "Yalaharian (Male)", 325 }, { "Yalaharian (Female)", 324 }
	};

	for (const auto& p : presets) {
		tlg_preset_choice->Append(p.name, (void*)(intptr_t)p.id);
	}
	tlg_preset_choice->SetSelection(5); // Mage (Female)
	tlg_next_btn = new wxButton(tab1, ID_NPC_TLG_NEXT, ">", wxDefaultPosition, wxSize(28, 26));

	presetSizer->Add(tlg_prev_btn, 0, wxRIGHT, 2);
	presetSizer->Add(tlg_preset_choice, 1, wxEXPAND | wxRIGHT, 2);
	presetSizer->Add(tlg_next_btn, 0);
	leftTLGSizer->Add(presetSizer, 0, wxEXPAND | wxBOTTOM, 6);

	// Action Buttons: Rotate, Front/Back, Step, Randomize
	wxBoxSizer* actBtnSizer = new wxBoxSizer(wxHORIZONTAL);
	tlg_btn_rotate = new wxButton(tab1, ID_NPC_TLG_BTN_ROTATE, "Rotate", wxDefaultPosition, wxSize(56, 26));
	tlg_btn_view = new wxButton(tab1, ID_NPC_TLG_BTN_VIEW, "Front", wxDefaultPosition, wxSize(56, 26));
	tlg_btn_animate = new wxButton(tab1, ID_NPC_TLG_BTN_ANIMATE, "Step", wxDefaultPosition, wxSize(56, 26));
	actBtnSizer->Add(tlg_btn_rotate, 0, wxRIGHT, 3);
	actBtnSizer->Add(tlg_btn_view, 0, wxRIGHT, 3);
	actBtnSizer->Add(tlg_btn_animate, 0);
	leftTLGSizer->Add(actBtnSizer, 0, wxALIGN_CENTER);

	tlgBox->Add(leftTLGSizer, 0, wxALL | wxALIGN_CENTER_VERTICAL, 6);

	// --- Right Side of TLG (Channels & Color Palette Matrix) ---
	wxBoxSizer* rightTLGSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* palLayoutSizer = new wxBoxSizer(wxHORIZONTAL);

	// 4 Channel Buttons (Head, Primary, Secondary, Detail)
	wxBoxSizer* chBtnSizer = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer* hSizer = new wxBoxSizer(wxHORIZONTAL);
	tlg_btn_head = new wxButton(tab1, ID_NPC_TLG_BTN_HEAD, "Head", wxDefaultPosition, wxSize(80, 28));
	tlg_col_preview_head = new wxPanel(tab1, wxID_ANY, wxDefaultPosition, wxSize(18, 18));
	hSizer->Add(tlg_btn_head, 1, wxRIGHT, 4);
	hSizer->Add(tlg_col_preview_head, 0, wxALIGN_CENTER_VERTICAL);
	chBtnSizer->Add(hSizer, 0, wxEXPAND | wxBOTTOM, 4);

	wxBoxSizer* pSizer = new wxBoxSizer(wxHORIZONTAL);
	tlg_btn_primary = new wxButton(tab1, ID_NPC_TLG_BTN_PRIMARY, "Primary", wxDefaultPosition, wxSize(80, 28));
	tlg_col_preview_primary = new wxPanel(tab1, wxID_ANY, wxDefaultPosition, wxSize(18, 18));
	pSizer->Add(tlg_btn_primary, 1, wxRIGHT, 4);
	pSizer->Add(tlg_col_preview_primary, 0, wxALIGN_CENTER_VERTICAL);
	chBtnSizer->Add(pSizer, 0, wxEXPAND | wxBOTTOM, 4);

	wxBoxSizer* sSizer = new wxBoxSizer(wxHORIZONTAL);
	tlg_btn_secondary = new wxButton(tab1, ID_NPC_TLG_BTN_SECONDARY, "Secondary", wxDefaultPosition, wxSize(80, 28));
	tlg_col_preview_secondary = new wxPanel(tab1, wxID_ANY, wxDefaultPosition, wxSize(18, 18));
	sSizer->Add(tlg_btn_secondary, 1, wxRIGHT, 4);
	sSizer->Add(tlg_col_preview_secondary, 0, wxALIGN_CENTER_VERTICAL);
	chBtnSizer->Add(sSizer, 0, wxEXPAND | wxBOTTOM, 4);

	wxBoxSizer* dSizer = new wxBoxSizer(wxHORIZONTAL);
	tlg_btn_detail = new wxButton(tab1, ID_NPC_TLG_BTN_DETAIL, "Detail", wxDefaultPosition, wxSize(80, 28));
	tlg_col_preview_detail = new wxPanel(tab1, wxID_ANY, wxDefaultPosition, wxSize(18, 18));
	dSizer->Add(tlg_btn_detail, 1, wxRIGHT, 4);
	dSizer->Add(tlg_col_preview_detail, 0, wxALIGN_CENTER_VERTICAL);
	chBtnSizer->Add(dSizer, 0, wxEXPAND);

	palLayoutSizer->Add(chBtnSizer, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 10);

	// Palette Matrix
	tlg_palette = new TibiaPalettePanel(tab1, ID_NPC_TLG_PALETTE);
	tlg_palette->SetSelectedColorId(outfit_head);
	palLayoutSizer->Add(tlg_palette, 0, wxALIGN_CENTER_VERTICAL);

	rightTLGSizer->Add(palLayoutSizer, 0, wxALL | wxALIGN_CENTER, 4);

	// Randomize Button
	wxBoxSizer* palBottomSizer = new wxBoxSizer(wxHORIZONTAL);
	tlg_btn_random = new wxButton(tab1, ID_NPC_TLG_BTN_RANDOM, "Randomize Colors", wxDefaultPosition, wxSize(140, 28));
	tlg_btn_random->SetBackgroundColour(wxColour(50, 70, 110));
	tlg_btn_random->SetForegroundColour(*wxWHITE);
	palBottomSizer->Add(tlg_btn_random, 0, wxALIGN_CENTER);

	rightTLGSizer->Add(palBottomSizer, 0, wxALL | wxALIGN_CENTER, 6);

	tlgBox->Add(rightTLGSizer, 1, wxALL | wxALIGN_CENTER_VERTICAL, 6);
	t1Sizer->Add(tlgBox, 0, wxEXPAND | wxALL, 6);

	// Live Code Bar (Under TLG)
	wxStaticBoxSizer* codeBox = new wxStaticBoxSizer(wxHORIZONTAL, tab1, "Live LookType Code");
	live_code_ctrl = new wxTextCtrl(tab1, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	wxButton* copyCodeBtn = new wxButton(tab1, ID_NPC_BTN_COPY_CODE, "Copy", wxDefaultPosition, wxSize(60, -1));
	codeBox->Add(live_code_ctrl, 1, wxEXPAND | wxRIGHT, 6);
	codeBox->Add(copyCodeBtn, 0);
	t1Sizer->Add(codeBox, 0, wxEXPAND | wxALL, 6);

	tab1->SetSizer(t1Sizer);
	notebook->AddPage(tab1, "Look");

	// =========================================================================
	// TAB 2: Config (Role Behaviors & Functions)
	// =========================================================================
	role_container = new wxPanel(notebook, wxID_ANY);
	role_container_sizer = new wxBoxSizer(wxVERTICAL);

	// Context menu helper for lists
	auto setupContextMenu = [this](wxListView* list, wxWindowID addId, wxWindowID delId) {
		list->Bind(wxEVT_CONTEXT_MENU, [this, list, addId, delId](wxContextMenuEvent&) {
			wxMenu menu;
			menu.Append(addId, "Add Offer / Entry");
			menu.Append(delId, "Delete Selected");
			list->PopupMenu(&menu);
		});
	};

	// Role 0: Shopkeeper (Single Unified Trade Table)
	shop_panel = new wxPanel(role_container, wxID_ANY);
	wxBoxSizer* spSizer = new wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer* shopBox = new wxStaticBoxSizer(wxVERTICAL, shop_panel, "Shopkeeper Trade Catalog (Buy & Sell Offers)");
	shop_unified_list = new wxListView(shop_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
	shop_unified_list->InsertColumn(0, "Item ID", wxLIST_FORMAT_RIGHT, 65);
	shop_unified_list->InsertColumn(1, "Item Name", wxLIST_FORMAT_LEFT, 180);
	shop_unified_list->InsertColumn(2, "Trade Mode", wxLIST_FORMAT_CENTRE, 110);
	shop_unified_list->InsertColumn(3, "Buy Price (Gold)", wxLIST_FORMAT_RIGHT, 110);
	shop_unified_list->InsertColumn(4, "Sell Price (Gold)", wxLIST_FORMAT_RIGHT, 110);
	shopBox->Add(shop_unified_list, 1, wxEXPAND | wxBOTTOM, 6);

	wxBoxSizer* addShopSizer = new wxBoxSizer(wxHORIZONTAL);
	wxButton* pickShopBtn = new wxButton(shop_panel, wxID_ANY, "Select..", wxDefaultPosition, wxSize(65, -1));
	pickShopBtn->SetBackgroundColour(wxColour(40, 70, 120));
	pickShopBtn->SetForegroundColour(*wxWHITE);
	pickShopBtn->Bind(wxEVT_BUTTON, &NPCWizardDialog::OnSelectShopItem, this);
	addShopSizer->Add(pickShopBtn, 0, wxRIGHT, 6);

	shop_selected_item_id = 2120; // default rope
	shop_item_label = new wxStaticText(shop_panel, wxID_ANY, "Item: rope (ID 2120)");
	addShopSizer->Add(shop_item_label, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	shop_chk_buy = new wxCheckBox(shop_panel, wxID_ANY, "Buy:");
	shop_chk_buy->SetValue(true);
	addShopSizer->Add(shop_chk_buy, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

	shop_buy_price_spin = new wxSpinCtrl(shop_panel, wxID_ANY, "50", wxDefaultPosition, wxSize(75, -1), wxSP_ARROW_KEYS, 1, 999999, 50);
	addShopSizer->Add(shop_buy_price_spin, 0, wxRIGHT, 8);

	shop_chk_sell = new wxCheckBox(shop_panel, wxID_ANY, "Sell:");
	shop_chk_sell->SetValue(false);
	addShopSizer->Add(shop_chk_sell, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

	shop_sell_price_spin = new wxSpinCtrl(shop_panel, wxID_ANY, "25", wxDefaultPosition, wxSize(75, -1), wxSP_ARROW_KEYS, 1, 999999, 25);
	addShopSizer->Add(shop_sell_price_spin, 0, wxRIGHT, 8);

	wxButton* addShopBtn = new wxButton(shop_panel, wxID_ANY, "+ Add / Update");
	addShopBtn->SetBackgroundColour(wxColour(40, 120, 60));
	addShopBtn->SetForegroundColour(*wxWHITE);
	addShopBtn->Bind(wxEVT_BUTTON, &NPCWizardDialog::OnAddShopOffer, this);
	addShopSizer->Add(addShopBtn, 0, wxRIGHT, 4);

	wxButton* remShopBtn = new wxButton(shop_panel, wxID_ANY, "- Delete");
	remShopBtn->Bind(wxEVT_BUTTON, &NPCWizardDialog::OnRemoveShopOffer, this);
	addShopSizer->Add(remShopBtn, 0);

	shopBox->Add(addShopSizer, 0, wxEXPAND);
	spSizer->Add(shopBox, 1, wxEXPAND | wxALL, 4);

	shop_panel->SetSizer(spSizer);
	role_container_sizer->Add(shop_panel, 1, wxEXPAND);

	// Context menus for shop list
	shop_unified_list->Bind(wxEVT_CONTEXT_MENU, [this](wxContextMenuEvent&) {
		long sel = shop_unified_list->GetFirstSelected();
		wxMenu menu;
		if (sel != -1) {
			menu.Append(2001, "Edit Selected Offer");
			menu.Append(2002, "Delete Selected Offer");
			menu.Bind(wxEVT_MENU, [this](wxCommandEvent& e) {
				if (e.GetId() == 2001) {
					wxCommandEvent dummy;
					OnEditShopOffer(dummy);
				} else if (e.GetId() == 2002) {
					wxCommandEvent dummy;
					OnRemoveShopOffer(dummy);
				}
			});
		} else {
			menu.Append(2003, "Select..");
			menu.Bind(wxEVT_MENU, [this](wxCommandEvent& e) {
				wxCommandEvent dummy;
				OnSelectShopItem(dummy);
			});
		}
		shop_unified_list->PopupMenu(&menu);
	});

	shop_unified_list->Bind(wxEVT_LIST_ITEM_SELECTED, [this](wxListEvent& event) {
		long idx = event.GetIndex();
		if (idx >= 0 && idx < static_cast<long>(shop_offers.size())) {
			const auto& o = shop_offers[idx];
			shop_selected_item_id = o.id;
			shop_item_label->SetLabel(wxString::Format("Item: %s (ID %d)", o.name, o.id));
			shop_chk_buy->SetValue(o.is_buy);
			shop_buy_price_spin->SetValue(o.buy_price);
			shop_chk_sell->SetValue(o.is_sell);
			shop_sell_price_spin->SetValue(o.sell_price);
		}
	});

	// Role 1: Ship Captain
	ship_panel = new wxPanel(role_container, wxID_ANY);
	wxStaticBoxSizer* shipBox = new wxStaticBoxSizer(wxVERTICAL, ship_panel, "Travel Routes & Destinations");
	ship_routes_list = new wxListView(ship_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
	ship_routes_list->InsertColumn(0, "Destination Town", wxLIST_FORMAT_LEFT, 160);
	ship_routes_list->InsertColumn(1, "Target Coords (X, Y, Z)", wxLIST_FORMAT_LEFT, 160);
	ship_routes_list->InsertColumn(2, "Cost (Gold)", wxLIST_FORMAT_RIGHT, 100);
	shipBox->Add(ship_routes_list, 1, wxEXPAND | wxBOTTOM, 8);

	wxBoxSizer* addRouteSizer = new wxBoxSizer(wxHORIZONTAL);
	ship_dest_choice = new wxChoice(ship_panel, wxID_ANY);
	PopulateTownChoices(ship_dest_choice);
	ship_pos_x_spin = new wxSpinCtrl(ship_panel, wxID_ANY, "1000", wxDefaultPosition, wxSize(70, -1), wxSP_ARROW_KEYS, 0, 65535, 1000);
	ship_pos_y_spin = new wxSpinCtrl(ship_panel, wxID_ANY, "1000", wxDefaultPosition, wxSize(70, -1), wxSP_ARROW_KEYS, 0, 65535, 1000);
	ship_pos_z_spin = new wxSpinCtrl(ship_panel, wxID_ANY, "7", wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, 0, 15, 7);
	ship_cost_spin = new wxSpinCtrl(ship_panel, wxID_ANY, "100", wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, 0, 999999, 100);
	wxButton* addRouteBtn = new wxButton(ship_panel, ID_NPC_ADD_ROUTE, "+ Add Route");
	wxButton* remRouteBtn = new wxButton(ship_panel, ID_NPC_REMOVE_ROUTE, "- Delete");

	addRouteSizer->Add(ship_dest_choice, 1, wxRIGHT, 4);
	addRouteSizer->Add(new wxStaticText(ship_panel, wxID_ANY, "Pos:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
	addRouteSizer->Add(ship_pos_x_spin, 0, wxRIGHT, 2);
	addRouteSizer->Add(ship_pos_y_spin, 0, wxRIGHT, 2);
	addRouteSizer->Add(ship_pos_z_spin, 0, wxRIGHT, 4);
	addRouteSizer->Add(new wxStaticText(ship_panel, wxID_ANY, "Gold:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
	addRouteSizer->Add(ship_cost_spin, 0, wxRIGHT, 4);
	addRouteSizer->Add(addRouteBtn, 0, wxRIGHT, 2);
	addRouteSizer->Add(remRouteBtn, 0);
	shipBox->Add(addRouteSizer, 0, wxEXPAND);
	ship_panel->SetSizer(shipBox);
	role_container_sizer->Add(ship_panel, 1, wxEXPAND);
	ship_panel->Hide();

	setupContextMenu(ship_routes_list, ID_NPC_ADD_ROUTE, ID_NPC_REMOVE_ROUTE);

	// Role 2: Temple Healer
	heal_panel = new wxPanel(role_container, wxID_ANY);
	wxStaticBoxSizer* healBox = new wxStaticBoxSizer(wxVERTICAL, heal_panel, "Temple Healer Parameters");
	wxFlexGridSizer* healGrid = new wxFlexGridSizer(4, 2, 8, 12);
	healGrid->AddGrowableCol(1, 1);

	healGrid->Add(new wxStaticText(heal_panel, wxID_ANY, "Heal Amount (% Max HP):"), 0, wxALIGN_CENTER_VERTICAL);
	heal_percent_spin = new wxSpinCtrl(heal_panel, wxID_ANY, "100", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 100);
	healGrid->Add(heal_percent_spin, 1, wxEXPAND);

	healGrid->Add(new wxStaticText(heal_panel, wxID_ANY, "Cost (Gold Coins):"), 0, wxALIGN_CENTER_VERTICAL);
	heal_cost_spin = new wxSpinCtrl(heal_panel, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 999999, 0);
	healGrid->Add(heal_cost_spin, 1, wxEXPAND);

	healGrid->Add(new wxStaticText(heal_panel, wxID_ANY, "Cure Conditions:"), 0, wxALIGN_CENTER_VERTICAL);
	heal_cure_cb = new wxCheckBox(heal_panel, wxID_ANY, "Cure Poison, Fire, Energy & Curse conditions");
	heal_cure_cb->SetValue(true);
	healGrid->Add(heal_cure_cb, 1, wxALIGN_CENTER_VERTICAL);

	healGrid->Add(new wxStaticText(heal_panel, wxID_ANY, "Blessing Dialogue:"), 0, wxALIGN_CENTER_VERTICAL);
	heal_msg_ctrl = new wxTextCtrl(heal_panel, wxID_ANY, "You are healed, child.");
	healGrid->Add(heal_msg_ctrl, 1, wxEXPAND);

	healBox->Add(healGrid, 1, wxEXPAND | wxALL, 6);
	heal_panel->SetSizer(healBox);
	role_container_sizer->Add(heal_panel, 1, wxEXPAND);
	heal_panel->Hide();

	// Role 3: Town Resident Registrar
	resident_panel = new wxPanel(role_container, wxID_ANY);
	wxStaticBoxSizer* resBox = new wxStaticBoxSizer(wxVERTICAL, resident_panel, "Citizenship & Town Registrar");
	wxFlexGridSizer* resGrid = new wxFlexGridSizer(3, 2, 8, 12);
	resGrid->AddGrowableCol(1, 1);

	resGrid->Add(new wxStaticText(resident_panel, wxID_ANY, "Grant Citizenship of Town:"), 0, wxALIGN_CENTER_VERTICAL);
	resident_town_choice = new wxChoice(resident_panel, wxID_ANY);
	PopulateTownChoices(resident_town_choice);
	resGrid->Add(resident_town_choice, 1, wxEXPAND);

	resGrid->Add(new wxStaticText(resident_panel, wxID_ANY, "Citizenship Fee (Gold Coins):"), 0, wxALIGN_CENTER_VERTICAL);
	resident_cost_spin = new wxSpinCtrl(resident_panel, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 999999, 0);
	resGrid->Add(resident_cost_spin, 1, wxEXPAND);

	resGrid->Add(new wxStaticText(resident_panel, wxID_ANY, "Registration Dialogue:"), 0, wxALIGN_CENTER_VERTICAL);
	resident_msg_ctrl = new wxTextCtrl(resident_panel, wxID_ANY, "Welcome! You are now a proud citizen of our town.");
	resGrid->Add(resident_msg_ctrl, 1, wxEXPAND);

	resBox->Add(resGrid, 1, wxEXPAND | wxALL, 6);
	resident_panel->SetSizer(resBox);
	role_container_sizer->Add(resident_panel, 1, wxEXPAND);
	resident_panel->Hide();

	// Role 4: Quest Giver & Task Master
	quest_panel = new wxPanel(role_container, wxID_ANY);
	wxBoxSizer* qMainSizer = new wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer* qParamsBox = new wxStaticBoxSizer(wxHORIZONTAL, quest_panel, "Quest Storage & Conditions");
	wxFlexGridSizer* qGrid = new wxFlexGridSizer(2, 4, 6, 10);
	qGrid->AddGrowableCol(1, 1);
	qGrid->AddGrowableCol(3, 1);

	qGrid->Add(new wxStaticText(quest_panel, wxID_ANY, "Storage Key:"), 0, wxALIGN_CENTER_VERTICAL);
	quest_storage_spin = new wxSpinCtrl(quest_panel, wxID_ANY, "50001", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 999999, 50001);
	qGrid->Add(quest_storage_spin, 1, wxEXPAND);

	qGrid->Add(new wxStaticText(quest_panel, wxID_ANY, "Done Value:"), 0, wxALIGN_CENTER_VERTICAL);
	quest_val_spin = new wxSpinCtrl(quest_panel, wxID_ANY, "1", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 1);
	qGrid->Add(quest_val_spin, 1, wxEXPAND);

	qGrid->Add(new wxStaticText(quest_panel, wxID_ANY, "Reward EXP:"), 0, wxALIGN_CENTER_VERTICAL);
	quest_exp_spin = new wxSpinCtrl(quest_panel, wxID_ANY, "5000", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 9999999, 5000);
	qGrid->Add(quest_exp_spin, 1, wxEXPAND);

	qGrid->Add(new wxStaticText(quest_panel, wxID_ANY, "Reward Gold:"), 0, wxALIGN_CENTER_VERTICAL);
	quest_gold_spin = new wxSpinCtrl(quest_panel, wxID_ANY, "1000", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 9999999, 1000);
	qGrid->Add(quest_gold_spin, 1, wxEXPAND);

	qParamsBox->Add(qGrid, 1, wxEXPAND | wxALL, 4);
	qMainSizer->Add(qParamsBox, 0, wxEXPAND | wxBOTTOM, 6);

	// Reward Item Grid
	wxStaticBoxSizer* rItemBox = new wxStaticBoxSizer(wxVERTICAL, quest_panel, "Reward Items");
	quest_rewards_list = new wxListView(quest_panel, wxID_ANY, wxDefaultPosition, wxSize(-1, 80), wxLC_REPORT | wxLC_SINGLE_SEL);
	quest_rewards_list->InsertColumn(0, "Item Name", wxLIST_FORMAT_LEFT, 180);
	quest_rewards_list->InsertColumn(1, "Count", wxLIST_FORMAT_RIGHT, 80);
	rItemBox->Add(quest_rewards_list, 1, wxEXPAND | wxBOTTOM, 4);

	wxBoxSizer* addRItemSizer = new wxBoxSizer(wxHORIZONTAL);
	wxButton* pickRewardBtn = new wxButton(quest_panel, wxID_ANY, "Select..", wxDefaultPosition, wxSize(65, -1));
	pickRewardBtn->SetBackgroundColour(wxColour(40, 70, 120));
	pickRewardBtn->SetForegroundColour(*wxWHITE);
	pickRewardBtn->Bind(wxEVT_BUTTON, &NPCWizardDialog::OnSelectRewardItem, this);

	quest_reward_item_label = new wxStaticText(quest_panel, wxID_ANY, "Item: crystal coin (ID 2160)");
	quest_item_count_spin = new wxSpinCtrl(quest_panel, wxID_ANY, "1", wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS, 1, 100, 1);
	wxButton* addRItemBtn = new wxButton(quest_panel, ID_NPC_ADD_REWARD_ITEM, "+ Add Reward");
	wxButton* remRItemBtn = new wxButton(quest_panel, ID_NPC_REMOVE_REWARD_ITEM, "- Del");

	addRItemSizer->Add(pickRewardBtn, 0, wxRIGHT, 4);
	addRItemSizer->Add(quest_reward_item_label, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	addRItemSizer->Add(quest_item_count_spin, 0, wxRIGHT, 4);
	addRItemSizer->Add(addRItemBtn, 0, wxRIGHT, 2);
	addRItemSizer->Add(remRItemBtn, 0);
	rItemBox->Add(addRItemSizer, 0, wxEXPAND);
	qMainSizer->Add(rItemBox, 0, wxEXPAND | wxBOTTOM, 6);

	setupContextMenu(quest_rewards_list, ID_NPC_ADD_REWARD_ITEM, ID_NPC_REMOVE_REWARD_ITEM);

	// Dialogues
	wxStaticBoxSizer* qDiagBox = new wxStaticBoxSizer(wxVERTICAL, quest_panel, "Quest Dialogue Messages");
	wxFlexGridSizer* qDiagGrid = new wxFlexGridSizer(3, 2, 4, 8);
	qDiagGrid->AddGrowableCol(1, 1);

	qDiagGrid->Add(new wxStaticText(quest_panel, wxID_ANY, "Quest Offer:"), 0, wxALIGN_CENTER_VERTICAL);
	quest_offer_text = new wxTextCtrl(quest_panel, wxID_ANY, "Travel north and clear the dungeon for me, and I will reward you.");
	qDiagGrid->Add(quest_offer_text, 1, wxEXPAND);

	qDiagGrid->Add(new wxStaticText(quest_panel, wxID_ANY, "In-Progress:"), 0, wxALIGN_CENTER_VERTICAL);
	quest_prog_text = new wxTextCtrl(quest_panel, wxID_ANY, "Have you slain the dungeon beasts yet? Hurry!");
	qDiagGrid->Add(quest_prog_text, 1, wxEXPAND);

	qDiagGrid->Add(new wxStaticText(quest_panel, wxID_ANY, "Completion:"), 0, wxALIGN_CENTER_VERTICAL);
	quest_done_text = new wxTextCtrl(quest_panel, wxID_ANY, "Incredible! Here is your well-deserved reward.");
	qDiagGrid->Add(quest_done_text, 1, wxEXPAND);

	qDiagBox->Add(qDiagGrid, 1, wxEXPAND | wxALL, 4);
	qMainSizer->Add(qDiagBox, 1, wxEXPAND);

	quest_panel->SetSizer(qMainSizer);
	role_container_sizer->Add(quest_panel, 1, wxEXPAND);
	quest_panel->Hide();

	// Role 5: Custom Dialogue & Lore NPC
	dialogue_panel = new wxPanel(role_container, wxID_ANY);
	wxStaticBoxSizer* diagBox = new wxStaticBoxSizer(wxVERTICAL, dialogue_panel, "Keyword Dialogues");
	dialogue_list = new wxListView(dialogue_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
	dialogue_list->InsertColumn(0, "Keyword(s)", wxLIST_FORMAT_LEFT, 140);
	dialogue_list->InsertColumn(1, "NPC Reply Text", wxLIST_FORMAT_LEFT, 320);
	diagBox->Add(dialogue_list, 1, wxEXPAND | wxBOTTOM, 6);

	wxBoxSizer* addDiagSizer = new wxBoxSizer(wxHORIZONTAL);
	diag_kw_ctrl = new wxTextCtrl(dialogue_panel, wxID_ANY, "lore, history");
	diag_reply_ctrl = new wxTextCtrl(dialogue_panel, wxID_ANY, "This land was founded hundreds of years ago by ancient explorers.");
	wxButton* addDiagBtn = new wxButton(dialogue_panel, ID_NPC_ADD_DIAG, "+ Add Line");
	wxButton* remDiagBtn = new wxButton(dialogue_panel, ID_NPC_REMOVE_DIAG, "- Delete");

	addDiagSizer->Add(diag_kw_ctrl, 1, wxRIGHT, 4);
	addDiagSizer->Add(diag_reply_ctrl, 2, wxRIGHT, 4);
	addDiagSizer->Add(addDiagBtn, 0, wxRIGHT, 2);
	addDiagSizer->Add(remDiagBtn, 0);
	diagBox->Add(addDiagSizer, 0, wxEXPAND);

	dialogue_panel->SetSizer(diagBox);
	role_container_sizer->Add(dialogue_panel, 1, wxEXPAND);
	dialogue_panel->Hide();

	setupContextMenu(dialogue_list, ID_NPC_ADD_DIAG, ID_NPC_REMOVE_DIAG);

	role_container->SetSizer(role_container_sizer);
	notebook->AddPage(role_container, "Config");

	rootSizer->Add(notebook, 1, wxALL | wxEXPAND, 8);

	// Bottom Bar
	wxBoxSizer* bottomSizer = new wxBoxSizer(wxHORIZONTAL);

	wxButton* saveFileBtn = new wxButton(this, ID_NPC_BTN_SAVE_FILE, "Export Script / XML...");
	saveFileBtn->SetBackgroundColour(wxColour(40, 70, 120));
	saveFileBtn->SetForegroundColour(*wxWHITE);

	wxButton* registerBtn = new wxButton(this, ID_NPC_BTN_REGISTER_PALETTE, "Add to Creature Palette");
	registerBtn->SetBackgroundColour(wxColour(200, 140, 30));
	registerBtn->SetForegroundColour(*wxWHITE);

	wxButton* closeBtn = new wxButton(this, wxID_CANCEL, "Close");

	bottomSizer->Add(saveFileBtn, 0, wxRIGHT, 8);
	bottomSizer->Add(registerBtn, 0, wxRIGHT, 8);
	bottomSizer->AddStretchSpacer();
	bottomSizer->Add(closeBtn, 0);

	rootSizer->Add(bottomSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

	SetSizer(rootSizer);
	RME::UI::StyleManager::ApplyThemeRecursively(this, RME::UI::StyleManager::GetTheme());
	Layout();
	CenterOnParent();

	// Populate Default Demo Offers
	shop_offers.push_back({ 2120, "rope", true, 50, false, 0 });
	shop_offers.push_back({ 2554, "shovel", true, 50, false, 0 });
	shop_offers.push_back({ 2050, "torch", true, 8, false, 0 });
	shop_offers.push_back({ 2148, "gold coin", false, 0, true, 1 });
	shop_offers.push_back({ 2152, "platinum coin", true, 100, true, 100 });
	for (const auto& o : shop_offers) {
		long idx = shop_unified_list->InsertItem(shop_unified_list->GetItemCount(), std::to_string(o.id));
		shop_unified_list->SetItem(idx, 1, o.name);
		shop_unified_list->SetItem(idx, 2, (o.is_buy && o.is_sell) ? "Buy & Sell" : (o.is_buy ? "Buy Only" : "Sell Only"));
		shop_unified_list->SetItem(idx, 3, o.is_buy ? std::to_string(o.buy_price) : "-");
		shop_unified_list->SetItem(idx, 4, o.is_sell ? std::to_string(o.sell_price) : "-");
	}

	travel_routes.push_back({ "Carlin", 32360, 31782, 7, 100, "Do you want to sail to Carlin for 100 gold coins?" });
	travel_routes.push_back({ "Venore", 32954, 32022, 7, 120, "Do you want to sail to Venore for 120 gold coins?" });
	for (const auto& r : travel_routes) {
		long idx = ship_routes_list->InsertItem(ship_routes_list->GetItemCount(), r.town);
		ship_routes_list->SetItem(idx, 1, wxString::Format("%d, %d, %d", r.x, r.y, r.z));
		ship_routes_list->SetItem(idx, 2, std::to_string(r.cost));
	}

	custom_dialogues.push_back({ "job, work", "I am the local guide and merchant around here." });
	custom_dialogues.push_back({ "quest, mission", "Talk to the mayor in town if you are seeking grand adventures." });
	for (const auto& d : custom_dialogues) {
		long idx = dialogue_list->InsertItem(dialogue_list->GetItemCount(), d.keyword);
		dialogue_list->SetItem(idx, 1, d.reply);
	}

	UpdateChannelButtonStyles();
	UpdateOutfitPreview();
}

NPCWizardDialog::~NPCWizardDialog() {
	if (npc_step_timer) {
		if (npc_step_timer->IsRunning()) {
			npc_step_timer->Stop();
		}
		delete npc_step_timer;
		npc_step_timer = nullptr;
	}
}

void NPCWizardDialog::OnGenderFilterChanged(wxCommandEvent& WXUNUSED(event)) {
	RepopulatePresets();
}

void NPCWizardDialog::RepopulatePresets() {
	int filter = tlg_gender_filter ? tlg_gender_filter->GetSelection() : 0; // 0=All, 1=Male, 2=Female

	struct LookPreset { std::string name; int id; };
	static const LookPreset all_presets[] = {
		{ "Citizen (Male)", 128 }, { "Citizen (Female)", 136 },
		{ "Hunter (Male)", 129 }, { "Hunter (Female)", 137 },
		{ "Mage (Male)", 130 }, { "Mage (Female)", 138 },
		{ "Knight (Male)", 131 }, { "Knight (Female)", 139 },
		{ "Nobleman (Male)", 132 }, { "Noblewoman (Female)", 140 },
		{ "Summoner (Male)", 133 }, { "Summoner (Female)", 141 },
		{ "Warrior (Male)", 134 }, { "Warrior (Female)", 142 },
		{ "Barbarian (Male)", 143 }, { "Barbarian (Female)", 147 },
		{ "Druid (Male)", 144 }, { "Druid (Female)", 148 },
		{ "Wizard (Male)", 145 }, { "Wizard (Female)", 149 },
		{ "Oriental (Male)", 146 }, { "Oriental (Female)", 150 },
		{ "Pirate (Male)", 151 }, { "Pirate (Female)", 155 },
		{ "Assassin (Male)", 152 }, { "Assassin (Female)", 156 },
		{ "Beggar (Male)", 153 }, { "Beggar (Female)", 157 },
		{ "Shaman (Male)", 154 }, { "Shaman (Female)", 158 },
		{ "Norseman (Male)", 251 }, { "Norsewoman (Female)", 252 },
		{ "Nightmare (Male)", 268 }, { "Nightmare (Female)", 269 },
		{ "Jester (Male)", 273 }, { "Jester (Female)", 270 },
		{ "Brotherhood (Male)", 278 }, { "Brotherhood (Female)", 279 },
		{ "Demon Hunter (Male)", 289 }, { "Demon Hunter (Female)", 288 },
		{ "Yalaharian (Male)", 325 }, { "Yalaharian (Female)", 324 }
	};

	int prev_looktype = outfit_looktype;
	tlg_preset_choice->Clear();
	int restore_sel = -1;
	int idx = 0;

	for (const auto& p : all_presets) {
		bool is_male = p.name.find("(Male)") != std::string::npos;
		bool is_female = p.name.find("(Female)") != std::string::npos;

		if (filter == 1 && !is_male) continue;
		if (filter == 2 && !is_female) continue;

		tlg_preset_choice->Append(p.name, (void*)(intptr_t)p.id);
		if (p.id == prev_looktype) {
			restore_sel = idx;
		}
		idx++;
	}

	if (restore_sel >= 0) {
		tlg_preset_choice->SetSelection(restore_sel);
	} else if (tlg_preset_choice->GetCount() > 0) {
		tlg_preset_choice->SetSelection(0);
		outfit_looktype = static_cast<int>(reinterpret_cast<intptr_t>(tlg_preset_choice->GetClientData(0)));
		UpdateOutfitPreview();
	}
}

void NPCWizardDialog::OnSelectRewardItem(wxCommandEvent& WXUNUSED(event)) {
	FindItemDialog dlg(this, "Select Item", true);
	if (dlg.ShowModal() == wxID_OK) {
		uint16_t id = dlg.getResultID();
		if (id > 0) {
			quest_reward_selected_item_id = id;
			ItemType& it = g_items[id];
			quest_reward_item_label->SetLabel(wxString::Format("Item: %s (ID %d)", it.name, id));
		}
	}
}

void NPCWizardDialog::PopulateTownChoices(wxChoice* choice) {
	choice->Clear();
	if (g_gui.GetCurrentEditor()) {
		for (const auto& pair : g_gui.GetCurrentEditor()->map.towns) {
			choice->Append(pair.second->getName(), (void*)(uintptr_t)pair.second->getID());
		}
	}
	if (choice->GetCount() == 0) {
		choice->Append("Thais");
		choice->Append("Carlin");
		choice->Append("Venore");
		choice->Append("Edron");
	}
	choice->SetSelection(0);
}

void NPCWizardDialog::OnRoleChanged(wxCommandEvent& WXUNUSED(event)) {
	int role = npc_role_choice->GetSelection();
	shop_panel->Show(role == 0);
	ship_panel->Show(role == 1);
	heal_panel->Show(role == 2);
	resident_panel->Show(role == 3);
	quest_panel->Show(role == 4);
	dialogue_panel->Show(role == 5);
	role_container_sizer->Layout();
}

void NPCWizardDialog::OnPresetPrev(wxCommandEvent& WXUNUSED(event)) {
	int cur = tlg_preset_choice->GetSelection();
	if (cur > 0) {
		tlg_preset_choice->SetSelection(cur - 1);
		wxCommandEvent dummy;
		OnPresetChoice(dummy);
	}
}

void NPCWizardDialog::OnPresetNext(wxCommandEvent& WXUNUSED(event)) {
	int cur = tlg_preset_choice->GetSelection();
	if (cur < static_cast<int>(tlg_preset_choice->GetCount()) - 1) {
		tlg_preset_choice->SetSelection(cur + 1);
		wxCommandEvent dummy;
		OnPresetChoice(dummy);
	}
}

void NPCWizardDialog::OnPresetChoice(wxCommandEvent& WXUNUSED(event)) {
	int sel = tlg_preset_choice->GetSelection();
	if (sel != wxNOT_FOUND) {
		outfit_looktype = static_cast<int>(reinterpret_cast<intptr_t>(tlg_preset_choice->GetClientData(sel)));
		UpdateOutfitPreview();
	}
}

void NPCWizardDialog::OnOutfitParamChanged(wxCommandEvent& event) {
	if (event.GetId() == ID_NPC_TLG_PALETTE) {
		int col_id = event.GetInt();
		if (active_channel == CHANNEL_HEAD) outfit_head = col_id;
		else if (active_channel == CHANNEL_PRIMARY) outfit_body = col_id;
		else if (active_channel == CHANNEL_SECONDARY) outfit_legs = col_id;
		else if (active_channel == CHANNEL_DETAIL) outfit_feet = col_id;
	}

	outfit_addons = 0;
	outfit_has_mount = false;
	outfit_mount = 0;

	UpdateColorPreviews();
	UpdateOutfitPreview();
}

void NPCWizardDialog::OnRotate(wxCommandEvent& WXUNUSED(event)) {
	if (tlg_preview_panel) {
		tlg_preview_panel->RotateDirection();
	}
}

void NPCWizardDialog::OnToggleView(wxCommandEvent& WXUNUSED(event)) {
	if (tlg_preview_panel) {
		tlg_preview_panel->ToggleFrontBack();
		tlg_btn_view->SetLabel(tlg_preview_panel->GetDirection() == 2 ? "Back" : "Front");
	}
}

void NPCWizardDialog::OnAnimate(wxCommandEvent& WXUNUSED(event)) {
	npc_is_stepping_loop = !npc_is_stepping_loop;
	if (npc_is_stepping_loop) {
		if (npc_step_timer) npc_step_timer->Start(200);
		if (tlg_btn_animate) {
			tlg_btn_animate->SetLabel("Stop");
			tlg_btn_animate->SetBackgroundColour(wxColour(180, 50, 50));
			tlg_btn_animate->SetForegroundColour(*wxWHITE);
		}
	} else {
		if (npc_step_timer) npc_step_timer->Stop();
		if (tlg_btn_animate) {
			tlg_btn_animate->SetLabel("Step");
			tlg_btn_animate->SetBackgroundColour(wxNullColour);
			tlg_btn_animate->SetForegroundColour(wxNullColour);
		}
	}
}

void NPCWizardDialog::OnStepTimer(wxTimerEvent& WXUNUSED(event)) {
	if (tlg_preview_panel) {
		tlg_preview_panel->StepFrame();
	}
}

void NPCWizardDialog::OnRandomizeColors(wxCommandEvent& WXUNUSED(event)) {
	outfit_head = rand() % 133;
	outfit_body = rand() % 133;
	outfit_legs = rand() % 133;
	outfit_feet = rand() % 133;

	if (active_channel == CHANNEL_HEAD) tlg_palette->SetSelectedColorId(outfit_head);
	else if (active_channel == CHANNEL_PRIMARY) tlg_palette->SetSelectedColorId(outfit_body);
	else if (active_channel == CHANNEL_SECONDARY) tlg_palette->SetSelectedColorId(outfit_legs);
	else if (active_channel == CHANNEL_DETAIL) tlg_palette->SetSelectedColorId(outfit_feet);

	UpdateColorPreviews();
	UpdateOutfitPreview();
}

void NPCWizardDialog::UpdateOutfitPreview() {
	if (tlg_preview_panel) {
		tlg_preview_panel->SetOutfit(outfit_looktype, outfit_head, outfit_body, outfit_legs, outfit_feet, outfit_addons, 0, false);
	}
	live_code_ctrl->SetValue(BuildLiveCode());
}

void NPCWizardDialog::OnChannelSelect(wxCommandEvent& event) {
	int id = event.GetId();
	if (id == ID_NPC_TLG_BTN_HEAD) {
		active_channel = CHANNEL_HEAD;
		tlg_palette->SetSelectedColorId(outfit_head);
	} else if (id == ID_NPC_TLG_BTN_PRIMARY) {
		active_channel = CHANNEL_PRIMARY;
		tlg_palette->SetSelectedColorId(outfit_body);
	} else if (id == ID_NPC_TLG_BTN_SECONDARY) {
		active_channel = CHANNEL_SECONDARY;
		tlg_palette->SetSelectedColorId(outfit_legs);
	} else if (id == ID_NPC_TLG_BTN_DETAIL) {
		active_channel = CHANNEL_DETAIL;
		tlg_palette->SetSelectedColorId(outfit_feet);
	}
	UpdateChannelButtonStyles();
}

void NPCWizardDialog::UpdateChannelButtonStyles() {
	auto setStyle = [](wxButton* btn, bool active) {
		if (active) {
			btn->SetBackgroundColour(wxColour(200, 140, 30));
			btn->SetForegroundColour(*wxWHITE);
		} else {
			btn->SetBackgroundColour(wxColour(45, 55, 75));
			btn->SetForegroundColour(wxColour(200, 205, 215));
		}
		btn->Refresh();
	};

	setStyle(tlg_btn_head, active_channel == CHANNEL_HEAD);
	setStyle(tlg_btn_primary, active_channel == CHANNEL_PRIMARY);
	setStyle(tlg_btn_secondary, active_channel == CHANNEL_SECONDARY);
	setStyle(tlg_btn_detail, active_channel == CHANNEL_DETAIL);

	UpdateColorPreviews();
}

void NPCWizardDialog::UpdateColorPreviews() {
	auto setPreview = [](wxPanel* panel, int colorId) {
		if (panel) {
			panel->SetBackgroundColour(GetTibiaColour(colorId));
			panel->Refresh();
		}
	};
	setPreview(tlg_col_preview_head, outfit_head);
	setPreview(tlg_col_preview_primary, outfit_body);
	setPreview(tlg_col_preview_secondary, outfit_legs);
	setPreview(tlg_col_preview_detail, outfit_feet);
}

std::string NPCWizardDialog::BuildLiveCode() {
	std::ostringstream ss;
	ss << "look type=\"" << outfit_looktype << "\" head=\"" << outfit_head << "\" body=\"" << outfit_body << "\" legs=\"" << outfit_legs << "\" feet=\"" << outfit_feet << "\"";
	return ss.str();
}

void NPCWizardDialog::OnSelectShopItem(wxCommandEvent& WXUNUSED(event)) {
	FindItemDialog dlg(this, "Select Item", true);
	if (dlg.ShowModal() == wxID_OK) {
		uint16_t id = dlg.getResultID();
		if (id > 0) {
			shop_selected_item_id = id;
			ItemType& it = g_items[id];
			shop_item_label->SetLabel(wxString::Format("Item: %s (ID %d)", it.name, id));
		}
	}
}

void NPCWizardDialog::OnAddShopOffer(wxCommandEvent& WXUNUSED(event)) {
	ItemType& it = g_items[shop_selected_item_id];
	std::string name = it.name.empty() ? "Item #" + std::to_string(shop_selected_item_id) : it.name;
	int id = shop_selected_item_id;
	bool is_b = shop_chk_buy->GetValue();
	int b_price = is_b ? shop_buy_price_spin->GetValue() : 0;
	bool is_s = shop_chk_sell->GetValue();
	int s_price = is_s ? shop_sell_price_spin->GetValue() : 0;

	if (!is_b && !is_s) {
		wxMessageBox("Please check at least 'Buy' or 'Sell' for this offer.", "Warning", wxOK | wxICON_WARNING, this);
		return;
	}

	// Check if existing item in list -> update it
	bool updated = false;
	for (size_t i = 0; i < shop_offers.size(); ++i) {
		if (shop_offers[i].id == id) {
			shop_offers[i].is_buy = is_b;
			shop_offers[i].buy_price = b_price;
			shop_offers[i].is_sell = is_s;
			shop_offers[i].sell_price = s_price;
			shop_unified_list->SetItem(i, 2, (is_b && is_s) ? "Buy & Sell" : (is_b ? "Buy Only" : "Sell Only"));
			shop_unified_list->SetItem(i, 3, is_b ? std::to_string(b_price) : "-");
			shop_unified_list->SetItem(i, 4, is_s ? std::to_string(s_price) : "-");
			updated = true;
			break;
		}
	}

	if (!updated) {
		shop_offers.push_back({ id, name, is_b, b_price, is_s, s_price });
		long idx = shop_unified_list->InsertItem(shop_unified_list->GetItemCount(), std::to_string(id));
		shop_unified_list->SetItem(idx, 1, name);
		shop_unified_list->SetItem(idx, 2, (is_b && is_s) ? "Buy & Sell" : (is_b ? "Buy Only" : "Sell Only"));
		shop_unified_list->SetItem(idx, 3, is_b ? std::to_string(b_price) : "-");
		shop_unified_list->SetItem(idx, 4, is_s ? std::to_string(s_price) : "-");
	}
}

void NPCWizardDialog::OnEditShopOffer(wxCommandEvent& WXUNUSED(event)) {
	long sel = shop_unified_list->GetFirstSelected();
	if (sel != -1 && sel < static_cast<long>(shop_offers.size())) {
		const auto& o = shop_offers[sel];
		shop_selected_item_id = o.id;
		shop_item_label->SetLabel(wxString::Format("Item: %s (ID %d)", o.name, o.id));
		shop_chk_buy->SetValue(o.is_buy);
		shop_buy_price_spin->SetValue(o.buy_price);
		shop_chk_sell->SetValue(o.is_sell);
		shop_sell_price_spin->SetValue(o.sell_price);
	}
}

void NPCWizardDialog::OnRemoveShopOffer(wxCommandEvent& WXUNUSED(event)) {
	long sel = shop_unified_list->GetFirstSelected();
	if (sel != -1) {
		shop_unified_list->DeleteItem(sel);
		if (sel < static_cast<long>(shop_offers.size())) {
			shop_offers.erase(shop_offers.begin() + sel);
		}
	}
}

void NPCWizardDialog::OnAddTravelRoute(wxCommandEvent& WXUNUSED(event)) {
	int sel = ship_dest_choice->GetSelection();
	if (sel == wxNOT_FOUND) return;
	std::string town = ship_dest_choice->GetString(sel).ToStdString();
	int x = ship_pos_x_spin->GetValue();
	int y = ship_pos_y_spin->GetValue();
	int z = ship_pos_z_spin->GetValue();
	int cost = ship_cost_spin->GetValue();
	std::string txt = "Do you want to sail to " + town + " for " + std::to_string(cost) + " gold coins?";

	travel_routes.push_back({ town, x, y, z, cost, txt });
	long idx = ship_routes_list->InsertItem(ship_routes_list->GetItemCount(), town);
	ship_routes_list->SetItem(idx, 1, wxString::Format("%d, %d, %d", x, y, z));
	ship_routes_list->SetItem(idx, 2, std::to_string(cost));
}

void NPCWizardDialog::OnRemoveTravelRoute(wxCommandEvent& WXUNUSED(event)) {
	long sel = ship_routes_list->GetFirstSelected();
	if (sel != -1) {
		ship_routes_list->DeleteItem(sel);
		if (sel < static_cast<long>(travel_routes.size())) {
			travel_routes.erase(travel_routes.begin() + sel);
		}
	}
}

void NPCWizardDialog::OnAddDialogue(wxCommandEvent& WXUNUSED(event)) {
	std::string kw = diag_kw_ctrl->GetValue().ToStdString();
	std::string rep = diag_reply_ctrl->GetValue().ToStdString();
	if (kw.empty() || rep.empty()) return;

	custom_dialogues.push_back({ kw, rep });
	long idx = dialogue_list->InsertItem(dialogue_list->GetItemCount(), kw);
	dialogue_list->SetItem(idx, 1, rep);
}

void NPCWizardDialog::OnRemoveDialogue(wxCommandEvent& WXUNUSED(event)) {
	long sel = dialogue_list->GetFirstSelected();
	if (sel != -1) {
		dialogue_list->DeleteItem(sel);
		if (sel < static_cast<long>(custom_dialogues.size())) {
			custom_dialogues.erase(custom_dialogues.begin() + sel);
		}
	}
}

void NPCWizardDialog::OnAddQuestRewardItem(wxCommandEvent& WXUNUSED(event)) {
	ItemType& it = g_items[quest_reward_selected_item_id];
	std::string name = it.name.empty() ? "Item #" + std::to_string(quest_reward_selected_item_id) : it.name;
	int id = quest_reward_selected_item_id;
	int count = quest_item_count_spin->GetValue();

	quest_reward_items.push_back({ id, name, count });
	long idx = quest_rewards_list->InsertItem(quest_rewards_list->GetItemCount(), name);
	quest_rewards_list->SetItem(idx, 1, std::to_string(count));
}

void NPCWizardDialog::OnRemoveQuestRewardItem(wxCommandEvent& WXUNUSED(event)) {
	long sel = quest_rewards_list->GetFirstSelected();
	if (sel != -1) {
		quest_rewards_list->DeleteItem(sel);
		if (sel < static_cast<long>(quest_reward_items.size())) {
			quest_reward_items.erase(quest_reward_items.begin() + sel);
		}
	}
}

std::string NPCWizardDialog::BuildXml() {
	std::string name = npc_name_ctrl->GetValue().ToStdString();
	std::ostringstream xml;
	xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	xml << "<npc name=\"" << name << "\" script=\"data/npc/scripts/" << name << ".lua\" walkinterval=\"2000\" floorchange=\"0\">\n";
	xml << "    <health now=\"100\" max=\"100\"/>\n";
	xml << "    <look type=\"" << outfit_looktype << "\" head=\"" << outfit_head << "\" body=\"" << outfit_body << "\" legs=\"" << outfit_legs << "\" feet=\"" << outfit_feet << "\" addons=\"" << outfit_addons << "\"";
	if (outfit_has_mount) xml << " mount=\"" << outfit_mount << "\"";
	xml << "/>\n";
	xml << "</npc>\n";
	return xml.str();
}

std::string NPCWizardDialog::BuildLua() {
	std::string name = npc_name_ctrl->GetValue().ToStdString();
	int role = npc_role_choice->GetSelection();

	std::ostringstream lua;
	lua << "-- TFS 1.6 / RevScript NPC: " << name << "\n";
	lua << "local internalNpcName = \"" << name << "\"\n";
	lua << "local npcType = Game.createNpcType(internalNpcName)\n";
	lua << "local npcConfig = {}\n\n";
	lua << "npcConfig.name = internalNpcName\n";
	lua << "npcConfig.description = \"" << npc_title_ctrl->GetValue().ToStdString() << "\"\n";
	lua << "npcConfig.experience = 0\n\n";

	lua << "npcType:outfit({lookType = " << outfit_looktype << ", lookHead = " << outfit_head << ", lookBody = " << outfit_body << ", lookLegs = " << outfit_legs << ", lookFeet = " << outfit_feet << ", lookAddons = " << outfit_addons;
	if (outfit_has_mount) lua << ", lookMount = " << outfit_mount;
	lua << "})\n\n";

	lua << "local keywordHandler = KeywordHandler:new()\n";
	lua << "local npcHandler = NpcHandler:new(keywordHandler)\n";
	lua << "NpcSystem.parseScopes(npcHandler)\n\n";

	if (role == 0) { // Shopkeeper
		lua << "npcConfig.shop = {\n";
		for (const auto& o : shop_offers) {
			if (o.is_buy && o.is_sell) {
				lua << "    { itemName = \"" << o.name << "\", clientId = " << o.id << ", buy = " << o.buy_price << ", sell = " << o.sell_price << " },\n";
			} else if (o.is_buy) {
				lua << "    { itemName = \"" << o.name << "\", clientId = " << o.id << ", buy = " << o.buy_price << " },\n";
			} else if (o.is_sell) {
				lua << "    { itemName = \"" << o.name << "\", clientId = " << o.id << ", sell = " << o.sell_price << " },\n";
			}
		}
		lua << "}\n\n";
		lua << "local function onTradeRequest(cid) return true end\n";
		lua << "npcHandler:setCallback(CALLBACK_ONTRADEREQUEST, onTradeRequest)\n";
	} else if (role == 1) { // Ship
		for (const auto& r : travel_routes) {
			lua << "local node = keywordHandler:addKeyword({'travel', '" << r.town << "'}, StdModule.say, {npcHandler = npcHandler, text = '" << r.text << "'})\n";
			lua << "node:addChildKeyword({'yes'}, StdModule.travel, {npcHandler = npcHandler, destination = Position(" << r.x << ", " << r.y << ", " << r.z << "), cost = " << r.cost << "})\n";
		}
	} else if (role == 2) { // Healer
		int pct = heal_percent_spin->GetValue();
		int cost = heal_cost_spin->GetValue();
		std::string msg = heal_msg_ctrl->GetValue().ToStdString();
		lua << "local function creatureSayCallback(cid, type, msgText)\n";
		lua << "    if not npcHandler:isFocused(cid) then return false end\n";
		lua << "    local player = Player(cid)\n";
		lua << "    if msgcontains(msgText, 'heal') then\n";
		if (cost > 0) lua << "        if not player:removeMoney(" << cost << ") then npcHandler:say(\"You need " << cost << " gold for healing.\", cid) return true end\n";
		lua << "        local maxHp = player:getMaxHealth()\n";
		lua << "        player:addHealth(math.floor(maxHp * " << (pct / 100.0) << "))\n";
		if (heal_cure_cb->GetValue()) lua << "        player:removeCondition(CONDITION_POISON)\n        player:removeCondition(CONDITION_FIRE)\n";
		lua << "        npcHandler:say(\"" << msg << "\", cid)\n";
		lua << "    end\n";
		lua << "    return true\n";
		lua << "end\n";
		lua << "npcHandler:setCallback(CALLBACK_MESSAGE_DEFAULT, creatureSayCallback)\n";
	} else if (role == 4) { // Quest
		int storage = quest_storage_spin->GetValue();
		int val = quest_val_spin->GetValue();
		int exp = quest_exp_spin->GetValue();
		int gold = quest_gold_spin->GetValue();
		lua << "local function creatureSayCallback(cid, type, msgText)\n";
		lua << "    if not npcHandler:isFocused(cid) then return false end\n";
		lua << "    local player = Player(cid)\n";
		lua << "    if msgcontains(msgText, 'quest') or msgcontains(msgText, 'mission') then\n";
		lua << "        if player:getStorageValue(" << storage << ") < " << val << " then\n";
		lua << "            player:setStorageValue(" << storage << ", " << val << ")\n";
		if (exp > 0) lua << "            player:addExperience(" << exp << ")\n";
		if (gold > 0) lua << "            player:addMoney(" << gold << ")\n";
		for (const auto& item : quest_reward_items) {
			lua << "            player:addItem(" << item.id << ", " << item.count << ")\n";
		}
		lua << "            npcHandler:say(\"" << quest_done_text->GetValue().ToStdString() << "\", cid)\n";
		lua << "        end\n";
		lua << "    end\n";
		lua << "    return true\n";
		lua << "end\n";
		lua << "npcHandler:setCallback(CALLBACK_MESSAGE_DEFAULT, creatureSayCallback)\n";
	}

	lua << "\nnpcType:register(npcConfig)\n";
	return lua.str();
}

std::string NPCWizardDialog::BuildSrv() {
	std::string name = npc_name_ctrl->GetValue().ToStdString();
	std::ostringstream srv;
	srv << "Type = " << outfit_looktype << "\n";
	srv << "Name = \"" << name << "\"\n";
	srv << "Behaviour = 1\n\n";
	srv << "Speech = {\n";
	srv << "    \"hi\" -> \"Greetings, %NAME%!\",\n";
	srv << "    \"job\" -> \"" << npc_title_ctrl->GetValue().ToStdString() << "\",\n";
	srv << "    \"bye\" -> \"Farewell!\"\n";
	srv << "}\n";
	return srv.str();
}

void NPCWizardDialog::OnSaveFile(wxCommandEvent& WXUNUSED(event)) {
	std::string name = npc_name_ctrl->GetValue().ToStdString();
	wxFileDialog saveDialog(this, "Save NPC File", "", name + ".xml", "OpenTibia NPC XML (*.xml)|*.xml|TFS Lua Scripts (*.lua)|*.lua|RealOTS Server Scripts (*.srv)|*.srv|All files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (saveDialog.ShowModal() == wxID_OK) {
		wxString path = saveDialog.GetPath();
		wxFileOutputStream output(path);
		if (output.IsOk()) {
			std::string content = path.EndsWith(".srv") ? BuildSrv() : (path.EndsWith(".xml") ? BuildXml() : BuildLua());
			output.Write(content.c_str(), content.length());
			wxMessageBox("NPC saved successfully to:\n" + path, "Success", wxOK | wxICON_INFORMATION, this);
		}
	}
}

void NPCWizardDialog::OnRegisterPalette(wxCommandEvent& WXUNUSED(event)) {
	std::string name = npc_name_ctrl->GetValue().ToStdString();
	Outfit outfit;
	outfit.lookType = outfit_looktype;
	outfit.lookItem = 0;
	outfit.lookHead = outfit_head;
	outfit.lookBody = outfit_body;
	outfit.lookLegs = outfit_legs;
	outfit.lookFeet = outfit_feet;
	outfit.lookAddon = outfit_addons;
	if (outfit_has_mount) {
		outfit.lookMount = outfit_mount;
	}

	CreatureType* ct = g_creatures[name];
	if (!ct) {
		ct = g_creatures.addCreatureType(name, true, outfit);
	} else {
		ct->outfit = outfit;
	}

	if (ct) {
		Tileset* tileSet = g_materials.tilesets["NPCs"];
		if (!tileSet) tileSet = g_materials.tilesets["Others"];
		if (tileSet) {
			Brush* brush = newd CreatureBrush(ct);
			g_brushes.addBrush(brush);
			TilesetCategory* cat = tileSet->getCategory(TILESET_CREATURE);
			if (cat) cat->brushlist.push_back(brush);
		}
		g_gui.RefreshPalettes();
	}

	wxMessageBox("NPC '" + name + "' registered directly into the NPC Palette!\nYou can now paint this NPC on the map.", "Palette Registered", wxOK | wxICON_INFORMATION, this);
	EndModal(wxID_OK);
}

void NPCWizardDialog::OnCopyLiveCode(wxCommandEvent& WXUNUSED(event)) {
	if (wxTheClipboard->Open()) {
		wxTheClipboard->SetData(new wxTextDataObject(live_code_ctrl->GetValue()));
		wxTheClipboard->Close();
		g_gui.SetStatusText("Copied LookType code to clipboard!");
	}
}

void NPCWizardDialog::OnClose(wxCommandEvent& WXUNUSED(event)) {
	if (npc_step_timer && npc_step_timer->IsRunning()) {
		npc_step_timer->Stop();
	}
	EndModal(wxID_CANCEL);
}
