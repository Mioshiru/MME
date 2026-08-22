//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "main.h"
#include "playtest_window.h"
#include "editor.h"
#include "map.h"
#include "tile.h"
#include "item.h"
#include "items.h"
#include "gui.h"
#include "map_tab.h"
#include "map_display.h"
#include "graphics.h"
#include "style_manager.h"
#include <GL/gl.h>
#include <cmath>

BEGIN_EVENT_TABLE(PlaytestDialog, wxDialog)
EVT_KEY_DOWN(PlaytestDialog::OnKeyDown)
EVT_TIMER(wxID_ANY, PlaytestDialog::OnTimer)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(PlaytestCanvas, wxGLCanvas)
EVT_PAINT(PlaytestCanvas::OnPaint)
EVT_KEY_DOWN(PlaytestCanvas::OnKeyDown)
EVT_LEFT_DOWN(PlaytestCanvas::OnMouseClick)
EVT_RIGHT_DOWN(PlaytestCanvas::OnMouseClick)
END_EVENT_TABLE()

PlaytestDialog::PlaytestDialog(wxWindow* parent, Editor& editor) :
	wxDialog(parent, wxID_ANY, "Map Playtest Mode - Mio's Map Editor", wxDefaultPosition, wxDefaultSize,
	         wxDEFAULT_DIALOG_STYLE | wxCAPTION | wxCLOSE_BOX),
	editor(editor),
	game_timer(this) {

	int center_x = 32000;
	int center_y = 32000;
	int center_z = g_gui.GetCurrentFloor();
	MapTab* current_tab = dynamic_cast<MapTab*>(g_gui.GetCurrentTab());
	if (current_tab && current_tab->GetCanvas()) {
		current_tab->GetCanvas()->GetScreenCenter(&center_x, &center_y);
	}
	player_pos = Position(center_x, center_y, center_z);

	wxColor bg(18, 22, 30);
	SetBackgroundColour(bg);

	wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);

	// ── Top Header Toolbar (Weather Selector & Status) ─────────────────────────
	wxPanel* headerPanel = new wxPanel(this, wxID_ANY);
	headerPanel->SetBackgroundColour(wxColor(12, 16, 24));
	wxBoxSizer* headerSizer = new wxBoxSizer(wxHORIZONTAL);

	wxStaticText* weatherLbl = new wxStaticText(headerPanel, wxID_ANY, "Atmospheric Weather:");
	weatherLbl->SetForegroundColour(wxColor(200, 210, 225));
	wxFont lblFont = weatherLbl->GetFont();
	lblFont.SetPointSize(9);
	weatherLbl->SetFont(lblFont);
	headerSizer->Add(weatherLbl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);

	wxArrayString weather_types;
	weather_types.Add("Weather: Off");
	weather_types.Add("Weather: Clouds & Shadows");
	weather_types.Add("Weather: Rain & Storm");
	weather_types.Add("Weather: Snow & Blizzard");
	weather_types.Add("Weather: Desert Heat");
	weather_types.Add("Weather: Dense Fog");

	weather_choice = new wxChoice(headerPanel, wxID_ANY, wxDefaultPosition, wxSize(180, -1), weather_types);
	weather_choice->SetSelection(0);
	weather_choice->SetBackgroundColour(wxColor(24, 30, 42));
	weather_choice->SetForegroundColour(wxColor(255, 215, 0));
	weather_choice->Bind(wxEVT_CHOICE, &PlaytestDialog::OnWeatherChanged, this);
	headerSizer->Add(weather_choice, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 8);

	headerSizer->AddStretchSpacer();

	wxStaticText* hintLbl = new wxStaticText(headerPanel, wxID_ANY, "[F6/ESC: Exit] [WASD: Move] [R-Click: Use/Stairs]");
	hintLbl->SetForegroundColour(wxColor(140, 160, 180));
	headerSizer->Add(hintLbl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);

	headerPanel->SetSizer(headerSizer);
	topSizer->Add(headerPanel, 0, wxEXPAND | wxALL, 2);

	// ── Playtest Canvas: Fixed classic Clientbox viewport (15x11 tiles = 480x352 px) ──
	canvas = new PlaytestCanvas(this, wxSize(480, 352));
	topSizer->Add(canvas, 0, wxALIGN_CENTER | wxALL, 4);

	// ── Bottom Information Panel ──────────────────────────────────────────────
	wxPanel* infoPanel = new wxPanel(this, wxID_ANY);
	infoPanel->SetBackgroundColour(wxColor(10, 14, 20));
	wxBoxSizer* infoSizer = new wxBoxSizer(wxVERTICAL);

	status_bar_txt = new wxStaticText(infoPanel, wxID_ANY, last_action_text);
	status_bar_txt->SetForegroundColour(wxColor(255, 215, 0));
	wxFont statusFont = status_bar_txt->GetFont();
	statusFont.SetPointSize(9);
	statusFont.SetWeight(wxFONTWEIGHT_BOLD);
	status_bar_txt->SetFont(statusFont);
	infoSizer->Add(status_bar_txt, 0, wxALL | wxEXPAND, 6);

	infoPanel->SetSizer(infoSizer);
	topSizer->Add(infoPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);

	SetSizerAndFit(topSizer);
	Centre(wxBOTH);

	game_timer.Start(33); // 30 FPS tick
	canvas->SetFocus();
}

