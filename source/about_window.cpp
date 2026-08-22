#include "main.h"
#include "gui.h"
#include "about_window.h"
#include "mme_updater.h"
#include <fstream>
#include <typeinfo>
#include <memory>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/statline.h>

class GamePanel : public wxPanel {
public:
	GamePanel(wxWindow* parent, int width, int height);
	virtual ~GamePanel();

	void OnPaint(wxPaintEvent&);
	void OnKeyDown(wxKeyEvent&);
	void OnKeyUp(wxKeyEvent&);
	void OnIdle(wxIdleEvent&);

	void pause() {
		paused_val = true;
	}
	void unpause() {
		paused_val = false;
	}
	bool paused() const {
		return paused_val || dead;
	}

protected:
	virtual void Render(wxDC& pdc) = 0;
	virtual void GameLoop(int time) = 0;
	virtual void OnKey(wxKeyEvent& event, bool down) = 0;

	virtual int getFPS() const = 0;

protected:
	wxStopWatch game_timer;

private:
	bool paused_val;

	DECLARE_EVENT_TABLE()

protected:
	bool dead;
};

const int TETRIS_MAPHEIGHT = 20;
const int TETRIS_MAPWIDTH = 10;

class TetrisPanel : public GamePanel {
public:
	TetrisPanel(wxWindow* parent);
	~TetrisPanel();

protected:
	virtual void Render(wxDC& pdc);
	virtual void GameLoop(int time);
	virtual void OnKey(wxKeyEvent& event, bool down);

	virtual int getFPS() const {
		return lines / 10 + 3;
	}

	enum Color {
		NO_COLOR,
		RED,
		BLUE,
		GREEN,
		STEEL,
		YELLOW,
		PURPLE,
		WHITE,
	};

	enum BlockType {
		FIRST_BLOCK,
		BLOCK_TOWER = FIRST_BLOCK,
		BLOCK_SQUARE,
		BLOCK_TRIANGLE,
		BLOCK_L,
		BLOCK_J,
		BLOCK_Z,
		BLOCK_S,
		LAST_BLOCK = BLOCK_S
	};

	int map[TETRIS_MAPWIDTH][TETRIS_MAPHEIGHT];

	void ClearMap();
	void NewBlock();
	void Rotate(bool clockwise = true);

	bool Step();
	void Fall();
	void Collapse();

	int x, y;
	int block[4][4];
	BlockType current_block;
	int score;
	int lines;
};

class SnakePanel : public GamePanel {
public:
	SnakePanel(wxWindow* parent);
	~SnakePanel();

protected:
	virtual void Render(wxDC& pdc);
	virtual void GameLoop(int time);
	virtual void OnKey(wxKeyEvent& event, bool down);

	virtual int getFPS() const {
		return length / 3 + 3;
	}

	enum Direction {
		NORTH,
		EAST,
		SOUTH,
		WEST,
	};

	static const int SNAKE_MAPWIDTH = 30;
	static const int SNAKE_MAPHEIGHT = 20;

	int map[SNAKE_MAPWIDTH][SNAKE_MAPHEIGHT];

	void NewApple();
	void Move(int dir);
	void UpdateTitle();
	void NewGame();

	int last_dir;
	int length;
};

BEGIN_EVENT_TABLE(AboutWindow, wxDialog)
EVT_BUTTON(wxID_OK, AboutWindow::OnClickOK)
EVT_BUTTON(ABOUT_VIEW_LICENSE, AboutWindow::OnClickLicense)
EVT_BUTTON(wxID_APPLY, AboutWindow::OnClickUpdate)
EVT_MENU(ABOUT_RUN_TETRIS, AboutWindow::OnTetris)
EVT_MENU(ABOUT_RUN_SNAKE, AboutWindow::OnSnake)
EVT_MENU(wxID_CANCEL, AboutWindow::OnClickOK)
END_EVENT_TABLE()

