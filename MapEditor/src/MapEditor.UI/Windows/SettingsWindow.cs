using System;
using System.Numerics;
using ImGuiNET;
using MapEditor.Core.Actions;
using MapEditor.Core.State;
using MapEditor.Core.Config;

namespace MapEditor.UI.Windows;

/// <summary>
/// The "Settings" window — exactly titled "Settings" as required.
/// Provides configuration for graphics presets, V-Sync, batch size, autosaves, and client files.
/// Persists choices to config.json.
/// </summary>
public sealed class SettingsWindow
{
    private readonly EditorStore _store;

    private static readonly string[] ThemeNames = { "Dark (Default)", "Light", "Classic" };
    private static readonly string[] PresetNames = { "Low", "Medium", "High", "Custom" };

    public SettingsWindow(EditorStore store)
    {
        _store = store;
    }

    /// <summary>
    /// Renders the Settings window. Should be called every frame.
    /// </summary>
    public void Render(EditorState state)
    {
        if (!state.SettingsWindowOpen) return;

        ImGui.SetNextWindowSize(new Vector2(480, 580), ImGuiCond.FirstUseEver);
        ImGui.SetNextWindowPos(new Vector2(200, 120), ImGuiCond.FirstUseEver);

        bool open = state.SettingsWindowOpen;

        // Window must be titled exactly "Settings"
        if (ImGui.Begin("Settings", ref open, ImGuiWindowFlags.NoCollapse | ImGuiWindowFlags.NoDocking))
        {
            if (!open)
            {
                _store.Dispatch(new ToggleSettingsWindowAction());
            }

            RenderAppearanceSection();
            ImGui.Spacing();
            RenderPerformanceSection();
            ImGui.Spacing();
            RenderMapSection();
            ImGui.Spacing();
            RenderAutosaveSection();
            ImGui.Spacing();
            RenderClientFilesSection();
            ImGui.Spacing();

            ImGui.Separator();
            RenderButtons();
        }
        ImGui.End();

        if (!open && state.SettingsWindowOpen)
        {
            _store.Dispatch(new ToggleSettingsWindowAction());
        }
    }

    private void RenderAppearanceSection()
    {
        if (!ImGui.CollapsingHeader("Appearance", ImGuiTreeNodeFlags.DefaultOpen)) return;

        ImGui.Indent();
        var config = SettingsManager.Current;

        // Theme selector
        int themeIdx = config.SelectedTheme;
        ImGui.Text("Theme:");
        ImGui.SameLine(120);
        ImGui.SetNextItemWidth(200);
        if (ImGui.Combo("##theme", ref themeIdx, ThemeNames, ThemeNames.Length))
        {
            config.SelectedTheme = themeIdx;
            ApplyTheme(themeIdx);
        }

        // UI Scale
        float uiScale = config.UiScale;
        ImGui.Text("UI Scale:");
        ImGui.SameLine(120);
        ImGui.SetNextItemWidth(200);
        if (ImGui.SliderFloat("##uiscale", ref uiScale, 0.5f, 2.0f, "%.1f×"))
        {
            config.UiScale = uiScale;
            ImGui.GetIO().FontGlobalScale = uiScale;
        }

        ImGui.Unindent();
    }

    private void RenderPerformanceSection()
    {
        if (!ImGui.CollapsingHeader("Performance & Hardware", ImGuiTreeNodeFlags.DefaultOpen)) return;

        ImGui.Indent();
        var config = SettingsManager.Current;

        // Performance Preset
        int presetIdx = (int)config.Preset;
        ImGui.Text("Preset:");
        ImGui.SameLine(120);
        ImGui.SetNextItemWidth(200);
        if (ImGui.Combo("##preset", ref presetIdx, PresetNames, PresetNames.Length))
        {
            config.ApplyPreset((PerformancePreset)presetIdx);
        }

        // VSync
        bool vsync = config.VSync;
        ImGui.Text("V-Sync:");
        ImGui.SameLine(120);
        if (ImGui.Checkbox("##vsync", ref vsync))
        {
            config.VSync = vsync;
            config.Preset = PerformancePreset.Custom;
        }

        // Batch Size
        int batchSize = config.BatchSize;
        ImGui.Text("Batch Size:");
        ImGui.SameLine(120);
        ImGui.SetNextItemWidth(200);
        if (ImGui.SliderInt("##batchsize", ref batchSize, 1024, 131072, "%d tiles"))
        {
            config.BatchSize = batchSize;
            config.Preset = PerformancePreset.Custom;
        }

        ImGui.Unindent();
    }

