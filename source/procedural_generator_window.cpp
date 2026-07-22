#include "procedural_generator_window.h"
#include "editor.h"
#include "gui.h"
#include "map_tab.h"
#include "ground_brush.h"
#include "doodad_brush.h"
#include "raw_brush.h"
#include "action.h"
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/msgdlg.h>

BEGIN_EVENT_TABLE(ProceduralGeneratorDialog, wxDialog)
EVT_BUTTON(wxID_OK, ProceduralGeneratorDialog::OnClickGenerate)
EVT_BUTTON(wxID_CANCEL, ProceduralGeneratorDialog::OnClickCancel)
END_EVENT_TABLE()

ProceduralGeneratorDialog::ProceduralGeneratorDialog(wxWindow* parent, Editor& editor) :
	wxDialog(parent, wxID_ANY, "Procedural Terrain Generator", wxDefaultPosition, wxSize(460, 520), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	editor(editor) {

	SetBackgroundColour(wxColour(15, 23, 42)); // Slate 900
	wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);

	// Header Banner Panel
	wxPanel* headerPanel = new wxPanel(this, wxID_ANY);
	headerPanel->SetBackgroundColour(wxColour(30, 41, 59));
	wxBoxSizer* headerSizer = new wxBoxSizer(wxVERTICAL);

	wxStaticText* header = new wxStaticText(headerPanel, wxID_ANY, "Procedural Terrain Generator");
	header->SetFont(wxFont(13, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	header->SetForegroundColour(wxColour(248, 250, 252));

	wxStaticText* subheader = new wxStaticText(headerPanel, wxID_ANY, "Generate natural caves, forests, rivers, or islands using FastNoiseLite algorithms.");
	subheader->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	subheader->SetForegroundColour(wxColour(148, 163, 184));
	subheader->Wrap(420);

	headerSizer->Add(header, 0, wxBOTTOM, 4);
	headerSizer->Add(subheader, 0);
	headerPanel->SetSizer(headerSizer);

	topsizer->Add(headerPanel, 0, wxEXPAND | wxALL, 12);

	// Card Panel Container
	wxPanel* cardPanel = new wxPanel(this, wxID_ANY);
	cardPanel->SetBackgroundColour(wxColour(30, 41, 59));
	wxBoxSizer* cardSizer = new wxBoxSizer(wxVERTICAL);

	wxFlexGridSizer* grid = new wxFlexGridSizer(2, 10, 12);
	grid->AddGrowableCol(1);

	auto addLabel = [cardPanel, grid](const wxString& labelText) {
		wxStaticText* label = new wxStaticText(cardPanel, wxID_ANY, labelText);
		label->SetForegroundColour(wxColour(203, 213, 225));
		label->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
		grid->Add(label, 0, wxALIGN_CENTER_VERTICAL);
	};

	auto styleChoice = [](wxChoice* ctrl) {
		ctrl->SetBackgroundColour(wxColour(51, 65, 85));
		ctrl->SetForegroundColour(wxColour(248, 250, 252));
	};

	// Target Area
	addLabel("Target Area:");
	wxArrayString targetAreas;
	targetAreas.Add("Active Screen Viewport (60x60 Area)");
	targetAreas.Add("Entire Map Floor");
	targetAreaChoice = new wxChoice(cardPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, targetAreas);
	styleChoice(targetAreaChoice);
	targetAreaChoice->SetSelection(0);
	grid->Add(targetAreaChoice, 1, wxEXPAND);

	// Generator Type
	addLabel("Landscape Type:");
	wxArrayString genTypes;
	genTypes.Add("Cave System");
	genTypes.Add("Forest & Vegetation");
	genTypes.Add("Island / River Path");
	generatorTypeChoice = new wxChoice(cardPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, genTypes);
	styleChoice(generatorTypeChoice);
	generatorTypeChoice->SetSelection(0);
	grid->Add(generatorTypeChoice, 1, wxEXPAND);

	// Noise Algorithm
	addLabel("Noise Algorithm:");
	wxArrayString noiseTypes;
	noiseTypes.Add("Cellular (Caves)");
	noiseTypes.Add("Perlin (Smooth)");
	noiseTypes.Add("OpenSimplex2 (Natural)");
	noiseTypeChoice = new wxChoice(cardPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, noiseTypes);
	styleChoice(noiseTypeChoice);
	noiseTypeChoice->SetSelection(0);
	grid->Add(noiseTypeChoice, 1, wxEXPAND);

	// Seed
	addLabel("Random Seed:");
	seedSpin = new wxSpinCtrl(cardPanel, wxID_ANY, wxString::Format("%d", rand() % 999999), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 9999999);
	seedSpin->SetBackgroundColour(wxColour(51, 65, 85));
	seedSpin->SetForegroundColour(wxColour(248, 250, 252));
	grid->Add(seedSpin, 1, wxEXPAND);

	// Density / Threshold
	addLabel("Density Threshold (%):");
	densitySlider = new wxSlider(cardPanel, wxID_ANY, 50, 10, 90, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	densitySlider->SetForegroundColour(wxColour(203, 213, 225));
	grid->Add(densitySlider, 1, wxEXPAND);

	// Frequency
	addLabel("Scale (Frequency):");
	frequencySpin = new wxSpinCtrlDouble(cardPanel, wxID_ANY, "0.05", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0.001, 0.5, 0.05, 0.01);
	frequencySpin->SetBackgroundColour(wxColour(51, 65, 85));
	frequencySpin->SetForegroundColour(wxColour(248, 250, 252));
	grid->Add(frequencySpin, 1, wxEXPAND);

	cardSizer->Add(grid, 1, wxEXPAND | wxALL, 12);
	cardPanel->SetSizer(cardSizer);

	topsizer->Add(cardPanel, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

	// Buttons
	wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	wxButton* okBtn = new wxButton(this, wxID_OK, "Generate Terrain");
	wxButton* cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");

	okBtn->SetBackgroundColour(wxColour(79, 70, 229));
	okBtn->SetForegroundColour(wxColour(255, 255, 255));
	okBtn->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));

	cancelBtn->SetBackgroundColour(wxColour(51, 65, 85));
	cancelBtn->SetForegroundColour(wxColour(203, 213, 225));

	buttonSizer->Add(okBtn, 0, wxRIGHT, 8);
	buttonSizer->Add(cancelBtn, 0);
	topsizer->Add(buttonSizer, 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT | wxBOTTOM, 12);

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

	int map_x = editor.map.getWidth() / 2;
	int map_y = editor.map.getHeight() / 2;

	MapTab* activeTab = dynamic_cast<MapTab*>(g_gui.GetCurrentTab());
	if (activeTab) {
		Position centerPos = activeTab->GetScreenCenterPosition();
		map_x = centerPos.x;
		map_y = centerPos.y;
	}
	int floor = g_gui.GetCurrentFloor();

	int min_x = 0, max_x = editor.map.getWidth();
	int min_y = 0, max_y = editor.map.getHeight();

	if (targetAreaChoice->GetSelection() == 0) {
		// Active Screen Viewport (60x60 Area)
		min_x = std::max(0, map_x - 30);
		max_x = std::min(editor.map.getWidth(), map_x + 30);
		min_y = std::max(0, map_y - 30);
		max_y = std::min(editor.map.getHeight(), map_y + 30);
	}

	float threshold = (float)densitySlider->GetValue() / 100.0f - 0.5f;

	Action* action = editor.actionQueue->createAction(ACTION_CHANGE_PROPERTIES);

	GroundBrush* dirtBrush = dynamic_cast<GroundBrush*>(g_brushes.getBrush("dirt"));
	GroundBrush* grassBrush = dynamic_cast<GroundBrush*>(g_brushes.getBrush("grass"));

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
	EndModal(wxID_OK);
}

void ProceduralGeneratorDialog::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	EndModal(wxID_CANCEL);
}
