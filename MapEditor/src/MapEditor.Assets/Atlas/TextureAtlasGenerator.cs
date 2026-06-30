using System;
using System.Collections.Generic;
using System.IO;
using MapEditor.Assets.Spr;

namespace MapEditor.Assets.Atlas;

/// <summary>
/// Mappings for a packed sprite in the virtual texture atlas sheet/layer.
/// </summary>
public readonly record struct SpriteMapping(int AtlasIndex, float U0, float V0, float U1, float V1);

/// <summary>
/// The result of the texture atlas generation process.
/// Contains raw pixel sheets (RGBA8) and a lookup table.
/// </summary>
public sealed class AtlasGenerationResult
{
    public int AtlasWidth { get; }
    public int AtlasHeight { get; }
    public List<byte[]> Sheets { get; }
    public SpriteMapping[] Mappings { get; }

    public AtlasGenerationResult(int width, int height, List<byte[]> sheets, SpriteMapping[] mappings)
    {
        AtlasWidth = width;
        AtlasHeight = height;
        Sheets = sheets;
        Mappings = mappings;
    }
}

/// <summary>
/// High-performance CPU-side texture atlas packer.
/// Arranges 32x32 sprites into larger power-of-two virtual sheets (RGBA8).
/// Packs only non-empty sprites sequentially to minimize memory usage and GPU state switches.
/// </summary>
public static class TextureAtlasGenerator
{
    private const int SpriteSize = 32;
    private const int BytesPerPixel = 4; // RGBA8

    /// <summary>
    /// Sequentially packs all non-empty sprites from a .spr database into virtual sheets of the specified size.
    /// Runs entirely on the CPU (can be executed on a background thread).
    /// </summary>
    /// <param name="db">The parsed spr database.</param>
    /// <param name="sprPath">Path to Tibia.spr.</param>
    /// <param name="sheetWidth">Width of the sheet (must be POT, e.g. 2048).</param>
    /// <param name="sheetHeight">Height of the sheet (must be POT, e.g. 2048).</param>
    /// <param name="progress">Optional progress reporting interface.</param>
    public static AtlasGenerationResult Generate(
        SprDatabase db, 
        string sprPath, 
        int sheetWidth = 2048, 
        int sheetHeight = 2048, 
        IProgress<float>? progress = null)
    {
        if (sheetWidth <= 0 || (sheetWidth & (sheetWidth - 1)) != 0)
            throw new ArgumentException("Sheet width must be a power of two.", nameof(sheetWidth));
        if (sheetHeight <= 0 || (sheetHeight & (sheetHeight - 1)) != 0)
            throw new ArgumentException("Sheet height must be a power of two.", nameof(sheetHeight));

        int spritesPerRow = sheetWidth / SpriteSize;
        int spritesPerCol = sheetHeight / SpriteSize;
        int spritesPerSheet = spritesPerRow * spritesPerCol;

        if (spritesPerSheet == 0)
            throw new ArgumentException("Sheet size is too small to fit a single 32x32 sprite.");

        // Fast lookup array indexed by spriteId (1-based index)
        var mappings = new SpriteMapping[db.SpriteCount + 1];
        var sheets = new List<byte[]>();

        // Open file stream for sequential parsing
        using var fs = new FileStream(sprPath, FileMode.Open, FileAccess.Read, FileShare.Read, 65536, FileOptions.SequentialScan);
        using var br = new BinaryReader(fs);

        byte[] rleBuffer = new byte[4096];
        int currentSheetIndex = -1;
        int currentSpriteInSheetCount = 0;
        byte[] currentSheetData = null!;

        int rowBytes = sheetWidth * BytesPerPixel;
        int processedCount = 0;
        int totalNonEmpty = db.NonEmptyCount;

        for (uint id = 1; id <= db.SpriteCount; id++)
        {
            uint offset = db.Offsets[id];
            if (offset == 0) continue; // Skip empty sprites

            // Read RLE compressed data
            fs.Seek(offset + 3, SeekOrigin.Begin); // Skip colorkey (3 bytes)
            ushort size = br.ReadUInt16();
            if (size == 0) continue;

            if (rleBuffer.Length < size)
            {
                rleBuffer = new byte[size];
            }
            int read = br.Read(rleBuffer, 0, size);
            if (read < size) continue;

            // Decode RLE to 32x32 RGBA8 flat pixel buffer
            var sprite = SprParser.DecodeRle(rleBuffer.AsSpan(0, read));

            // Check if we need to allocate a new virtual sheet
            if (currentSheetIndex == -1 || currentSpriteInSheetCount >= spritesPerSheet)
            {
                currentSheetData = new byte[sheetWidth * sheetHeight * BytesPerPixel];
                sheets.Add(currentSheetData);
                currentSheetIndex++;
                currentSpriteInSheetCount = 0;
            }

            // Calculate grid position within the current sheet
            int col = currentSpriteInSheetCount % spritesPerRow;
            int row = currentSpriteInSheetCount / spritesPerRow;

            int destX = col * SpriteSize;
            int destY = row * SpriteSize;

            // Blit 32x32 sprite into the virtual sheet array
            ReadOnlySpan<byte> srcSpan = sprite.AsSpan();
            for (int y = 0; y < SpriteSize; y++)
            {
                int srcOffset = y * SpriteSize * BytesPerPixel;
                int destOffset = ((destY + y) * sheetWidth + destX) * BytesPerPixel;

                srcSpan.Slice(srcOffset, SpriteSize * BytesPerPixel)
                       .CopyTo(currentSheetData.AsSpan(destOffset));
            }

            // Calculate normalized UV coordinates
            float u0 = (float)destX / sheetWidth;
            float v0 = (float)destY / sheetHeight;
            float u1 = (float)(destX + SpriteSize) / sheetWidth;
            float v1 = (float)(destY + SpriteSize) / sheetHeight;

            mappings[id] = new SpriteMapping(currentSheetIndex, u0, v0, u1, v1);
            currentSpriteInSheetCount++;

            processedCount++;
            if (progress != null && (processedCount % 500) == 0)
            {
                progress.Report((float)processedCount / totalNonEmpty);
            }
        }

        if (progress != null) progress.Report(1.0f);

        return new AtlasGenerationResult(sheetWidth, sheetHeight, sheets, mappings);
    }
}
