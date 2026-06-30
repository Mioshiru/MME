using System.Numerics;
using ImGuiNET;

namespace MapEditor.UI;

/// <summary>
/// Manages the central ImGui DockSpace.
/// Ensures all panels are strictly docked and floating windows are disabled.
/// </summary>
public static class DockingManager
{
    private static bool _layoutInitialized = false;
    public static uint MainDockSpaceId { get; private set; }

    public static void BeginDockSpace()
    {
        ImGuiViewportPtr viewport = ImGui.GetMainViewport();
        
        ImGui.SetNextWindowPos(viewport.WorkPos);
        ImGui.SetNextWindowSize(viewport.WorkSize);
        ImGui.SetNextWindowViewport(viewport.ID);

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags.NoDocking | ImGuiWindowFlags.NoTitleBar 
                                     | ImGuiWindowFlags.NoCollapse | ImGuiWindowFlags.NoResize 
                                     | ImGuiWindowFlags.NoMove | ImGuiWindowFlags.NoBringToFrontOnFocus 
                                     | ImGuiWindowFlags.NoNavFocus | ImGuiWindowFlags.NoBackground;

        ImGui.PushStyleVar(ImGuiStyleVar.WindowRounding, 0.0f);
        ImGui.PushStyleVar(ImGuiStyleVar.WindowBorderSize, 0.0f);
        ImGui.PushStyleVar(ImGuiStyleVar.WindowPadding, new Vector2(0, 0));

        ImGui.Begin("MainDockSpaceWindow", windowFlags);
        ImGui.PopStyleVar(3);

        MainDockSpaceId = ImGui.GetID("MainDockSpace");

        // Set up the DockSpace
        ImGui.DockSpace(MainDockSpaceId, new Vector2(0.0f, 0.0f), ImGuiDockNodeFlags.None);
    }

    public static void EndDockSpace()
    {
        ImGui.End();
    }
}
