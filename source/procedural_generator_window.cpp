#include "procedural_generator_window.h"
#include "editor.h"
#include "gui.h"
#include "map_tab.h"
#include "map_display.h"
#include "ground_brush.h"
#include "doodad_brush.h"
#include "wall_brush.h"
#include "raw_brush.h"
#include "action.h"
#include "house.h"
#include "town.h"
#include "items.h"
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/msgdlg.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <queue>
#include <set>

BEGIN_EVENT_TABLE(GeneratorPreviewCanvas, wxPanel)
	EVT_PAINT(GeneratorPreviewCanvas::OnPaint)
END_EVENT_TABLE()

GeneratorPreviewCanvas::GeneratorPreviewCanvas(wxWindow* parent, wxSize size) :
	wxPanel(parent, wxID_ANY, wxDefaultPosition, size, wxBORDER_SIMPLE),
	gridW(0), gridH(0) {
	SetBackgroundColour(wxColour(8, 14, 24));
}

void GeneratorPreviewCanvas::SetGridData(const std::vector<std::vector<uint8_t>>& grid, int w, int h) {
	gridData = grid;
	gridW = w;
	gridH = h;
	Refresh();
}

void GeneratorPreviewCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
	wxPaintDC dc(this);
	wxSize sz = GetSize();
	dc.SetBackground(wxBrush(wxColour(8, 14, 24)));
	dc.Clear();

	if (gridW <= 0 || gridH <= 0 || gridData.empty()) {
		dc.SetTextForeground(wxColour(120, 140, 165));
		dc.DrawLabel("Adjust parameters for live preview", wxRect(0, 0, sz.x, sz.y), wxALIGN_CENTER);
		return;
	}

	double cell_w = (double)sz.x / gridW;
	double cell_h = (double)sz.y / gridH;

	wxBrush floorBrush(wxColour(50, 75, 110));
	wxBrush wallBrush(wxColour(190, 150, 95));

	for (int y = 0; y < gridH; ++y) {
		for (int x = 0; x < gridW; ++x) {
			uint8_t val = gridData[y][x];
			if (val == 0) continue;

			int px = (int)(x * cell_w);
			int py = (int)(y * cell_h);
			int pw = std::max(1, (int)std::ceil(cell_w));
			int ph = std::max(1, (int)std::ceil(cell_h));

			if (val == 1) dc.SetBrush(floorBrush);
			else if (val == 2) dc.SetBrush(wallBrush);

			dc.SetPen(*wxTRANSPARENT_PEN);
			dc.DrawRectangle(px, py, pw, ph);
		}
	}
}

BEGIN_EVENT_TABLE(ProceduralGeneratorDialog, wxDialog)
	EVT_CHOICE(ID_GEN_MODE_CHOICE, ProceduralGeneratorDialog::OnModeChange)
	EVT_CHOICE(ID_GEN_AREA_CHOICE, ProceduralGeneratorDialog::OnAreaChange)
	EVT_CHOICE(ID_GEN_D_THEME_CHOICE, ProceduralGeneratorDialog::OnThemeChange)
	EVT_BUTTON(ID_GEN_D_PICK_FLOOR, ProceduralGeneratorDialog::OnPickFromPalette)
	EVT_BUTTON(ID_GEN_D_PICK_WALL, ProceduralGeneratorDialog::OnPickFromPalette)
	EVT_BUTTON(ID_GEN_C_PICK_FLOOR, ProceduralGeneratorDialog::OnPickFromPalette)
	EVT_BUTTON(ID_GEN_C_PICK_WALL, ProceduralGeneratorDialog::OnPickFromPalette)
	EVT_BUTTON(ID_GEN_H_PICK_FLOOR, ProceduralGeneratorDialog::OnPickFromPalette)
	EVT_BUTTON(ID_GEN_H_PICK_WALL, ProceduralGeneratorDialog::OnPickFromPalette)
	EVT_SPINCTRL(wxID_ANY, ProceduralGeneratorDialog::OnParamSpin)
	EVT_CHOICE(wxID_ANY, ProceduralGeneratorDialog::OnParamChanged)
	EVT_SLIDER(wxID_ANY, ProceduralGeneratorDialog::OnParamChanged)
	EVT_CHECKBOX(wxID_ANY, ProceduralGeneratorDialog::OnParamChanged)
	EVT_BUTTON(wxID_OK, ProceduralGeneratorDialog::OnClickGenerate)
	EVT_BUTTON(wxID_CANCEL, ProceduralGeneratorDialog::OnClickCancel)
	EVT_CLOSE(ProceduralGeneratorDialog::OnClose)
END_EVENT_TABLE()

