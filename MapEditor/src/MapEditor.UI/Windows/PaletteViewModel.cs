using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Xml.Linq;
using MapEditor.Assets;
using MapEditor.Assets.Dat;

namespace MapEditor.UI.Windows;

public sealed class TilesetData
{
    public string Name { get; set; } = string.Empty;
    public List<ushort> ItemIds { get; } = new();
}

public sealed class PaletteViewModel
{
    private readonly AssetManager _assetManager;

    public Dictionary<ushort, string> ItemNames { get; } = new();
    public List<TilesetData> Tilesets { get; } = new();

    public int SelectedTilesetIndex { get; set; } = 0;
    public string SearchQuery { get; set; } = string.Empty;

    public bool IsDataLoaded { get; private set; }

    public PaletteViewModel(AssetManager assetManager)
    {
        _assetManager = assetManager;
    }

    public void LoadData()
    {
        if (!_assetManager.IsReady) return;

        ItemNames.Clear();
        Tilesets.Clear();
        SelectedTilesetIndex = 0;

        LoadItemsXml();
        LoadTilesetsXml();

        if (Tilesets.Count == 0)
        {
            BuildFallbackCategories();
        }

        IsDataLoaded = true;
    }

    private void LoadItemsXml()
    {
        string[] candidates = {
            Path.Combine(AppContext.BaseDirectory, "data", "1098", "items.xml"),
            Path.Combine("data", "1098", "items.xml"),
        };

        string? foundPath = candidates.FirstOrDefault(File.Exists);
        if (foundPath == null)
        {
            Console.WriteLine("[PaletteViewModel] WARNING: items.xml not found. Using default ID labels.");
            return;
        }

        try
        {
            var doc = XDocument.Load(foundPath);
            var itemsNode = doc.Element("items");
            if (itemsNode == null) return;

            foreach (var el in itemsNode.Elements("item"))
            {
                var idAttr = el.Attribute("id");
                var fromIdAttr = el.Attribute("fromid");
                var toIdAttr = el.Attribute("toid");
                var nameAttr = el.Attribute("name");

                string name = nameAttr?.Value ?? "Unnamed";

                if (idAttr != null && ushort.TryParse(idAttr.Value, out ushort id))
                {
                    ItemNames[id] = name;
                }
                else if (fromIdAttr != null && toIdAttr != null &&
                         ushort.TryParse(fromIdAttr.Value, out ushort fromId) &&
                         ushort.TryParse(toIdAttr.Value, out ushort toId))
                {
                    for (ushort i = fromId; i <= toId; i++)
                    {
                        ItemNames[i] = name;
                    }
                }
            }
            Console.WriteLine($"[PaletteViewModel] Parsed {ItemNames.Count} item names from items.xml");
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[PaletteViewModel] ERROR parsing items.xml: {ex.Message}");
        }
    }

    private void LoadTilesetsXml()
    {
        string[] candidates = {
            Path.Combine(AppContext.BaseDirectory, "data", "1098", "tilesets.xml"),
            Path.Combine("data", "1098", "tilesets.xml"),
        };

        string? foundPath = candidates.FirstOrDefault(File.Exists);
        if (foundPath == null)
        {
            Console.WriteLine("[PaletteViewModel] WARNING: tilesets.xml not found.");
            return;
        }

        try
        {
            var doc = XDocument.Load(foundPath);
            var materialsNode = doc.Element("materials");
            if (materialsNode == null) return;

            foreach (var tilesetEl in materialsNode.Elements("tileset"))
            {
                var nameAttr = tilesetEl.Attribute("name");
                if (nameAttr == null) continue;

                var tileset = new TilesetData { Name = nameAttr.Value };

                foreach (var container in tilesetEl.Elements())
                {
                    foreach (var itemEl in container.Elements("item"))
                    {
                        var idAttr = itemEl.Attribute("id");
                        if (idAttr != null && ushort.TryParse(idAttr.Value, out ushort id))
                        {
                            if (!tileset.ItemIds.Contains(id))
                            {
                                tileset.ItemIds.Add(id);
                            }
                        }
                    }
                }

                if (tileset.ItemIds.Count > 0)
                {
                    Tilesets.Add(tileset);
                }
            }

            Console.WriteLine($"[PaletteViewModel] Loaded {Tilesets.Count} tilesets from tilesets.xml");
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[PaletteViewModel] ERROR parsing tilesets.xml: {ex.Message}");
        }
    }

    private void BuildFallbackCategories()
    {
        Console.WriteLine("[PaletteViewModel] Building fallback tileset groups from DAT database.");
        if (_assetManager.DatDb == null) return;

        var groundGroup = new TilesetData { Name = "Grounds" };
        var borderGroup = new TilesetData { Name = "Borders" };
        var doodadGroup = new TilesetData { Name = "Doodads" };
        var itemGroup   = new TilesetData { Name = "Items" };

        foreach (var info in _assetManager.DatDb.Items)
        {
            if (info == null) continue;
            ushort id = info.Id;

            if (info.HasFlag(DatFlags.Ground))
                groundGroup.ItemIds.Add(id);
            else if (info.HasFlag(DatFlags.GroundBorder))
                borderGroup.ItemIds.Add(id);
            else if (info.HasFlag(DatFlags.OnBottom) || info.HasFlag(DatFlags.OnTop))
                doodadGroup.ItemIds.Add(id);
            else
                itemGroup.ItemIds.Add(id);
        }

        if (groundGroup.ItemIds.Count > 0) Tilesets.Add(groundGroup);
        if (borderGroup.ItemIds.Count > 0) Tilesets.Add(borderGroup);
        if (doodadGroup.ItemIds.Count > 0) Tilesets.Add(doodadGroup);
        if (itemGroup.ItemIds.Count > 0)   Tilesets.Add(itemGroup);
    }
}
