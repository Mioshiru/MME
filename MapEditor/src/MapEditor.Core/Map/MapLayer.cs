namespace MapEditor.Core.Map;

/// <summary>
/// A single floor layer (z-level) of the map, holding sparse tile data.
/// </summary>
public sealed class MapLayer
{
    public int Floor { get; }

    private readonly Dictionary<(int x, int y), Tile> _tiles = new();

    public MapLayer(int floor) => Floor = floor;

    public Tile? GetTile(int x, int y) =>
        _tiles.TryGetValue((x, y), out var tile) ? tile : null;

    public Tile GetOrCreateTile(int x, int y)
    {
        var key = (x, y);
        if (!_tiles.TryGetValue(key, out var tile))
        {
            tile = new Tile(new Position(x, y, Floor));
            _tiles[key] = tile;
        }
        return tile;
    }

    public void SetTile(Tile tile) => _tiles[(tile.Position.X, tile.Position.Y)] = tile;

    public void RemoveTile(int x, int y) => _tiles.Remove((x, y));

    public IEnumerable<Tile> GetTilesInRegion(int minX, int minY, int maxX, int maxY)
    {
        foreach (var ((x, y), tile) in _tiles)
        {
            if (x >= minX && x <= maxX && y >= minY && y <= maxY)
                yield return tile;
        }
    }

    public IReadOnlyDictionary<(int x, int y), Tile> AllTiles => _tiles;
}
