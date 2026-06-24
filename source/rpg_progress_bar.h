#ifndef RME_RPG_PROGRESS_BAR_H_
#define RME_RPG_PROGRESS_BAR_H_

#include <wx/panel.h>
#include <wx/dcclient.h>

class RPGProgressBar : public wxPanel {
public:
    // Korrigierter Konstruktor
    RPGProgressBar(wxWindow* parent, wxWindowID id);

    void SetProgress(float value);

private:
    void OnPaint(wxPaintEvent& event);

    float m_progress;
};

#endif
