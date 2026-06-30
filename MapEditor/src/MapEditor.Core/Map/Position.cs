namespace MapEditor.Core.Map;

/// <summary>
/// Represents a 3-dimensional position on the map (x, y, z/floor).
/// </summary>
public readonly record struct Position(int X, int Y, int Z)
{
    public static readonly Position Zero = new(0, 0, 0);

    public Position Offset(int dx, int dy, int dz = 0) => new(X + dx, Y + dy, Z + dz);

    public override string ToString() => $"({X}, {Y}, {Z})";
}
