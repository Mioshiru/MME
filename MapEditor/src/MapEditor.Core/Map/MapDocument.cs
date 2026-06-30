namespace MapEditor.Core.Map;

/// <summary>
/// The top-level map document holding all 16 floor layers (z 0–15).
/// </summary>
public sealed class MapDocument
{
    public const int MinFloor = 0;
    public const int MaxFloor = 15;
    public const int FloorCount = MaxFloor - MinFloor + 1;

    public string Name { get; set; } = "Untitled";
    public string? FilePath { get; set; }
    public bool IsDirty { get; set; }

    public string Description { get; set; } = string.Empty;
    public string HouseFile { get; set; } = string.Empty;
    public string SpawnFile { get; set; } = string.Empty;

    private readonly MapLayer[] _layers = Enumerable
        .Range(MinFloor, FloorCount)
        .Select(f => new MapLayer(f))
        .ToArray();

    public MapLayer GetLayer(int floor)
    {
        ArgumentOutOfRangeException.ThrowIfLessThan(floor, MinFloor);
        ArgumentOutOfRangeException.ThrowIfGreaterThan(floor, MaxFloor);
        return _layers[floor - MinFloor];
    }

    public Tile? GetTile(Position pos) =>
        GetLayer(pos.Z).GetTile(pos.X, pos.Y);

    public Tile GetOrCreateTile(Position pos) =>
        GetLayer(pos.Z).GetOrCreateTile(pos.X, pos.Y);

    public IEnumerable<MapLayer> AllLayers => _layers;
}