PlaytestDialog::~PlaytestDialog() {
	game_timer.Stop();
}

void PlaytestDialog::OnWeatherChanged(wxCommandEvent& event) {
	if (weather_choice) {
		current_weather = weather_choice->GetSelection();
		if (canvas) canvas->Refresh(false);
	}
}

void PlaytestDialog::SetStatusMessage(const wxString& msg) {
	last_action_text = msg;
	if (status_bar_txt) {
		status_bar_txt->SetLabel(msg);
	}
}

void PlaytestDialog::ChangeFloor(int delta) {
	int new_z = player_pos.z + delta;
	if (new_z >= 0 && new_z <= 15) {
		player_pos.z = new_z;
		SetStatusMessage(wxString::Format("Floor changed to %d. Pos: (%d, %d, %d)", player_pos.z, player_pos.x, player_pos.y, player_pos.z));
		if (canvas) canvas->Refresh(false);
	}
}

bool PlaytestDialog::MovePlayer(int dx, int dy) {
	if (dy < 0) player_dir = 0;      // North
	else if (dx > 0) player_dir = 1; // East
	else if (dy > 0) player_dir = 2; // South
	else if (dx < 0) player_dir = 3; // West

	step_counter++;
	anim_frame = (anim_frame + 1) % 4;

	int tx = player_pos.x + dx;
	int ty = player_pos.y + dy;
	int tz = player_pos.z;

	Tile* target_tile = editor.map.getTile(tx, ty, tz);
	if (!target_tile || !target_tile->ground) {
		SetStatusMessage(wxString::Format("Blocked: No ground at (%d, %d, %d).", tx, ty, tz));
		return false;
	}

	ItemType& ground_type = g_items[target_tile->ground->getID()];
	if (ground_type.unpassable && !ground_type.floorChangeNorth && !ground_type.floorChangeSouth &&
	    !ground_type.floorChangeEast && !ground_type.floorChangeWest && !ground_type.floorChangeDown) {
		SetStatusMessage(wxString::Format("Blocked by ground (%s).", ground_type.name));
		return false;
	}

	// Check items on target tile for solid blocks
	for (auto* itm : target_tile->items) {
		if (itm) {
			ItemType& it = g_items[itm->getID()];
			if (it.unpassable && !it.isOpen && !it.floorChangeNorth && !it.floorChangeSouth &&
			    !it.floorChangeEast && !it.floorChangeWest && !it.floorChangeDown) {
				SetStatusMessage(wxString::Format("Blocked by %s.", it.name));
				return false;
			}
		}
	}

	// Move accepted!
	player_pos.x = tx;
	player_pos.y = ty;

	// Check floor change triggers (Stairs, Ramps, Ladders, Holes)
	Tile* new_tile = editor.map.getTile(player_pos.x, player_pos.y, player_pos.z);
	if (new_tile) {
		std::vector<Item*> all_items;
		if (new_tile->ground) all_items.push_back(new_tile->ground);
		for (auto* itm : new_tile->items) if (itm) all_items.push_back(itm);

		bool changed_floor = false;
		for (auto* itm : all_items) {
			ItemType& it = g_items[itm->getID()];
			std::string lname = it.name;
			std::transform(lname.begin(), lname.end(), lname.begin(), ::tolower);

			if (it.floorChangeNorth || lname.find("ramp north") != std::string::npos || lname.find("stairs north") != std::string::npos) {
				player_pos.z = std::max(0, player_pos.z - 1);
				player_pos.y = player_pos.y - 1;
				SetStatusMessage(wxString::Format("Climbed north to Floor %d!", player_pos.z));
				changed_floor = true;
				break;
			} else if (it.floorChangeSouth || lname.find("ramp south") != std::string::npos || lname.find("stairs south") != std::string::npos) {
				player_pos.z = std::max(0, player_pos.z - 1);
				player_pos.y = player_pos.y + 1;
				SetStatusMessage(wxString::Format("Climbed south to Floor %d!", player_pos.z));
				changed_floor = true;
				break;
			} else if (it.floorChangeEast || lname.find("ramp east") != std::string::npos || lname.find("stairs east") != std::string::npos) {
				player_pos.z = std::max(0, player_pos.z - 1);
				player_pos.x = player_pos.x + 1;
				SetStatusMessage(wxString::Format("Climbed east to Floor %d!", player_pos.z));
				changed_floor = true;
				break;
			} else if (it.floorChangeWest || lname.find("ramp west") != std::string::npos || lname.find("stairs west") != std::string::npos) {
				player_pos.z = std::max(0, player_pos.z - 1);
				player_pos.x = player_pos.x - 1;
				SetStatusMessage(wxString::Format("Climbed west to Floor %d!", player_pos.z));
				changed_floor = true;
				break;
			} else if (it.floorChangeDown || lname.find("hole") != std::string::npos || lname.find("pit") != std::string::npos || lname.find("trapdoor") != std::string::npos) {
				player_pos.z = std::min(15, player_pos.z + 1);
				SetStatusMessage(wxString::Format("Fell through hole to Floor %d!", player_pos.z));
				changed_floor = true;
				break;
			} else if (lname.find("stair") != std::string::npos || lname.find("ramp") != std::string::npos || lname.find("ladder") != std::string::npos) {
				player_pos.z = std::max(0, player_pos.z - 1);
				SetStatusMessage(wxString::Format("Climbed stairs up to Floor %d!", player_pos.z));
				changed_floor = true;
				break;
			}
		}
	}

	SetStatusMessage(wxString::Format("Pos: (%d, %d, %d) | Floor: %d", player_pos.x, player_pos.y, player_pos.z, player_pos.z));
	return true;
}

