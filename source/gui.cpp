//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "gui_preview.h"
#include "main.h"
#include "palette_brushlist.h"


#include "core_forward.h"
#include <wx/display.h>
#include <wx/statline.h>

#include "gui.h"
#include "main_menubar.h"

#include "async_loader.h"
#include "brush.h"
#include "doodad_brush.h"
#include "editor.h"
#include "map.h"
#include "materials.h"
#include "spawn_brush.h"
#include "sprites.h"
#include "svg_icons.h"

#include "application.h"
#include "common_windows.h"
#include "imgui_palette.h"
#include "lua/lua_api.h"
#include "map_display.h"
#include "map_drawer.h"
#include "minimap_window.h"
#include "palette_brushlist.h"
#include "palette_window.h"
#include "result_window.h"
#include "welcome_dialog.h"
#include <fstream>

#include "live_client.h"
#include "live_server.h"
#include "live_tab.h"
#include "lua/lua_script_manager.h"

const wxEventType EVT_UPDATE_MENUS = wxNewEventType();
// Global GUI instance
GUI g_gui;

GUI::GUI()
    : aui_manager(nullptr), async_loader(nullptr), root(nullptr),
      in_game_preview(nullptr), tools_panel(nullptr), size_panel(nullptr),
      gem(nullptr), search_result_window(nullptr), secondary_map(nullptr),
      doodad_buffer_map(nullptr), OGLContext(nullptr),
      loaded_version(CLIENT_VERSION_NONE), mode(SELECTION_MODE), pasting(false),
      hotkeys_enabled(true), current_brush(nullptr), previous_brush(nullptr),
      brush_shape(BRUSHSHAPE_SQUARE), brush_size(0), brush_variation(0),
      creature_spawntime(0), draw_locked_doors(false),
      use_custom_thickness(false), custom_thickness_mod(0.0),
      progressBar(nullptr), disabled_counter(0) {
  doodad_buffer_map = newd BaseMap();
}

GUI::~GUI() {
  delete doodad_buffer_map;
  delete aui_manager;
  delete OGLContext;
}
// Removed duplicate inline bodies at the top of file
// Primary implementations are located in gui_tabs.cpp and gui_layout.cpp

bool GUI::CloseLiveEditors(LiveSocket *sock) {
  for (int i = 0; i < tabbook->GetTabCount(); ++i) {
    auto *mapTab = dynamic_cast<MapTab *>(tabbook->GetTab(i));
    if (mapTab) {
      Editor *editor = mapTab->GetEditor();
      if (editor->GetLiveClient() == sock) {
        tabbook->DeleteTab(i--);
      }
    }
    auto *liveLogTab = dynamic_cast<LiveLogTab *>(tabbook->GetTab(i));
    if (liveLogTab) {
      if (liveLogTab->GetSocket() == sock) {
        liveLogTab->Disconnect();
        tabbook->DeleteTab(i--);
      }
    }
  }
  root->UpdateMenubar();
  return true;
}

bool GUI::CloseAllEditors() {
  for (int i = 0; i < tabbook->GetTabCount(); ++i) {
    auto *mapTab = dynamic_cast<MapTab *>(tabbook->GetTab(i));
    if (mapTab) {
      if (mapTab->IsUniqueReference() && mapTab->GetMap() &&
          mapTab->GetMap()->hasChanged()) {
        tabbook->SetFocusedTab(i);
        if (!root->DoQuerySave(false)) {
          return false;
        } else {
          RefreshPalettes();
          tabbook->DeleteTab(i--);
        }
      } else {
        tabbook->DeleteTab(i--);
      }
    }
  }
  if (root) {
    root->UpdateMenubar();
  }
  return true;
}

void GUI::NewMapView() {
  MapTab *mapTab = GetCurrentMapTab();
  if (mapTab) {
    auto *newMapTab = newd MapTab(mapTab);
    newMapTab->OnSwitchEditorMode(mode);

    SetStatusText("Created new view");
    UpdateTitle();
    RefreshPalettes();
    root->UpdateMenubar();
    root->Refresh();
  }
}

void GUI::HideSearchWindow() {
  if (search_result_window) {
    aui_manager->GetPane(search_result_window).Show(false);
    aui_manager->Update();
  }
}

SearchResultWindow *GUI::ShowSearchWindow() {
  if (search_result_window == nullptr) {
    search_result_window = newd SearchResultWindow(root);
    aui_manager->AddPane(
        search_result_window,
        wxAuiPaneInfo().Caption("Search Results").MinSize(wxSize(200, 150)));
  } else {
    aui_manager->GetPane(search_result_window).Show();
  }
  aui_manager->Update();
  return search_result_window;
}

//=============================================================================
// Palette Window Interface implementation

PaletteWindow *GUI::GetPalette() {
  if (palettes.empty()) {
    return nullptr;
  }
  return palettes.front();
}

