using System;
using System.Numerics;
using ImGuiNET;
using MapEditor.Core.Map;
using MapEditor.Core.Actions;
using MapEditor.Core.State;
using MapEditor.Assets.Dat;
using MapEditor.Rendering;
using MapEditor.Rendering.Renderer;

namespace MapEditor.UI.Windows;

/// <summary>
/// ImGui Window container acting as the OpenGL viewport.
/// Renders the FBO color texture and captures mouse interactions (pan, zoom, draw) over the map.
/// </summary>
public sealed class MapViewportWindow
{
    private readonly EditorStore _store;
    private Vector2 _lastMousePos;
    private bool _isDrawing;

    public MapViewportWindow(EditorStore store)
    {
        _store = store;
    }

    public void Render(EditorState state, Camera camera, ViewportFbo fbo, DatDatabase? datDb)
    {
        // Disable scrollbars to prevent clipping of the map image canvas
        ImGuiWindowFlags flags = ImGuiWindowFlags.NoScrollbar | ImGuiWindowFlags.NoScrollWithMouse;

        // Eliminate window padding so the map fills the window edges completely
        ImGui.PushStyleVar(ImGuiStyleVar.WindowPadding, new Vector2(0, 0));

        if (ImGui.Begin("Viewport", flags))
        {
            Vector2 contentSize = ImGui.GetContentRegionAvail();

            // Resize the FBO buffer and update camera viewport bounds
            int w = (int)contentSize.X;
            int h = (int)contentSize.Y;
            
            fbo.Resize(w, h);
            camera.ViewportWidth = w;
            camera.ViewportHeight = h;

            // Render FBO texture inside ImGui. OpenGL textures are Y-inverted, so UVs are flipped.
            ImGui.Image(
                (IntPtr)fbo.ColorTexture, 
                contentSize, 
                new Vector2(0, 1), 
                new Vector2(1, 0)
            );

            // Capture mouse coordinates
            Vector2 mousePos = ImGui.GetMousePos();
            Vector2 windowPos = ImGui.GetWindowPos();
            Vector2 windowContentStart = windowPos + ImGui.GetCursorStartPos();
            
            // Calculate relative mouse coordinates inside the viewport canvas
            Vector2 relativeMousePos = mousePos - windowContentStart;

            bool isHovered = ImGui.IsItemHovered();

            // Translate mouse coordinate to grid coordinate indices
            Vector2 tileCoords = camera.ScreenToTile(relativeMousePos);
            int gridX = (int)Math.Floor(tileCoords.X);
            int gridY = (int)Math.Floor(tileCoords.Y);
            int gridZ = state.ActiveFloor;

            // 1. Panning: Dragging using Right or Middle mouse button
            if (ImGui.IsMouseDown(ImGuiMouseButton.Middle) || ImGui.IsMouseDown(ImGuiMouseButton.Right))
            {
                if (isHovered || ImGui.IsWindowFocused())
                {
                    Vector2 delta = mousePos - _lastMousePos;
                    camera.Pan(delta);
                }
            }

            // 2. Zooming: Scrolling mouse wheel
            if (isHovered)
            {
                float scroll = ImGui.GetIO().MouseWheel;
                if (Math.Abs(scroll) > 0.01f)
                {
                    float factor = scroll > 0 ? 1.1f : 0.9f;
                    camera.ZoomAt(relativeMousePos, factor);
                }
            }

            // 3. Editor Interaction (Drawing / Erasing / Filling)
            if (state.ActiveMap != null)
            {
                if (ImGui.IsMouseDown(ImGuiMouseButton.Left))
                {
                    if (isHovered && !_isDrawing)
                    {
                        // Start of the draw transaction (stroke): push a single undo snapshot
                        _isDrawing = true;
                        byte[] snapshot = MapSerializer.Serialize(state.ActiveMap);
                        _store.Dispatch(new PushUndoSnapshotAction($"Edit Stroke ({state.ActiveTool})", snapshot));
                    }

                    if (_isDrawing)
                    {
                        // Modify map tile directly during drag stroke
                        if (state.ActiveTool == EditorTool.Erase || datDb != null)
                        {
                            var itemInfo = datDb?.GetItem(state.SelectedItemId);
                            bool isGround = itemInfo?.HasFlag(DatFlags.Ground) ?? false;

                            ToolManager.ApplyToolDirect(
                                state.ActiveMap, 
                                new Position(gridX, gridY, gridZ), 
                                state.ActiveTool, 
                                state.SelectedItemId, 
                                isGround
                            );
                        }
                    }
                }
                else
                {
                    if (_isDrawing)
                    {
                        // End of the draw transaction
                        _isDrawing = false;
                        _store.Dispatch(new MapDirtyChangedAction(true));
                    }
                }
            }

            // 4. Overlay Info: Render current coordinate coordinates in the corner
            RenderCoordsOverlay(gridX, gridY, gridZ, state.ActiveTool);

            _lastMousePos = mousePos;
        }
        ImGui.End();
        ImGui.PopStyleVar();
    }

    private static void RenderCoordsOverlay(int x, int y, int z, EditorTool activeTool)
    {
        // Place overlay in the bottom-left corner of the viewport
        var overlayPos = ImGui.GetWindowPos() + new Vector2(10, ImGui.GetWindowHeight() - 40);
        ImGui.GetWindowDrawList().AddText(
            overlayPos, 
            ImGui.ColorConvertFloat4ToU32(new Vector4(1f, 1f, 1f, 0.8f)), 
            $"Tool: {activeTool}  |  X: {x}  Y: {y}  Z: {z}"
        );
    }
}
