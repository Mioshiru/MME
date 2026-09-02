#include "main.h"
#include "creature_wiki_dialog.h"
#include "creature_bestiary.h"
#include "creatures.h"
#include "creature_brush.h"
#include "gui.h"
#include "palette_window.h"
#include "palette_creature.h"
#include "graphics.h"
#include <iomanip>
#include <sstream>
#include <fstream>
#include <algorithm>

enum {
	ID_WIKI_SEARCH = 15000,
	ID_WIKI_CATEGORY,
	ID_WIKI_TIER,
	ID_WIKI_LIST,
	ID_WIKI_PLACE_MAP,
	ID_WIKI_SELECT_PALETTE,
	ID_WIKI_OPEN_WIKI,
	ID_WIKI_FAVORITE
};

BEGIN_EVENT_TABLE(CreatureWikiDialog, wxDialog)
	EVT_TEXT(ID_WIKI_SEARCH, CreatureWikiDialog::OnFilterChangeEvent)
	EVT_CHOICE(ID_WIKI_CATEGORY, CreatureWikiDialog::OnFilterChangeEvent)
	EVT_CHOICE(ID_WIKI_TIER, CreatureWikiDialog::OnFilterChangeEvent)
	EVT_LIST_ITEM_SELECTED(ID_WIKI_LIST, CreatureWikiDialog::OnSelectionChange)
	EVT_LIST_COL_CLICK(ID_WIKI_LIST, CreatureWikiDialog::OnColumnClick)
	EVT_BUTTON(ID_WIKI_PLACE_MAP, CreatureWikiDialog::OnPlaceCreature)
	EVT_BUTTON(ID_WIKI_SELECT_PALETTE, CreatureWikiDialog::OnSelectInPalette)
	EVT_BUTTON(ID_WIKI_OPEN_WIKI, CreatureWikiDialog::OnOpenWikiPage)
	EVT_BUTTON(ID_WIKI_FAVORITE, CreatureWikiDialog::OnToggleFavorite)
END_EVENT_TABLE()

CreatureWikiDialog::CreatureWikiDialog(wxWindow* parent) :
	wxDialog(parent, wxID_ANY, "Creature Wiki & Bestiary Knowledge Base", wxDefaultPosition, wxSize(900, 600),
	         wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX),
	sort_column(1),       // Default sort by Creature Name
	sort_ascending(true)
{
	SetBackgroundColour(wxColor(15, 23, 42));
	LoadFavorites();
	BuildUI();
	PopulateList();
	CenterOnParent();
}

CreatureWikiDialog::~CreatureWikiDialog() {
	SaveFavorites();
}

void CreatureWikiDialog::LoadFavorites() {
	m_favorite_creatures.clear();
	std::ifstream in("creature_favorites.cfg");
	if (in.is_open()) {
		std::string line;
		while (std::getline(in, line)) {
			while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) {
				line.pop_back();
			}
			if (!line.empty()) {
				std::string low = line;
				std::transform(low.begin(), low.end(), low.begin(), [](unsigned char c) { return (char)std::tolower(c); });
				m_favorite_creatures.insert(low);
			}
		}
		in.close();
	}
}

void CreatureWikiDialog::SaveFavorites() {
	std::ofstream out("creature_favorites.cfg");
	if (out.is_open()) {
		for (const auto& fav : m_favorite_creatures) {
			out << fav << "\n";
		}
		out.close();
	}
}