PaletteWindow *GUI::NewPalette() { return CreatePalette(); }

void GUI::RefreshPalettes(Map *m, bool usedefault) {
  for (auto &palette : palettes) {
    palette->OnUpdate(m ? m
                        : (usedefault
                               ? (IsEditorOpen() ? &GetCurrentMap() : nullptr)
                               : nullptr));
  }
  SelectBrush();
}

void GUI::RefreshOtherPalettes(PaletteWindow *p) {
  for (auto &palette : palettes) {
    if (palette != p) {
      palette->OnUpdate(IsEditorOpen() ? &GetCurrentMap() : nullptr);
    }
  }
  SelectBrush();
}

PaletteWindow *GUI::CreatePalette() {
  if (!IsVersionLoaded()) {
    return nullptr;
  }

  // Sicherstellen, dass die Flags auch beim allerersten Erstellen der Paneele
  // aktiv sind
  aui_manager->SetFlags(aui_manager->GetFlags() | wxAUI_MGR_TRANSPARENT_HINT |
                        wxAUI_MGR_LIVE_RESIZE | wxAUI_MGR_HINT_FADE);

  auto *palette = newd PaletteWindow(root, g_materials.tilesets);
  aui_manager->AddPane(palette, wxAuiPaneInfo()
                                    .Name("Palette")
                                    .Caption("Palette")
                                    .Left()
                                    .Layer(1)
                                    .Position(1)
                                    .CloseButton(true)
                                    .BestSize(230, 450)
                                    .MinSize(wxSize(180, 200)));

  if (palette->collection_palette) {
    aui_manager->AddPane(palette->collection_palette,
                         wxAuiPaneInfo()
                             .Name("Collections Palette")
                             .Caption("Collections Palette / Tileset")
                             .Bottom()
                             .Layer(1)
                             .Position(1)
                             .CloseButton(true)
                             .BestSize(800, 150)
                             .MinSize(wxSize(200, 100)));
  }

  if (GetCurrentEditor()) {
    if (!in_game_preview) {
      in_game_preview = (wxWindow *)newd InGamePreviewCanvas(
          root, *GetCurrentEditor()); // In-game preview is always top-right
      aui_manager->AddPane(in_game_preview, wxAuiPaneInfo()
                                                .Name("In-game Preview")
                                                .Caption("In-game Preview")
                                                .Right()
                                                .Layer(1)
                                                .Position(1)
                                                .CloseButton(true)
                                                .BestSize(230, 250)
                                                .MinSize(wxSize(180, 150)));
    }
  }

  if (!tools_panel || !size_panel) {
    tools_panel = newd BrushToolPanel(root);
    tools_panel->SetToolbarIconSize(
        g_settings.getBoolean(Config::USE_LARGE_TERRAIN_TOOLBAR));

    size_panel = newd BrushSizePanel(root);
    size_panel->SetToolbarIconSize(
        g_settings.getBoolean(Config::USE_LARGE_TERRAIN_TOOLBAR));

    aui_manager->AddPane(tools_panel, wxAuiPaneInfo()
                                          .Name("Tools")
                                          .Caption("Tools")
                                          .ToolbarPane()
                                          .Right()
                                          .Layer(1)
                                          .Position(3)
                                          .CloseButton(true)
                                          .Floatable(true)
                                          .Dockable(true)
                                          .TopDockable(true)
                                          .BottomDockable(true)
                                          .LeftDockable(true)
                                          .RightDockable(true)
                                          .BestSize(180, 50)
                                          .Hide());

    aui_manager->AddPane(size_panel, wxAuiPaneInfo()
                                         .Name("Brush Size")
                                         .Caption("Brush Size")
                                         .ToolbarPane()
                                         .Right()
                                         .Layer(1)
                                         .Position(4)
                                         .CloseButton(true)
                                         .Floatable(true)
                                         .Dockable(true)
                                         .TopDockable(true)
                                         .BottomDockable(true)
                                         .LeftDockable(true)
                                         .RightDockable(true)
                                         .BestSize(180, 150)
                                         .Hide());
  }

  aui_manager->Update();

  // Make us the active palette
  palettes.push_front(palette);
  // Select brush from this palette
  palette->SelectPage(static_cast<PaletteType>(
      TILESET_CREATURE)); // Explicit cast to PaletteType
  SelectBrushInternal(palette->GetSelectedBrush());
  // fix for blank house list on f5 or new palette
  palette->OnUpdate(IsEditorOpen() ? &GetCurrentMap() : nullptr);

  // Force the standalone panels to load their contents if they haven't already
  tools_panel->LoadCurrentContents();
  size_panel->LoadCurrentContents();

  return palette;
}

