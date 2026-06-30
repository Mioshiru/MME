using ImGuiNET;
using MapEditor.Core.State;

namespace MapEditor.UI.Widgets;

public sealed class ToolbarFactory
{
    private readonly MainToolbar _mainToolbar;

    public ToolbarFactory(EditorStore store)
    {
        _mainToolbar = new MainToolbar(store);
    }

    public void Render(EditorState state, ToolbarIcons icons)
    {
        var flags = ImGuiWindowFlags.NoCollapse | ImGuiWindowFlags.NoTitleBar 
                  | ImGuiWindowFlags.NoScrollbar;

        if (ImGui.Begin("Toolbars", flags))
        {
            _mainToolbar.RenderContents(state, icons);

            // In the future: Render Brush/Size toolbars here on the same line or below
            // BrushToolbar.RenderContents(state);
            // SizeToolbar.RenderContents(state);
        }
        ImGui.End();
    }
}
