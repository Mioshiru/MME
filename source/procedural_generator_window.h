#ifndef RME_PROCEDURAL_GENERATOR_WINDOW_H_
#define RME_PROCEDURAL_GENERATOR_WINDOW_H_

#include "main.h"
#include "fast_noise_lite.h"
#include <wx/dialog.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/slider.h>

#include "editor.h"

class ProceduralGeneratorDialog : public wxDialog {
public:
	ProceduralGeneratorDialog(wxWindow* parent, Editor& editor);
	virtual ~ProceduralGeneratorDialog();

	void OnClickGenerate(wxCommandEvent& event);
	void OnClickCancel(wxCommandEvent& event);

private:
	Editor& editor;

	wxChoice* generatorTypeChoice;
	wxChoice* noiseTypeChoice;
	wxSpinCtrl* seedSpin;
	wxSlider* densitySlider;
	wxSpinCtrlDouble* frequencySpin;

	DECLARE_EVENT_TABLE()
};

#endif