void GUI::ActivatePalette(PaletteWindow *p) {
  if (!p)
    return;
  palettes.erase(std::find(palettes.begin(), palettes.end(), p));
  palettes.push_front(p);
}

void GUI::DestroyPalettes() {
  for (auto palette : palettes) {
    aui_manager->DetachPane(palette);
    palette->Destroy();
  }
  palettes.clear();

  if (tools_panel) {
    aui_manager->DetachPane(tools_panel);
    tools_panel->Destroy();
    tools_panel = nullptr;
  }
  if (size_panel) {
    aui_manager->DetachPane(size_panel);
    size_panel->Destroy();
    size_panel = nullptr;
  }

  aui_manager->Update();
}

void GUI::RebuildPalettes() {
  // Palette lits might be modified due to active palette changes
  // Use a temporary list for iterating
  PaletteList tmp = palettes;
  for (auto &piter : tmp) {
    piter->ReloadSettings(IsEditorOpen() ? &GetCurrentMap() : nullptr);
  }
  aui_manager->Update();
}

void GUI::ShowPalette() {
  if (palettes.empty()) {
    return;
  }

  for (auto &palette : palettes) {
    if (aui_manager->GetPane(palette).IsShown()) {
      return;
    }
  }

  aui_manager->GetPane(palettes.front()).Show(true);
  aui_manager->Update();
}

void GUI::SelectPalettePage(PaletteType pt) {
  if (palettes.empty()) {
    CreatePalette();
  }
  PaletteWindow *p = GetPalette();
  if (!p) {
    return;
  }

  ShowPalette();
  p->SelectPage(pt);
  aui_manager->Update();
  SelectBrushInternal(p->GetSelectedBrush());
}

//=============================================================================

void GUI::RefreshView() {
  EditorTab *editorTab = GetCurrentTab();
  if (!editorTab) {
    return;
  }

  if (in_game_preview) {
    static_cast<InGamePreviewCanvas *>(in_game_preview)->SyncView();
    in_game_preview->Refresh();
  }

  if (!dynamic_cast<MapTab *>(editorTab)) {
    editorTab->GetWindow()->Refresh();
    return;
  }

  std::vector<EditorTab *> editorTabs;
  for (int32_t index = 0; index < tabbook->GetTabCount(); ++index) {
    auto *mapTab = dynamic_cast<MapTab *>(tabbook->GetTab(index));
    if (mapTab) {
      editorTabs.push_back(mapTab);
    }
  }

  for (EditorTab *editorTab : editorTabs) {
    editorTab->GetWindow()->Refresh();
  }
}

void GUI::CreateLoadBar(wxString message, bool canCancel /* = false */) {
  progressText = message;

  progressFrom = 0;
  progressTo = 100;
  currentProgress = -1;

  progressBar = newd RPGLoadingWindow(root, "Loading");
  progressBar->Show(true);

  for (int idx = 0; idx < tabbook->GetTabCount(); ++idx) {
    auto *mt = dynamic_cast<MapTab *>(tabbook->GetTab(idx));
    if (mt && mt->GetEditor()->IsLiveServer()) {
      mt->GetEditor()->GetLiveServer()->startOperation(progressText);
    }
  }
  progressBar->SetProgress(0, progressText);
}

void GUI::SetLoadScale(int32_t from, int32_t to) {
  progressFrom = from;
  progressTo = to;
}

bool GUI::SetLoadDone(int32_t done, const wxString &newMessage) {
  if (done == 100) {
    DestroyLoadBar();
    return true;
  } else if (done == currentProgress) {
    return true;
  }

  if (!newMessage.empty()) {
    progressText = newMessage;
  }

  int32_t newProgress =
      progressFrom +
      static_cast<int32_t>((done / 100.f) * (progressTo - progressFrom));
  newProgress = std::max<int32_t>(0, std::min<int32_t>(100, newProgress));

  bool skip = false;
  if (progressBar) {
    progressBar->SetProgress(newProgress, progressText);
    currentProgress = newProgress;
  }

  for (int32_t index = 0; index < tabbook->GetTabCount(); ++index) {
    auto *mapTab = dynamic_cast<MapTab *>(tabbook->GetTab(index));
    if (mapTab && mapTab->GetEditor()) {
      LiveServer *server = mapTab->GetEditor()->GetLiveServer();
      if (server) {
        server->updateOperation(newProgress);
      }
    }
  }

  return skip;
}

void GUI::DestroyLoadBar() {
  if (progressBar) {
    progressBar->Show(false);
    currentProgress = -1;

    progressBar->Destroy();
    progressBar = nullptr;

    if (root->IsActive()) {
      root->Raise();
    } else {
      root->RequestUserAttention();
    }
  }
}

