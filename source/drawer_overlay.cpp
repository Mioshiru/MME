#include "complexitem.h"
#include "editor.h"
#include "gui.h"
#include "main.h"
#include "map_display.h"
#include "map_drawer.h"
#include "nanovg.h"
#include "nanovg_gl.h"
#include <imgui.h>

void MapDrawer::DrawSpriteMagnified(uint32_t item_id, float size) {
  if (!m_nvg)
    return;
  ItemType &it = g_items[item_id];
  if (!it.sprite)
    return;

  // Hol dir die GL-Textur-ID vom Sprite-System
  GLuint texid = it.sprite->getHardwareID(0, 0, 0, -1, 0, 0, 0, 0);
  if (texid == 0)
    return;

  // Erstelle ein temporäres NanoVG Image-Handle aus der GL-ID
  int img = 0;
#if defined(NANOVG_GL3_IMPLEMENTATION)
  img = nvglCreateImageFromHandleGL3(m_nvg, texid, 32, 32, 0);
#endif
  if (img == 0)
    return;

  ImVec2 pos = ImGui::GetCursorScreenPos();
  nvgBeginPath(m_nvg);
  nvgRoundedRect(m_nvg, pos.x, pos.y, size, size, 5.0f);
  NVGpaint imgPaint =
      nvgImagePattern(m_nvg, pos.x, pos.y, size, size, 0, img, 1.0f);
  nvgFillPaint(m_nvg, imgPaint);
  nvgFill(m_nvg);

  ImGui::Dummy(ImVec2(size, size)); // Platzhalter für ImGui Layout
  nvgDeleteImage(m_nvg, img); // Handle freigeben (Textur bleibt in GL erhalten)
}

void MapDrawer::DrawHoverPreview() {
  uint32_t hovered_id = g_gui.GetHoveredPaletteItemID();
  if (hovered_id == 0)
    return;

  ImGuiIO &io = ImGui::GetIO();
  float preview_size =
      128.0f * (g_gui.GetCurrentZoom() < 1.0f ? 1.0f : g_gui.GetCurrentZoom());

  ImGui::SetNextWindowPos(ImVec2(io.MousePos.x + 20, io.MousePos.y + 20));
  ImGui::Begin("AssetPreview", nullptr,
               ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoTitleBar |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_AlwaysAutoResize);

  // Wir nutzen NanoVG für ein scharfes Rendering des Sprites im Tooltip
  // Das Sprite wird hier vergrößert gezeichnet
  DrawSpriteMagnified(hovered_id, preview_size);

  ItemType &it = g_items[hovered_id];
  ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", it.name.c_str());
  ImGui::Text("ID: %d | ClientID: %d", it.id, it.clientID);

  ImGui::End();
}

