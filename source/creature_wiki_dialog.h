#ifndef RME_CREATURE_WIKI_DIALOG_H_
#define RME_CREATURE_WIKI_DIALOG_H_

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/stattext.h>
#include <wx/panel.h>
#include <wx/hyperlink.h>
#include <vector>
#include <string>
#include <unordered_set>
#include "creature_bestiary.h"

class CreatureType;

struct WikiListEntry {
	std::string name;
	std::string className;
	int hp;
	int exp;
	CreatureDifficultyTier tier;
	std::string tierName;
	bool isFavorite;
};

class CreatureWikiDialog : public wxDialog {
public:
	CreatureWikiDialog(wxWindow* parent);
	virtual ~CreatureWikiDialog();

	void SelectCreature(const std::string& name);

private:
	void BuildUI();
	void PopulateList();
	void OnFilterChange();
	void OnFilterChangeEvent(wxCommandEvent& event);
	void OnSelectionChange(wxListEvent& event);
	void OnColumnClick(wxListEvent& event);
	void UpdateDetailPane(const std::string& name);
	void OnPlaceCreature(wxCommandEvent& event);
	void OnSelectInPalette(wxCommandEvent& event);
	void OnOpenWikiPage(wxCommandEvent& event);
	void OnToggleFavorite(wxCommandEvent& event);
	void OnClose(wxCommandEvent& event);

	void LoadFavorites();
	void SaveFavorites();
	bool IsFavorite(const std::string& name) const;
	void SetFavorite(const std::string& name, bool fav);

	wxTextCtrl* search_box;
	wxChoice* category_choice;
	wxChoice* tier_choice;
	wxListView* creature_list;

	// Detail Pane Controls
	wxPanel* detail_panel;
	wxStaticText* detail_name;
	wxButton* btn_favorite;
	wxStaticText* detail_class;
	wxStaticText* detail_tier;
	wxStaticText* detail_hp;
	wxStaticText* detail_exp;
	wxStaticText* detail_speed;
	wxStaticText* detail_armor_def;
	wxStaticText* detail_elements;
	wxStaticText* detail_weaknesses;
	wxHyperlinkCtrl* detail_wiki_link;
	wxTextCtrl* detail_description;
	wxPanel* sprite_preview_box;

	wxButton* btn_place_map;
	wxButton* btn_select_palette;
	wxButton* btn_open_wiki;

	std::string current_selected_creature;
	std::vector<WikiListEntry> current_entries;

	// Sorting state: column index and ascending flag
	int sort_column;
	bool sort_ascending;

	std::unordered_set<std::string> m_favorite_creatures;

	DECLARE_EVENT_TABLE()
};

#endif // RME_CREATURE_WIKI_DIALOG_H_