void GUI::ShowWelcomeDialog(const wxBitmap &icon) {
  std::vector<wxString> recent_files = root->GetRecentFiles();
  welcomeDialog =
      newd WelcomeDialog("Mio's Map Editor", "v4.5 (a Fork by Mioshiro)",
                         FROM_DIP(root, wxSize(800, 480)), icon, recent_files);
  welcomeDialog->Bind(wxEVT_CLOSE_WINDOW, &GUI::OnWelcomeDialogClosed, this);
  welcomeDialog->Bind(WELCOME_DIALOG_ACTION, &GUI::OnWelcomeDialogAction, this);
  welcomeDialog->Show();
  UpdateMenubar();
}

void GUI::FinishWelcomeDialog() {
  if (welcomeDialog != nullptr) {
    welcomeDialog->Hide();
    root->Show();
    welcomeDialog->Destroy();
    welcomeDialog = nullptr;
  }
  UpdateMenubar();
}

bool GUI::IsWelcomeDialogShown() {
  return welcomeDialog != nullptr && welcomeDialog->IsShown();
}

void GUI::OnWelcomeDialogClosed(wxCloseEvent &event) {
  welcomeDialog->Destroy();
  root->Close();
}

void GUI::OnWelcomeDialogAction(wxCommandEvent &event) {
  if (event.GetId() == wxID_NEW) {
    NewMap();
  } else if (event.GetId() == wxID_OPEN) {
    LoadMap(FileName(event.GetString()));
  }
}

void GUI::UpdateMenubar() { root->UpdateMenubar(); }

void GUI::SetScreenCenterPosition(Position position) {
  MapTab *mapTab = GetCurrentMapTab();
  if (mapTab) {
    mapTab->SetScreenCenterPosition(position);
  }
}

void GUI::DoCut() {
  if (!IsSelectionMode()) {
    return;
  }

  Editor *editor = GetCurrentEditor();
  if (!editor) {
    return;
  }

  editor->copybuffer.cut(*editor, GetCurrentFloor());
  RefreshView();
  root->UpdateMenubar();
}

void GUI::DoCopy() {
  if (!IsSelectionMode()) {
    return;
  }

  Editor *editor = GetCurrentEditor();
  if (!editor) {
    return;
  }

  editor->copybuffer.copy(*editor, GetCurrentFloor());
  RefreshView();
  root->UpdateMenubar();
}

void GUI::DoPaste() {
  MapTab *mapTab = GetCurrentMapTab();
  if (mapTab) {
    mapTab->GetEditor()->copybuffer.paste(
        *mapTab->GetEditor(), mapTab->GetCanvas()->GetCursorPosition());
  }
}

void GUI::PreparePaste() {
  Editor *editor = GetCurrentEditor();
  if (editor) {
    SetSelectionMode();
    editor->selection.start();
    editor->selection.clear();
    editor->selection.finish();
    StartPasting();
    RefreshView();
  }
}

void GUI::StartPasting() {
  if (GetCurrentEditor()) {
    pasting = true;
    secondary_map = &GetCurrentEditor()->copybuffer.getBufferMap();
  }
}

void GUI::EndPasting() {
  if (pasting) {
    pasting = false;
    secondary_map = nullptr;
  }
}

bool GUI::CanUndo() {
  Editor *editor = GetCurrentEditor();
  return (editor && editor->actionQueue->canUndo());
}

bool GUI::CanRedo() {
  Editor *editor = GetCurrentEditor();
  return (editor && editor->actionQueue->canRedo());
}

bool GUI::DoUndo() {
  Editor *editor = GetCurrentEditor();
  if (editor && editor->actionQueue->canUndo()) {
    editor->actionQueue->undo();
    if (editor->selection.size() > 0) {
      SetSelectionMode();
    }
    SetStatusText("Undo action");
    root->UpdateMenubar();
    root->Refresh();
    return true;
  }
  return false;
}

bool GUI::DoRedo() {
  Editor *editor = GetCurrentEditor();
  if (editor && editor->actionQueue->canRedo()) {
    editor->actionQueue->redo();
    if (editor->selection.size() > 0) {
      SetSelectionMode();
    }
    SetStatusText("Redo action");
    root->UpdateMenubar();
    root->Refresh();
    return true;
  }
  return false;
}

int GUI::GetCurrentFloor() {
  MapTab *tab = GetCurrentMapTab();
  ASSERT(tab);
  return tab->GetCanvas()->GetFloor();
}

void GUI::ChangeFloor(int new_floor) {
  MapTab *tab = GetCurrentMapTab();
  if (tab) {
    int old_floor = GetCurrentFloor();
    if (new_floor < 0 || new_floor > MAP_MAX_LAYER) {
      return;
    }

    if (old_floor != new_floor) {
      tab->GetCanvas()->ChangeFloor(new_floor);
    }
  }
}

void GUI::SetStatusText(wxString text) { g_gui.root->SetStatusText(text, 0); }

