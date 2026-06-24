#include "main.h"
#include "rpg_progress_bar.h"
#include <algorithm>
#include <wx/graphics.h> // Für wxGraphicsContext

RPGProgressBar::RPGProgressBar(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id, wxDefaultPosition, wxSize(-1, 30))
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    m_progress = 0.0f;
    // Events dynamisch binden, um DLL-Import-Probleme zu vermeiden
    Bind(wxEVT_PAINT, &RPGProgressBar::OnPaint, this);
}

// Implementierung des SetProgress-Setters
void RPGProgressBar::SetProgress(float value) { 
    m_progress = std::max(0.0f, std::min(1.0f, value)); 
    Refresh(); 
    Update(); // Erzwingt sofortiges Neuzeichnen beim Laden
}

void RPGProgressBar::OnPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();
    
    wxGraphicsContext* gc = wxGraphicsContext::Create(dc); // wxGraphicsContext::Create erwartet wxDC&
    if (!gc) return;

    wxSize size = GetSize();
    double margin = 2.0;
    
    gc->SetBrush(wxBrush(wxColor(10, 20, 35))); // Hintergrundfarbe
    gc->SetPen(wxPen(wxColor(180, 150, 50), 2)); // Rahmenfarbe
    gc->DrawRectangle(margin, margin, size.x - 2 * margin, size.y - 2 * margin);

    if (m_progress > 0) {
        double fillWidth = (size.x - 4 * margin) * m_progress;
        
        wxGraphicsGradientStops stops(wxColor(60, 120, 220), wxColor(20, 50, 130));
        gc->SetBrush(gc->CreateLinearGradientBrush(0, 0, 0, size.y, stops));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRectangle(2 * margin, 2 * margin, fillWidth, size.y - 4 * margin);
        
        gc->SetBrush(wxBrush(wxColor(255, 255, 255, 40)));
        gc->DrawRectangle(2 * margin, 2 * margin, fillWidth, (size.y - 4 * margin) / 2.0);
    }
    delete gc;
}
