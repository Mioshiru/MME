namespace MapEditor.Core.Map;

/// <summary>
/// Represents a single map tile, holding its ground item ID and a stack of additional items.
/// </summary>
public sealed class Tile
{
    public Position Position { get; }

    /// <summary>Ground item/terrain ID (0 = empty).</summary>
    public ushort GroundId { get; set; }

    /// <summary>Stacked item IDs on this tile (creatures, items, etc.).</summary>
    public List<ushort> Items { get; } = new();

    /// <summary>Bitfield of tile flags (protection zone, no PvP, etc.).</summary>
    public uint Flags { get; set; }

    public Tile(Position position, ushort groundId = 0)
    {
        Position = position;
        GroundId = groundId;
    }

    public bool IsEmpty => GroundId == 0 && Items.Count == 0;
}
