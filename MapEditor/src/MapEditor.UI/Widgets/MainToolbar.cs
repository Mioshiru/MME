using System;
using System.Numerics;
using ImGuiNET;
using MapEditor.Core.Actions;
using MapEditor.Core.State;

namespace MapEditor.UI.Widgets;

/// <summary>
/// Icon toolbar row rendered immediately below the main menu bar.
/// Hosts tool selection buttons (Draw, Erase, Select, Fill, etc.) using PNG icons.
/// </summary>
public sealed class MainToolbar
{
    private readonly EditorStore _store;

    // Button size in pixels for the toolbar buttons
    private static readonly Vector2 ButtonSize = new(32, 32);

    public MainToolbar(EditorStore store)
    {
        _store = store;
    }

    /// <summary>
    /// Renders the toolbar contents without creating its own window.
    /// </summary>
    public void RenderContents(EditorState state, ToolbarIcons icons)
    {
        ImGui.PushStyleVar(ImGuiStyleVar.WindowPadding, new Vector2(6, 6));
        ImGui.PushStyleVar(ImGuiStyleVar.ItemSpacing,   new Vector2(6, 0));

        RenderToolButton(state, EditorTool.Draw,         "D",  "Draw Tool (B)",       icons.DrawIcon);
        ImGui.SameLine();
        RenderToolButton(state, EditorTool.Erase,        "E",  "Erase Tool (DEL)",     icons.EraseIcon);
        ImGui.SameLine();
        RenderToolButton(state, EditorTool.Select,       "S",  "Selection Tool (M)",   icons.SelectIcon);
        ImGui.SameLine();
        RenderToolButton(state, EditorTool.Fill,         "F",  "Fill Bucket Tool (G)", icons.FillIcon);
        ImGui.SameLine();
        RenderToolButton(state, EditorTool.SpawnBrush,   "Sp", "Spawn Brush",          icons.SpawnIcon);
        ImGui.SameLine();
        RenderToolButton(state, EditorTool.WaypointBrush,"Wp", "Waypoint Brush",       icons.WaypointIcon);

        ImGui.SameLine();
        // Vertical separator line
        ImGui.Dummy(new Vector2(1, ButtonSize.Y));
        ImGui.SameLine();

        // Floor Selection input
        ImGui.AlignTextToFramePadding();
        ImGui.Text("Floor:");
        ImGui.SameLine();
        ImGui.SetNextItemWidth(60);
        
        int floor = state.ActiveFloor;
        if (ImGui.InputInt("##floor", ref floor, 1, 1))
        {
            floor = Math.Clamp(floor, 0, 15);
            _store.Dispatch(new SelectFloorAction(floor));
        }

        ImGui.PopStyleVar(2);
    }

    private void RenderToolButton(EditorState state, EditorTool tool, string label, string tooltip, uint iconTexture)
    {
        bool active = state.ActiveTool == tool;

        if (active)
        {
            ImGui.PushStyleColor(ImGuiCol.Button,        new Vector4(0.20f, 0.45f, 0.80f, 1f));
            ImGui.PushStyleColor(ImGuiCol.ButtonHovered, new Vector4(0.25f, 0.55f, 0.90f, 1f));
        }

        bool clicked = false;
        if (iconTexture != 0)
        {
            // Draw image icon button
            clicked = ImGui.ImageButton("##" + label, (IntPtr)iconTexture, ButtonSize);
        }
        else
        {
            // Fallback to text button if the texture is missing
            clicked = ImGui.Button(label, ButtonSize);
        }

        if (clicked)
        {
            _store.Dispatch(new SelectToolAction(tool));
        }

        if (active)
        {
            ImGui.PopStyleColor(2);
        }

        if (ImGui.IsItemHovered())
        {
            ImGui.SetTooltip(tooltip);
        }
    }
}