bool CreatureWikiDialog::IsFavorite(const std::string& name) const {
	std::string low = name;
	std::transform(low.begin(), low.end(), low.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	return m_favorite_creatures.find(low) != m_favorite_creatures.end();
}

void CreatureWikiDialog::SetFavorite(const std::string& name, bool fav) {
	std::string low = name;
	std::transform(low.begin(), low.end(), low.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	if (fav) {
		m_favorite_creatures.insert(low);
	} else {
		m_favorite_creatures.erase(low);
	}
	SaveFavorites();
}

void CreatureWikiDialog::BuildUI() {
	wxBoxSizer* rootSizer = new wxBoxSizer(wxVERTICAL);

	// Top Header Bar
	wxPanel* headerPanel = new wxPanel(this, wxID_ANY);
	headerPanel->SetBackgroundColour(wxColor(30, 41, 59));
	wxBoxSizer* headerSizer = new wxBoxSizer(wxHORIZONTAL);

	wxStaticText* title = new wxStaticText(headerPanel, wxID_ANY, "Tibia Creature Wiki & Bestiary Browser");
	wxFont tFont = title->GetFont();
	tFont.SetPointSize(11);
	tFont.SetWeight(wxFONTWEIGHT_BOLD);
	title->SetFont(tFont);
	title->SetForegroundColour(wxColor(241, 245, 249));
	headerSizer->Add(title, 1, wxALIGN_CENTER_VERTICAL | wxALL, 10);

	wxStaticText* subtitle = new wxStaticText(headerPanel, wxID_ANY, "Official Bestiary Stats - Click column headers to sort");
	subtitle->SetForegroundColour(wxColor(148, 163, 184));
	headerSizer->Add(subtitle, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

	headerPanel->SetSizer(headerSizer);
	rootSizer->Add(headerPanel, 0, wxEXPAND);

	// Filter Bar
	wxPanel* filterPanel = new wxPanel(this, wxID_ANY);
	filterPanel->SetBackgroundColour(wxColor(20, 30, 48));
	wxBoxSizer* filterSizer = new wxBoxSizer(wxHORIZONTAL);

	filterSizer->Add(new wxStaticText(filterPanel, wxID_ANY, "Search:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
	search_box = new wxTextCtrl(filterPanel, ID_WIKI_SEARCH, "", wxDefaultPosition, wxSize(140, 24));
	search_box->SetHint("Filter creature name...");
	filterSizer->Add(search_box, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 6);

	filterSizer->Add(new wxStaticText(filterPanel, wxID_ANY, "Category / Class:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
	wxArrayString categories;
	categories.Add("All Categories");
	std::vector<std::string> bestiaryClasses = CreatureBestiary::GetAllClassNames();
	for (const auto& cname : bestiaryClasses) {
		categories.Add(wxstr(cname));
	}
	category_choice = new wxChoice(filterPanel, ID_WIKI_CATEGORY, wxDefaultPosition, wxDefaultSize, categories);
	category_choice->SetSelection(0);
	filterSizer->Add(category_choice, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 6);

	filterSizer->Add(new wxStaticText(filterPanel, wxID_ANY, "Difficulty:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
	wxArrayString tiers;
	tiers.Add("All Difficulties");
	tiers.Add("Harmless");
	tiers.Add("Trivial");
	tiers.Add("Easy");
	tiers.Add("Medium");
	tiers.Add("Hard");
	tiers.Add("Challenging");
	tiers.Add("Favorites Only");
	tier_choice = new wxChoice(filterPanel, ID_WIKI_TIER, wxDefaultPosition, wxDefaultSize, tiers);
	tier_choice->SetSelection(0);
	filterSizer->Add(tier_choice, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 6);

	filterPanel->SetSizer(filterSizer);
	rootSizer->Add(filterPanel, 0, wxEXPAND | wxTOP | wxBOTTOM, 2);

	// Main Split Content: Left Table & Right Detail Pane
	wxBoxSizer* contentSizer = new wxBoxSizer(wxHORIZONTAL);

	// Left: List View
	creature_list = new wxListView(this, ID_WIKI_LIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
	creature_list->SetBackgroundColour(wxColor(15, 23, 42));
	creature_list->SetForegroundColour(wxColor(226, 232, 240));

	creature_list->InsertColumn(0, "Fav", wxLIST_FORMAT_CENTER, 38);
	creature_list->InsertColumn(1, "Creature Name", wxLIST_FORMAT_LEFT, 150);
	creature_list->InsertColumn(2, "Bestiary Class", wxLIST_FORMAT_LEFT, 115);
	creature_list->InsertColumn(3, "Health (HP)", wxLIST_FORMAT_RIGHT, 85);
	creature_list->InsertColumn(4, "EXP", wxLIST_FORMAT_RIGHT, 85);
	creature_list->InsertColumn(5, "Rec. Diff.", wxLIST_FORMAT_LEFT, 120);

	contentSizer->Add(creature_list, 1, wxEXPAND | wxALL, 6);

	// Right: Detail & Sprite Preview Pane
	detail_panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(280, -1));
	detail_panel->SetBackgroundColour(wxColor(24, 34, 53));
	wxBoxSizer* detailSizer = new wxBoxSizer(wxVERTICAL);

	// Sprite Preview Box
	sprite_preview_box = new wxPanel(detail_panel, wxID_ANY, wxDefaultPosition, wxSize(96, 96));
	sprite_preview_box->SetBackgroundColour(wxColor(10, 16, 28));
	sprite_preview_box->Bind(wxEVT_PAINT, [this](wxPaintEvent&) {
		wxPaintDC dc(sprite_preview_box);
		dc.SetBackground(wxBrush(wxColor(10, 16, 28)));
		dc.Clear();
		if (!current_selected_creature.empty()) {
			CreatureType* ct = g_creatures[current_selected_creature];
			if (ct && !g_gui.gfx.isUnloaded()) {
				GameSprite* spr = g_gui.gfx.getCreatureSprite(ct->outfit.lookType);
				if (spr) {
					spr->DrawOutfitTo(&dc, ct->outfit, 16, 16, 64, 64, 2, ct->outfit.lookAddon, 0, 0);
				}
			}
		}
	});

	detailSizer->Add(sprite_preview_box, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP | wxBOTTOM, 10);

	// Title Row: Creature Name + Heart Button
	wxBoxSizer* nameRowSizer = new wxBoxSizer(wxHORIZONTAL);
	detail_name = new wxStaticText(detail_panel, wxID_ANY, "-");
	wxFont nFont = detail_name->GetFont();
	nFont.SetPointSize(11);
	nFont.SetWeight(wxFONTWEIGHT_BOLD);
	detail_name->SetFont(nFont);
	detail_name->SetForegroundColour(wxColor(255, 215, 0));
	nameRowSizer->Add(detail_name, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);

	btn_favorite = new wxButton(detail_panel, ID_WIKI_FAVORITE, wxString::FromUTF8("\xE2\x99\xA1"), wxDefaultPosition, wxSize(28, 26));
	btn_favorite->SetToolTip("Mark as Favorite");
	btn_favorite->SetBackgroundColour(wxColor(30, 41, 59));
	btn_favorite->SetForegroundColour(wxColor(239, 68, 68));
	nameRowSizer->Add(btn_favorite, 0, wxALIGN_CENTER_VERTICAL);

	detailSizer->Add(nameRowSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 4);

	detail_tier = new wxStaticText(detail_panel, wxID_ANY, "Rec. Diff.: -");
	detail_tier->SetForegroundColour(wxColor(56, 189, 248));
	detailSizer->Add(detail_tier, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 8);

	// Stat Grid
	wxFlexGridSizer* statGrid = new wxFlexGridSizer(7, 2, 4, 8);
	statGrid->AddGrowableCol(1);

	auto addStatRow = [this, statGrid](const wxString& label, wxStaticText*& outVal) {
		wxStaticText* l = new wxStaticText(detail_panel, wxID_ANY, label);
		l->SetForegroundColour(wxColor(148, 163, 184));
		statGrid->Add(l, 0, wxALIGN_CENTER_VERTICAL);
		outVal = new wxStaticText(detail_panel, wxID_ANY, "-");
		outVal->SetForegroundColour(wxColor(241, 245, 249));
		statGrid->Add(outVal, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);
	};

	addStatRow("Class:", detail_class);
	addStatRow("Health:", detail_hp);
	addStatRow("Experience:", detail_exp);
	addStatRow("Speed:", detail_speed);
	addStatRow("Armor / Def:", detail_armor_def);
	addStatRow("Weaknesses:", detail_weaknesses);

	wxStaticText* wikiLabel = new wxStaticText(detail_panel, wxID_ANY, "Wiki Entry:");
	wikiLabel->SetForegroundColour(wxColor(148, 163, 184));
	statGrid->Add(wikiLabel, 0, wxALIGN_CENTER_VERTICAL);
	detail_wiki_link = new wxHyperlinkCtrl(detail_panel, wxID_ANY, "Open Wiki Page", "https://tibia.fandom.com/wiki/List_of_Creatures");
	detail_wiki_link->SetNormalColour(wxColor(56, 189, 248));
	detail_wiki_link->SetVisitedColour(wxColor(167, 139, 250));
	statGrid->Add(detail_wiki_link, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);

	detailSizer->Add(statGrid, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	wxStaticText* descLabel = new wxStaticText(detail_panel, wxID_ANY, "Bestiary Lore & Placement Advice:");
	descLabel->SetForegroundColour(wxColor(148, 163, 184));
	detailSizer->Add(descLabel, 0, wxLEFT | wxRIGHT | wxTOP, 8);

	detail_description = new wxTextCtrl(detail_panel, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 70),
	                                    wxTE_MULTILINE | wxTE_READONLY | wxNO_BORDER);
	detail_description->SetBackgroundColour(wxColor(15, 23, 42));
	detail_description->SetForegroundColour(wxColor(203, 213, 225));
	detailSizer->Add(detail_description, 1, wxEXPAND | wxALL, 8);

	// Action buttons
	wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
	btn_place_map = new wxButton(detail_panel, ID_WIKI_PLACE_MAP, "Place on Map");
	btn_place_map->SetBackgroundColour(wxColor(16, 185, 129));
	btn_place_map->SetForegroundColour(wxColor(255, 255, 255));
	btn_select_palette = new wxButton(detail_panel, ID_WIKI_SELECT_PALETTE, "In Palette");
	btn_select_palette->SetBackgroundColour(wxColor(59, 130, 246));
	btn_select_palette->SetForegroundColour(wxColor(255, 255, 255));
	btn_open_wiki = new wxButton(detail_panel, ID_WIKI_OPEN_WIKI, "Online Wiki");
	btn_open_wiki->SetBackgroundColour(wxColor(139, 92, 246));
	btn_open_wiki->SetForegroundColour(wxColor(255, 255, 255));

	btnSizer->Add(btn_place_map, 1, wxEXPAND | wxRIGHT, 4);
	btnSizer->Add(btn_select_palette, 1, wxEXPAND | wxRIGHT, 4);
	btnSizer->Add(btn_open_wiki, 1, wxEXPAND);
	detailSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 8);

	detail_panel->SetSizer(detailSizer);
	contentSizer->Add(detail_panel, 0, wxEXPAND | wxTOP | wxBOTTOM | wxRIGHT, 6);

	rootSizer->Add(contentSizer, 1, wxEXPAND);
	SetSizer(rootSizer);
}

void CreatureWikiDialog::PopulateList() {
	creature_list->DeleteAllItems();
	current_entries.clear();

	wxString searchFilter = search_box->GetValue().Lower();
	int catSel = category_choice->GetSelection();
	std::string catFilter = (catSel > 0) ? category_choice->GetString(catSel).ToStdString() : "";
	int tierSel = tier_choice->GetSelection();

	for (auto iter = g_creatures.begin(); iter != g_creatures.end(); ++iter) {
		CreatureType* ct = iter->second;
		if (!ct || ct->isNpc) continue;

		std::string name = ct->name;
		wxString wxName = wxstr(name);
		if (!searchFilter.IsEmpty() && !wxName.Lower().Contains(searchFilter)) {
			continue;
		}

		CreatureBestiaryClass cid = CreatureBestiary::GetClassForCreature(name, false);
		std::string cname = CreatureBestiary::GetClassName(cid);
		if (!catFilter.empty() && cname != catFilter) {
			continue;
		}

		const CreatureBestiaryInfo* info = CreatureBestiary::GetInfo(name);
		int hp = info ? info->health : 100;
		int exp = info ? info->experience : 50;
		CreatureDifficultyTier tier = info ? info->tier : CreatureBestiary::EstimateDifficulty(hp, exp);
		bool isFav = IsFavorite(name);

		// tierSel: 0 = All, 1 = Harmless, 2 = Trivial, 3 = Easy, 4 = Medium, 5 = Hard, 6 = Challenging, 7 = Favorites
		if (tierSel == 1 && tier != TIER_HARMLESS) continue;
		if (tierSel == 2 && tier != TIER_TRIVIAL) continue;
		if (tierSel == 3 && tier != TIER_EASY) continue;
		if (tierSel == 4 && tier != TIER_MEDIUM) continue;
		if (tierSel == 5 && tier != TIER_HARD) continue;
		if (tierSel == 6 && tier != TIER_CHALLENGING) continue;
		if (tierSel == 7 && !isFav) continue;

		WikiListEntry entry;
		entry.name = name;
		entry.className = cname;
		entry.hp = hp;
		entry.exp = exp;
		entry.tier = tier;
		entry.tierName = CreatureBestiary::GetTierName(tier);
		entry.isFavorite = isFav;

		current_entries.push_back(entry);
	}

	// Sort current_entries based on sort_column and sort_ascending
	std::sort(current_entries.begin(), current_entries.end(), [this](const WikiListEntry& a, const WikiListEntry& b) {
		int cmp = 0;
		switch (sort_column) {
			case 0: // Favorite (Favs first if ascending)
				if (a.isFavorite != b.isFavorite) {
					cmp = a.isFavorite ? -1 : 1;
				} else {
					cmp = a.name.compare(b.name);
				}
				break;
			case 1: // Creature Name
				cmp = a.name.compare(b.name);
				break;
			case 2: // Bestiary Class
				cmp = a.className.compare(b.className);
				if (cmp == 0) cmp = a.name.compare(b.name);
				break;
			case 3: // Health (HP)
				cmp = (a.hp < b.hp) ? -1 : ((a.hp > b.hp) ? 1 : 0);
				if (cmp == 0) cmp = a.name.compare(b.name);
				break;
			case 4: // EXP
				cmp = (a.exp < b.exp) ? -1 : ((a.exp > b.exp) ? 1 : 0);
				if (cmp == 0) cmp = a.name.compare(b.name);
				break;
			case 5: // Rec. Diff. (by tier enum rank)
				cmp = (a.tier < b.tier) ? -1 : ((a.tier > b.tier) ? 1 : 0);
				if (cmp == 0) cmp = (a.hp < b.hp) ? -1 : ((a.hp > b.hp) ? 1 : 0);
				break;
			default:
				cmp = a.name.compare(b.name);
				break;
		}
		return sort_ascending ? (cmp < 0) : (cmp > 0);
	});

	// Insert into wxListView
	long itemIdx = 0;
	for (const auto& entry : current_entries) {
		wxString favIcon = entry.isFavorite ? wxString::FromUTF8("\xE2\x99\xA5") : wxString("");
		long row = creature_list->InsertItem(itemIdx, favIcon);
		creature_list->SetItem(row, 1, wxstr(entry.name));
		creature_list->SetItem(row, 2, wxstr(entry.className));
		creature_list->SetItem(row, 3, wxString::Format("%d", entry.hp));
		creature_list->SetItem(row, 4, wxString::Format("%d", entry.exp));
		creature_list->SetItem(row, 5, wxstr(entry.tierName));

		// Color code rows based on official difficulty tier
		if (entry.tier == TIER_HARMLESS) {
			creature_list->SetItemTextColour(row, wxColor(148, 163, 184)); // Muted slate gray
		} else if (entry.tier == TIER_TRIVIAL) {
			creature_list->SetItemTextColour(row, wxColor(134, 239, 172)); // Soft green
		} else if (entry.tier == TIER_EASY) {
			creature_list->SetItemTextColour(row, wxColor(186, 230, 253)); // Soft blue
		} else if (entry.tier == TIER_MEDIUM) {
			creature_list->SetItemTextColour(row, wxColor(254, 240, 138)); // Soft yellow / amber
		} else if (entry.tier == TIER_HARD) {
			creature_list->SetItemTextColour(row, wxColor(253, 186, 116)); // Orange
		} else if (entry.tier == TIER_CHALLENGING) {
			creature_list->SetItemTextColour(row, wxColor(252, 165, 165)); // Red
		}

		itemIdx++;
	}

	// Restore selection or select first item
	int selIdx = -1;
	if (!current_selected_creature.empty()) {
		for (size_t i = 0; i < current_entries.size(); ++i) {
			if (current_entries[i].name == current_selected_creature) {
				selIdx = (int)i;
				break;
			}
		}
	}

	if (selIdx >= 0) {
		creature_list->Select(selIdx);
		creature_list->EnsureVisible(selIdx);
		UpdateDetailPane(current_selected_creature);
	} else if (creature_list->GetItemCount() > 0) {
		creature_list->Select(0);
		UpdateDetailPane(current_entries[0].name);
	}
}

void CreatureWikiDialog::OnColumnClick(wxListEvent& event) {
	int col = event.GetColumn();
	if (col == sort_column) {
		sort_ascending = !sort_ascending; // Toggle direction
	} else {
		sort_column = col;
		sort_ascending = (col != 0 && col != 3 && col != 4); // Default descending for HP and EXP, ascending for names
	}
	PopulateList();
}

void CreatureWikiDialog::OnFilterChange() {
	PopulateList();
}

void CreatureWikiDialog::OnFilterChangeEvent(wxCommandEvent& WXUNUSED(event)) {
	PopulateList();
}

void CreatureWikiDialog::OnSelectionChange(wxListEvent& event) {
	long idx = event.GetIndex();
	if (idx >= 0 && idx < (long)current_entries.size()) {
		UpdateDetailPane(current_entries[idx].name);
	}
}

void CreatureWikiDialog::UpdateDetailPane(const std::string& name) {
	current_selected_creature = name;
	const CreatureBestiaryInfo* info = CreatureBestiary::GetInfo(name);

	detail_name->SetLabel(wxstr(name));
	std::string wikiUrl = CreatureBestiary::GetWikiUrl(name);
	if (info) {
		detail_class->SetLabel(wxstr(info->bestiary_class));
		detail_tier->SetLabel("Rec. Diff.: " + wxstr(info->tier_name));
		detail_hp->SetLabel(wxString::Format("%d HP", info->health));
		detail_exp->SetLabel(wxString::Format("%d EXP", info->experience));
		detail_speed->SetLabel(wxString::Format("%d", info->speed));
		detail_armor_def->SetLabel(wxString::Format("%d / %d", info->armor, info->defense));
		detail_weaknesses->SetLabel(wxstr(info->weaknesses));
		detail_description->SetValue(wxstr(info->description));
		if (!info->wiki_url.empty()) {
			wikiUrl = info->wiki_url;
		}
	} else {
		CreatureBestiaryClass cid = CreatureBestiary::GetClassForCreature(name, false);
		detail_class->SetLabel(wxstr(CreatureBestiary::GetClassName(cid)));
		detail_tier->SetLabel("Rec. Diff.: Easy");
		detail_hp->SetLabel("Custom / Server Defined");
		detail_exp->SetLabel("Custom / Server Defined");
		detail_speed->SetLabel("200");
		detail_armor_def->SetLabel("15 / 15");
		detail_weaknesses->SetLabel("Physical, Elements");
		detail_description->SetValue("Server-specific custom monster. Review XML data in monster editor for custom spells and loot drops.");
	}

	detail_wiki_link->SetURL(wxstr(wikiUrl));
	detail_wiki_link->SetLabel("View " + wxstr(name) + " on Wiki");

	// Update heart button icon & appearance
	bool fav = IsFavorite(name);
	if (fav) {
		btn_favorite->SetLabel(wxString::FromUTF8("\xE2\x99\xA5")); // Filled heart
		btn_favorite->SetToolTip("Remove from Favorites");
		btn_favorite->SetForegroundColour(wxColor(239, 68, 68)); // Red
	} else {
		btn_favorite->SetLabel(wxString::FromUTF8("\xE2\x99\xA1")); // Outline heart
		btn_favorite->SetToolTip("Add to Favorites");
		btn_favorite->SetForegroundColour(wxColor(148, 163, 184)); // Muted slate
	}
	btn_favorite->Refresh();

	sprite_preview_box->Refresh();
}

void CreatureWikiDialog::OnToggleFavorite(wxCommandEvent& WXUNUSED(event)) {
	if (current_selected_creature.empty()) return;
	bool current = IsFavorite(current_selected_creature);
	SetFavorite(current_selected_creature, !current);

	// Update UI
	UpdateDetailPane(current_selected_creature);

	// Update row icon in list without full reset if possible, or reload list
	PopulateList();
}

void CreatureWikiDialog::SelectCreature(const std::string& name) {
	search_box->SetValue("");
	category_choice->SetSelection(0);
	tier_choice->SetSelection(0);
	current_selected_creature = name;
	PopulateList();

	for (size_t i = 0; i < current_entries.size(); ++i) {
		if (current_entries[i].name == name) {
			creature_list->Select(i);
			creature_list->EnsureVisible(i);
			UpdateDetailPane(name);
			break;
		}
	}
}

void CreatureWikiDialog::OnPlaceCreature(wxCommandEvent& WXUNUSED(event)) {
	if (current_selected_creature.empty()) return;
	CreatureType* ct = g_creatures[current_selected_creature];
	if (ct && ct->brush) {
		g_gui.SelectBrush(static_cast<Brush*>(ct->brush), TILESET_CREATURE);
		EndModal(wxID_OK);
	}
}

void CreatureWikiDialog::OnSelectInPalette(wxCommandEvent& WXUNUSED(event)) {
	if (current_selected_creature.empty()) return;
	CreatureType* ct = g_creatures[current_selected_creature];
	if (ct && ct->brush) {
		g_gui.SelectPalettePage(TILESET_CREATURE);
		g_gui.SelectBrush(static_cast<Brush*>(ct->brush), TILESET_CREATURE);
		EndModal(wxID_OK);
	}
}

void CreatureWikiDialog::OnOpenWikiPage(wxCommandEvent& WXUNUSED(event)) {
	if (current_selected_creature.empty()) return;
	std::string url = CreatureBestiary::GetWikiUrl(current_selected_creature);
	const CreatureBestiaryInfo* info = CreatureBestiary::GetInfo(current_selected_creature);
	if (info && !info->wiki_url.empty()) {
		url = info->wiki_url;
	}
	wxLaunchDefaultBrowser(wxstr(url));
}
