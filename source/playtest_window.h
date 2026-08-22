//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_PLAYTEST_WINDOW_H_
#define RME_PLAYTEST_WINDOW_H_

#include "main.h"
#include "editor.h"
#include <wx/glcanvas.h>
#include <wx/dialog.h>
#include <wx/timer.h>
#include <wx/choice.h>
#include "position.h"

class MapDrawer;
class PlaytestCanvas;

class PlaytestDialog : public wxDialog {
public:
	PlaytestDialog(wxWindow* parent, Editor& editor);
	virtual ~PlaytestDialog();

	Editor& GetEditor() { return editor; }
	Position GetPlayerPos() const { return player_pos; }
	int GetPlayerDir() const { return player_dir; }
	int GetAnimFrame() const { return anim_frame; }
	int GetWeather() const { return current_weather; }
	const wxString& GetLastActionText() const { return last_action_text; }

	bool MovePlayer(int dx, int dy);
	void InteractWithFacing();
	void ChangeFloor(int delta);
	void SetStatusMessage(const wxString& msg);

protected:
	void OnKeyDown(wxKeyEvent& event);
	void OnTimer(wxTimerEvent& event);
	void OnWeatherChanged(wxCommandEvent& event);

	Editor& editor;
	Position player_pos;
	int player_dir = 2; // 0=North, 1=East, 2=South, 3=West
	int anim_frame = 0;
	int step_counter = 0;
	int current_weather = 0; // 0: Off, 1: Clouds, 2: Rain, 3: Snow, 4: Desert Heat, 5: Dense Fog

	wxString last_action_text = "Playtest ready! [WASD/Arrows]: Walk | [Right-Click/Space]: Use/Ladders/Doors | [ESC]: Exit";
	wxChoice* weather_choice = nullptr;
	wxStaticText* status_bar_txt = nullptr;
	PlaytestCanvas* canvas = nullptr;
	wxTimer game_timer;

	DECLARE_EVENT_TABLE()
};

class PlaytestCanvas : public wxGLCanvas {
public:
	PlaytestCanvas(PlaytestDialog* parent, wxSize size);
	virtual ~PlaytestCanvas();

protected:
	void OnPaint(wxPaintEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnMouseClick(wxMouseEvent& event);
	void Render();
	void RenderWeather(int weather, int w, int h, float time);

	PlaytestDialog* dialog;
	wxGLContext* gl_context = nullptr;
	float sim_time = 0.0f;

	DECLARE_EVENT_TABLE()
};

#endif // RME_PLAYTEST_WINDOW_H_
