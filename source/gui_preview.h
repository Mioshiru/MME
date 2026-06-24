#ifndef RME_GUI_PREVIEW_H
#define RME_GUI_PREVIEW_H
#include "main.h"
#include "gui.h"
#include "map_display.h"
#include "editor.h"
#include "map_drawer.h"
class InGamePreviewCanvas : public MapCanvas {
public:
	InGamePreviewCanvas(wxWindow* parent, Editor& ed) : MapCanvas(parent, ed, nullptr), vx(0), vy(0) {
		floor = 7; zoom = 1.0; drawer->getOptions().SetIngame(); drawer->getOptions().show_ingame_box = true;
	}
	void SetZoom(double v) override { zoom = std::clamp(v, 0.1, 25.0); Refresh(); }
	void GetViewBox(int* sx, int* sy, int* sw, int* sh) const override { 
		wxSize s = GetClientSize(); *sw = s.x; *sh = s.y; *sx = vx; *sy = vy; 
	}
	void ScreenToMap(int sx, int sy, int* mx, int* my) override {
		sx *= GetContentScaleFactor(); sy *= GetContentScaleFactor();
		*mx = (sx < 0 ? (vx + sx) : int(vx + (sx * zoom))) / 32 + (floor <= 7 ? 7 - floor : 0);
		*my = (sy < 0 ? (vy + sy) : int(vy + (sy * zoom))) / 32 + (floor <= 7 ? 7 - floor : 0);
	}
	void SetPosition(int x, int y, int z) {
		wxSize s = GetClientSize(); vx = (x * 32) - (s.x * zoom) / 2; vy = (y * 32) - (s.y * zoom) / 2;
		floor = z; Refresh();
	}
	void SyncView() {
		if (MapTab* t = g_gui.GetCurrentMapTab()) {
			int cx, cy; t->GetCanvas()->GetScreenCenter(&cx, &cy);
			SetPosition(cx, cy, (int)t->GetCanvas()->GetFloor());
		}
	}
private:
	int vx, vy;
	DECLARE_EVENT_TABLE()
};
#endif