void GUI::SetTitle(wxString title) {
  if (g_gui.root == nullptr) {
    return;
  }

#ifdef NIGHTLY_BUILD
#ifdef SVN_BUILD
#define TITLE_APPEND (wxString(" (Nightly Build #") << i2ws(SVN_BUILD) << ")")
#else
#define TITLE_APPEND (wxString(" (Nightly Build)"))
#endif
#else
#ifdef SVN_BUILD
#define TITLE_APPEND (wxString(" (Build #") << i2ws(SVN_BUILD) << ")")
#else
#define TITLE_APPEND (wxString(""))
#endif
#endif
#ifdef __EXPERIMENTAL__
  if (title != "") {
    g_gui.root->SetTitle(title << " - OTAcademy Map Editor BETA"
                               << TITLE_APPEND);
  } else {
    g_gui.root->SetTitle(wxString("OTAcademy Map Editor BETA") << TITLE_APPEND);
  }
#elif __SNAPSHOT__
  if (title != "") {
    g_gui.root->SetTitle(title << " - OTAcademy Map Editor - SNAPSHOT"
                               << TITLE_APPEND);
  } else {
    g_gui.root->SetTitle(wxString("OTAcademy Map Editor - SNAPSHOT")
                         << TITLE_APPEND);
  }
#else
  if (!title.empty()) {
    g_gui.root->SetTitle(title << " - OTAcademy Map Editor" << TITLE_APPEND);
  } else {
    g_gui.root->SetTitle(wxString("OTAcademy Map Editor") << TITLE_APPEND);
  }
#endif
}

void GUI::UpdateTitle() {
  if (tabbook->GetTabCount() > 0) {
    SetTitle(tabbook->GetCurrentTab()->GetTitle());
    for (int idx = 0; idx < tabbook->GetTabCount(); ++idx) {
      if (tabbook->GetTab(idx)) {
        tabbook->SetTabLabel(idx, tabbook->GetTab(idx)->GetTitle());
      }
    }
  } else {
    SetTitle("");
  }
}

void GUI::UpdateMenus() {
  wxCommandEvent evt(EVT_UPDATE_MENUS);
  g_gui.root->AddPendingEvent(evt);
}

void GUI::ShowToolbar(ToolBarID id, bool show) {
  if (root && root->GetAuiToolBar()) {
    root->GetAuiToolBar()->Show(id, show);
  }
}

void GUI::SwitchMode() {
  if (mode == DRAWING_MODE) {
    SetSelectionMode();
  } else {
    SetDrawingMode();
  }
}

void GUI::SetSelectionMode() {
  if (mode == SELECTION_MODE) {
    return;
  }

  if (current_brush && current_brush->isDoodad()) {
    secondary_map = nullptr;
  }

  tabbook->OnSwitchEditorMode(SELECTION_MODE);
  mode = SELECTION_MODE;
}

void GUI::SetDrawingMode() {
  if (mode == DRAWING_MODE) {
    return;
  }

  std::set<MapTab *> al;
  for (int idx = 0; idx < tabbook->GetTabCount(); ++idx) {
    EditorTab *editorTab = tabbook->GetTab(idx);
    if (auto *mapTab = dynamic_cast<MapTab *>(editorTab)) {
      if (al.find(mapTab) != al.end()) {
        continue;
      }

      Editor *editor = mapTab->GetEditor();
      editor->selection.start();
      editor->selection.clear();
      editor->selection.finish();
      al.insert(mapTab);
    }
  }

  if (current_brush && current_brush->isDoodad()) {
    secondary_map = doodad_buffer_map;
  } else {
    secondary_map = nullptr;
  }

  tabbook->OnSwitchEditorMode(DRAWING_MODE);
  mode = DRAWING_MODE;
}

void GUI::SetBrushSizeInternal(int nz) {
  if (nz != brush_size && current_brush && current_brush->isDoodad() &&
      !current_brush->oneSizeFitsAll()) {
    brush_size = nz;
    FillDoodadPreviewBuffer();
    secondary_map = doodad_buffer_map;
  } else {
    brush_size = nz;
  }
}

void GUI::SetBrushSize(int nz) {
  SetBrushSizeInternal(nz);

  for (auto &palette : palettes) {
    palette->OnUpdateBrushSize(brush_shape, brush_size);
  }
  if (size_panel) {
    size_panel->OnUpdateBrushSize(brush_shape, brush_size);
  }

  root->GetAuiToolBar()->UpdateBrushSize(brush_shape, brush_size);
}

void GUI::SetBrushVariation(int nz) {
  if (nz != brush_variation && current_brush && current_brush->isDoodad()) {
    // Monkey!
    brush_variation = nz;
    FillDoodadPreviewBuffer();
    secondary_map = doodad_buffer_map;
  }
}