void PlaytestDialog::InteractWithFacing() {
	int f_dx = 0, f_dy = 0;
	if (player_dir == 0) f_dy = -1;
	else if (player_dir == 1) f_dx = 1;
	else if (player_dir == 2) f_dy = 1;
	else if (player_dir == 3) f_dx = -1;

	Tile* front_tile = editor.map.getTile(player_pos.x + f_dx, player_pos.y + f_dy, player_pos.z);
	Tile* stand_tile = editor.map.getTile(player_pos.x, player_pos.y, player_pos.z);

	// 1. Check standing tile for Rope Spot, Ladder, Hole Up, Sewer Grate
	if (stand_tile) {
		std::vector<Item*> items;
		if (stand_tile->ground) items.push_back(stand_tile->ground);
		for (auto* itm : stand_tile->items) if (itm) items.push_back(itm);

		for (auto* itm : items) {
			ItemType& it = g_items[itm->getID()];
			std::string lname = it.name;
			std::transform(lname.begin(), lname.end(), lname.begin(), ::tolower);

			if (lname.find("rope") != std::string::npos || lname.find("ladder") != std::string::npos ||
			    lname.find("sewer grate") != std::string::npos || itm->getID() == 384 || itm->getID() == 416 ||
			    itm->getID() == 417 || itm->getID() == 8592) {
				player_pos.z = std::max(0, player_pos.z - 1);
				SetStatusMessage(wxString::Format("Climbed rope/ladder up to Floor %d!", player_pos.z));
				if (canvas) canvas->Refresh(false);
				return;
			}
		}
	}

	Tile* target = front_tile ? front_tile : stand_tile;
	if (!target) {
		SetStatusMessage("Nothing to interact with.");
		return;
	}

	bool interacted = false;
	for (auto* itm : target->items) {
		if (!itm) continue;
		ItemType& it = g_items[itm->getID()];
		std::string lname = it.name;
		std::transform(lname.begin(), lname.end(), lname.begin(), ::tolower);

		// 1. Ladders / Stairs in front
		if (lname.find("ladder") != std::string::npos || lname.find("stair") != std::string::npos || lname.find("ramp") != std::string::npos) {
			player_pos.z = std::max(0, player_pos.z - 1);
			SetStatusMessage(wxString::Format("Used ladder/stairs up to Floor %d!", player_pos.z));
			interacted = true;
			break;
		}

		// 2. Door toggle (open / close)
		if (it.isDoor() || it.isBrushDoor || lname.find("door") != std::string::npos || it.isOpen || it.rotateTo != 0) {
			if (it.rotateTo != 0) {
				itm->setID(it.rotateTo);
			} else if (it.isOpen) {
				itm->setID(itm->getID() - 1);
			} else {
				itm->setID(itm->getID() + 1);
			}
			SetStatusMessage(wxString::Format("Used %s (toggled).", it.name));
			interacted = true;
			break;
		}

		// 3. Chest / Container
		if (it.isContainer() || it.group == ITEM_GROUP_CONTAINER || lname.find("chest") != std::string::npos || lname.find("box") != std::string::npos) {
			SetStatusMessage(wxString::Format("You opened %s! The container is secure.", it.name));
			interacted = true;
			break;
		}

		// 4. Lever / Switch
		if (lname.find("lever") != std::string::npos || lname.find("switch") != std::string::npos) {
			if (it.rotateTo != 0) {
				itm->setID(it.rotateTo);
			} else if (itm->getID() == 1945) {
				itm->setID(1946);
			} else if (itm->getID() == 1946) {
				itm->setID(1945);
			}
			SetStatusMessage(wxString::Format("Flipped %s!", it.name));
			interacted = true;
			break;
		}

		// 5. Readable / Sign / Book
		if (it.canReadText || lname.find("sign") != std::string::npos || lname.find("book") != std::string::npos) {
			SetStatusMessage(wxString::Format("You read %s: \"%s\"", it.name, itm->getText().empty() ? "The page is blank." : itm->getText()));
			interacted = true;
			break;
		}
	}

	if (!interacted) {
		SetStatusMessage("Nothing to use here.");
	}

	if (canvas) canvas->Refresh(false);
}

