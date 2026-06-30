using System;
using System.IO;
using System.Linq;
using MapEditor.Core.Map;

namespace MapEditor.Core.State;

/// <summary>
/// High-performance binary serializer for MapDocuments to support Undo/Redo state snapshots.
/// </summary>
public static class MapSerializer
{
    public static byte[] Serialize(MapDocument map)
    {
        using var ms = new MemoryStream();
        using var bw = new BinaryWriter(ms);

        bw.Write(map.Name);
        bw.Write(map.Description);
        bw.Write(map.HouseFile);
        bw.Write(map.SpawnFile);

        var layers = map.AllLayers.ToList();
        bw.Write(layers.Count);

        foreach (var layer in layers)
        {
            bw.Write(layer.Floor);
            var tiles = layer.AllTiles.Values.ToList();
            bw.Write(tiles.Count);

            foreach (var tile in tiles)
            {
                bw.Write(tile.Position.X);
                bw.Write(tile.Position.Y);
                bw.Write(tile.Position.Z);
                bw.Write(tile.GroundId);
                bw.Write(tile.Flags);

                bw.Write(tile.Items.Count);
                foreach (var item in tile.Items)
                {
                    bw.Write(item);
                }
            }
        }

        return ms.ToArray();
    }

    public static void Deserialize(byte[] data, MapDocument map)
    {
        using var ms = new MemoryStream(data);
        using var br = new BinaryReader(ms);

        map.Name = br.ReadString();
        map.Description = br.ReadString();
        map.HouseFile = br.ReadString();
        map.SpawnFile = br.ReadString();

        int layerCount = br.ReadInt32();
        for (int i = 0; i < layerCount; i++)
        {
            int floor = br.ReadInt32();
            var layer = map.GetLayer(floor);
            
            // Clear existing tiles in this layer
            var tilesToRemove = layer.AllTiles.Keys.ToList();
            foreach (var key in tilesToRemove)
            {
                layer.RemoveTile(key.x, key.y);
            }

            int tileCount = br.ReadInt32();
            for (int t = 0; t < tileCount; t++)
            {
                int x = br.ReadInt32();
                int y = br.ReadInt32();
                int z = br.ReadInt32();
                ushort groundId = br.ReadUInt16();
                uint flags = br.ReadUInt32();

                var tile = layer.GetOrCreateTile(x, y);
                tile.GroundId = groundId;
                tile.Flags = flags;

                tile.Items.Clear();
                int itemCount = br.ReadInt32();
                for (int itemIdx = 0; itemIdx < itemCount; itemIdx++)
                {
                    tile.Items.Add(br.ReadUInt16());
                }
            }
        }
    }
}