void GUI::SetBrushShape(BrushShape bs) {
  if (bs != brush_shape && current_brush && current_brush->isDoodad() &&
      !current_brush->oneSizeFitsAll()) {
    // Donkey!
    brush_shape = bs;
    FillDoodadPreviewBuffer();
    secondary_map = doodad_buffer_map;
  }
  brush_shape = bs;

  for (auto &palette : palettes) {
    palette->OnUpdateBrushSize(brush_shape, brush_size);
  }
  if (size_panel) {
    size_panel->OnUpdateBrushSize(brush_shape, brush_size);
  }

  root->GetAuiToolBar()->UpdateBrushSize(brush_shape, brush_size);
}

void GUI::SetBrushThickness(bool on, int x, int y) {
  use_custom_thickness = on;

  if (x != -1 || y != -1) {
    custom_thickness_mod = float(max(x, 1)) / float(max(y, 1));
  }

  if (current_brush && current_brush->isDoodad()) {
    FillDoodadPreviewBuffer();
  }

  RefreshView();
}

void GUI::SetBrushThickness(int low, int ceil) {
  custom_thickness_mod = float(max(low, 1)) / float(max(ceil, 1));

  if (use_custom_thickness && current_brush && current_brush->isDoodad()) {
    FillDoodadPreviewBuffer();
  }

  RefreshView();
}

void GUI::DecreaseBrushSize(bool wrap) {
  switch (brush_size) {
  case 0: {
    if (wrap) {
      SetBrushSize(11);
    }
    break;
  }
  case 1: {
    SetBrushSize(0);
    break;
  }
  case 2:
  case 3: {
    SetBrushSize(1);
    break;
  }
  case 4:
  case 5: {
    SetBrushSize(2);
    break;
  }
  case 6:
  case 7: {
    SetBrushSize(4);
    break;
  }
  case 8:
  case 9:
  case 10: {
    SetBrushSize(6);
    break;
  }
  case 11:
  default: {
    SetBrushSize(8);
    break;
  }
  }
}

void GUI::IncreaseBrushSize(bool wrap) {
  switch (brush_size) {
  case 0: {
    SetBrushSize(1);
    break;
  }
  case 1: {
    SetBrushSize(2);
    break;
  }
  case 2:
  case 3: {
    SetBrushSize(4);
    break;
  }
  case 4:
  case 5: {
    SetBrushSize(6);
    break;
  }
  case 6:
  case 7: {
    SetBrushSize(8);
    break;
  }
  case 8:
  case 9:
  case 10: {
    SetBrushSize(11);
    break;
  }
  case 11:
  default: {
    if (wrap) {
      SetBrushSize(0);
    }
    break;
  }
  }
}

void GUI::SetDoorLocked(bool on) {
  draw_locked_doors = on;
  RefreshView();
}

bool GUI::HasDoorLocked() { return draw_locked_doors; }

Brush *GUI::GetCurrentBrush() const { return current_brush; }

BrushShape GUI::GetBrushShape() const {
  if (current_brush == spawn_brush) {
    return BRUSHSHAPE_SQUARE;
  }

  return brush_shape;
}

int GUI::GetBrushSize() const { return brush_size; }

int GUI::GetBrushVariation() const { return brush_variation; }

int GUI::GetSpawnTime() const { return creature_spawntime; }

void GUI::SelectBrush() {
  if (palettes.empty()) {
    return;
  }

  SelectBrushInternal(palettes.front()->GetSelectedBrush());

  RefreshView();
}

bool GUI::SelectBrush(const Brush *whatbrush, PaletteType primary) {
  if (palettes.empty()) {
    if (!CreatePalette()) {
      return false;
    }
  }

  bool found = palettes.front()->OnSelectBrush(whatbrush, primary);
  if (found) {
    if (tools_panel) {
      tools_panel->DeselectAll();
    }
  } else {
    if (tools_panel && tools_panel->SelectBrush(whatbrush)) {
      found = true;
      palettes.front()->OnSelectBrush(nullptr, primary);
    }
  }

  if (!found) {
    return false;
  }

  SelectBrushInternal(const_cast<Brush *>(whatbrush));
  root->GetAuiToolBar()->UpdateBrushButtons();
  return true;
}

void GUI::SelectBrushInternal(Brush *brush) {
  // Fear no evil don't you say no evil
  if (current_brush != brush && brush) {
    previous_brush = current_brush;
  }

  current_brush = brush;
  if (!current_brush) {
    return;
  }

  brush_variation = min(brush_variation, brush->getMaxVariation());
  FillDoodadPreviewBuffer();
  if (brush->isDoodad()) {
    secondary_map = doodad_buffer_map;
  }

  SetDrawingMode();
  RefreshView();

  if (g_luaScripts.isInitialized()) {
    g_luaScripts.emit("brushChange", current_brush->getName());
  }
}

