using System;
using System.Collections.Generic;
using MapEditor.Core.Actions;
using MapEditor.Core.State;

namespace MapEditor.Core.Map;

/// <summary>
/// Executes drawing tools (Draw, Erase, Fill) directly on the MapDocument.
/// Does not depend on the Assets project, preventing circular project references.
/// </summary>
public static class ToolManager
{
    /// <summary>
    /// Applies the specified tool directly to the map coordinate.
    /// </summary>
    /// <param name="map">The map document.</param>
    /// <param name="pos">Grid position.</param>
    /// <param name="tool">The active editor tool.</param>
    /// <param name="brushItemId">The item/brush ID.</param>
    /// <param name="isGround">True if the item is a ground/terrain tile in the client database.</param>
    public static void ApplyToolDirect(
        MapDocument map, 
        Position pos, 
        EditorTool tool, 
        ushort brushItemId, 
        bool isGround)
    {
        if (pos.Z < MapDocument.MinFloor || pos.Z > MapDocument.MaxFloor) return;

        var layer = map.GetLayer(pos.Z);
        var tile = layer.GetOrCreateTile(pos.X, pos.Y);

        switch (tool)
        {
            case EditorTool.Draw:
                ExecuteDraw(tile, brushItemId, isGround);
                break;
            case EditorTool.Erase:
                ExecuteErase(tile);
                break;
            case EditorTool.Fill:
                ExecuteFill(layer, pos, brushItemId, isGround);
                break;
        }
    }

    private static void ExecuteDraw(Tile tile, ushort brushItemId, bool isGround)
    {
        if (brushItemId == 0) return;

        if (isGround)
        {
            tile.GroundId = brushItemId;
        }
        else
        {
            if (!tile.Items.Contains(brushItemId))
            {
                tile.Items.Add(brushItemId);
            }
        }
    }

    private static void ExecuteErase(Tile tile)
    {
        tile.GroundId = 0;
        tile.Items.Clear();
    }

    private static void ExecuteFill(
        MapLayer layer, 
        Position startPos, 
        ushort brushItemId, 
        bool isGround)
    {
        if (brushItemId == 0 || !isGround)
        {
            return; // flood fill ground items only
        }

        var startTile = layer.GetTile(startPos.X, startPos.Y);
        ushort targetGroundId = startTile?.GroundId ?? 0;

        if (targetGroundId == brushItemId) return;

        var queue = new Queue<(int x, int y)>();
        var visited = new HashSet<(int x, int y)>();
        
        queue.Enqueue((startPos.X, startPos.Y));
        visited.Add((startPos.X, startPos.Y));

        const int maxFillTiles = 10000;
        int filledCount = 0;

        while (queue.Count > 0 && filledCount < maxFillTiles)
        {
            var (cx, cy) = queue.Dequeue();
            var tile = layer.GetOrCreateTile(cx, cy);

            if (tile.GroundId == targetGroundId)
            {
                tile.GroundId = brushItemId;
                filledCount++;

                EnqueueNeighbor(cx + 1, cy);
                EnqueueNeighbor(cx - 1, cy);
                EnqueueNeighbor(cx, cy + 1);
                EnqueueNeighbor(cx, cy - 1);
            }
        }

        void EnqueueNeighbor(int nx, int ny)
        {
            if (nx < 0 || nx > 65535 || ny < 0 || ny > 65535) return;
            
            var key = (nx, ny);
            if (!visited.Contains(key))
            {
                var t = layer.GetTile(nx, ny);
                ushort gid = t?.GroundId ?? 0;
                if (gid == targetGroundId)
                {
                    queue.Enqueue(key);
                    visited.Add(key);
                }
            }
        }
    }
}
