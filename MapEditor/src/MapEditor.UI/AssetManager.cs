using System;
using System.IO;
using MapEditor.Assets.Dat;
using MapEditor.Assets.Spr;
using MapEditor.Assets.Atlas;
using MapEditor.Rendering.Textures;
using Silk.NET.OpenGL;
using System.Collections.Generic;

namespace MapEditor.UI;

public class AssetManager : IDisposable
{
    public DatDatabase? DatDb { get; private set; }
    public TextureAtlas? UiAtlas { get; private set; }
    public TextureManager? TextureManager { get; private set; }

    public bool IsReady => DatDb != null && UiAtlas != null && TextureManager != null && TextureManager.IsUploaded;

    private readonly GL _gl;
    private bool _disposed;

    public AssetManager(GL gl)
    {
        _gl = gl;
        TextureManager = new TextureManager(_gl);
        UiAtlas = new TextureAtlas(_gl);
    }

    public void LoadAssets(string baseDirectory)
    {
        Console.WriteLine("[AssetManager] Loading assets...");
        
        // 1. Load Tibia.dat
        string[] datCandidates = {
            Path.Combine(baseDirectory, "data", "1098", "Tibia.dat"),
            Path.Combine("data", "1098", "Tibia.dat"),
        };
        foreach (var candidate in datCandidates)
        {
            if (File.Exists(candidate))
            {
                try
                {
                    DatDb = DatParser.Parse(candidate);
                    Console.WriteLine($"[AssetManager] Loaded Tibia.dat: {DatDb.ItemCount} items from '{candidate}'");
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"[AssetManager] WARNING: Failed to parse Tibia.dat at '{candidate}': {ex.Message}");
                }
                break;
            }
        }

        if (DatDb == null)
        {
            Console.WriteLine("[AssetManager] WARNING: Tibia.dat not found. Terrain palette will be empty.");
        }

        // 2. Load Tibia.spr
        string[] sprCandidates = {
            Path.Combine(baseDirectory, "data", "1098", "Tibia.spr"),
            Path.Combine("data", "1098", "Tibia.spr"),
        };
        foreach (var candidate in sprCandidates)
        {
            if (File.Exists(candidate))
            {
                try
                {
                    var sprDb = SprParser.ParseOffsetTable(candidate);
                    Console.WriteLine($"[AssetManager] Loaded Tibia.spr: {sprDb.SpriteCount} sprites from '{candidate}'");

                    // Map atlas
                    var mapAtlasResult = TextureAtlasGenerator.Generate(sprDb, candidate);
                    TextureManager.UploadAtlases(mapAtlasResult);
                    Console.WriteLine($"[AssetManager] Map TextureManager atlas uploaded: {mapAtlasResult.Sheets.Count} sheets.");

                    // UI atlas
                    var spriteDict = new Dictionary<uint, SpriteData>();
                    for (uint i = 1; i <= sprDb.SpriteCount; i++)
                    {
                        if (sprDb.HasSprite(i))
                        {
                            var sprite = SprParser.DecodeSprite(sprDb, candidate, i);
                            if (sprite != null)
                            {
                                spriteDict[i] = sprite;
                            }
                        }
                    }
                    UiAtlas.Build(spriteDict);
                    Console.WriteLine($"[AssetManager] UI TextureAtlas built successfully: {UiAtlas.AtlasWidth}x{UiAtlas.AtlasHeight} ({spriteDict.Count} sprites)");
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"[AssetManager] WARNING: Failed to parse Tibia.spr or build atlases at '{candidate}': {ex.Message}");
                }
                break;
            }
        }
    }

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;

        TextureManager?.Dispose();
        UiAtlas?.Dispose();
    }
}
