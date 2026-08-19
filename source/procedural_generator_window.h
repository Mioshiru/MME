#ifndef RME_PROCEDURAL_GENERATOR_WINDOW_H_
#define RME_PROCEDURAL_GENERATOR_WINDOW_H_

#include "main.h"
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include "fast_noise_lite.h"
#include <wx/dialog.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/slider.h>
#include <wx/checkbox.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/panel.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/statbmp.h>

#include "editor.h"

enum {
	ID_GEN_MODE_CHOICE = 10001,
	ID_GEN_AREA_CHOICE,
	ID_GEN_D_THEME_CHOICE,
	ID_GEN_D_PICK_FLOOR,
	ID_GEN_D_PICK_WALL,
	ID_GEN_C_PICK_FLOOR,
	ID_GEN_C_PICK_WALL,
	ID_GEN_H_PICK_FLOOR,
	ID_GEN_H_PICK_WALL,
};

class GeneratorPreviewCanvas : public wxPanel {
public:
	GeneratorPreviewCanvas(wxWindow* parent, wxSize size);
	void SetGridData(const std::vector<std::vector<uint8_t>>& grid, int w, int h);

private:
	void OnPaint(wxPaintEvent& event);
	std::vector<std::vector<uint8_t>> gridData;
	int gridW, gridH;

	DECLARE_EVENT_TABLE()
};

class ProceduralGeneratorDialog : public wxDialog {
public:
	ProceduralGeneratorDialog(wxWindow* parent, Editor& editor);
	virtual ~ProceduralGeneratorDialog();

	void OnModeChange(wxCommandEvent& event);
	void OnAreaChange(wxCommandEvent& event);
	void OnThemeChange(wxCommandEvent& event);
	void OnPickFromPalette(wxCommandEvent& event);
	void OnParamChanged(wxCommandEvent& event);
	void OnParamSpin(wxSpinEvent& event);
	void OnClickGenerate(wxCommandEvent& event);
	void OnClickCancel(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);

	void UpdatePreview();

private:
	void GenerateDungeon(BatchAction* batch, int start_x, int start_y, int area_w, int area_h, int floor);
	void GenerateCave(BatchAction* batch, int start_x, int start_y, int area_w, int area_h, int floor);
	void GenerateHouse(BatchAction* batch, int center_x, int center_y, int floor);

	uint16_t GetActivePaletteItemId() const;
	wxString GetItemNameById(uint16_t id) const;
	bool IsTileOccupiedByPlayer(Tile* tile) const;

	Editor& editor;

	wxChoice* modeChoice;
	wxChoice* targetAreaChoice;
	wxButton* okBtn;
	bool has_generated;

	// Live Preview Canvas
	GeneratorPreviewCanvas* previewCanvas;

	// Custom Area Size
	wxPanel* customSizePanel;
	wxSpinCtrl* customWidthSpin;
	wxSpinCtrl* customHeightSpin;

	// Panels for each mode
	wxPanel* dungeonPanel;
	wxPanel* cavePanel;
	wxPanel* housePanel;

	// Dungeon controls
	wxChoice* d_themeChoice;
	wxChoice* d_corridorStyleChoice;
	wxSpinCtrl* d_roomCountSpin;
	wxSpinCtrl* d_corridorWidthSpin;
	wxSpinCtrl* d_minRoomWSpin;
	wxSpinCtrl* d_minRoomHSpin;
	wxSpinCtrl* d_maxRoomWSpin;
	wxSpinCtrl* d_maxRoomHSpin;
	wxSpinCtrl* d_floorItemSpin;
	wxStaticText* d_floorNameLabel;
	wxSpinCtrl* d_wallItemSpin;
	wxStaticText* d_wallNameLabel;

	// Cave controls
	wxSpinCtrl* c_floorItemSpin;
	wxStaticText* c_floorNameLabel;
	wxSpinCtrl* c_wallItemSpin;
	wxStaticText* c_wallNameLabel;
	wxSlider* c_densitySlider;
	wxSpinCtrl* c_smoothStepsSpin;
	wxChoice* c_noiseTypeChoice;
	wxSpinCtrl* c_seedSpin;

	// House controls
	wxSpinCtrl* h_widthSpin;
	wxSpinCtrl* h_heightSpin;
	wxChoice* h_shapeChoice;
	wxSpinCtrl* h_floorItemSpin;
	wxStaticText* h_floorNameLabel;
	wxSpinCtrl* h_wallItemSpin;
	wxStaticText* h_wallNameLabel;
	wxChoice* h_townChoice;
	wxTextCtrl* h_nameText;
	wxSpinCtrl* h_rentSpin;

	DECLARE_EVENT_TABLE()
};

#endif // RME_PROCEDURAL_GENERATOR_WINDOW_H_
