using ImGuiNET;
using MapEditor.Core.Actions;
using MapEditor.Core.State;
using MapEditor.Assets.Otbm;
using System.IO;
using System;

namespace MapEditor.UI.Widgets;

/// <summary>
/// Main menu bar rendered at the top of the application window.
/// Menus: File | Edit | View | Settings | Help
/// </summary>
public sealed class MainMenuBar
{
    private readonly EditorStore _store;

    public MainMenuBar(EditorStore store) => _store = store;

    public void Render(EditorState state)
    {
        if (!ImGui.BeginMainMenuBar()) return;

        RenderFileMenu(state);
        RenderEditMenu(state);
        RenderViewMenu(state);
        RenderSettingsMenu(state);
        RenderHelpMenu();

        // Right-aligned FPS display
        float fps = ImGui.GetIO().Framerate;
        string fpsText = $"{fps:F0} FPS";
        float fpsWidth = ImGui.CalcTextSize(fpsText).X + 16f;
        ImGui.SetCursorPosX(ImGui.GetContentRegionAvail().X - fpsWidth + ImGui.GetCursorPosX());
        ImGui.TextDisabled(fpsText);

        ImGui.EndMainMenuBar();
    }

    private void RenderFileMenu(EditorState state)
    {
        if (!ImGui.BeginMenu("File")) return;

        if (ImGui.MenuItem("New Map",  "Ctrl+N")) { /* TODO */ }
        if (ImGui.MenuItem("Open Map", "Ctrl+O")) { /* TODO: open file dialog */ }

        ImGui.Separator();

        bool hasMap = state.ActiveMap is not null;
        if (ImGui.MenuItem("Save", "Ctrl+S", false, hasMap))
        {
            string path = state.ActiveMap!.FilePath ?? "world.otbm";
            
            // Run pre-export validation checks
            var validationErrors = MapValidator.Validate(state.ActiveMap, null);
            foreach (var err in validationErrors)
            {
                Console.WriteLine($"[Validation {(err.IsWarning ? "Warning" : "Error")}] at {err.Position.X},{err.Position.Y},{err.Position.Z}: {err.Message}");
            }

            try
            {
                OtbmExporter.Export(state.ActiveMap, path);
                state.ActiveMap.FilePath = path;
                _store.Dispatch(new MapDirtyChangedAction(false));
                Console.WriteLine($"[MainMenuBar] Map saved successfully to {Path.GetFullPath(path)}");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[MainMenuBar] Error exporting map: {ex.Message}");
            }
        }
        if (ImGui.MenuItem("Save As…", "Ctrl+Shift+S", false, hasMap))
        {
            // Fallback to saving to a default backup name
            string path = "world_backup.otbm";
            try
            {
                OtbmExporter.Export(state.ActiveMap!, path);
                state.ActiveMap!.FilePath = path;
                _store.Dispatch(new MapDirtyChangedAction(false));
                Console.WriteLine($"[MainMenuBar] Map saved successfully to {Path.GetFullPath(path)}");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[MainMenuBar] Error exporting map: {ex.Message}");
            }
        }

        ImGui.Separator();

        if (ImGui.MenuItem("Exit", "Alt+F4")) { /* Request close */ }

        ImGui.EndMenu();
    }

    private void RenderEditMenu(EditorState state)
    {
        if (!ImGui.BeginMenu("Edit")) return;

        if (ImGui.MenuItem("Undo", "Ctrl+Z", false, state.CanUndo))
            _store.Dispatch(new UndoAction());

        if (ImGui.MenuItem("Redo", "Ctrl+Y", false, state.CanRedo))
            _store.Dispatch(new RedoAction());

        ImGui.Separator();
        if (ImGui.MenuItem("Cut",   "Ctrl+X")) { /* TODO */ }
        if (ImGui.MenuItem("Copy",  "Ctrl+C")) { /* TODO */ }
        if (ImGui.MenuItem("Paste", "Ctrl+V")) { /* TODO */ }

        ImGui.EndMenu();
    }

    private void RenderViewMenu(EditorState state)
    {
        if (!ImGui.BeginMenu("View")) return;

        ImGui.TextDisabled($"Floor: {state.ActiveFloor}");
        ImGui.Separator();

        for (int f = 0; f <= 15; f++)
        {
            int floor = f; // capture
            bool active = state.ActiveFloor == floor;
            if (ImGui.MenuItem($"Floor {floor:D2}", $"F{floor + 1}", active))
                _store.Dispatch(new SelectFloorAction(floor));
        }

        ImGui.Separator();
        if (ImGui.MenuItem("Zoom In",  "=")) { /* TODO */ }
        if (ImGui.MenuItem("Zoom Out", "-")) { /* TODO */ }
        if (ImGui.MenuItem("Reset Zoom", "0")) { /* TODO */ }

        ImGui.EndMenu();
    }

    private void RenderSettingsMenu(EditorState state)
    {
        if (!ImGui.BeginMenu("Settings")) return;

        bool open = state.SettingsWindowOpen;
        if (ImGui.MenuItem("Settings", null, open))
            _store.Dispatch(new ToggleSettingsWindowAction());

        ImGui.EndMenu();
    }

    private void RenderHelpMenu()
    {
        if (!ImGui.BeginMenu("Help")) return;

        if (ImGui.MenuItem("About OT Map Editor")) { /* TODO */ }
        if (ImGui.MenuItem("GitHub Repository"))   { /* TODO */ }

        ImGui.EndMenu();
    }
}
