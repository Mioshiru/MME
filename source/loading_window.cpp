#include "main.h"
#include "loading_window.h"
#include "rpg_progress_bar.h"

RPGLoadingWindow::RPGLoadingWindow(wxWindow* parent, const wxString& title) :
    wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxSTAY_ON_TOP),
    m_percent(0),
    m_gold(180, 140, 50),
    m_bg(10, 15, 25) {
    
    SetBackgroundColour(m_bg);
    SetClientSize(FROM_DIP(this, wxSize(450, 150)));
    
    // Layout-Management
    wxBoxSizer* mainSizer = newd wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(FROM_DIP(this, 20));
    
    // Wir erstellen den RPG-Balken als eigenständiges Widget
    m_rpg_bar = new RPGProgressBar(this, wxID_ANY); // Korrigierter Konstruktor
    
    // Nachrichtentext (wird über OnPaint oder ein StaticText gezeichnet)
    // Hier nutzen wir weiterhin OnPaint für das restliche Branding (Hintergrund/Rahmen)
    mainSizer->AddSpacer(FROM_DIP(this, 40)); 
    mainSizer->Add(m_rpg_bar, 0, wxEXPAND | wxLEFT | wxRIGHT, FROM_DIP(this, 30));
    
    SetSizer(mainSizer);
    Centre();
    
    Bind(wxEVT_PAINT, &RPGLoadingWindow::OnPaint, this);
}

void RPGLoadingWindow::SetProgress(int percent, const wxString& message) {
    m_percent = std::clamp(percent, 0, 100);
    m_message = message;
    
    if (m_rpg_bar) {
        m_rpg_bar->SetProgress(m_percent / 100.0f);
    }
    
    Refresh();
    Update();
}

void RPGLoadingWindow::OnPaint(wxPaintEvent& event) {
    wxPaintDC dc(this);
    wxSize sz = GetClientSize();

    // Hintergrund
    dc.SetBrush(wxBrush(m_bg));
    dc.SetPen(wxPen(m_gold, 2));
    dc.DrawRectangle(0, 0, sz.x, sz.y);

    // Text
    dc.SetFont(GetFont().Bold());
    dc.SetTextForeground(m_gold);
    wxSize textSize = dc.GetTextExtent(m_message);
    dc.DrawText(m_message, (sz.x - textSize.x) / 2, FROM_DIP(this, 25));

    // Prozentanzeige
    wxString percStr = wxString::Format("%d%%", m_percent);
    dc.SetFont(GetFont().Smaller());
    wxSize percSize = dc.GetTextExtent(percStr);
    dc.DrawText(percStr, sz.x - percSize.x - FROM_DIP(this, 30), FROM_DIP(this, 105));
}