void PlaytestDialog::OnKeyDown(wxKeyEvent& event) {
	int key = event.GetKeyCode();
	switch (key) {
		case 'W': case 'w': case WXK_UP:
			MovePlayer(0, -1);
			break;
		case 'S': case 's': case WXK_DOWN:
			MovePlayer(0, 1);
			break;
		case 'A': case 'a': case WXK_LEFT:
			MovePlayer(-1, 0);
			break;
		case 'D': case 'd': case WXK_RIGHT:
			MovePlayer(1, 0);
			break;
		case WXK_SPACE: case WXK_RETURN:
			InteractWithFacing();
			break;
		case WXK_PAGEUP: case '+':
			ChangeFloor(-1);
			break;
		case WXK_PAGEDOWN: case '-':
			ChangeFloor(1);
			break;
		case WXK_ESCAPE:
			EndModal(wxID_OK);
			return;
		default:
			event.Skip();
			return;
	}
	if (canvas) canvas->Refresh(false);
}

void PlaytestDialog::OnTimer(wxTimerEvent& event) {
	if (canvas) {
		canvas->Refresh(false);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// PlaytestCanvas
// ─────────────────────────────────────────────────────────────────────────────

PlaytestCanvas::PlaytestCanvas(PlaytestDialog* parent, wxSize size) :
	wxGLCanvas(parent, wxID_ANY, nullptr, wxDefaultPosition, size, wxWANTS_CHARS | wxBORDER_SUNKEN),
	dialog(parent) {
	gl_context = g_gui.GetGLContext(this);
}

PlaytestCanvas::~PlaytestCanvas() {
}

void PlaytestCanvas::OnKeyDown(wxKeyEvent& event) {
	if (dialog) {
		dialog->GetEventHandler()->ProcessEvent(event);
	}
}

void PlaytestCanvas::OnMouseClick(wxMouseEvent& event) {
	SetFocus();
	if (dialog) {
		if (event.RightDown() || event.LeftDown()) {
			dialog->InteractWithFacing();
		}
	}
}

void PlaytestCanvas::OnPaint(wxPaintEvent& event) {
	wxPaintDC dc(this);
	Render();
}

static void DrawPlaytestSprite(GameSprite* spr, int screenx, int screeny, float time = 0.0f) {
	if (!spr) return;
	screenx -= spr->getDrawOffset().first;
	screeny -= spr->getDrawOffset().second;

	for (int cx = 0; cx < spr->width; ++cx) {
		for (int cy = 0; cy < spr->height; ++cy) {
			for (int cf = 0; cf < spr->layers; ++cf) {
				GLuint tex = spr->getHardwareID(cx, cy, cf, -1, 0, 0, 0, 0);
				if (tex != 0) {
					glBindTexture(GL_TEXTURE_2D, tex);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

					int px = screenx - cx * 32;
					int py = screeny - cy * 32;

					glColor4ub(255, 255, 255, 255);
					glBegin(GL_QUADS);
					glTexCoord2f(0.0f, 0.0f); glVertex2i(px, py);
					glTexCoord2f(1.0f, 0.0f); glVertex2i(px + 32, py);
					glTexCoord2f(1.0f, 1.0f); glVertex2i(px + 32, py + 32);
					glTexCoord2f(0.0f, 1.0f); glVertex2i(px, py + 32);
					glEnd();
				}
			}
		}
	}
}

void PlaytestCanvas::RenderWeather(int weather, int w, int h, float time) {
	if (weather <= 0) return;

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Weather 1: Clouds & Shadows
	if (weather == 1) {
		for (int i = 0; i < 4; ++i) {
			float cx = fmod(time * (15.0f + i * 8.0f) + i * 150.0f, w + 200.0f) - 100.0f;
			float cy = fmod(i * 100.0f + time * 6.0f, h + 100.0f) - 50.0f;
			glColor4ub(0, 0, 0, 45);
			glBegin(GL_TRIANGLE_FAN);
			glVertex2f(cx, cy);
			for (int a = 0; a <= 16; ++a) {
				float ang = a * (3.14159f * 2.0f / 16.0f);
				glVertex2f(cx + cos(ang) * (90.0f + i * 20.0f), cy + sin(ang) * (50.0f + i * 15.0f));
			}
			glEnd();
		}
	}
	// Weather 2: Rain & Storm
	else if (weather == 2) {
		// Dark stormy ambient tint
		glColor4ub(10, 20, 35, 75);
		glBegin(GL_QUADS);
		glVertex2i(0, 0); glVertex2i(w, 0); glVertex2i(w, h); glVertex2i(0, h);
		glEnd();

		// Rain streaks
		glColor4ub(180, 210, 255, 180);
		glLineWidth(1.5f);
		glBegin(GL_LINES);
		for (int i = 0; i < 60; ++i) {
			float rx = fmod((float)(i * 47) + time * 80.0f, (float)w);
			float ry = fmod((float)(i * 29) + time * 450.0f, (float)h);
			glVertex2f(rx, ry);
			glVertex2f(rx - 6.0f, ry + 16.0f);
		}
		glEnd();
	}
	// Weather 3: Snow & Blizzard
	else if (weather == 3) {
		glColor4ub(240, 248, 255, 210);
		for (int i = 0; i < 50; ++i) {
			float sx = fmod((float)(i * 37) + sin(time * 2.0f + i) * 12.0f + time * 25.0f, (float)w);
			float sy = fmod((float)(i * 23) + time * 70.0f, (float)h);
			glBegin(GL_QUADS);
			glVertex2f(sx, sy);
			glVertex2f(sx + 3.0f, sy);
			glVertex2f(sx + 3.0f, sy + 3.0f);
			glVertex2f(sx, sy + 3.0f);
			glEnd();
		}
	}
	// Weather 4: Desert Heat / Haze
	else if (weather == 4) {
		float pulse = 0.5f + 0.5f * sin(time * 2.0f);
		glColor4ub(255, 170, 40, (uint8_t)(30 + 15 * pulse));
		glBegin(GL_QUADS);
		glVertex2i(0, 0); glVertex2i(w, 0); glVertex2i(w, h); glVertex2i(0, h);
		glEnd();
	}
	// Weather 5: Dense Fog / Mist
	else if (weather == 5) {
		glColor4ub(200, 210, 220, 110);
		glBegin(GL_QUADS);
		glVertex2i(0, 0); glVertex2i(w, 0); glVertex2i(w, h); glVertex2i(0, h);
		glEnd();
	}

	glEnable(GL_TEXTURE_2D);
}

void PlaytestCanvas::Render() {
	if (!IsShown() || !dialog || !gl_context) return;
	SetCurrent(*gl_context);

	sim_time += 0.033f;
	wxSize sz = GetClientSize();
	glViewport(0, 0, sz.x, sz.y);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, sz.x, sz.y, 0, -1000.0, 1000.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	Position pos = dialog->GetPlayerPos();
	const int tile_px = 32;
	const int half_w = sz.x / 2;
	const int half_h = sz.y / 2;

	int tiles_x = (sz.x / tile_px) + 2;
	int tiles_y = (sz.y / tile_px) + 2;
	int start_x = pos.x - tiles_x / 2;
	int end_x   = pos.x + tiles_x / 2;
	int start_y = pos.y - tiles_y / 2;
	int end_y   = pos.y + tiles_y / 2;

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	Editor& ed = dialog->GetEditor();

	// Render floor tiles from lowest visible floor up to current player floor
	for (int z = std::min(15, pos.z + 2); z >= pos.z; --z) {
		for (int ty = start_y; ty <= end_y; ++ty) {
			for (int tx = start_x; tx <= end_x; ++tx) {
				Tile* t = ed.map.getTile(tx, ty, z);
				if (!t) continue;

				int screen_x = half_w + (tx - pos.x) * tile_px - 16;
				int screen_y = half_h + (ty - pos.y) * tile_px - 16;

				// 1. Ground
				if (t->ground) {
					ItemType& it = g_items[t->ground->getID()];
					DrawPlaytestSprite(it.sprite, screen_x, screen_y, sim_time);
				}

				// 2. Items on tile
				for (auto* itm : t->items) {
					if (!itm) continue;
					ItemType& it = g_items[itm->getID()];
					DrawPlaytestSprite(it.sprite, screen_x, screen_y, sim_time);
				}
			}
		}
	}

	// ── Draw Centered Hero Avatar ─────────────────────────────────────────────
	glDisable(GL_TEXTURE_2D);
	int px = half_w - 16;
	int py = half_h - 16;

	// Hero Shadow
	glColor4ub(0, 0, 0, 110);
	glBegin(GL_TRIANGLE_FAN);
	glVertex2f(px + 16, py + 26);
	for (int a = 0; a <= 16; ++a) {
		float angle = a * (3.14159f * 2.0f / 16.0f);
		glVertex2f(px + 16 + cos(angle) * 12.0f, py + 26 + sin(angle) * 6.0f);
	}
	glEnd();

	// Hero Body (Golden Hero Armor)
	glColor4ub(255, 205, 50, 255);
	glBegin(GL_QUADS);
	glVertex2i(px + 6, py + 8);
	glVertex2i(px + 26, py + 8);
	glVertex2i(px + 24, py + 28);
	glVertex2i(px + 8, py + 28);
	glEnd();

	// Hero Helmet / Head
	glColor4ub(240, 230, 210, 255);
	glBegin(GL_QUADS);
	glVertex2i(px + 10, py + 2);
	glVertex2i(px + 22, py + 2);
	glVertex2i(px + 22, py + 12);
	glVertex2i(px + 10, py + 12);
	glEnd();

	// Directional Indicator
	glColor4ub(50, 120, 220, 255);
	int dir = dialog->GetPlayerDir();
	if (dir == 0) {      // North
		glBegin(GL_TRIANGLES);
		glVertex2i(px + 16, py - 4);
		glVertex2i(px + 10, py + 4);
		glVertex2i(px + 22, py + 4);
		glEnd();
	} else if (dir == 1) { // East
		glBegin(GL_TRIANGLES);
		glVertex2i(px + 32, py + 16);
		glVertex2i(px + 24, py + 10);
		glVertex2i(px + 24, py + 22);
		glEnd();
	} else if (dir == 2) { // South
		glBegin(GL_TRIANGLES);
		glVertex2i(px + 16, py + 34);
		glVertex2i(px + 10, py + 26);
		glVertex2i(px + 22, py + 26);
		glEnd();
	} else if (dir == 3) { // West
		glBegin(GL_TRIANGLES);
		glVertex2i(px, py + 16);
		glVertex2i(px + 8, py + 10);
		glVertex2i(px + 8, py + 22);
		glEnd();
	}

	// ── Render Live Atmospheric Weather Effect ───────────────────────────────
	RenderWeather(dialog->GetWeather(), sz.x, sz.y, sim_time);

	// ── Top HUD (Coordinates & Health/Mana) ──────────────────────────────────
	glDisable(GL_TEXTURE_2D);
	glColor4ub(12, 16, 24, 210);
	glBegin(GL_QUADS);
	glVertex2i(8, 8);
	glVertex2i(180, 8);
	glVertex2i(180, 42);
	glVertex2i(8, 42);
	glEnd();

	// Health bar (Green)
	glColor4ub(40, 190, 60, 255);
	glBegin(GL_QUADS);
	glVertex2i(12, 12);
	glVertex2i(176, 12);
	glVertex2i(176, 22);
	glVertex2i(12, 22);
	glEnd();

	// Mana bar (Blue)
	glColor4ub(40, 120, 240, 255);
	glBegin(GL_QUADS);
	glVertex2i(12, 26);
	glVertex2i(150, 26);
	glVertex2i(150, 36);
	glVertex2i(12, 36);
	glEnd();

	glEnable(GL_TEXTURE_2D);
	SwapBuffers();
}
