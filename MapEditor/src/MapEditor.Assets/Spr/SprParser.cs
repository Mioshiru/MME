using System.IO.MemoryMappedFiles;
using System.Runtime.InteropServices;

namespace MapEditor.Assets.Spr;

// ────────────────────────────────────────────────────────────────────────────
// Public data types
// ────────────────────────────────────────────────────────────────────────────

/// <summary>
/// Decoded pixel data for a single 32×32 sprite.
/// Pixels are stored in row-major RGBA8 order (128 bytes wide, 32 rows = 4096 bytes total).
/// Alpha=0 means fully transparent.
/// </summary>
public sealed class SpriteData
{
    public const int Width        = 32;
    public const int Height       = 32;
    public const int BytesPerPixel = 4; // RGBA8
    public const int TotalBytes   = Width * Height * BytesPerPixel; // 4096

    /// <summary>RGBA8 flat pixel buffer, never null.</summary>
    public readonly byte[] Pixels = new byte[TotalBytes];

    public ReadOnlySpan<byte> AsSpan() => Pixels;
}

/// <summary>
/// Holds the parsed sprite count and per-sprite file offsets
/// (raw compressed RLE data is read on demand via <see cref="SprParser.DecodeSprite"/>).
/// </summary>
public sealed class SprDatabase
{
    /// <summary>4-byte file signature read from offset 0.</summary>
    public uint Signature { get; }

    /// <summary>Total number of sprite slots (many are empty / offset = 0).</summary>
    public uint SpriteCount { get; }

    // Internal: file-offset table, 0 = empty slot (1-based, slot[0] unused).
    internal readonly uint[] Offsets;

    internal SprDatabase(uint signature, uint count, uint[] offsets)
    {
        Signature   = signature;
        SpriteCount = count;
        Offsets     = offsets;
    }

    /// <summary>Returns true if the given 1-based sprite ID has data.</summary>
    public bool HasSprite(uint id) => id >= 1 && id <= SpriteCount && Offsets[id] != 0;

    /// <summary>Number of non-empty sprite slots.</summary>
    public int NonEmptyCount
    {
        get
        {
            int n = 0;
            for (uint i = 1; i <= SpriteCount; i++)
                if (Offsets[i] != 0) n++;
            return n;
        }
    }
}

// ────────────────────────────────────────────────────────────────────────────
// Parser
// ────────────────────────────────────────────────────────────────────────────

/// <summary>
/// High-performance binary parser for Tibia's .spr sprite file.
///
/// <para>Supported format: <b>Tibia 9.60–10.x "extended"</b> (sprite count as uint32,
/// sprite IDs in .dat as uint32). Specifically validated against client 10.98.</para>
///
/// <para>File layout:</para>
/// <code>
///   [0x00] uint32  – file signature (e.g. 0x57BBD603 for 10.98)
///   [0x04] uint32  – total sprite count N
///   [0x08] uint32[N] – per-sprite file offsets (0 = empty/blank)
///   [offset+0] byte[3] – colorkey header (always 0xFF 0x00 0xFF, skipped)
///   [offset+3] uint16  – compressed RLE size in bytes
///   [offset+5] byte[size] – RLE pixel data (see below)
/// </code>
///
/// <para>RLE pixel encoding (pairs until <c>size</c> bytes consumed):</para>
/// <code>
///   uint16 transparentCount  – N fully transparent (RGBA=0) pixels
///   uint16 coloredCount      – N opaque RGB pixels (3 bytes each)
/// </code>
///
/// <para>Usage — parse the offset table once, then decode sprites on demand:</para>
/// <code>
///   var db  = SprParser.ParseOffsetTable("Tibia.spr");
///   var spr = SprParser.DecodeSprite(db, "Tibia.spr", spriteId);
/// </code>
/// </summary>
public static class SprParser
{
    // 4-byte sig + 4-byte count + count×4-byte offsets
    private const int SignatureBytes    = 4;
    private const int CountBytes        = 4;
    private const int OffsetEntryBytes  = 4;
    private const int ColorKeyBytes     = 3;

    // ── Step 1: parse the offset table (fast, reads only the header) ─────────

    /// <summary>
    /// Reads the file header and builds the offset table.
    /// Memory-mapped for maximum efficiency; does not decode any pixel data.
    /// </summary>
    /// <param name="path">Absolute path to the .spr file.</param>
    public static SprDatabase ParseOffsetTable(string path)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(path);
        if (!File.Exists(path))
            throw new FileNotFoundException(".spr file not found.", path);

        long fileSize = new FileInfo(path).Length;

        using var mmf = MemoryMappedFile.CreateFromFile(
            path, FileMode.Open, null, 0, MemoryMappedFileAccess.Read);

        // Header region: sig(4) + count(4) = 8 bytes
        using var headerView = mmf.CreateViewAccessor(0, 8, MemoryMappedFileAccess.Read);
        uint signature   = headerView.ReadUInt32(0);
        uint spriteCount = headerView.ReadUInt32(4);

        if (spriteCount == 0)
            return new SprDatabase(signature, 0, Array.Empty<uint>());

        // Validate table fits in file
        long tableBytes = (long)spriteCount * OffsetEntryBytes;
        long tableStart = SignatureBytes + CountBytes;
        if (tableStart + tableBytes > fileSize)
            throw new InvalidDataException(
                $".spr offset table extends beyond file size (count={spriteCount}).");

        // Read the entire offset table in one shot using safe ReadArray
        var offsets = new uint[spriteCount + 1]; // 1-based; offsets[0] unused
        using var tableView = mmf.CreateViewAccessor(
            tableStart, tableBytes, MemoryMappedFileAccess.Read);