void MapDrawer::DrawImGuiProperties() {
  Item *selected = g_gui.GetSelectedPropertiesItem();
  if (!selected)
    return;

  ImGui::Begin("Item Properties", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

  // IDs
  int aid = selected->getActionID();
  if (ImGui::InputInt("Action ID", &aid))
    selected->setActionID(aid);

  int uid = selected->getUniqueID();
  if (ImGui::InputInt("Unique ID", &uid))
    selected->setUniqueID(uid);

  // Texte
  static char text_buf[1024];
  strncpy(text_buf, selected->getText().c_str(), 1024);
  if (ImGui::InputTextMultiline("Text", text_buf, 1024))
    selected->setText(text_buf);

  strncpy(text_buf, selected->getDescription().c_str(), 1024);
  if (ImGui::InputTextMultiline("Description", text_buf, 1024))
    selected->setDescription(text_buf);

  // Spezial-Logik für Teleporte
  if (auto *tele = dynamic_cast<Teleport *>(selected)) {
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Teleport Destination");
    Position dest = tele->getDestination();
    int coords[3] = {(int)dest.x, (int)dest.y, (int)dest.z};
    if (ImGui::InputInt3("Target X/Y/Z", coords)) {
      tele->setDestination(Position(coords[0], coords[1], coords[2]));
    }
  }

  // Spezial-Logik für Türen
  if (auto *door = dynamic_cast<Door *>(selected)) {
    ImGui::Separator();
    int door_id = door->getDoorID();
    if (ImGui::InputInt("Door ID", &door_id))
      door->setDoorID((uint8_t)door_id);
  }

  if (ImGui::Button("Close"))
    g_gui.CloseProperties();
  ImGui::End();
}

void MapDrawer::DrawDebugConsole() { /* g_gui.DrawDebugConsole(); */ }

void MapDrawer::DrawMinimapHUD() {
  if (!g_settings.getInteger(Config::SHOW_MINIMAP_HUD))
    return;

  ImGuiIO &io = ImGui::GetIO();
  float margin = 10.0f;
  float size = 150.0f;

  // Position: Unten rechts im Canvas
  ImVec2 window_pos = ImVec2(io.DisplaySize.x - size - margin,
                             io.DisplaySize.y - size - margin);
  ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(size, size));

  if (ImGui::Begin("MinimapHUD", nullptr,
                   ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                       ImGuiWindowFlags_NoInputs |
                       ImGuiWindowFlags_NoBackground)) {
    // Hol dir die Minimap-Textur ID vom Canvas
    GLuint tex_id = canvas->GetMinimapTextureID();
    if (tex_id != 0) {
      ImDrawList *draw_list = ImGui::GetWindowDrawList();
      draw_list->AddRectFilled(window_pos,
                               ImVec2(window_pos.x + size, window_pos.y + size),
                               IM_COL32(0, 0, 0, 150), 5.0f);
      draw_list->AddImage((ImTextureID)tex_id, window_pos,
                          ImVec2(window_pos.x + size, window_pos.y + size));
      draw_list->AddRect(window_pos,
                         ImVec2(window_pos.x + size, window_pos.y + size),
                         IM_COL32(180, 140, 50, 255), 5.0f, 0, 2.0f);
    }
    ImGui::End();
  }
}

void MapDrawer::DrawTooltips() {
  if (ImGui::GetCurrentContext() == nullptr)
    return;
  ImDrawList *draw_list = ImGui::GetForegroundDrawList();

  for (MapTooltip *tooltip : tooltips) {
    float scale = canvas->GetZoom() < 1.0f ? canvas->GetZoom() : 1.0f;
    float x = tooltip->x + (TileSize / 2.0f);
    float y = tooltip->y;
    ImVec2 text_size =
        ImGui::CalcTextSize(tooltip->text.c_str(), NULL, false, 200.0f);

    float width = (text_size.x + 8.0f) * scale;
    float height = (text_size.y + 4.0f) * scale;
    ImVec2 p_min(x - (width / 2) - canvas->view_scroll_x,
                 y - height - (7.0f * scale) - canvas->view_scroll_y);
    ImVec2 p_max(x + (width / 2) - canvas->view_scroll_x,
                 y - (7.0f * scale) - canvas->view_scroll_y);

    draw_list->AddRectFilled(
        p_min, p_max, IM_COL32(tooltip->r, tooltip->g, tooltip->b, 200), 3.0f);
    draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize() * scale,
                       ImVec2(p_min.x + 4 * scale, p_min.y + 2 * scale),
                       IM_COL32(0, 0, 0, 255), tooltip->text.c_str());
  }
}

void MapDrawer::DrawGrid() {
  uint8_t grid_opacity = g_settings.getInteger(Config::GRID_OPACITY);
  glDisable(GL_TEXTURE_2D);
  glColor4ub(0, 0, 0, grid_opacity);
  for (int y = start_y; y < end_y; ++y) {
    glBegin(GL_LINES);
    glVertex2f(start_x * TileSize - canvas->view_scroll_x,
               y * TileSize - canvas->view_scroll_y);
    glVertex2f(end_x * TileSize - canvas->view_scroll_x,
               y * TileSize - canvas->view_scroll_y);
    glEnd();
  }
  glEnable(GL_TEXTURE_2D);
}

void MapDrawer::DrawBrush() {
  if (!g_gui.IsDrawingMode() || !g_gui.GetCurrentBrush() || options.ingame)
    return;
  Brush *brush = g_gui.GetCurrentBrush();
  // ... (Brush-Indikator Zeichnung basierend auf Shapes)
}

void MapDrawer::MakeTooltip(int sx, int sy, const std::string &text, uint8_t r,
                            uint8_t g, uint8_t b) {
  if (text.empty())
    return;
  MapTooltip *t = newd MapTooltip(sx, sy, text, r, g, b);
  t->checkLineEnding();
  tooltips.push_back(t);
}