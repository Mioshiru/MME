using System;
using System.Collections.Generic;
using MapEditor.Core.Map;
using MapEditor.Assets.Dat;

namespace MapEditor.Assets.Otbm;

public sealed record ValidationError(Position Position, string Message, bool IsWarning);

/// <summary>
/// Pre-export integrity check validation for MapDocuments.
/// Detects missing ground tiles, floating doodads, and broken stairs/floor connections.
/// </summary>
public static class MapValidator
{
    public static List<ValidationError> Validate(MapDocument map, DatDatabase? datDb)
    {
        var errors = new List<ValidationError>();

        foreach (var layer in map.AllLayers)
        {
            foreach (var tile in layer.AllTiles.Values)
            {
                if (tile.IsEmpty) continue;

                // 1. Missing ground tile warning
                if (tile.GroundId == 0 && tile.Items.Count > 0)
                {
                    errors.Add(new ValidationError(
                        tile.Position,
                        $"Tile contains {tile.Items.Count} stacked items but has no ground tile.",
                        IsWarning: true
                    ));
                }

                // 2. Flying/Floating doodads check (no supporting tile on lower floor Z-1)
                // In Tibia, Z=7 is the surface, Z < 7 are upper floors (8 to 15, wait - Z 0-7 are surface/underground, 8-15 upper?)
                // Actually, floor Z level indices in RME:
                // Z=7 is ground level. Z=0 is highest roof. Z=15 is lowest dungeon.
                // So Z-1 is the floor above! Z+1 is the floor below!
                // Let's verify: Yes! Z decreases as we go up, and increases as we go down.
                // So if Z > 0, the floor directly below it (which is physically below, i.e., Z+1) should support it.
                // Wait! If Z is an upper floor (Z < 7, i.e., 0 to 6), the floor below is Z+1.
                // If Z is underground (Z > 7, i.e., 8 to 15), the floor above it (Z-1) supports it? No, Z-1 is closer to the surface.
                // In general, a floor is supported by the floor below it (which is Z+1).
                // Let's check: if tile is on an upper floor (Z < 7), it must have a supporting tile at Z+1!
                if (tile.Position.Z < 7)
                {
                    var supportPos = new Position(tile.Position.X, tile.Position.Y, tile.Position.Z + 1);
                    var supportTile = map.GetTile(supportPos);
                    if (supportTile == null || supportTile.IsEmpty)
                    {
                        errors.Add(new ValidationError(
                            tile.Position,
                            $"Tile is floating in mid-air (no supporting tile on the floor below Z={tile.Position.Z + 1}).",
                            IsWarning: true
                        ));
                    }
                }

                // 3. Broken stairs/floor changes check
                if (datDb != null)
                {
                    // Check items on the tile for floor change flags
                    foreach (ushort itemId in tile.Items)
                    {
                        var info = datDb.GetItem(itemId);
                        if (info != null && info.HasFlag(DatFlags2.FloorChange))
                        {
                            // Stairs lead either up (Z-1) or down (Z+1)
                            // Let's verify if landing tiles exist on both Z-1 and Z+1
                            var floorAbove = map.GetTile(new Position(tile.Position.X, tile.Position.Y, tile.Position.Z - 1));
                            var floorBelow = map.GetTile(new Position(tile.Position.X, tile.Position.Y, tile.Position.Z + 1));
                            
                            if ((floorAbove == null || floorAbove.IsEmpty) && (floorBelow == null || floorBelow.IsEmpty))
                            {
                                errors.Add(new ValidationError(
                                    tile.Position,
                                    $"Stairs/Floor change item #{itemId} has no landing tile on adjacent floors (Z-1 or Z+1).",
                                    IsWarning: false // treated as critical check error
                                ));
                            }
                        }
                    }
                }
            }
        }

        return errors;
    }
}
