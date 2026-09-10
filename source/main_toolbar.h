//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#ifndef RME_MAINTOOLBAR_H_
#define RME_MAINTOOLBAR_H_

#include <wx/wx.h>
#include <wx/aui/aui.h>
#include <wx/aui/auibar.h>

#include "gui_ids.h"
#include "numbertextctrl.h"

class MainToolBar : public wxEvtHandler {
public:
	MainToolBar(wxWindow* parent, wxAuiManager* manager);
	~MainToolBar();

	wxAuiPaneInfo& GetPane(ToolBarID id);
	void UpdateButtons();
	void UpdateBrushButtons();
	void UpdateBrushSize(BrushShape shape, int size);
	void Show(ToolBarID id, bool show);
	void HideAll(bool update = true);
	void LoadPerspective();
	void SavePerspective();
	void SetFloor(int floor);

	void OnBrushesButtonClick(wxCommandEvent& event);
	void OnZonesDropdown(wxCommandEvent& event);
	void OnDoorsDropdown(wxCommandEvent& event);
	void OnWindowsDropdown(wxCommandEvent& event);
	void OnPastePositionText(wxClipboardTextEvent& event);
	void OnZChoiceChanged(wxCommandEvent& event);
	void OnSizesButtonClick(wxCommandEvent& event);

	// Update the right-aligned hover-info labels in the position toolbar
	void SetItemInfo(const wxString& text);
	void SetPosInfo(const wxString& text);

	// Re-layout toolbars according to current TOOLBAR_ALIGNMENT setting
	void ApplyAlignment();
	void ApplyIconScale();

private:
	static const wxString BRUSHES_BAR_NAME;
	static const wxString POSITION_BAR_NAME;
	static const wxString SIZES_BAR_NAME;

	wxBitmap LoadBitmapFromCandidates(const wxSize& icon_size, const std::vector<wxString>& candidates) const;

	wxAuiToolBar* brushes_toolbar;
	wxAuiToolBar* position_toolbar;
	wxChoice* z_choice;
	wxAuiToolBar* sizes_toolbar;
	wxStaticText* toolbar_left_spacer = nullptr;
	wxStaticText* toolbar_right_spacer = nullptr;
	int toolbar_content_width = 0;

	// Status & hover labels inside position_toolbar
	wxStaticText* item_label  = nullptr;
	wxStaticText* pos_label   = nullptr;
	wxBitmapButton* day_night_button = nullptr;
};

#endif // RME_MAINTOOLBAR_H_
