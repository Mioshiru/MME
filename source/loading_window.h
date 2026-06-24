#ifndef __RME_LOADING_WINDOW_H__
#define __RME_LOADING_WINDOW_H__

#include "main.h"

class RPGProgressBar;

class RPGLoadingWindow : public wxDialog {
public:
    RPGLoadingWindow(wxWindow* parent, const wxString& title);
    void SetProgress(int percent, const wxString& message);

private:
    void OnPaint(wxPaintEvent& event);

    int m_percent;
    wxString m_message;
    wxColour m_gold, m_bg;
    RPGProgressBar* m_rpg_bar;

};

#endif