AboutWindow::AboutWindow(wxWindow* parent) :
	wxDialog(parent, wxID_ANY, "About Mio's Map Editor", wxDefaultPosition, wxSize(440, 520), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	game_panel(nullptr) {

	SetBackgroundColour(wxColour(12, 22, 38));
	SetForegroundColour(wxColour(240, 245, 255));

	topsizer = new wxBoxSizer(wxVERTICAL);

	// Header Banner Panel (Corporate Design)
	wxPanel* headerPanel = new wxPanel(this, wxID_ANY);
	headerPanel->SetBackgroundColour(wxColour(18, 32, 54));
	wxBoxSizer* headerSizer = new wxBoxSizer(wxVERTICAL);

	wxStaticText* title = new wxStaticText(headerPanel, wxID_ANY, "Mio's Map Editor");
	title->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	title->SetForegroundColour(wxColour(240, 210, 120));

	wxStaticText* verBadge = new wxStaticText(headerPanel, wxID_ANY, wxString::Format("Version %s", __W_RME_VERSION__.c_str()));
	verBadge->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	verBadge->SetForegroundColour(wxColour(140, 210, 255));

	wxStaticText* maintainer = new wxStaticText(headerPanel, wxID_ANY, "Maintainer: Mioshiru | Community Edition");
	maintainer->SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	maintainer->SetForegroundColour(wxColour(180, 195, 215));

	headerSizer->Add(title, 0, wxBOTTOM, 2);
	headerSizer->Add(verBadge, 0, wxBOTTOM, 2);
	headerSizer->Add(maintainer, 0);
	headerPanel->SetSizer(headerSizer);
	topsizer->Add(headerPanel, 0, wxEXPAND | wxALL, 10);

	// Content Card Panel
	wxPanel* contentPanel = new wxPanel(this, wxID_ANY);
	contentPanel->SetBackgroundColour(wxColour(18, 32, 54));
	wxBoxSizer* contentSizer = new wxBoxSizer(wxVERTICAL);

	wxStaticText* featsTitle = new wxStaticText(contentPanel, wxID_ANY, "Features & Changelog Highlights (v1.9.0):");
	featsTitle->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	featsTitle->SetForegroundColour(wxColour(240, 210, 120));
	contentSizer->Add(featsTitle, 0, wxBOTTOM, 4);

	wxString feats;
	feats << "- Integrated Interactive Map Playtester with Real-Time Avatar & Weather (F6)\n";
	feats << "- Super-Smooth Dynamic HD Asset Upscaling & Clean Pixel Edge Filtering\n";
	feats << "- Realistic 2D Raycasted Light Wall Collision & Directional Wall Torches\n";
	feats << "- Fully Synchronized Animated Grounds & Shore/Lava Border Cascades\n";
	feats << "- Smart Zoom Performance Throttling (Automatic Idle/Anim Pause from 50% to 1% Zoom)\n";
	feats << "- Monster Creator, NPC Generator & Procedural Map Generation Suite\n";

	wxStaticText* featsText = new wxStaticText(contentPanel, wxID_ANY, feats);
	featsText->SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	featsText->SetForegroundColour(wxColour(200, 215, 235));
	contentSizer->Add(featsText, 0, wxBOTTOM, 8);

	wxString sysInfo;
	sysInfo << "Interface: " << wxVERSION_STRING << "\n";
	sysInfo << "OpenGL: " << wxString((char*)glGetString(GL_VERSION), wxConvUTF8) << "\n";
	sysInfo << "Compiled: " << __TDATE__ << " " << __TTIME__ << "\n";
	sysInfo << "Compiler: " << BOOST_COMPILER << "\n";

	wxStaticText* sysText = new wxStaticText(contentPanel, wxID_ANY, sysInfo);
	sysText->SetFont(wxFont(7, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	sysText->SetForegroundColour(wxColour(130, 150, 175));
	contentSizer->Add(sysText, 0);

	contentPanel->SetSizer(contentSizer);
	topsizer->Add(contentPanel, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	// Action Buttons
	wxBoxSizer* choicesizer = new wxBoxSizer(wxHORIZONTAL);
	wxButton* updateBtn = new wxButton(this, wxID_APPLY, "Check for Updates");
	updateBtn->SetBackgroundColour(wxColour(35, 75, 150));
	updateBtn->SetForegroundColour(wxColour(240, 210, 120));
	updateBtn->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));

	wxButton* okBtn = new wxButton(this, wxID_OK, "OK");
	okBtn->SetBackgroundColour(wxColour(22, 36, 58));
	okBtn->SetForegroundColour(wxColour(240, 245, 255));

	choicesizer->Add(updateBtn, 0, wxRIGHT, 8);
	choicesizer->Add(okBtn, 0);
	topsizer->Add(choicesizer, 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	wxAcceleratorEntry entries[3];
	entries[0].Set(wxACCEL_NORMAL, WXK_ESCAPE, wxID_CANCEL);
	entries[1].Set(wxACCEL_NORMAL, 't', ABOUT_RUN_TETRIS);
	entries[2].Set(wxACCEL_NORMAL, 's', ABOUT_RUN_SNAKE);
	wxAcceleratorTable accel(3, entries);
	SetAcceleratorTable(accel);

	SetSizerAndFit(topsizer);
	Centre(wxBOTH);
}

AboutWindow::~AboutWindow() {
}

void AboutWindow::OnClickUpdate(wxCommandEvent& WXUNUSED(event)) {
	MMEUpdater::Instance().CheckForUpdates(this, true);
}

void AboutWindow::OnClickOK(wxCommandEvent& WXUNUSED(event)) {
	EndModal(0);
}

void AboutWindow::OnClickLicense(wxCommandEvent& WXUNUSED(event)) {
	// Not used
}

void AboutWindow::OnTetris(wxCommandEvent& WXUNUSED(event)) {
	if (game_panel) return;
	topsizer->Clear(true);
	game_panel = new TetrisPanel(this);
	topsizer->Add(game_panel, 1, wxEXPAND);
	SetClientSize(game_panel->GetSize());
	Layout();
	game_panel->SetFocus();
}

void AboutWindow::OnSnake(wxCommandEvent& WXUNUSED(event)) {
	if (game_panel) return;
	topsizer->Clear(true);
	game_panel = new SnakePanel(this);
	topsizer->Add(game_panel, 1, wxEXPAND);
	SetClientSize(game_panel->GetSize());
	Layout();
	game_panel->SetFocus();
}

// Tetris and Snake game panel methods follow
BEGIN_EVENT_TABLE(GamePanel, wxPanel)
EVT_PAINT(GamePanel::OnPaint)
EVT_KEY_DOWN(GamePanel::OnKeyDown)
EVT_KEY_UP(GamePanel::OnKeyUp)
EVT_IDLE(GamePanel::OnIdle)
END_EVENT_TABLE()

GamePanel::GamePanel(wxWindow* parent, int width, int height) :
	wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(width, height), wxWANTS_CHARS),
	paused_val(false),
	dead(false) {
	game_timer.Start();
}

GamePanel::~GamePanel() {
}

void GamePanel::OnPaint(wxPaintEvent& WXUNUSED(event)) {
	wxPaintDC dc(this);
	Render(dc);
}

void GamePanel::OnKeyDown(wxKeyEvent& event) {
	OnKey(event, true);
}

void GamePanel::OnKeyUp(wxKeyEvent& event) {
	OnKey(event, false);
}

void GamePanel::OnIdle(wxIdleEvent& WXUNUSED(event)) {
	if (game_timer.Time() >= 1000 / getFPS()) {
		int time = game_timer.Time();
		game_timer.Start();
		GameLoop(time);
	}
}

TetrisPanel::TetrisPanel(wxWindow* parent) :
	GamePanel(parent, 200, 400),
	current_block(FIRST_BLOCK),
	score(0),
	lines(0) {
	ClearMap();
	NewBlock();
}

TetrisPanel::~TetrisPanel() {
}

void TetrisPanel::ClearMap() {
	for (int x = 0; x < TETRIS_MAPWIDTH; ++x) {
		for (int y = 0; y < TETRIS_MAPHEIGHT; ++y) {
			map[x][y] = NO_COLOR;
		}
	}
}

void TetrisPanel::NewBlock() {
	x = TETRIS_MAPWIDTH / 2 - 2;
	y = 0;
	current_block = (BlockType)random(FIRST_BLOCK, LAST_BLOCK);
	for (int bx = 0; bx < 4; ++bx) {
		for (int by = 0; by < 4; ++by) {
			block[bx][by] = NO_COLOR;
		}
	}
	switch (current_block) {
		case BLOCK_TOWER:
			block[1][0] = RED;
			block[1][1] = RED;
			block[1][2] = RED;
			block[1][3] = RED;
			break;
		case BLOCK_SQUARE:
			block[1][1] = BLUE;
			block[2][1] = BLUE;
			block[1][2] = BLUE;
			block[2][2] = BLUE;
			break;
		case BLOCK_TRIANGLE:
			block[0][1] = GREEN;
			block[1][1] = GREEN;
			block[2][1] = GREEN;
			block[1][2] = GREEN;
			break;
		case BLOCK_L:
			block[1][1] = STEEL;
			block[1][2] = STEEL;
			block[1][3] = STEEL;
			block[2][3] = STEEL;
			break;
		case BLOCK_J:
			block[2][1] = YELLOW;
			block[2][2] = YELLOW;
			block[2][3] = YELLOW;
			block[1][3] = YELLOW;
			break;
		case BLOCK_Z:
			block[1][1] = PURPLE;
			block[2][1] = PURPLE;
			block[2][2] = PURPLE;
			block[3][2] = PURPLE;
			break;
		case BLOCK_S:
			block[2][1] = WHITE;
			block[1][1] = WHITE;
			block[1][2] = WHITE;
			block[0][2] = WHITE;
			break;
	}
}

void TetrisPanel::Rotate(bool clockwise) {
	int new_block[4][4];
	for (int bx = 0; bx < 4; ++bx) {
		for (int by = 0; by < 4; ++by) {
			if (clockwise) {
				new_block[by][3 - bx] = block[bx][by];
			} else {
				new_block[3 - by][bx] = block[bx][by];
			}
		}
	}
	for (int bx = 0; bx < 4; ++bx) {
		for (int by = 0; by < 4; ++by) {
			if (new_block[bx][by] != NO_COLOR) {
				if (x + bx < 0 || x + bx >= TETRIS_MAPWIDTH || y + by >= TETRIS_MAPHEIGHT || (y + by >= 0 && map[x + bx][y + by] != NO_COLOR)) {
					return;
				}
			}
		}
	}
	for (int bx = 0; bx < 4; ++bx) {
		for (int by = 0; by < 4; ++by) {
			block[bx][by] = new_block[bx][by];
		}
	}
}

bool TetrisPanel::Step() {
	for (int bx = 0; bx < 4; ++bx) {
		for (int by = 0; by < 4; ++by) {
			if (block[bx][by] != NO_COLOR) {
				if (y + by + 1 >= TETRIS_MAPHEIGHT || (y + by + 1 >= 0 && map[x + bx][y + by + 1] != NO_COLOR)) {
					return false;
				}
			}
		}
	}
	y += 1;
	return true;
}

void TetrisPanel::Fall() {
	while (Step()) {}
	Collapse();
}

void TetrisPanel::Collapse() {
	for (int bx = 0; bx < 4; ++bx) {
		for (int by = 0; by < 4; ++by) {
			if (block[bx][by] != NO_COLOR && y + by >= 0 && y + by < TETRIS_MAPHEIGHT && x + bx >= 0 && x + bx < TETRIS_MAPWIDTH) {
				map[x + bx][y + by] = block[bx][by];
			}
		}
	}
	for (int my = TETRIS_MAPHEIGHT - 1; my >= 0; --my) {
		bool full = true;
		for (int mx = 0; mx < TETRIS_MAPWIDTH; ++mx) {
			if (map[mx][my] == NO_COLOR) {
				full = false;
				break;
			}
		}
		if (full) {
			lines++;
			for (int cy = my; cy > 0; --cy) {
				for (int cx = 0; cx < TETRIS_MAPWIDTH; ++cx) {
					map[cx][cy] = map[cx][cy - 1];
				}
			}
			for (int cx = 0; cx < TETRIS_MAPWIDTH; ++cx) {
				map[cx][0] = NO_COLOR;
			}
			my++;
		}
	}
	NewBlock();
}

void TetrisPanel::GameLoop(int time) {
	if (!Step()) {
		Collapse();
	}
	Refresh();
}

void TetrisPanel::OnKey(wxKeyEvent& event, bool down) {
	if (!down) return;
	switch (event.GetKeyCode()) {
		case WXK_LEFT:
			x--;
			break;
		case WXK_RIGHT:
			x++;
			break;
		case WXK_UP:
			Rotate(true);
			break;
		case WXK_DOWN:
			Step();
			break;
		case WXK_SPACE:
			Fall();
			break;
	}
	Refresh();
}

void TetrisPanel::Render(wxDC& dc) {
	dc.SetBackground(wxBrush(wxColour(12, 22, 38)));
	dc.Clear();
	int cw = GetSize().x / TETRIS_MAPWIDTH;
	int ch = GetSize().y / TETRIS_MAPHEIGHT;

	for (int mx = 0; mx < TETRIS_MAPWIDTH; ++mx) {
		for (int my = 0; my < TETRIS_MAPHEIGHT; ++my) {
			if (map[mx][my] != NO_COLOR) {
				dc.SetBrush(wxBrush(wxColour(200, 150, 80)));
				dc.DrawRectangle(mx * cw, my * ch, cw, ch);
			}
		}
	}
	for (int bx = 0; bx < 4; ++bx) {
		for (int by = 0; by < 4; ++by) {
			if (block[bx][by] != NO_COLOR) {
				dc.SetBrush(wxBrush(wxColour(240, 210, 120)));
				dc.DrawRectangle((x + bx) * cw, (y + by) * ch, cw, ch);
			}
		}
	}
}

// SnakePanel implementation
SnakePanel::SnakePanel(wxWindow* parent) :
	GamePanel(parent, 300, 200),
	last_dir(EAST),
	length(3) {
	NewGame();
}

SnakePanel::~SnakePanel() {
}

void SnakePanel::NewGame() {
	for (int x = 0; x < SNAKE_MAPWIDTH; ++x) {
		for (int y = 0; y < SNAKE_MAPHEIGHT; ++y) {
			map[x][y] = 0;
		}
	}
	length = 3;
	map[5][5] = 1;
	map[6][5] = 2;
	map[7][5] = 3;
	last_dir = EAST;
	NewApple();
}

void SnakePanel::NewApple() {
	int ax = rand() % SNAKE_MAPWIDTH;
	int ay = rand() % SNAKE_MAPHEIGHT;
	if (map[ax][ay] == 0) {
		map[ax][ay] = -1;
	}
}

void SnakePanel::UpdateTitle() {
}

void SnakePanel::Move(int dir) {
	last_dir = dir;
	Refresh();
}

void SnakePanel::GameLoop(int time) {
	Refresh();
}

void SnakePanel::OnKey(wxKeyEvent& event, bool down) {
	if (!down) return;
	switch (event.GetKeyCode()) {
		case WXK_UP: if (last_dir != SOUTH) Move(NORTH); break;
		case WXK_DOWN: if (last_dir != NORTH) Move(SOUTH); break;
		case WXK_LEFT: if (last_dir != EAST) Move(WEST); break;
		case WXK_RIGHT: if (last_dir != WEST) Move(EAST); break;
	}
}

void SnakePanel::Render(wxDC& dc) {
	dc.SetBackground(wxBrush(wxColour(12, 22, 38)));
	dc.Clear();
}
