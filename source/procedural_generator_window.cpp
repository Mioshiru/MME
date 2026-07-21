#include "procedural_generator_window.h"
#include "editor.h"
#include "gui.h"
#include "ground_brush.h"
#include "doodad_brush.h"
#include "raw_brush.h"
#include "action.h"
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/msgdlg.h>

BEGIN_EVENT_TABLE(ProceduralGeneratorDialog, wxDialog)
EVT_BUTTON(wxID_OK, ProceduralGeneratorDialog::OnClickGenerate)
EVT_BUTTON(wxID_CANCEL, ProceduralGeneratorDialog::OnClickCancel)
END_EVENT_TABLE()

ProceduralGeneratorDialog::ProceduralGeneratorDialog(wxWindow* parent, Editor& editor) :
	wxDialog(parent, wxID_ANY, "Prozeduraler Map-Generator (FastNoise)", wxDefaultPosition, wxSize(400, 360), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	editor(editor) {

	wxSizer* topsizer = new wxBoxSizer(wxVERTICAL);

	// Title
	wxStaticText* header = new wxStaticText(this, wxID_ANY, "Prozedurale Landschaftsgenerierung");
	wxFont headerFont = header->GetFont();
	headerFont.SetPointSize(12);
	headerFont.SetWeight(wxFONTWEIGHT_BOLD);
	header->SetFont(headerFont);
	header->SetForegroundColour(wxColor(180, 140, 50));
	topsizer->Add(header, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 10);

	wxFlexGridSizer* grid = new wxFlexGridSizer(2, 10, 10);
	grid->AddGrowableCol(1);

	// Generator Type
	grid->Add(new wxStaticText(this, wxID_ANY, "Landschaftstyp:"), 0, wxALIGN_CENTER_VERTICAL);
	wxArrayString genTypes;
	genTypes.Add("Höhlensystem (Cave)");
	genTypes.Add("Wald & Vegetation (Forest)");
	genTypes.Add("Insel / Flussverlauf (Island/River)");
	generatorTypeChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, genTypes);
	generatorTypeChoice->SetSelection(0);
	grid->Add(generatorTypeChoice, 1, wxEXPAND);

	// Noise Type
	grid->Add(new wxStaticText(this, wxID_ANY, "Rausch-Algorithmus:"), 0, wxALIGN_CENTER_VERTICAL);
	wxArrayString noiseTypes;
	noiseTypes.Add("Cellular (Zellulär / Höhlen)");
	noiseTypes.Add("Perlin (Weich)");
	noiseTypes.Add("OpenSimplex2 (Natürlich)");
	noiseTypeChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, noiseTypes);
	noiseTypeChoice->SetSelection(0);
	grid->Add(noiseTypeChoice, 1, wxEXPAND);

	// Seed
	grid->Add(new wxStaticText(this, wxID_ANY, "Seed (Zufallswert):"), 0, wxALIGN_CENTER_VERTICAL);
	seedSpin = new wxSpinCtrl(this, wxID_ANY, wxString::Format("%d", rand() % 999999), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 9999999);
	grid->Add(seedSpin, 1, wxEXPAND);

	// Density / Threshold
	grid->Add(new wxStaticText(this, wxID_ANY, "Dichte / Schwelle (%):"), 0, wxALIGN_CENTER_VERTICAL);
	densitySlider = new wxSlider(this, wxID_ANY, 50, 10, 90, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	grid->Add(densitySlider, 1, wxEXPAND);

	// Frequency
	grid->Add(new wxStaticText(this, wxID_ANY, "Skalierung (Frequency):"), 0, wxALIGN_CENTER_VERTICAL);
	frequencySpin = new wxSpinCtrlDouble(this, wxID_ANY, "0.05", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0.001, 0.5, 0.05, 0.01);
	grid->Add(frequencySpin, 1, wxEXPAND);

	topsizer->Add(grid, 1, wxEXPAND | wxALL, 15);

	// Buttons
	wxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	buttonSizer->Add(new wxButton(this, wxID_OK, "Generieren"), 0, wxRIGHT, 10);
	buttonSizer->Add(new wxButton(this, wxID_CANCEL, "Abbrechen"), 0);
	topsizer->Add(buttonSizer, 0, wxALIGN_RIGHT | wxALL, 10);

	SetSizerAndFit(topsizer);
}

ProceduralGeneratorDialog::~ProceduralGeneratorDialog() {
}

void ProceduralGeneratorDialog::OnClickGenerate(wxCommandEvent& WXUNUSED(event)) {
	FastNoiseLite noise;
	int noiseSel = noiseTypeChoice->GetSelection();
	if (noiseSel == 0) noise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
	else if (noiseSel == 1) noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	else noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

	noise.SetSeed(seedSpin->GetValue());
	noise.SetFrequency((float)frequencySpin->GetValue());

	int map_x, map_y;
	g_gui.GetScreenCenterPosition(&map_x, &map_y);
	int floor = g_gui.GetCurrentFloor();

	int min_x = std::max(0, map_x - 30);
	int max_x = std::min(editor.map.getWidth(), map_x + 30);
	int min_y = std::max(0, map_y - 30);
	int max_y = std::min(editor.map.getHeight(), map_y + 30);

	float threshold = (float)densitySlider->GetValue() / 100.0f - 0.5f;

	Action* action = editor.actionQueue->createAction(ACTION_CHANGE_PROPERTIES);

	GroundBrush* dirtBrush = g_brushes.getGroundBrush("dirt");
	GroundBrush* grassBrush = g_brushes.getGroundBrush("grass");

	for (int y = min_y; y <= max_y; ++y) {
		for (int x = min_x; x <= max_x; ++x) {
			float val = noise.GetNoise((float)x, (float)y);
			Tile* tile = editor.map.getTile(x, y, floor);
			if (!tile) {
				tile = editor.map.createTile(x, y, floor);
			}

			Tile* newTile = tile->deepCopy(editor.map);
			if (val > threshold) {
				if (dirtBrush) {
					dirtBrush->draw(&editor.map, newTile, nullptr);
				}
			} else {
				if (grassBrush) {
					grassBrush->draw(&editor.map, newTile, nullptr);
				}
			}
			action->addChange(newd Change(newTile));
		}
	}

	editor.addAction(action);
	g_gui.RefreshView();
	g_gui.SetStatusText("Prozedurale Generierung erfolgreich durchgeführt.");
	EndModal(wxID_OK);
}

void ProceduralGeneratorDialog::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	EndModal(wxID_CANCEL);
}