void GUI::SelectPreviousBrush() {
  if (previous_brush) {
    SelectBrush(previous_brush);
  }
}

void GUI::FillDoodadPreviewBuffer() {
  if (!current_brush || !current_brush->isDoodad()) {
    return;
  }

  doodad_buffer_map->clear();

  DoodadBrush *brush = current_brush->asDoodad();
  if (brush->isEmpty(GetBrushVariation())) {
    return;
  }

  int object_count = 0;
  int area;
  if (GetBrushShape() == BRUSHSHAPE_SQUARE) {
    area = 2 * GetBrushSize();
    area = area * area + 1;
  } else {
    if (GetBrushSize() == 1) {
      // There is a huge deviation here with the other formula.
      area = 5;
    } else {
      area = int(0.5 + GetBrushSize() * GetBrushSize() * PI);
    }
  }
  const int object_range =
      (use_custom_thickness ? int(area * custom_thickness_mod)
                            : brush->getThickness() * area /
                                  max(1, brush->getThicknessCeiling()));
  const int final_object_count = max(1, object_range + random(object_range));

  Position center_pos(0x8000, 0x8000, 0x8);

  if (brush_size > 0 && !brush->oneSizeFitsAll()) {
    while (object_count < final_object_count) {
      int retries = 0;
      bool exit = false;

      // Try to place objects 5 times
      while (retries < 5 && !exit) {

        int pos_retries = 0;
        int xpos = 0, ypos = 0;
        bool found_pos = false;
        if (GetBrushShape() == BRUSHSHAPE_CIRCLE) {
          while (pos_retries < 5 && !found_pos) {
            xpos = random(-brush_size, brush_size);
            ypos = random(-brush_size, brush_size);
            float distance = sqrt(float(xpos * xpos) + float(ypos * ypos));
            if (distance < g_gui.GetBrushSize() + 0.005) {
              found_pos = true;
            } else {
              ++pos_retries;
            }
          }
        } else {
          found_pos = true;
          xpos = random(-brush_size, brush_size);
          ypos = random(-brush_size, brush_size);
        }

        if (!found_pos) {
          ++retries;
          continue;
        }

        // Decide whether the zone should have a composite or several single
        // objects.
        bool fail = false;
        if (random(brush->getTotalChance(GetBrushVariation())) <=
            brush->getCompositeChance(GetBrushVariation())) {
          // Composite
          const CompositeTileList &composites =
              brush->getComposite(GetBrushVariation());

          // Figure out if the placement is valid
          for (const auto &composite : composites) {
            Position pos =
                center_pos + composite.first + Position(xpos, ypos, 0);
            if (Tile *tile = doodad_buffer_map->getTile(pos)) {
              if (!tile->empty()) {
                fail = true;
                break;
              }
            }
          }
          if (fail) {
            ++retries;
            break;
          }

          // Transfer items to the stack
          for (const auto &composite : composites) {
            Position pos =
                center_pos + composite.first + Position(xpos, ypos, 0);
            const ItemVector &items = composite.second;
            Tile *tile = doodad_buffer_map->getTile(pos);

            if (!tile) {
              tile = doodad_buffer_map->allocator(
                  doodad_buffer_map->createTileL(pos));
            }

            for (auto item : items) {
              tile->addItem(item->deepCopy());
            }
            doodad_buffer_map->setTile(tile->getPosition(), tile);
          }
          exit = true;
        } else if (brush->hasSingleObjects(GetBrushVariation())) {
          Position pos = center_pos + Position(xpos, ypos, 0);
          Tile *tile = doodad_buffer_map->getTile(pos);
          if (tile) {
            if (!tile->empty()) {
              fail = true;
              break;
            }
          } else {
            tile = doodad_buffer_map->allocator(
                doodad_buffer_map->createTileL(pos));
          }
          int variation = GetBrushVariation();
          brush->draw(doodad_buffer_map, tile, &variation);
          // std::cout << "\tpos: " << tile->getPosition() << std::endl;
          doodad_buffer_map->setTile(tile->getPosition(), tile);
          exit = true;
        }
        if (fail) {
          ++retries;
          break;
        }
      }
      ++object_count;
    }
  } else {
    if (brush->hasCompositeObjects(GetBrushVariation()) &&
        random(brush->getTotalChance(GetBrushVariation())) <=
            brush->getCompositeChance(GetBrushVariation())) {
      // Composite
      const CompositeTileList &composites =
          brush->getComposite(GetBrushVariation());

      // All placement is valid...

      // Transfer items to the buffer
      for (const auto &composite : composites) {
        Position pos = center_pos + composite.first;
        const ItemVector &items = composite.second;
        Tile *tile =
            doodad_buffer_map->allocator(doodad_buffer_map->createTileL(pos));
        // std::cout << pos << " = " << center_pos << " + " <<
        // buffer_tile->getPosition() << std::endl;

        for (auto item : items) {
          tile->addItem(item->deepCopy());
        }
        doodad_buffer_map->setTile(tile->getPosition(), tile);
      }
    } else if (brush->hasSingleObjects(GetBrushVariation())) {
      Tile *tile = doodad_buffer_map->allocator(
          doodad_buffer_map->createTileL(center_pos));
      int variation = GetBrushVariation();
      brush->draw(doodad_buffer_map, tile, &variation);
      doodad_buffer_map->setTile(center_pos, tile);
    }
  }
}