ProceduralGeneratorDialog::ProceduralGeneratorDialog(wxWindow* parent, Editor& editor) :
	wxDialog(parent, wxID_ANY, "Procedural Map & Dungeon Generator", wxDefaultPosition, wxSize(740, 580), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxSTAY_ON_TOP),
	editor(editor),
	okBtn(nullptr),
	has_generated(false),
	previewCanvas(nullptr),
	customSizePanel(nullptr),
	customWidthSpin(nullptr),
	customHeightSpin(nullptr),
	dungeonPanel(nullptr),
	cavePanel(nullptr),
	housePanel(nullptr) {

	SetBackgroundColour(wxColour(12, 22, 38));
	SetForegroundColour(wxColour(240, 245, 255));

	wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);

	// Header Banner Panel
	wxPanel* headerPanel = new wxPanel(this, wxID_ANY);
	headerPanel->SetBackgroundColour(wxColour(18, 32, 54));
	wxBoxSizer* headerSizer = new wxBoxSizer(wxVERTICAL);

	wxStaticText* header = new wxStaticText(headerPanel, wxID_ANY, "Procedural Generator Suite");
	header->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	header->SetForegroundColour(wxColour(240, 210, 120));

	wxStaticText* subheader = new wxStaticText(headerPanel, wxID_ANY, "Generate clean, fully enclosed Dungeons, organic Caves, and Houses with proper wall autobordering.");
	subheader->SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	subheader->SetForegroundColour(wxColour(180, 195, 215));

	headerSizer->Add(header, 0, wxBOTTOM, 2);
	headerSizer->Add(subheader, 0);
	headerPanel->SetSizer(headerSizer);
	topsizer->Add(headerPanel, 0, wxEXPAND | wxALL, 8);

	// Mode & Target Area Card
	wxPanel* topControlsPanel = new wxPanel(this, wxID_ANY);
	topControlsPanel->SetBackgroundColour(wxColour(18, 32, 54));
	wxBoxSizer* topControlsSizer = new wxBoxSizer(wxHORIZONTAL);

	wxStaticText* modeLabel = new wxStaticText(topControlsPanel, wxID_ANY, "Mode:");
	modeLabel->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	modeLabel->SetForegroundColour(wxColour(240, 210, 120));
	topControlsSizer->Add(modeLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

	wxArrayString modes;
	modes.Add("Dungeon (Rooms & Corridors)");
	modes.Add("Cave System (Organic Caverns)");
	modes.Add("House / Building (Enclosed Structure)");
	modeChoice = new wxChoice(topControlsPanel, ID_GEN_MODE_CHOICE, wxDefaultPosition, wxDefaultSize, modes);
	modeChoice->SetBackgroundColour(wxColour(12, 22, 38));
	modeChoice->SetForegroundColour(wxColour(240, 245, 255));
	modeChoice->SetSelection(0);
	topControlsSizer->Add(modeChoice, 1, wxRIGHT | wxALIGN_CENTER_VERTICAL, 12);

	auto styleChoice = [](wxChoice* ctrl) {
		ctrl->SetBackgroundColour(wxColour(12, 22, 38));
		ctrl->SetForegroundColour(wxColour(240, 245, 255));
	};
	auto styleSpin = [](wxSpinCtrl* ctrl) {
		ctrl->SetBackgroundColour(wxColour(12, 22, 38));
		ctrl->SetForegroundColour(wxColour(240, 245, 255));
	};
	auto stylePickBtn = [](wxButton* btn) {
		btn->SetBackgroundColour(wxColour(30, 60, 105));
		btn->SetForegroundColour(wxColour(240, 210, 120));
		btn->SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	};

	int vis_w = 60, vis_h = 60;
	MapTab* activeTab = dynamic_cast<MapTab*>(g_gui.GetCurrentTab());
	if (activeTab && activeTab->GetCanvas()) {
		MapCanvas* canvas = activeTab->GetCanvas();
		int screen_w = canvas->GetSize().GetWidth();
		int screen_h = canvas->GetSize().GetHeight();
		int x1, y1, x2, y2;
		canvas->ScreenToMap(0, 0, &x1, &y1);
		canvas->ScreenToMap(screen_w, screen_h, &x2, &y2);
		vis_w = std::max(4, std::abs(x2 - x1) + 1);
		vis_h = std::max(4, std::abs(y2 - y1) + 1);
	}

	wxStaticText* areaLabel = new wxStaticText(topControlsPanel, wxID_ANY, "Area:");
	areaLabel->SetForegroundColour(wxColour(180, 195, 215));
	areaLabel->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	topControlsSizer->Add(areaLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

	wxArrayString targetAreas;
	targetAreas.Add(wxString::Format("Visible Viewport (%dx%d)", vis_w, vis_h));
	targetAreas.Add("Selection Area");
	targetAreas.Add("Custom Dimensions");
	targetAreas.Add("Entire Map Floor");
	targetAreaChoice = new wxChoice(topControlsPanel, ID_GEN_AREA_CHOICE, wxDefaultPosition, wxDefaultSize, targetAreas);
	styleChoice(targetAreaChoice);
	targetAreaChoice->SetSelection(0);
	topControlsSizer->Add(targetAreaChoice, 1, wxALIGN_CENTER_VERTICAL);

	topControlsPanel->SetSizer(topControlsSizer);
	topsizer->Add(topControlsPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

	// Custom Area Size row
	customSizePanel = new wxPanel(this, wxID_ANY);
	customSizePanel->SetBackgroundColour(wxColour(18, 32, 54));
	wxBoxSizer* customSizeSizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* wLbl = new wxStaticText(customSizePanel, wxID_ANY, "Custom Width:");
	wLbl->SetForegroundColour(wxColour(180, 195, 215));
	customWidthSpin = new wxSpinCtrl(customSizePanel, wxID_ANY, wxString::Format("%d", vis_w), wxDefaultPosition, wxSize(65, -1), wxSP_ARROW_KEYS, 4, 1000, vis_w);
	styleSpin(customWidthSpin);

	wxStaticText* hLbl = new wxStaticText(customSizePanel, wxID_ANY, "Custom Height:");
	hLbl->SetForegroundColour(wxColour(180, 195, 215));
	customHeightSpin = new wxSpinCtrl(customSizePanel, wxID_ANY, wxString::Format("%d", vis_h), wxDefaultPosition, wxSize(65, -1), wxSP_ARROW_KEYS, 4, 1000, vis_h);
	styleSpin(customHeightSpin);

	customSizeSizer->Add(wLbl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	customSizeSizer->Add(customWidthSpin, 0, wxRIGHT, 10);
	customSizeSizer->Add(hLbl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	customSizeSizer->Add(customHeightSpin, 0);
	customSizePanel->SetSizer(customSizeSizer);
	customSizePanel->Hide();
	topsizer->Add(customSizePanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

	// Main Layout
	wxBoxSizer* mainContentSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* leftSettingsSizer = new wxBoxSizer(wxVERTICAL);

	uint16_t activePaletteId = GetActivePaletteItemId();

	// =========================================================================
	// 1. DUNGEON PANEL
	// =========================================================================
	dungeonPanel = new wxPanel(this, wxID_ANY);
	dungeonPanel->SetBackgroundColour(wxColour(18, 32, 54));
	wxBoxSizer* d_mainSizer = new wxBoxSizer(wxVERTICAL);

	wxFlexGridSizer* d_themeGrid = new wxFlexGridSizer(2, 4, 6);
	d_themeGrid->AddGrowableCol(1);
	d_themeGrid->AddGrowableCol(3);

	wxStaticText* tmLbl = new wxStaticText(dungeonPanel, wxID_ANY, "Theme Preset:");
	tmLbl->SetForegroundColour(wxColour(240, 210, 120));
	tmLbl->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	d_themeGrid->Add(tmLbl, 0, wxALIGN_CENTER_VERTICAL);

	wxArrayString dThemes;
	dThemes.Add("(Custom / Manual Selection)");
	dThemes.Add("Ancient Catacombs (Cobbled & Stone)");
	dThemes.Add("Lava / Inferno Vault (Obsidian & Magma)");
	dThemes.Add("Ice / Glacier Cavern (Snow & Crystal)");
	dThemes.Add("Desert Tomb / Pyramid (Sandstone)");
	dThemes.Add("Subterranean Sewers (Brick & Slime)");
	d_themeChoice = new wxChoice(dungeonPanel, ID_GEN_D_THEME_CHOICE, wxDefaultPosition, wxDefaultSize, dThemes);
	styleChoice(d_themeChoice);
	d_themeChoice->SetSelection(1);
	d_themeGrid->Add(d_themeChoice, 1, wxEXPAND);

	wxStaticText* csLbl = new wxStaticText(dungeonPanel, wxID_ANY, "Corridor Path:");
	csLbl->SetForegroundColour(wxColour(180, 195, 215));
	csLbl->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	d_themeGrid->Add(csLbl, 0, wxALIGN_CENTER_VERTICAL);

	wxArrayString dStyles;
	dStyles.Add("Straight L-Corridors");
	dStyles.Add("Organic / Winding Corridors");
	dStyles.Add("Labyrinth / Maze Mode");
	d_corridorStyleChoice = new wxChoice(dungeonPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, dStyles);
	styleChoice(d_corridorStyleChoice);
	d_corridorStyleChoice->SetSelection(0);
	d_themeGrid->Add(d_corridorStyleChoice, 1, wxEXPAND);

	d_mainSizer->Add(d_themeGrid, 0, wxEXPAND | wxALL, 6);

	wxFlexGridSizer* d_paramGrid = new wxFlexGridSizer(3, 4, 6, 8);
	d_paramGrid->AddGrowableCol(1);
	d_paramGrid->AddGrowableCol(3);

	auto addDFieldLabel = [this, d_paramGrid](const wxString& text) {
		wxStaticText* l = new wxStaticText(dungeonPanel, wxID_ANY, text);
		l->SetForegroundColour(wxColour(180, 195, 215));
		l->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
		d_paramGrid->Add(l, 0, wxALIGN_CENTER_VERTICAL);
	};

	addDFieldLabel("Room Count:");
	d_roomCountSpin = new wxSpinCtrl(dungeonPanel, wxID_ANY, "8", wxDefaultPosition, wxSize(65, -1), wxSP_ARROW_KEYS, 2, 50, 8);
	styleSpin(d_roomCountSpin);
	d_paramGrid->Add(d_roomCountSpin, 0, wxALIGN_LEFT);

	addDFieldLabel("Corridor Width:");
	d_corridorWidthSpin = new wxSpinCtrl(dungeonPanel, wxID_ANY, "2", wxDefaultPosition, wxSize(65, -1), wxSP_ARROW_KEYS, 1, 10, 2);
	styleSpin(d_corridorWidthSpin);
	d_paramGrid->Add(d_corridorWidthSpin, 0, wxALIGN_LEFT);

	addDFieldLabel("Min Room Size:");
	wxBoxSizer* minSizeSizer = new wxBoxSizer(wxHORIZONTAL);
	d_minRoomWSpin = new wxSpinCtrl(dungeonPanel, wxID_ANY, "6", wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, 4, 30, 6);
	d_minRoomHSpin = new wxSpinCtrl(dungeonPanel, wxID_ANY, "6", wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, 4, 30, 6);
	styleSpin(d_minRoomWSpin);
	styleSpin(d_minRoomHSpin);
	wxStaticText* xLbl1 = new wxStaticText(dungeonPanel, wxID_ANY, "x");
	xLbl1->SetForegroundColour(wxColour(180, 195, 215));
	minSizeSizer->Add(d_minRoomWSpin, 0);
	minSizeSizer->Add(xLbl1, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 2);
	minSizeSizer->Add(d_minRoomHSpin, 0);
	d_paramGrid->Add(minSizeSizer, 0, wxALIGN_LEFT);

	addDFieldLabel("Max Room Size:");
	wxBoxSizer* maxSizeSizer = new wxBoxSizer(wxHORIZONTAL);
	d_maxRoomWSpin = new wxSpinCtrl(dungeonPanel, wxID_ANY, "10", wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, 4, 40, 10);
	d_maxRoomHSpin = new wxSpinCtrl(dungeonPanel, wxID_ANY, "8", wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, 4, 40, 8);
	styleSpin(d_maxRoomWSpin);
	styleSpin(d_maxRoomHSpin);
	wxStaticText* xLbl2 = new wxStaticText(dungeonPanel, wxID_ANY, "x");
	xLbl2->SetForegroundColour(wxColour(180, 195, 215));
	maxSizeSizer->Add(d_maxRoomWSpin, 0);
	maxSizeSizer->Add(xLbl2, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 2);
	maxSizeSizer->Add(d_maxRoomHSpin, 0);
	d_paramGrid->Add(maxSizeSizer, 0, wxALIGN_LEFT);

	d_mainSizer->Add(d_paramGrid, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

	wxFlexGridSizer* d_itemsGrid = new wxFlexGridSizer(2, 6, 8);
	d_itemsGrid->AddGrowableCol(1);

	auto addDItemLabel = [this, d_itemsGrid](const wxString& text) {
		wxStaticText* l = new wxStaticText(dungeonPanel, wxID_ANY, text);
		l->SetForegroundColour(wxColour(180, 195, 215));
		l->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
		d_itemsGrid->Add(l, 0, wxALIGN_CENTER_VERTICAL);
	};

	addDItemLabel("Walkable Floor:");
	wxBoxSizer* d_floorRow = new wxBoxSizer(wxHORIZONTAL);
	uint16_t d_floorDef = 10774;
	d_floorItemSpin = new wxSpinCtrl(dungeonPanel, wxID_ANY, wxString::Format("%d", d_floorDef), wxDefaultPosition, wxSize(70, -1), wxSP_ARROW_KEYS, 1, 65535, d_floorDef);
	styleSpin(d_floorItemSpin);
	wxButton* d_floorPick = new wxButton(dungeonPanel, ID_GEN_D_PICK_FLOOR, "Select via Palette");
	stylePickBtn(d_floorPick);
	d_floorNameLabel = new wxStaticText(dungeonPanel, wxID_ANY, GetItemNameById(d_floorDef));
	d_floorNameLabel->SetForegroundColour(wxColour(140, 175, 210));

	d_floorRow->Add(d_floorItemSpin, 0, wxRIGHT, 6);
	d_floorRow->Add(d_floorPick, 0, wxRIGHT, 6);
	d_floorRow->Add(d_floorNameLabel, 0, wxALIGN_CENTER_VERTICAL);
	d_itemsGrid->Add(d_floorRow, 1, wxEXPAND);

	addDItemLabel("Perimeter Wall:");
	wxBoxSizer* d_wallRow = new wxBoxSizer(wxHORIZONTAL);
	uint16_t d_wallDef = 22763;
	d_wallItemSpin = new wxSpinCtrl(dungeonPanel, wxID_ANY, wxString::Format("%d", d_wallDef), wxDefaultPosition, wxSize(70, -1), wxSP_ARROW_KEYS, 1, 65535, d_wallDef);
	styleSpin(d_wallItemSpin);
	wxButton* d_wallPick = new wxButton(dungeonPanel, ID_GEN_D_PICK_WALL, "Select via Palette");
	stylePickBtn(d_wallPick);
	d_wallNameLabel = new wxStaticText(dungeonPanel, wxID_ANY, GetItemNameById(d_wallDef));
	d_wallNameLabel->SetForegroundColour(wxColour(140, 175, 210));

	d_wallRow->Add(d_wallItemSpin, 0, wxRIGHT, 6);
	d_wallRow->Add(d_wallPick, 0, wxRIGHT, 6);
	d_wallRow->Add(d_wallNameLabel, 0, wxALIGN_CENTER_VERTICAL);
	d_itemsGrid->Add(d_wallRow, 1, wxEXPAND);

	d_mainSizer->Add(d_itemsGrid, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
	dungeonPanel->SetSizer(d_mainSizer);
	leftSettingsSizer->Add(dungeonPanel, 1, wxEXPAND);

	// =========================================================================
	// 2. CAVE PANEL
	// =========================================================================
	cavePanel = new wxPanel(this, wxID_ANY);
	cavePanel->SetBackgroundColour(wxColour(18, 32, 54));
	wxBoxSizer* c_mainSizer = new wxBoxSizer(wxVERTICAL);
	wxFlexGridSizer* c_paramGrid = new wxFlexGridSizer(4, 6, 8);
	c_paramGrid->AddGrowableCol(1);
	c_paramGrid->AddGrowableCol(3);

	auto addCFieldLabel = [this, c_paramGrid](const wxString& text) {
		wxStaticText* l = new wxStaticText(cavePanel, wxID_ANY, text);
		l->SetForegroundColour(wxColour(180, 195, 215));
		l->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
		c_paramGrid->Add(l, 0, wxALIGN_CENTER_VERTICAL);
	};

	addCFieldLabel("Cavern Density (%):");
	c_densitySlider = new wxSlider(cavePanel, wxID_ANY, 48, 20, 80, wxDefaultPosition, wxSize(120, -1), wxSL_HORIZONTAL | wxSL_LABELS);
	c_densitySlider->SetForegroundColour(wxColour(180, 195, 215));
	c_paramGrid->Add(c_densitySlider, 0, wxALIGN_LEFT);

	addCFieldLabel("Smooth Steps:");
	c_smoothStepsSpin = new wxSpinCtrl(cavePanel, wxID_ANY, "3", wxDefaultPosition, wxSize(65, -1), wxSP_ARROW_KEYS, 1, 6, 3);
	styleSpin(c_smoothStepsSpin);
	c_paramGrid->Add(c_smoothStepsSpin, 0, wxALIGN_LEFT);

	addCFieldLabel("Algorithm:");
	wxArrayString caveNoise;
	caveNoise.Add("Cellular Automata");
	caveNoise.Add("OpenSimplex2");
	c_noiseTypeChoice = new wxChoice(cavePanel, wxID_ANY, wxDefaultPosition, wxSize(130, -1), caveNoise);
	styleChoice(c_noiseTypeChoice);
	c_noiseTypeChoice->SetSelection(0);
	c_paramGrid->Add(c_noiseTypeChoice, 0, wxALIGN_LEFT);

	addCFieldLabel("Seed:");
	c_seedSpin = new wxSpinCtrl(cavePanel, wxID_ANY, wxString::Format("%d", rand() % 999999), wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, 0, 9999999);
	styleSpin(c_seedSpin);
	c_paramGrid->Add(c_seedSpin, 0, wxALIGN_LEFT);

	c_mainSizer->Add(c_paramGrid, 0, wxEXPAND | wxALL, 8);

	wxFlexGridSizer* c_itemsGrid = new wxFlexGridSizer(2, 6, 8);
	c_itemsGrid->AddGrowableCol(1);

	auto addCItemLabel = [this, c_itemsGrid](const wxString& text) {
		wxStaticText* l = new wxStaticText(cavePanel, wxID_ANY, text);
		l->SetForegroundColour(wxColour(180, 195, 215));
		l->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
		c_itemsGrid->Add(l, 0, wxALIGN_CENTER_VERTICAL);
	};

	addCItemLabel("Walkable Floor:");
	wxBoxSizer* c_floorRow = new wxBoxSizer(wxHORIZONTAL);
	uint16_t c_floorDef = 4414;
	c_floorItemSpin = new wxSpinCtrl(cavePanel, wxID_ANY, wxString::Format("%d", c_floorDef), wxDefaultPosition, wxSize(75, -1), wxSP_ARROW_KEYS, 1, 65535, c_floorDef);
	styleSpin(c_floorItemSpin);
	wxButton* c_floorPick = new wxButton(cavePanel, ID_GEN_C_PICK_FLOOR, "Select via Palette");
	stylePickBtn(c_floorPick);
	c_floorNameLabel = new wxStaticText(cavePanel, wxID_ANY, GetItemNameById(c_floorDef));
	c_floorNameLabel->SetForegroundColour(wxColour(140, 175, 210));

	c_floorRow->Add(c_floorItemSpin, 0, wxRIGHT, 6);
	c_floorRow->Add(c_floorPick, 0, wxRIGHT, 6);
	c_floorRow->Add(c_floorNameLabel, 0, wxALIGN_CENTER_VERTICAL);
	c_itemsGrid->Add(c_floorRow, 1, wxEXPAND);

	addCItemLabel("Cave Wall / Border:");
	wxBoxSizer* c_wallRow = new wxBoxSizer(wxHORIZONTAL);
	uint16_t c_wallDef = 101;
	c_wallItemSpin = new wxSpinCtrl(cavePanel, wxID_ANY, wxString::Format("%d", c_wallDef), wxDefaultPosition, wxSize(75, -1), wxSP_ARROW_KEYS, 1, 65535, c_wallDef);
	styleSpin(c_wallItemSpin);
	wxButton* c_wallPick = new wxButton(cavePanel, ID_GEN_C_PICK_WALL, "Select via Palette");
	stylePickBtn(c_wallPick);
	c_wallNameLabel = new wxStaticText(cavePanel, wxID_ANY, GetItemNameById(c_wallDef));
	c_wallNameLabel->SetForegroundColour(wxColour(140, 175, 210));

	c_wallRow->Add(c_wallItemSpin, 0, wxRIGHT, 6);
	c_wallRow->Add(c_wallPick, 0, wxRIGHT, 6);
	c_wallRow->Add(c_wallNameLabel, 0, wxALIGN_CENTER_VERTICAL);
	c_itemsGrid->Add(c_wallRow, 1, wxEXPAND);

	c_mainSizer->Add(c_itemsGrid, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
	cavePanel->SetSizer(c_mainSizer);
	cavePanel->Hide();
	leftSettingsSizer->Add(cavePanel, 1, wxEXPAND);

	// =========================================================================
	// 3. HOUSE PANEL
	// =========================================================================
	housePanel = new wxPanel(this, wxID_ANY);
	housePanel->SetBackgroundColour(wxColour(18, 32, 54));
	wxBoxSizer* h_mainSizer = new wxBoxSizer(wxVERTICAL);
	wxFlexGridSizer* h_paramGrid = new wxFlexGridSizer(4, 6, 8);
	h_paramGrid->AddGrowableCol(1);
	h_paramGrid->AddGrowableCol(3);

	auto addHFieldLabel = [this, h_paramGrid](const wxString& text) {
		wxStaticText* l = new wxStaticText(housePanel, wxID_ANY, text);
		l->SetForegroundColour(wxColour(180, 195, 215));
		l->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
		h_paramGrid->Add(l, 0, wxALIGN_CENTER_VERTICAL);
	};

	addHFieldLabel("Dimensions (WxH):");
	wxBoxSizer* hSizeSizer = new wxBoxSizer(wxHORIZONTAL);
	h_widthSpin = new wxSpinCtrl(housePanel, wxID_ANY, "8", wxDefaultPosition, wxSize(55, -1), wxSP_ARROW_KEYS, 4, 30, 8);
	h_heightSpin = new wxSpinCtrl(housePanel, wxID_ANY, "7", wxDefaultPosition, wxSize(55, -1), wxSP_ARROW_KEYS, 4, 30, 7);
	styleSpin(h_widthSpin);
	styleSpin(h_heightSpin);
	wxStaticText* xLblH = new wxStaticText(housePanel, wxID_ANY, "x");
	xLblH->SetForegroundColour(wxColour(180, 195, 215));
	hSizeSizer->Add(h_widthSpin, 0);
	hSizeSizer->Add(xLblH, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 3);
	hSizeSizer->Add(h_heightSpin, 0);
	h_paramGrid->Add(hSizeSizer, 0, wxALIGN_LEFT);

	addHFieldLabel("Shape:");
	wxArrayString shapes;
	shapes.Add("Rectangular");
	shapes.Add("L-Shape Layout");
	h_shapeChoice = new wxChoice(housePanel, wxID_ANY, wxDefaultPosition, wxSize(120, -1), shapes);
	styleChoice(h_shapeChoice);
	h_shapeChoice->SetSelection(0);
	h_paramGrid->Add(h_shapeChoice, 0, wxALIGN_LEFT);

	addHFieldLabel("Name & Rent:");
	wxBoxSizer* nameRentSizer = new wxBoxSizer(wxHORIZONTAL);
	h_nameText = new wxTextCtrl(housePanel, wxID_ANY, "Townhouse", wxDefaultPosition, wxSize(90, -1));
	h_nameText->SetBackgroundColour(wxColour(12, 22, 38));
	h_nameText->SetForegroundColour(wxColour(240, 245, 255));
	h_rentSpin = new wxSpinCtrl(housePanel, wxID_ANY, "500", wxDefaultPosition, wxSize(65, -1), wxSP_ARROW_KEYS, 0, 1000000, 500);
	styleSpin(h_rentSpin);
	nameRentSizer->Add(h_nameText, 0, wxRIGHT, 4);
	nameRentSizer->Add(h_rentSpin, 0);
	h_paramGrid->Add(nameRentSizer, 0, wxALIGN_LEFT);

	addHFieldLabel("Town:");
	wxArrayString townList;
	townList.Add("(Auto-Detect Nearest)");
	for (TownMap::const_iterator it = editor.map.towns.begin(); it != editor.map.towns.end(); ++it) {
		if (it->second) {
			townList.Add(wxString::Format("%s", it->second->getName().c_str()));
		}
	}
	h_townChoice = new wxChoice(housePanel, wxID_ANY, wxDefaultPosition, wxSize(130, -1), townList);
	styleChoice(h_townChoice);
	h_townChoice->SetSelection(0);
	h_paramGrid->Add(h_townChoice, 0, wxALIGN_LEFT);

	h_mainSizer->Add(h_paramGrid, 0, wxEXPAND | wxALL, 8);

	wxFlexGridSizer* h_itemsGrid = new wxFlexGridSizer(2, 6, 8);
	h_itemsGrid->AddGrowableCol(1);

	auto addHItemLabel = [this, h_itemsGrid](const wxString& text) {
		wxStaticText* l = new wxStaticText(housePanel, wxID_ANY, text);
		l->SetForegroundColour(wxColour(180, 195, 215));
		l->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
		h_itemsGrid->Add(l, 0, wxALIGN_CENTER_VERTICAL);
	};

	addHItemLabel("Floor Tile:");
	wxBoxSizer* h_floorRow = new wxBoxSizer(wxHORIZONTAL);
	uint16_t h_floorDef = 405;
	h_floorItemSpin = new wxSpinCtrl(housePanel, wxID_ANY, wxString::Format("%d", h_floorDef), wxDefaultPosition, wxSize(75, -1), wxSP_ARROW_KEYS, 1, 65535, h_floorDef);
	styleSpin(h_floorItemSpin);
	wxButton* h_floorPick = new wxButton(housePanel, ID_GEN_H_PICK_FLOOR, "Select via Palette");
	stylePickBtn(h_floorPick);
	h_floorNameLabel = new wxStaticText(housePanel, wxID_ANY, GetItemNameById(h_floorDef));
	h_floorNameLabel->SetForegroundColour(wxColour(140, 175, 210));

	h_floorRow->Add(h_floorItemSpin, 0, wxRIGHT, 6);
	h_floorRow->Add(h_floorPick, 0, wxRIGHT, 6);
	h_floorRow->Add(h_floorNameLabel, 0, wxALIGN_CENTER_VERTICAL);
	h_itemsGrid->Add(h_floorRow, 1, wxEXPAND);

	addHItemLabel("Wall Item:");
	wxBoxSizer* h_wallRow = new wxBoxSizer(wxHORIZONTAL);
	uint16_t h_wallDef = 1025;
	h_wallItemSpin = new wxSpinCtrl(housePanel, wxID_ANY, wxString::Format("%d", h_wallDef), wxDefaultPosition, wxSize(75, -1), wxSP_ARROW_KEYS, 1, 65535, h_wallDef);
	styleSpin(h_wallItemSpin);
	wxButton* h_wallPick = new wxButton(housePanel, ID_GEN_H_PICK_WALL, "Select via Palette");
	stylePickBtn(h_wallPick);
	h_wallNameLabel = new wxStaticText(housePanel, wxID_ANY, GetItemNameById(h_wallDef));
	h_wallNameLabel->SetForegroundColour(wxColour(140, 175, 210));

	h_wallRow->Add(h_wallItemSpin, 0, wxRIGHT, 6);
	h_wallRow->Add(h_wallPick, 0, wxRIGHT, 6);
	h_wallRow->Add(h_wallNameLabel, 0, wxALIGN_CENTER_VERTICAL);
	h_itemsGrid->Add(h_wallRow, 1, wxEXPAND);

	h_mainSizer->Add(h_itemsGrid, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
	housePanel->SetSizer(h_mainSizer);
	housePanel->Hide();
	leftSettingsSizer->Add(housePanel, 1, wxEXPAND);

	mainContentSizer->Add(leftSettingsSizer, 1, wxEXPAND | wxRIGHT, 8);

	// Right Side: Live Mini-Map Preview Card
	wxPanel* previewCard = new wxPanel(this, wxID_ANY);
	previewCard->SetBackgroundColour(wxColour(18, 32, 54));
	wxBoxSizer* previewCardSizer = new wxBoxSizer(wxVERTICAL);

	wxStaticText* prevTitle = new wxStaticText(previewCard, wxID_ANY, "Live Preview");
	prevTitle->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	prevTitle->SetForegroundColour(wxColour(240, 210, 120));
	previewCardSizer->Add(prevTitle, 0, wxBOTTOM, 4);

	previewCanvas = new GeneratorPreviewCanvas(previewCard, wxSize(200, 200));
	previewCardSizer->Add(previewCanvas, 1, wxEXPAND | wxBOTTOM, 4);

	wxStaticText* legendText = new wxStaticText(previewCard, wxID_ANY, "Blue: Floor | Tan: Wall");
	legendText->SetFont(wxFont(7, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	legendText->SetForegroundColour(wxColour(140, 160, 185));
	previewCardSizer->Add(legendText, 0);

	previewCard->SetSizer(previewCardSizer);
	mainContentSizer->Add(previewCard, 0, wxEXPAND);

	topsizer->Add(mainContentSizer, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

	// Action Buttons
	wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	okBtn = new wxButton(this, wxID_OK, "Generate on Map");
	wxButton* cancelBtn = new wxButton(this, wxID_CANCEL, "Close");

	okBtn->SetBackgroundColour(wxColour(35, 75, 150));
	okBtn->SetForegroundColour(wxColour(240, 210, 120));
	okBtn->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));

	cancelBtn->SetBackgroundColour(wxColour(22, 36, 58));
	cancelBtn->SetForegroundColour(wxColour(180, 190, 205));

	buttonSizer->Add(okBtn, 0, wxRIGHT, 8);
	buttonSizer->Add(cancelBtn, 0);
	topsizer->Add(buttonSizer, 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT | wxBOTTOM, 8);

	SetSizerAndFit(topsizer);
	UpdatePreview();
}

ProceduralGeneratorDialog::~ProceduralGeneratorDialog() {
}

void ProceduralGeneratorDialog::OnClose(wxCloseEvent& event) {
	if (IsModal()) {
		EndModal(wxID_CANCEL);
	} else {
		Destroy();
	}
}

uint16_t ProceduralGeneratorDialog::GetActivePaletteItemId() const {
	Brush* b = g_gui.GetCurrentBrush();
	if (!b) return 0;
	if (RAWBrush* raw = dynamic_cast<RAWBrush*>(b)) {
		if (raw->getItemType()) return raw->getItemType()->id;
	}
	int look = b->getLookID();
	if (look > 0) return (uint16_t)look;
	return 0;
}

wxString ProceduralGeneratorDialog::GetItemNameById(uint16_t id) const {
	if (id == 0) return "";
	const ItemType& it = g_items[id];
	if (!it.name.empty()) {
		return wxString::Format("(%s)", it.name.c_str());
	}
	return "";
}

bool ProceduralGeneratorDialog::IsTileOccupiedByPlayer(Tile* tile) const {
	if (!tile) return false;
	if (tile->isHouseTile() || tile->house_id != 0 || tile->creature != nullptr || tile->spawn != nullptr || tile->isPZ()) {
		return true;
	}
	for (Item* item : tile->items) {
		if (item && (item->isContainer() || item->isDoor() || item->isWall())) {
			return true;
		}
	}
	return false;
}

static WallBrush* FindWallBrushForId(uint16_t wallId) {
	if (g_items[wallId].brush && g_items[wallId].brush->isWall()) {
		return g_items[wallId].brush->asWall();
	}
	for (auto& pair : g_brushes.getMap()) {
		if (pair.second && pair.second->isWall()) {
			WallBrush* wb = pair.second->asWall();
			if (wb && wb->hasWallId(wallId)) {
				return wb;
			}
		}
	}
	return nullptr;
}



void ProceduralGeneratorDialog::OnThemeChange(wxCommandEvent& WXUNUSED(event)) {
	int sel = d_themeChoice->GetSelection();
	if (sel == 1) {
		d_floorItemSpin->SetValue(10774);
		d_wallItemSpin->SetValue(22763);
	} else if (sel == 2) {
		d_floorItemSpin->SetValue(598);
		d_wallItemSpin->SetValue(1025);
	} else if (sel == 3) {
		d_floorItemSpin->SetValue(670);
		d_wallItemSpin->SetValue(6811);
	} else if (sel == 4) {
		d_floorItemSpin->SetValue(405);
		d_wallItemSpin->SetValue(1030);
	} else if (sel == 5) {
		d_floorItemSpin->SetValue(4414);
		d_wallItemSpin->SetValue(3361);
	}
	d_floorNameLabel->SetLabel(GetItemNameById(d_floorItemSpin->GetValue()));
	d_wallNameLabel->SetLabel(GetItemNameById(d_wallItemSpin->GetValue()));
	UpdatePreview();
}

void ProceduralGeneratorDialog::OnPickFromPalette(wxCommandEvent& event) {
	uint16_t activeId = GetActivePaletteItemId();
	if (activeId == 0) {
		wxMessageBox("No active brush or item selected in the palette.\nPlease select a tile or item in the palette first.", "Palette Item Picker", wxOK | wxICON_INFORMATION, this);
		return;
	}

	wxString name = GetItemNameById(activeId);

	switch (event.GetId()) {
		case ID_GEN_D_PICK_FLOOR:
			d_floorItemSpin->SetValue(activeId);
			d_floorNameLabel->SetLabel(name);
			d_themeChoice->SetSelection(0);
			break;
		case ID_GEN_D_PICK_WALL:
			d_wallItemSpin->SetValue(activeId);
			d_wallNameLabel->SetLabel(name);
			d_themeChoice->SetSelection(0);
			break;
		case ID_GEN_C_PICK_FLOOR:
			c_floorItemSpin->SetValue(activeId);
			c_floorNameLabel->SetLabel(name);
			break;
		case ID_GEN_C_PICK_WALL:
			c_wallItemSpin->SetValue(activeId);
			c_wallNameLabel->SetLabel(name);
			break;
		case ID_GEN_H_PICK_FLOOR:
			h_floorItemSpin->SetValue(activeId);
			h_floorNameLabel->SetLabel(name);
			break;
		case ID_GEN_H_PICK_WALL:
			h_wallItemSpin->SetValue(activeId);
			h_wallNameLabel->SetLabel(name);
			break;
	}
	UpdatePreview();
}

void ProceduralGeneratorDialog::OnModeChange(wxCommandEvent& WXUNUSED(event)) {
	int sel = modeChoice->GetSelection();
	dungeonPanel->Show(sel == 0);
	cavePanel->Show(sel == 1);
	housePanel->Show(sel == 2);
	Layout();
	Fit();
	UpdatePreview();
}

void ProceduralGeneratorDialog::OnAreaChange(wxCommandEvent& WXUNUSED(event)) {
	int sel = targetAreaChoice->GetSelection();
	customSizePanel->Show(sel == 2);
	Layout();
	Fit();
	UpdatePreview();
}

void ProceduralGeneratorDialog::OnParamChanged(wxCommandEvent& WXUNUSED(event)) {
	UpdatePreview();
}

void ProceduralGeneratorDialog::OnParamSpin(wxSpinEvent& WXUNUSED(event)) {
	UpdatePreview();
}

void ProceduralGeneratorDialog::UpdatePreview() {
	if (!previewCanvas) return;

	int mode = modeChoice->GetSelection();
	int pw = 40, ph = 40;
	std::vector<std::vector<uint8_t>> grid(ph, std::vector<uint8_t>(pw, 0));

	if (mode == 0) {
		int roomCount = std::min(10, d_roomCountSpin->GetValue());
		int minW = std::max(4, d_minRoomWSpin->GetValue());
		int minH = std::max(4, d_minRoomHSpin->GetValue());
		int maxW = std::max(minW, d_maxRoomWSpin->GetValue());
		int maxH = std::max(minH, d_maxRoomHSpin->GetValue());
		int corridorW = d_corridorWidthSpin->GetValue();

		struct PrevRoom { int x, y, w, h; int cx() const { return x + w / 2; } int cy() const { return y + h / 2; } };
		std::vector<PrevRoom> rooms;
		srand(12345);

		for (int a = 0; a < 300 && (int)rooms.size() < roomCount; ++a) {
			int rw = minW + rand() % (maxW - minW + 1);
			int rh = minH + rand() % (maxH - minH + 1);
			int rx = 2 + rand() % (pw - rw - 3);
			int ry = 2 + rand() % (ph - rh - 3);

			bool overlap = false;
			for (const auto& r : rooms) {
				if (rx - 2 < r.x + r.w && rx + rw + 2 > r.x && ry - 2 < r.y + r.h && ry + rh + 2 > r.y) {
					overlap = true;
					break;
				}
			}
			if (!overlap) {
				rooms.push_back({ rx, ry, rw, rh });
				for (int y = ry; y < ry + rh; ++y) {
					for (int x = rx; x < rx + rw; ++x) {
						if (x == rx || x == rx + rw - 1 || y == ry || y == ry + rh - 1) {
							grid[y][x] = 2; // Perimeter Wall
						} else {
							grid[y][x] = 1; // Interior Floor
						}
					}
				}
			}
		}

		// Corridors carving floor paths
		for (size_t i = 1; i < rooms.size(); ++i) {
			int x1 = rooms[i - 1].cx(), y1 = rooms[i - 1].cy();
			int x2 = rooms[i].cx(), y2 = rooms[i].cy();
			int cx = x1, cy = y1;
			while (cx != x2) {
				for (int cw = 0; cw < corridorW; ++cw) {
					if (cy + cw < ph && cx < pw && cy + cw >= 0 && cx >= 0) grid[cy + cw][cx] = 1;
				}
				cx += (x2 > cx) ? 1 : -1;
			}
			while (cy != y2) {
				for (int cw = 0; cw < corridorW; ++cw) {
					if (cy < ph && cx + cw < pw && cy >= 0 && cx + cw >= 0) grid[cy][cx + cw] = 1;
				}
				cy += (y2 > cy) ? 1 : -1;
			}
		}

		// Walls surrounding open corridors
		for (int y = 0; y < ph; ++y) {
			for (int x = 0; x < pw; ++x) {
				if (grid[y][x] == 0) {
					bool adj = false;
					if (y > 0 && grid[y - 1][x] == 1) adj = true;
					else if (y + 1 < ph && grid[y + 1][x] == 1) adj = true;
					else if (x > 0 && grid[y][x - 1] == 1) adj = true;
					else if (x + 1 < pw && grid[y][x + 1] == 1) adj = true;
					if (adj) grid[y][x] = 2;
				}
			}
		}
	} else if (mode == 1) {
		int dens = c_densitySlider->GetValue();
		int seed = c_seedSpin->GetValue();
		srand(seed);
		for (int y = 0; y < ph; ++y) {
			for (int x = 0; x < pw; ++x) {
				if (x > 1 && x < pw - 2 && y > 1 && y < ph - 2) {
					if (rand() % 100 < dens) grid[y][x] = 1;
				}
			}
		}
		for (int s = 0; s < 2; ++s) {
			auto next_g = grid;
			for (int y = 1; y < ph - 1; ++y) {
				for (int x = 1; x < pw - 1; ++x) {
					int count = 0;
					for (int dy = -1; dy <= 1; ++dy) {
						for (int dx = -1; dx <= 1; ++dx) {
							if (grid[y + dy][x + dx] == 1) count++;
						}
					}
					next_g[y][x] = (count >= 5) ? 1 : 0;
				}
			}
			grid = next_g;
		}
		for (int y = 0; y < ph; ++y) {
			for (int x = 0; x < pw; ++x) {
				if (grid[y][x] == 0) {
					bool adj = false;
					if (y > 0 && grid[y - 1][x] == 1) adj = true;
					else if (y + 1 < ph && grid[y + 1][x] == 1) adj = true;
					else if (x > 0 && grid[y][x - 1] == 1) adj = true;
					else if (x + 1 < pw && grid[y][x + 1] == 1) adj = true;
					if (adj) grid[y][x] = 2;
				}
			}
		}
	} else if (mode == 2) {
		int hw = h_widthSpin->GetValue();
		int hh = h_heightSpin->GetValue();
		int sx = (pw - hw) / 2;
		int sy = (ph - hh) / 2;
		for (int y = sy; y < sy + hh; ++y) {
			for (int x = sx; x < sx + hw; ++x) {
				if (x == sx || x == sx + hw - 1 || y == sy || y == sy + hh - 1) {
					grid[y][x] = 2;
				} else {
					grid[y][x] = 1;
				}
			}
		}
	}

	previewCanvas->SetGridData(grid, pw, ph);
}

void ProceduralGeneratorDialog::OnClickGenerate(wxCommandEvent& WXUNUSED(event)) {
	if (has_generated) {
		g_gui.DoUndo();
	}

	int map_x = editor.map.getWidth() / 2;
	int map_y = editor.map.getHeight() / 2;

	MapTab* activeTab = dynamic_cast<MapTab*>(g_gui.GetCurrentTab());
	if (activeTab) {
		Position centerPos = activeTab->GetScreenCenterPosition();
		map_x = centerPos.x;
		map_y = centerPos.y;
	}
	int floor = g_gui.GetCurrentFloor();

	int min_x = std::max(1, map_x - 30);
	int max_x = std::min(editor.map.getWidth() - 2, map_x + 30);
	int min_y = std::max(1, map_y - 30);
	int max_y = std::min(editor.map.getHeight() - 2, map_y + 30);

	int areaSel = targetAreaChoice->GetSelection();
	if (areaSel == 0) {
		if (activeTab && activeTab->GetCanvas()) {
			MapCanvas* canvas = activeTab->GetCanvas();
			int screen_w = canvas->GetSize().GetWidth();
			int screen_h = canvas->GetSize().GetHeight();
			int x1, y1, x2, y2;
			canvas->ScreenToMap(0, 0, &x1, &y1);
			canvas->ScreenToMap(screen_w, screen_h, &x2, &y2);

			min_x = std::max(1, std::min(x1, x2));
			max_x = std::min(editor.map.getWidth() - 2, std::max(x1, x2));
			min_y = std::max(1, std::min(y1, y2));
			max_y = std::min(editor.map.getHeight() - 2, std::max(y1, y2));
		}
	} else if (areaSel == 1 && editor.selection.size() > 0) {
		const auto& selTiles = editor.selection.getTiles();
		min_x = editor.map.getWidth(); max_x = 0;
		min_y = editor.map.getHeight(); max_y = 0;
		for (Tile* t : selTiles) {
			if (t) {
				min_x = std::min(min_x, t->getX());
				max_x = std::max(max_x, t->getX());
				min_y = std::min(min_y, t->getY());
				max_y = std::max(max_y, t->getY());
			}
		}
	} else if (areaSel == 2) {
		int cw = customWidthSpin->GetValue();
		int ch = customHeightSpin->GetValue();
		min_x = std::max(1, map_x - cw / 2);
		max_x = std::min(editor.map.getWidth() - 2, min_x + cw - 1);
		min_y = std::max(1, map_y - ch / 2);
		max_y = std::min(editor.map.getHeight() - 2, min_y + ch - 1);
	} else if (areaSel == 3) {
		min_x = 1;
		max_x = editor.map.getWidth() - 2;
		min_y = 1;
		max_y = editor.map.getHeight() - 2;
	}

	BatchAction* batch = editor.actionQueue->createBatch(ACTION_DRAW);

	int mode = modeChoice->GetSelection();
	if (mode == 0) {
		GenerateDungeon(batch, min_x, min_y, max_x - min_x + 1, max_y - min_y + 1, floor);
	} else if (mode == 1) {
		GenerateCave(batch, min_x, min_y, max_x - min_x + 1, max_y - min_y + 1, floor);
	} else if (mode == 2) {
		GenerateHouse(batch, map_x, map_y, floor);
	}

	if (batch->size() > 0) {
		editor.actionQueue->addBatch(batch, 2);
		editor.map.doChange();
		has_generated = true;
		if (okBtn) {
			okBtn->SetLabel("Retry");
		}
	} else {
		delete batch;
	}

	g_gui.RefreshView();
	if (IsModal()) {
		EndModal(wxID_OK);
	}
}

void ProceduralGeneratorDialog::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	if (IsModal()) {
		EndModal(wxID_CANCEL);
	} else {
		Destroy();
	}
}

// =============================================================================
// 1. CLEAN STRUCTURE DUNGEON GENERATOR (Solid Enclosed Rooms & Corridors)
// =============================================================================
struct D_Room {
	int id;
	int x, y, w, h;
	int cx() const { return x + w / 2; }
	int cy() const { return y + h / 2; }
	bool contains(int px, int py) const {
		return px >= x && px < x + w && py >= y && py < y + h;
	}
	bool intersects(const D_Room& other, int margin = 2) const {
		return (x - margin < other.x + other.w && x + w + margin > other.x &&
				y - margin < other.y + other.h && y + h + margin > other.y);
	}
};

void ProceduralGeneratorDialog::GenerateDungeon(BatchAction* batch, int start_x, int start_y, int area_w, int area_h, int floor) {
	int roomCount = d_roomCountSpin->GetValue();
	int minRoomW = d_minRoomWSpin->GetValue();
	int minRoomH = d_minRoomHSpin->GetValue();
	int maxRoomW = std::max(minRoomW, d_maxRoomWSpin->GetValue());
	int maxRoomH = std::max(minRoomH, d_maxRoomHSpin->GetValue());
	int corridorWidth = d_corridorWidthSpin->GetValue();
	int corridorStyle = d_corridorStyleChoice->GetSelection();

	uint16_t floorId = (uint16_t)d_floorItemSpin->GetValue();
	uint16_t wallId = (uint16_t)d_wallItemSpin->GetValue();

	std::vector<std::vector<bool>> is_floor(area_h, std::vector<bool>(area_w, false));
	std::vector<std::vector<bool>> is_wall(area_h, std::vector<bool>(area_w, false));
	std::vector<D_Room> rooms;

	// 1. Place Fully Enclosed Rooms
	int attempts = 0;
	while ((int)rooms.size() < roomCount && attempts++ < 500) {
		int rw = minRoomW + (maxRoomW > minRoomW ? rand() % (maxRoomW - minRoomW + 1) : 0);
		int rh = minRoomH + (maxRoomH > minRoomH ? rand() % (maxRoomH - minRoomH + 1) : 0);
		if (rw >= area_w - 4 || rh >= area_h - 4) continue;

		int rx = 2 + rand() % (area_w - rw - 3);
		int ry = 2 + rand() % (area_h - rh - 3);

		D_Room newRoom = { (int)rooms.size(), rx, ry, rw, rh };
		bool overlap = false;
		for (const auto& r : rooms) {
			if (newRoom.intersects(r, 2)) {
				overlap = true;
				break;
			}
		}
		if (!overlap) {
			rooms.push_back(newRoom);
			// Fill interior floor (leaving 1-tile outer perimeter for walls)
			for (int y = ry + 1; y < ry + rh - 1; ++y) {
				for (int x = rx + 1; x < rx + rw - 1; ++x) {
					is_floor[y][x] = true;
				}
			}
			// Mark 4 outer walls of the room
			for (int x = rx; x < rx + rw; ++x) {
				is_wall[ry][x] = true;
				is_wall[ry + rh - 1][x] = true;
			}
			for (int y = ry; y < ry + rh; ++y) {
				is_wall[y][rx] = true;
				is_wall[y][rx + rw - 1] = true;
			}
		}
	}

	if (rooms.empty()) return;

	// 2. Connect Rooms with Corridors (punching clean openings through perimeter)
	auto carveCorridor = [&](int x1, int y1, int x2, int y2) {
		int cur_x = x1;
		int cur_y = y1;

		while (cur_x != x2) {
			for (int cw = 0; cw < corridorWidth; ++cw) {
				int cy = cur_y + cw;
				if (corridorStyle == 1 && rand() % 4 == 0) {
					cy += (rand() % 3) - 1;
				}
				if (cy < area_h && cur_x < area_w && cy >= 0 && cur_x >= 0) {
					is_floor[cy][cur_x] = true;
					is_wall[cy][cur_x] = false; // Open doorway
				}
			}
			cur_x += (x2 > cur_x) ? 1 : -1;
		}
		while (cur_y != y2) {
			for (int cw = 0; cw < corridorWidth; ++cw) {
				int cx = cur_x + cw;
				if (corridorStyle == 1 && rand() % 4 == 0) {
					cx += (rand() % 3) - 1;
				}
				if (cur_y < area_h && cx < area_w && cur_y >= 0 && cx >= 0) {
					is_floor[cur_y][cx] = true;
					is_wall[cur_y][cx] = false; // Open doorway
				}
			}
			cur_y += (y2 > cur_y) ? 1 : -1;
		}
	};

	for (size_t i = 1; i < rooms.size(); ++i) {
		carveCorridor(rooms[i - 1].cx(), rooms[i - 1].cy(), rooms[i].cx(), rooms[i].cy());
	}
	if (rooms.size() > 2) {
		carveCorridor(rooms.back().cx(), rooms.back().cy(), rooms.front().cx(), rooms.front().cy());
	}
	if (corridorStyle == 2 && rooms.size() >= 4) {
		carveCorridor(rooms[1].cx(), rooms[1].cy(), rooms[3].cx(), rooms[3].cy());
	}

	// 3. Mark corridor walls directly adjacent (4-way orthogonal) to corridor floors
	for (int y = 0; y < area_h; ++y) {
		for (int x = 0; x < area_w; ++x) {
			if (!is_floor[y][x]) {
				bool orthoFloor = false;
				if (y > 0 && is_floor[y - 1][x]) orthoFloor = true;
				else if (y + 1 < area_h && is_floor[y + 1][x]) orthoFloor = true;
				else if (x > 0 && is_floor[y][x - 1]) orthoFloor = true;
				else if (x + 1 < area_w && is_floor[y][x + 1]) orthoFloor = true;

				if (orthoFloor) {
					is_wall[y][x] = true;
				}
			}
		}
	}

	// Also fill outer 90-degree corner vertices where horizontal and vertical walls meet
	for (int y = 1; y < area_h - 1; ++y) {
		for (int x = 1; x < area_w - 1; ++x) {
			if (!is_floor[y][x] && !is_wall[y][x]) {
				if ((is_wall[y - 1][x] && is_wall[y][x - 1]) ||
					(is_wall[y - 1][x] && is_wall[y][x + 1]) ||
					(is_wall[y + 1][x] && is_wall[y][x - 1]) ||
					(is_wall[y + 1][x] && is_wall[y][x + 1])) {
					if (is_floor[y - 1][x - 1] || is_floor[y - 1][x + 1] ||
						is_floor[y + 1][x - 1] || is_floor[y + 1][x + 1]) {
						is_wall[y][x] = true;
					}
				}
			}
		}
	}

	std::set<Position> touched_positions;

	// =========================================================================
	// PASS 1: PLACE BASE FLOORS AND BASE WALLS & COMMIT TO MAP
	// =========================================================================
	Action* action1 = editor.actionQueue->createAction(batch);

	for (int y = 0; y < area_h; ++y) {
		for (int x = 0; x < area_w; ++x) {
			int mx = start_x + x;
			int my = start_y + y;
			Position pos(mx, my, floor);

			if (is_floor[y][x]) {
				Tile* tile = editor.map.getOrCreateTile(pos);
				if (tile && !IsTileOccupiedByPlayer(tile)) {
					Tile* newTile = tile->deepCopy(editor.map);
					Item* gr = Item::Create(floorId);
					if (gr) {
						newTile->addItem(gr);
					}
					action1->addChange(newd Change(newTile));
					touched_positions.insert(pos);
				}
			} else if (is_wall[y][x]) {
				Tile* tile = editor.map.getOrCreateTile(pos);
				if (tile && !IsTileOccupiedByPlayer(tile)) {
					Tile* newTile = tile->deepCopy(editor.map);
					newTile->cleanWalls();
					Item* wallItem = Item::Create(wallId);
					if (wallItem) {
						newTile->addItem(wallItem);
					}
					action1->addChange(newd Change(newTile));
					touched_positions.insert(pos);
				}
			}
		}
	}

	batch->addAndCommitAction(action1);

	// =========================================================================
	// PASS 2: AUTOBORDERING (GROUNDS & WALLS ON COMMITTED MAP)
	// =========================================================================
	Action* action2 = editor.actionQueue->createAction(batch);

	std::set<Position> borderize_positions;
	for (const Position& pos : touched_positions) {
		borderize_positions.insert(pos);
		for (int dy = -1; dy <= 1; ++dy) {
			for (int dx = -1; dx <= 1; ++dx) {
				borderize_positions.insert(Position(pos.x + dx, pos.y + dy, pos.z));
			}
		}
	}

	for (const Position& pos : borderize_positions) {
		Tile* tile = editor.map.getTile(pos);
		if (tile && !IsTileOccupiedByPlayer(tile)) {
			Tile* newTile = tile->deepCopy(editor.map);
			newTile->borderize(&editor.map);
			newTile->wallize(&editor.map);
			action2->addChange(newd Change(newTile));
		}
	}

	batch->addAndCommitAction(action2);
}

// =============================================================================
// 2. CAVE SYSTEM GENERATOR IMPLEMENTATION
// =============================================================================
void ProceduralGeneratorDialog::GenerateCave(BatchAction* batch, int start_x, int start_y, int area_w, int area_h, int floor) {
	uint16_t floorId = (uint16_t)c_floorItemSpin->GetValue();
	uint16_t wallId = (uint16_t)c_wallItemSpin->GetValue();
	int density = c_densitySlider->GetValue();
	int smoothSteps = c_smoothStepsSpin->GetValue();
	int seed = c_seedSpin->GetValue();

	std::vector<std::vector<bool>> cave(area_h, std::vector<bool>(area_w, false));

	// 1. Initial Cellular Noise Grid
	srand(seed);
	for (int y = 0; y < area_h; ++y) {
		for (int x = 0; x < area_w; ++x) {
			if (x <= 1 || x >= area_w - 2 || y <= 1 || y >= area_h - 2) {
				cave[y][x] = false;
			} else {
				cave[y][x] = (rand() % 100 < density);
			}
		}
	}

	// 2. Cellular Automata Smoothing
	for (int step = 0; step < smoothSteps; ++step) {
		std::vector<std::vector<bool>> next_cave = cave;
		for (int y = 1; y < area_h - 1; ++y) {
			for (int x = 1; x < area_w - 1; ++x) {
				int neighbors = 0;
				for (int dy = -1; dy <= 1; ++dy) {
					for (int dx = -1; dx <= 1; ++dx) {
						if (cave[y + dy][x + dx]) {
							neighbors++;
						}
					}
				}
				if (neighbors >= 5) {
					next_cave[y][x] = true;
				} else {
					next_cave[y][x] = false;
				}
			}
		}
		cave = next_cave;
	}

	std::vector<std::vector<bool>> is_c_wall(area_h, std::vector<bool>(area_w, false));
	for (int y = 0; y < area_h; ++y) {
		for (int x = 0; x < area_w; ++x) {
			if (!cave[y][x]) {
				bool adjacentToCave = false;
				if (y > 0 && cave[y - 1][x]) adjacentToCave = true;
				else if (y + 1 < area_h && cave[y + 1][x]) adjacentToCave = true;
				else if (x > 0 && cave[y][x - 1]) adjacentToCave = true;
				else if (x + 1 < area_w && cave[y][x + 1]) adjacentToCave = true;
				if (adjacentToCave) {
					is_c_wall[y][x] = true;
				}
			}
		}
	}

	std::set<Position> touched_positions;

	// PASS 1: Place Cave Floors & Walls
	Action* action1 = editor.actionQueue->createAction(batch);

	for (int y = 0; y < area_h; ++y) {
		for (int x = 0; x < area_w; ++x) {
			int mx = start_x + x;
			int my = start_y + y;
			Position pos(mx, my, floor);

			if (cave[y][x]) {
				Tile* tile = editor.map.getOrCreateTile(pos);
				if (tile && !IsTileOccupiedByPlayer(tile)) {
					Tile* newTile = tile->deepCopy(editor.map);
					Item* gr = Item::Create(floorId);
					if (gr) {
						newTile->addItem(gr);
					}
					action1->addChange(newd Change(newTile));
					touched_positions.insert(pos);
				}
			} else if (is_c_wall[y][x]) {
				Tile* tile = editor.map.getOrCreateTile(pos);
				if (tile && !IsTileOccupiedByPlayer(tile)) {
					Tile* newTile = tile->deepCopy(editor.map);
					newTile->cleanWalls();
					Item* wItem = Item::Create(wallId);
					if (wItem) {
						newTile->addItem(wItem);
					}
					action1->addChange(newd Change(newTile));
					touched_positions.insert(pos);
				}
			}
		}
	}

	batch->addAndCommitAction(action1);

	// PASS 2: Autoborder (Ground borders & Wall borders)
	Action* action2 = editor.actionQueue->createAction(batch);

	std::set<Position> borderize_positions;
	for (const Position& pos : touched_positions) {
		borderize_positions.insert(pos);
		for (int dy = -1; dy <= 1; ++dy) {
			for (int dx = -1; dx <= 1; ++dx) {
				borderize_positions.insert(Position(pos.x + dx, pos.y + dy, pos.z));
			}
		}
	}

	for (const Position& pos : borderize_positions) {
		Tile* tile = editor.map.getTile(pos);
		if (tile && !IsTileOccupiedByPlayer(tile)) {
			Tile* newTile = tile->deepCopy(editor.map);
			newTile->borderize(&editor.map);
			newTile->wallize(&editor.map);
			action2->addChange(newd Change(newTile));
		}
	}

	batch->addAndCommitAction(action2);
}

// =============================================================================
// 3. HOUSE / BUILDING GENERATOR IMPLEMENTATION
// =============================================================================
void ProceduralGeneratorDialog::GenerateHouse(BatchAction* batch, int center_x, int center_y, int floor) {
	int w = h_widthSpin->GetValue();
	int h = h_heightSpin->GetValue();
	bool isLshape = (h_shapeChoice->GetSelection() == 1);
	uint16_t floorId = (uint16_t)h_floorItemSpin->GetValue();
	uint16_t wallId = (uint16_t)h_wallItemSpin->GetValue();

	int x1 = center_x - w / 2;
	int y1 = center_y - h / 2;
	int x2 = x1 + w - 1;
	int y2 = y1 + h - 1;

	uint32_t resolved_town_id = 0;
	int townSel = h_townChoice->GetSelection();
	if (townSel > 0) {
		int idx = 1;
		for (TownMap::const_iterator it = editor.map.towns.begin(); it != editor.map.towns.end(); ++it) {
			if (it->second) {
				if (idx == townSel) {
					resolved_town_id = it->second->getID();
					break;
				}
				idx++;
			}
		}
	} else {
		double minDist = 1e9;
		for (TownMap::const_iterator it = editor.map.towns.begin(); it != editor.map.towns.end(); ++it) {
			if (it->second) {
				Position temple = it->second->getTemplePosition();
				double dist = std::sqrt(std::pow(temple.x - center_x, 2) + std::pow(temple.y - center_y, 2));
				if (dist < minDist) {
					minDist = dist;
					resolved_town_id = it->second->getID();
				}
			}
		}
	}

	House* house = newd House(editor.map);
	house->setID(editor.map.houses.getEmptyID());
	house->name = h_nameText->GetValue().ToStdString();
	house->rent = h_rentSpin->GetValue();
	house->townid = resolved_town_id;
	editor.map.houses.addHouse(house);

	std::set<Position> touched_positions;

	// PASS 1: Place Floors & Walls
	Action* action1 = editor.actionQueue->createAction(batch);

	for (int y = y1; y <= y2; ++y) {
		for (int x = x1; x <= x2; ++x) {
			if (isLshape && x > x1 + w / 2 && y < y1 + h / 2) {
				continue;
			}

			bool isOuter = (x == x1 || x == x2 || y == y1 || y == y2 ||
				(isLshape && (x == x1 + w / 2 && y <= y1 + h / 2)) ||
				(isLshape && (y == y1 + h / 2 && x >= x1 + w / 2)));

			Position pos(x, y, floor);
			Tile* tile = editor.map.getOrCreateTile(pos);
			if (!tile || IsTileOccupiedByPlayer(tile)) continue;

			Tile* newTile = tile->deepCopy(editor.map);
			if (isOuter) {
				newTile->cleanWalls();
				Item* wItem = Item::Create(wallId);
				if (wItem) {
					newTile->addItem(wItem);
				}
			} else {
				Item* gr = Item::Create(floorId);
				if (gr) {
					newTile->addItem(gr);
				}
				newTile->setHouse(house);
			}
			action1->addChange(newd Change(newTile));
			touched_positions.insert(pos);
		}
	}

	batch->addAndCommitAction(action1);

	// PASS 2: Autoborder (Ground & Walls)
	Action* action2 = editor.actionQueue->createAction(batch);

	std::set<Position> borderize_positions;
	for (const Position& pos : touched_positions) {
		borderize_positions.insert(pos);
		for (int dy = -1; dy <= 1; ++dy) {
			for (int dx = -1; dx <= 1; ++dx) {
				borderize_positions.insert(Position(pos.x + dx, pos.y + dy, pos.z));
			}
		}
	}

	for (const Position& pos : borderize_positions) {
		Tile* tile = editor.map.getTile(pos);
		if (tile && !IsTileOccupiedByPlayer(tile)) {
			Tile* newTile = tile->deepCopy(editor.map);
			newTile->borderize(&editor.map);
			newTile->wallize(&editor.map);
			action2->addChange(newd Change(newTile));
		}
	}

	batch->addAndCommitAction(action2);
}