        tableView.ReadArray(0, offsets, 1, (int)spriteCount);

        return new SprDatabase(signature, spriteCount, offsets);
    }

    // ── Step 2: on-demand sprite decode ──────────────────────────────────────

    /// <summary>
    /// Decodes a single sprite's RLE pixel data into a 32×32 RGBA8 buffer.
    /// Returns <c>null</c> for empty/blank sprite slots (offset = 0).
    /// </summary>
    /// <param name="db">Database returned by <see cref="ParseOffsetTable"/>.</param>
    /// <param name="path">Same .spr file path used to build <paramref name="db"/>.</param>
    /// <param name="spriteId">1-based sprite ID.</param>
    public static SpriteData? DecodeSprite(SprDatabase db, string path, uint spriteId)
    {
        if (spriteId < 1 || spriteId > db.SpriteCount)
            throw new ArgumentOutOfRangeException(nameof(spriteId));

        uint fileOffset = db.Offsets[spriteId];
        if (fileOffset == 0) return null; // blank sprite

        using var fs = new FileStream(path, FileMode.Open, FileAccess.Read,
            FileShare.Read, bufferSize: 256, FileOptions.RandomAccess);
        using var br = new BinaryReader(fs);

        // Skip 3-byte colorkey header; read compressed size
        fs.Seek(fileOffset + ColorKeyBytes, SeekOrigin.Begin);
        ushort compressedSize = br.ReadUInt16();
        if (compressedSize == 0) return null;

        // Read the compressed RLE block into a pooled/stack buffer
        byte[] rleData = new byte[compressedSize];
        int bytesRead = br.Read(rleData, 0, compressedSize);
        if (bytesRead != compressedSize)
            throw new EndOfStreamException(
                $"Truncated sprite data at offset {fileOffset} (id={spriteId}).");

        return DecodeRle(rleData.AsSpan(0, bytesRead));
    }

    /// <summary>
    /// Batch-decodes all non-empty sprites and returns a populated dictionary.
    /// Uses sequential reads for cache efficiency.
    /// </summary>
    public static Dictionary<uint, SpriteData> DecodeAll(SprDatabase db, string path,
        IProgress<float>? progress = null)
    {
        var result = new Dictionary<uint, SpriteData>((int)db.SpriteCount / 2);

        using var fs = new FileStream(path, FileMode.Open, FileAccess.Read,
            FileShare.Read, bufferSize: 65536, FileOptions.SequentialScan);
        using var br = new BinaryReader(fs);

        byte[] rleBuffer = new byte[4096];
        uint processed = 0;

        for (uint id = 1; id <= db.SpriteCount; id++)
        {
            uint offset = db.Offsets[id];
            if (offset == 0) continue;

            fs.Seek(offset + ColorKeyBytes, SeekOrigin.Begin);
            ushort size = br.ReadUInt16();
            if (size == 0) continue;

            if (rleBuffer.Length < size) rleBuffer = new byte[size];
            int read = br.Read(rleBuffer, 0, size);
            if (read < size) continue; // truncated

            result[id] = DecodeRle(rleBuffer.AsSpan(0, read));

            processed++;
            if (progress is not null && (processed % 1000) == 0)
                progress.Report((float)id / db.SpriteCount);
        }

        return result;
    }

    // ── RLE decoder ──────────────────────────────────────────────────────────

    /// <summary>
    /// Decodes one sprite's compressed RLE block into a <see cref="SpriteData"/>.
    ///
    /// <para>Encoding: pairs of (uint16 transparent, uint16 colored); each colored
    /// pixel is 3 bytes RGB with implicit alpha=255.</para>
    /// </summary>
    internal static SpriteData DecodeRle(ReadOnlySpan<byte> rle)
    {
        var sprite = new SpriteData();
        Span<byte> pixels = sprite.Pixels;

        int readPos  = 0;
        int writePos = 0; // byte offset into pixels (each pixel = 4 bytes)
        const int MaxPixelBytes = SpriteData.TotalBytes;

        while (readPos < rle.Length && writePos < MaxPixelBytes)
        {
            // Guard: need at least 4 bytes for a (transparent, colored) pair header
            if (readPos + 4 > rle.Length) break;

            int transparent = rle[readPos] | (rle[readPos + 1] << 8);
            readPos += 2;
            int colored     = rle[readPos] | (rle[readPos + 1] << 8);
            readPos += 2;

            // Write transparent pixels (RGBA = 0,0,0,0)
            int transparentBytes = transparent * SpriteData.BytesPerPixel;
            if (writePos + transparentBytes > MaxPixelBytes)
                transparentBytes = MaxPixelBytes - writePos;
            pixels.Slice(writePos, transparentBytes).Clear();
            writePos += transparentBytes;

            // Write colored pixels (RGB from stream, A = 255)
            for (int i = 0; i < colored && writePos < MaxPixelBytes; i++)
            {
                if (readPos + 3 > rle.Length) goto done;

                pixels[writePos + 0] = rle[readPos + 0]; // R
                pixels[writePos + 1] = rle[readPos + 1]; // G
                pixels[writePos + 2] = rle[readPos + 2]; // B
                pixels[writePos + 3] = 0xFF;              // A (fully opaque)

                readPos  += 3;
                writePos += 4;
            }

            // Both counts zero = RLE terminator (defensive)
            if (transparent == 0 && colored == 0) break;
        }

        done:
        // Remaining pixels are already zero-initialised by the array ctor
        return sprite;
    }
}