long GUI::PopupDialog(wxWindow *parent, wxString title, wxString text,
                      long style, wxString confisavename,
                      uint32_t configsavevalue) {
  if (text.empty()) {
    return wxID_ANY;
  }

  wxMessageDialog dlg(parent, text, title, style);
  return dlg.ShowModal();
}

long GUI::PopupDialog(wxString title, wxString text, long style,
                      wxString configsavename, uint32_t configsavevalue) {
  return g_gui.PopupDialog(g_gui.root, title, text, style, configsavename,
                           configsavevalue);
}

void GUI::ListDialog(wxWindow *parent, wxString title,
                     const wxArrayString &param_items) {
  if (param_items.empty()) {
    return;
  }

  wxArrayString list_items(param_items);

  // Create the window
  wxDialog *dlg =
      newd wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize,
                    wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX);

  wxSizer *sizer = newd wxBoxSizer(wxVERTICAL);
  wxListBox *item_list = newd wxListBox(dlg, wxID_ANY, wxDefaultPosition,
                                        wxDefaultSize, 0, nullptr, wxLB_SINGLE);
  item_list->SetMinSize(wxSize(500, 300));

  for (size_t i = 0; i != list_items.GetCount();) {
    wxString str = list_items[i];
    size_t pos = str.find("\n");
    if (pos != wxString::npos) {
      // Split string!
      item_list->Append(str.substr(0, pos));
      list_items[i] = str.substr(pos + 1);
      continue;
    }
    item_list->Append(list_items[i]);
    ++i;
  }
  sizer->Add(item_list, 1, wxEXPAND);

  wxSizer *stdsizer = newd wxBoxSizer(wxHORIZONTAL);
  stdsizer->Add(newd wxButton(dlg, wxID_OK, "OK"), wxSizerFlags(1).Center());
  sizer->Add(stdsizer, wxSizerFlags(0).Center());

  dlg->SetSizerAndFit(sizer);

  // Show the window
  dlg->ShowModal();
  delete dlg;
}

void GUI::ShowTextBox(wxWindow *parent, wxString title, wxString content) {
  wxDialog *dlg =
      newd wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize,
                    wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX);
  wxSizer *topsizer = newd wxBoxSizer(wxVERTICAL);
  wxTextCtrl *text_field =
      newd wxTextCtrl(dlg, wxID_ANY, content, wxDefaultPosition, wxDefaultSize,
                      wxTE_MULTILINE | wxTE_READONLY);
  text_field->SetMinSize(wxSize(400, 550));
  topsizer->Add(text_field, wxSizerFlags(5).Expand());

  wxSizer *choicesizer = newd wxBoxSizer(wxHORIZONTAL);
  choicesizer->Add(newd wxButton(dlg, wxID_CANCEL, "OK"),
                   wxSizerFlags(1).Center());
  topsizer->Add(choicesizer, wxSizerFlags(0).Center());
  dlg->SetSizerAndFit(topsizer);

  dlg->ShowModal();
}

RME_Rendering::RenderBackend *GUI::GetRenderBackend() {
  if (g_settings.getInteger(Config::RENDER_BACKEND) == 1) { // 1 = Vulkan
    if (m_vulkan_failed)
      return nullptr;

    if (!render_backend) {
      render_backend = std::make_unique<RME_Rendering::VulkanBackend>();
      try {
        // Initialisierung mit dem Handle des Hauptfensters
        render_backend->Initialize(root->GetHandle());
      } catch (const std::exception &e) {
        wxLogError("Vulkan failed to start: %s. Falling back to Legacy OpenGL.",
                   e.what());
        m_graphicsErrorLog = std::string("Vulkan Error: ") + e.what();
        g_gui.SetStatusText(
            "Graphics Error: Vulkan not supported. Using OpenGL.");
        m_vulkan_failed = true;
        render_backend.reset();
        return nullptr;
      }
    }
    return render_backend.get();
  }
  return nullptr;
}

void SetWindowToolTip(wxWindow *a, const wxString &tip) {
  if (a) {
    a->SetToolTip(tip);
  }
}

void SetWindowToolTip(wxWindow *a, wxWindow *b, const wxString &tip) {
  if (a) {
    a->SetToolTip(tip);
  }
  if (b) {
    b->SetToolTip(tip);
  }
}
