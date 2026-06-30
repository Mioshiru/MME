using MapEditor.Core.Map;

namespace MapEditor.Assets.Otbm;

// ── Node type constants (mirrors RME's OTBM_NodeTypes_t) ─────────────────────

public enum OtbmNodeType : byte
{
    RootV1       = 0x00,
    MapData      = 0x02,
    ItemDef      = 0x03,
    TileArea     = 0x04,
    Tile         = 0x05,
    Item         = 0x06,
    TileSquare   = 0x07,
    TileRef      = 0x08,
    Spawns       = 0x09,
    SpawnArea    = 0x0A,
    Spawn        = 0x0B,
    Towns        = 0x0C,
    Town         = 0x0D,
    HouseData    = 0x0E,
    Waypoints    = 0x0F,
    Waypoint     = 0x10,
}

// ── OTBM attribute constants ──────────────────────────────────────────────────

public enum OtbmAttribute : byte
{
    Description  = 0x01,
    ExtFile      = 0x02,
    TileFlags    = 0x03,
    ActionId     = 0x04,
    UniqueId     = 0x05,
    Text         = 0x06,
    Desc         = 0x07,
    TeleportDest = 0x08,
    Item         = 0x09,
    DepotId      = 0x0A,
    ExtSpawnFile = 0x0B,
    RuneCharges  = 0x0C,
    ExtHouseFile = 0x0D,
    HouseId      = 0x0E,
    Count        = 0x0F,
    Duration     = 0x10,
    DecayingState= 0x11,
    WrittenDate  = 0x12,
    WrittenBy    = 0x13,
    SlotPos      = 0x14,
    LiquidContainer=0x15,
    Charged      = 0x16,
    AnimateAlways= 0x17,
    SkullOwner   = 0x19,
    Corpse       = 0x1A,
}

/// <summary>
/// Imports a .otbm file into the runtime <see cref="MapDocument"/> model.
///
/// The OTBM format uses an escape-coded node tree (each node begins with 0xFE,
/// child nodes are nested, 0xFF marks end-of-node, and 0xFD is the escape byte).
/// </summary>
public sealed class OtbmImporter
{
    private const byte NodeStart  = 0xFE;
    private const byte NodeEnd    = 0xFF;
    private const byte EscapeByte = 0xFD;

    /// <summary>
    /// Loads a .otbm file and populates a new <see cref="MapDocument"/>.
    /// </summary>
    public MapDocument Import(string path)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(path);
        if (!File.Exists(path)) throw new FileNotFoundException($".otbm file not found: {path}");

        var raw = File.ReadAllBytes(path);
        // Validate OTBM magic (first 4 bytes = 0x00000000)
        if (raw.Length < 4 || BitConverter.ToUInt32(raw, 0) != 0)
            throw new InvalidDataException("Invalid OTBM file: bad magic bytes.");

        var doc = new MapDocument
        {
            FilePath = path,
            Name = Path.GetFileName(path)
        };

        // Parse the node tree starting after the 4-byte magic
        var reader = new OtbmNodeReader(raw.AsMemory(4));
        ParseRootNode(reader, doc);

        return doc;
    }

    private static void ParseRootNode(OtbmNodeReader reader, MapDocument doc)
    {
        // Root node
        reader.ExpectNodeStart();
        byte rootType = reader.ReadByte(); // should be 0x00 (RootV1)
        // Skip root attributes (version info etc.) — read until first child or end
        SkipAttributes(reader);

        while (reader.PeekNodeStart())
        {
            reader.ExpectNodeStart();
            byte nodeType = reader.ReadByte();

            if ((OtbmNodeType)nodeType == OtbmNodeType.MapData)
                ParseMapData(reader, doc);
            else
                reader.SkipNode();
        }

        reader.ExpectNodeEnd();
    }

    private static void ParseMapData(OtbmNodeReader reader, MapDocument doc)
    {
        // Read map-level attributes
        while (!reader.PeekNodeStart() && !reader.PeekNodeEnd())
        {
            byte attr = reader.ReadByte();
            switch ((OtbmAttribute)attr)
            {
                case OtbmAttribute.Description:  doc.Description  = reader.ReadString(); break;
                case OtbmAttribute.ExtSpawnFile: doc.SpawnFile    = reader.ReadString(); break;
                case OtbmAttribute.ExtHouseFile: doc.HouseFile    = reader.ReadString(); break;
                default: goto done; // unknown attr — stop attribute parsing
            }
        }
        done:

        while (reader.PeekNodeStart())
        {
            reader.ExpectNodeStart();
            byte nodeType = reader.ReadByte();

            if ((OtbmNodeType)nodeType == OtbmNodeType.TileArea)
                ParseTileArea(reader, doc);
            else
                reader.SkipNode();
        }

        reader.ExpectNodeEnd();
    }

    private static void ParseTileArea(OtbmNodeReader reader, MapDocument doc)
    {
        ushort areaX = reader.ReadUInt16();
        ushort areaY = reader.ReadUInt16();
        byte   areaZ = reader.ReadByte();

        while (reader.PeekNodeStart())
        {
            reader.ExpectNodeStart();
            byte nodeType = reader.ReadByte();

            if ((OtbmNodeType)nodeType is OtbmNodeType.Tile or OtbmNodeType.HouseData)
            {
                byte relX = reader.ReadByte();
                byte relY = reader.ReadByte();
                var pos = new Position(areaX + relX, areaY + relY, areaZ);
                var tile = doc.GetOrCreateTile(pos);

                // Parse tile attributes
                while (!reader.PeekNodeStart() && !reader.PeekNodeEnd())
                {
                    byte attr = reader.ReadByte();
                    switch ((OtbmAttribute)attr)
                    {
                        case OtbmAttribute.TileFlags:
                            tile.Flags = reader.ReadUInt32();
                            break;
                        case OtbmAttribute.Item:
                            ushort groundId = reader.ReadUInt16();
                            tile.GroundId = groundId;
                            break;
                        default: goto tileAttrsDone;
                    }
                }
                tileAttrsDone:

                // Parse child item nodes
                while (reader.PeekNodeStart())
                {
                    reader.ExpectNodeStart();
                    byte childType = reader.ReadByte();
                    if ((OtbmNodeType)childType == OtbmNodeType.Item)
                    {
                        ushort itemId = reader.ReadUInt16();
                        tile.Items.Add(itemId);
                        SkipAttributes(reader);
                        // Skip any nested children
                        while (reader.PeekNodeStart()) reader.SkipNode();
                    }
                    else
                    {
                        reader.SkipNode();
                    }
                    reader.ExpectNodeEnd();
                }
            }
            else
            {
                reader.SkipNode();
            }

            reader.ExpectNodeEnd();
        }

        reader.ExpectNodeEnd();
    }

    private static void SkipAttributes(OtbmNodeReader reader)
    {
        // Consume bytes until we hit a node boundary
        while (!reader.PeekNodeStart() && !reader.PeekNodeEnd())
            reader.ReadByte();
    }
}