    private void RenderMapSection()
    {
        if (!ImGui.CollapsingHeader("Map Display", ImGuiTreeNodeFlags.DefaultOpen)) return;

        ImGui.Indent();
        var config = SettingsManager.Current;

        bool showGrid = config.ShowGrid;
        if (ImGui.Checkbox("Show Grid", ref showGrid))
        {
            config.ShowGrid = showGrid;
        }

        ImGui.SameLine();
        bool showMinimap = config.ShowMinimap;
        if (ImGui.Checkbox("Show Minimap", ref showMinimap))
        {
            config.ShowMinimap = showMinimap;
        }

        ImGui.Unindent();
    }

    private void RenderAutosaveSection()
    {
        if (!ImGui.CollapsingHeader("Autosave")) return;

        ImGui.Indent();
        var config = SettingsManager.Current;

        bool autosave = config.EnableAutosave;
        if (ImGui.Checkbox("Enable Autosave", ref autosave))
        {
            config.EnableAutosave = autosave;
        }

        if (autosave)
        {
            ImGui.SameLine();
            int interval = config.AutosaveIntervalMinutes;
            ImGui.SetNextItemWidth(80);
            if (ImGui.InputInt("minutes##autosave", ref interval, 1, 5))
            {
                config.AutosaveIntervalMinutes = Math.Clamp(interval, 1, 60);
            }
        }

        ImGui.Unindent();
    }

    private void RenderClientFilesSection()
    {
        if (!ImGui.CollapsingHeader("Client Files")) return;

        ImGui.Indent();
        var config = SettingsManager.Current;

        ImGui.TextDisabled("Tibia.dat path:");
        ImGui.SameLine();
        ImGui.SetNextItemWidth(-80);
        string datPath = config.ClientDatPath;
        ImGui.InputText("##datpath", ref datPath, 512, ImGuiInputTextFlags.ReadOnly);
        ImGui.SameLine();
        if (ImGui.Button("Browse##dat"))
        {
            // Future enhancement: file dialog integration
        }

        ImGui.TextDisabled("Tibia.spr path:");
        ImGui.SameLine();
        ImGui.SetNextItemWidth(-80);
        string sprPath = config.ClientSprPath;
        ImGui.InputText("##sprpath", ref sprPath, 512, ImGuiInputTextFlags.ReadOnly);
        ImGui.SameLine();
        if (ImGui.Button("Browse##spr"))
        {
            // Future enhancement: file dialog integration
        }

        ImGui.Unindent();
    }

    private void RenderButtons()
    {
        float btnW = 100f;
        float spacing = ImGui.GetStyle().ItemSpacing.X;
        float totalW = btnW * 2 + spacing;
        
        ImGui.SetCursorPosX((ImGui.GetContentRegionAvail().X - totalW) * 0.5f + ImGui.GetCursorScreenPos().X - ImGui.GetWindowPos().X);

        if (ImGui.Button("Apply", new Vector2(btnW, 0)))
        {
            SettingsManager.Save();
        }
        
        ImGui.SameLine();
        
        if (ImGui.Button("Close", new Vector2(btnW, 0)))
        {
            _store.Dispatch(new ToggleSettingsWindowAction());
        }
    }

    private static void ApplyTheme(int index)
    {
        switch (index)
        {
            case 0: ImGui.StyleColorsDark();    break;
            case 1: ImGui.StyleColorsLight();   break;
            case 2: ImGui.StyleColorsClassic(); break;
        }
    }
}
