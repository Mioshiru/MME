using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using MapEditor.Core.Map;

namespace MapEditor.Assets.Otbm;

/// <summary>
/// Exports a <see cref="MapDocument"/> into the Tibia Map Format (.otbm).
/// Groups tiles into 256x256 area blocks and encodes using escape-coded node structures.
/// </summary>
public static class OtbmExporter
{
    private const byte OtbmFormatVersion = 2;
    private const ushort OtbItemsMajor = 3;
    private const uint OtbItemsMinor = 57; // Tibia 10.98 standard OTB minor version

    /// <summary>
    /// Exports the MapDocument to the specified file path.
    /// Runs validation prior to export.
    /// </summary>
    public static void Export(MapDocument map, string path)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(path);
        if (map == null) throw new ArgumentNullException(nameof(map));

        // Ensure directories exist
        var dir = Path.GetDirectoryName(path);
        if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir))
        {
            Directory.CreateDirectory(dir);
        }

        using (var fs = new FileStream(path, FileMode.Create, FileAccess.Write, FileShare.None))
        {
            // 1. Magic bytes (4 bytes: 0x00 0x00 0x00 0x00)
            fs.Write(new byte[] { 0, 0, 0, 0 }, 0, 4);

            using (var writer = new OtbmNodeWriter(fs))
            {
                // 2. Root Node start (RootV1 = 0x00)
                writer.WriteNodeStart((byte)OtbmNodeType.RootV1);

                // Write Root Node Header
                writer.WriteUInt32(OtbmFormatVersion); // version (uint32)
                writer.WriteUInt16(0);                 // map width (uint16, dummy)
                writer.WriteUInt16(0);                 // map height (uint16, dummy)
                writer.WriteUInt32(OtbItemsMajor);     // major version of OTB (uint32)
                writer.WriteUInt32(OtbItemsMinor);     // minor version of OTB (uint32)

                // 3. Map Data Node start (MapData = 0x02)
                writer.WriteNodeStart((byte)OtbmNodeType.MapData);

                // Write Map attributes
                if (!string.IsNullOrEmpty(map.Description))
                {
                    writer.WriteByte((byte)OtbmAttribute.Description);
                    writer.WriteString(map.Description);
                }
                if (!string.IsNullOrEmpty(map.SpawnFile))
                {
                    writer.WriteByte((byte)OtbmAttribute.ExtSpawnFile);
                    writer.WriteString(map.SpawnFile);
                }
                if (!string.IsNullOrEmpty(map.HouseFile))
                {
                    writer.WriteByte((byte)OtbmAttribute.ExtHouseFile);
                    writer.WriteString(map.HouseFile);
                }

                // Partition all tiles on the map into 256x256 areas
                // Key: (AreaX_256, AreaY_256, Floor)
                var areas = new Dictionary<(int ax, int ay, int z), List<Tile>>();
                foreach (var layer in map.AllLayers)
                {
                    foreach (var tile in layer.AllTiles.Values)
                    {
                        if (tile.IsEmpty) continue;

                        int ax = tile.Position.X / 256;
                        int ay = tile.Position.Y / 256;
                        var key = (ax, ay, tile.Position.Z);

                        if (!areas.TryGetValue(key, out var tilesList))
                        {
                            tilesList = new List<Tile>();
                            areas[key] = tilesList;
                        }
                        tilesList.Add(tile);
                    }
                }

                // Export each TileArea
                foreach (var kvp in areas)
                {
                    var key = kvp.Key;
                    var tiles = kvp.Value;

                    ushort baseAreaX = (ushort)(key.ax * 256);
                    ushort baseAreaY = (ushort)(key.ay * 256);
                    byte baseAreaZ = (byte)key.z;

                    // TileArea Node start (0x04)
                    writer.WriteNodeStart((byte)OtbmNodeType.TileArea);
                    writer.WriteUInt16(baseAreaX);
                    writer.WriteUInt16(baseAreaY);
                    writer.WriteByte(baseAreaZ);

                    foreach (var tile in tiles)
                    {
                        byte relX = (byte)(tile.Position.X - baseAreaX);
                        byte relY = (byte)(tile.Position.Y - baseAreaY);

                        // Tile Node start (0x05)
                        writer.WriteNodeStart((byte)OtbmNodeType.Tile);
                        writer.WriteByte(relX);
                        writer.WriteByte(relY);

                        // Tile flags attribute (0x03)
                        if (tile.Flags != 0)
                        {
                            writer.WriteByte((byte)OtbmAttribute.TileFlags);
                            writer.WriteUInt32(tile.Flags);
                        }

                        // Tile ground/terrain attribute (0x09)
                        if (tile.GroundId != 0)
                        {
                            writer.WriteByte((byte)OtbmAttribute.Item);
                            writer.WriteUInt16(tile.GroundId);
                        }

                        // Nested Item Nodes
                        foreach (ushort itemId in tile.Items)
                        {
                            writer.WriteNodeStart((byte)OtbmNodeType.Item);
                            writer.WriteUInt16(itemId);
                            // End Item node
                            writer.WriteNodeEnd();
                        }

                        // End Tile node
                        writer.WriteNodeEnd();
                    }

                    // End TileArea node
                    writer.WriteNodeEnd();
                }

                // End MapData node
                writer.WriteNodeEnd();

                // End Root Node
                writer.WriteNodeEnd();
            }
        }

        // 4. Verification loop: Read file header and check magic bytes/version integrity
        if (!VerifySavedFile(path))
        {
            throw new InvalidDataException("OTBM Export Verification Failed: File header is corrupt or version mismatch!");
        }
    }

    /// <summary>
    /// Verifies the structural integrity of the exported file's headers.
    /// Returns true if the file header and root node match the OTBM specification.
    /// </summary>
    public static bool VerifySavedFile(string path)
    {
        if (!File.Exists(path)) return false;

        try
        {
            byte[] raw = File.ReadAllBytes(path);
            if (raw.Length < 16) return false;

            // 1. Check Magic bytes
            if (BitConverter.ToUInt32(raw, 0) != 0) return false;

            // 2. Read Root Node header using the verified escape-coded reader
            var reader = new OtbmNodeReader(raw.AsMemory(4));
            
            // Expected FE (NodeStart)
            if (!reader.PeekNodeStart()) return false;
            reader.ExpectNodeStart();

            // Expected 0x00 (RootV1)
            byte rootType = reader.ReadByte();
            if (rootType != (byte)OtbmNodeType.RootV1) return false;

            // Expected version
            uint version = reader.ReadUInt32();
            if (version != OtbmFormatVersion) return false;

            return true;
        }
        catch
        {
            return false;
        }
    }

    /// <summary>
    /// Sequential helper to write binary structures transparently handling FD escape coding.
    /// </summary>
    private sealed class OtbmNodeWriter : IDisposable
    {
        private readonly Stream _stream;
        private const byte NodeStart  = 0xFE;
        private const byte NodeEnd    = 0xFF;
        private const byte EscapeByte = 0xFD;

        public OtbmNodeWriter(Stream stream) => _stream = stream;

        public void WriteByte(byte b)
        {
            if (b == NodeStart || b == NodeEnd || b == EscapeByte)
            {
                _stream.WriteByte(EscapeByte);
            }
            _stream.WriteByte(b);
        }

        public void WriteUInt16(ushort v)
        {
            WriteByte((byte)(v & 0xFF));
            WriteByte((byte)((v >> 8) & 0xFF));
        }

        public void WriteUInt32(uint v)
        {
            WriteByte((byte)(v & 0xFF));
            WriteByte((byte)((v >> 8) & 0xFF));
            WriteByte((byte)((v >> 16) & 0xFF));
            WriteByte((byte)((v >> 24) & 0xFF));
        }

        public void WriteString(string s)
        {
            byte[] bytes = System.Text.Encoding.UTF8.GetBytes(s);
            WriteUInt16((ushort)bytes.Length);
            foreach (byte b in bytes)
            {
                WriteByte(b);
            }
        }

        public void WriteNodeStart(byte type)
        {
            // Node Start FE is written raw
            _stream.WriteByte(NodeStart);
            WriteByte(type);
        }

        public void WriteNodeEnd()
        {
            // Node End FF is written raw
            _stream.WriteByte(NodeEnd);
        }

        public void Dispose() => _stream.Dispose();
    }
}
