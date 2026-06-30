using Silk.NET.OpenGL;
using MapEditor.Assets.Spr;

namespace MapEditor.Assets.Atlas;

/// <summary>UV coordinates (normalised [0,1]) for a single sprite inside an atlas.</summary>
public readonly record struct SpriteUV(float U0, float V0, float U1, float V1);

/// <summary>
/// Packs 32×32 sprites into a single OpenGL RGBA8 texture atlas.
///
/// <para><b>Packing strategy:</b>
/// Sprites are arranged in a square grid. The atlas dimensions are the smallest
/// <b>power-of-two</b> value (e.g. 2048×2048, 4096×4096) that fits all sprites.
/// This maximises GPU cache efficiency and guarantees compatibility with all
/// OpenGL drivers including those requiring POT textures.</para>
///
/// <para>Call <see cref="Build"/> from the OpenGL thread; call <see cref="GetUV"/>
/// from any thread after the atlas is built.</para>
/// </summary>
public sealed class TextureAtlas : IDisposable
{
    private readonly GL _gl;
    private uint _textureHandle;
    private int  _atlasWidth;
    private int  _atlasHeight;
    private int  _spritesPerRow;
    private bool _built;
    private bool _disposed;

    // Maps 1-based sprite ID → atlas slot index (0-based)
    private readonly Dictionary<uint, int> _slotMap = new();

    public TextureAtlas(GL gl) => _gl = gl;

    /// <summary>OpenGL texture handle. Valid only after <see cref="Build"/>.</summary>
    public uint Handle => _textureHandle;

    /// <summary>True after <see cref="Build"/> completes successfully.</summary>
    public bool IsBuilt => _built;

    /// <summary>Atlas width in pixels (power-of-two).</summary>
    public int AtlasWidth => _atlasWidth;

    /// <summary>Atlas height in pixels (power-of-two).</summary>
    public int AtlasHeight => _atlasHeight;

    /// <summary>Total number of sprite slots packed into this atlas.</summary>
    public int SpriteCount => _slotMap.Count;

    // ── Build ─────────────────────────────────────────────────────────────────

    /// <summary>
    /// Packs the given decoded sprites into a GPU texture atlas.
    /// <para>Must be called from the OpenGL thread.</para>
    /// </summary>
    /// <param name="sprites">Decoded sprites keyed by 1-based sprite ID.</param>
    public unsafe void Build(IReadOnlyDictionary<uint, SpriteData> sprites)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        if (_built) Destroy();
        if (sprites.Count == 0) return;

        // ── Compute power-of-two atlas dimensions ─────────────────────────────
        // We want the smallest POT side length that fits all sprites in a square grid.
        // spritesPerRow = ceil(sqrt(count)); atlasSize = NextPow2(spritesPerRow × 32)
        _spritesPerRow = (int)Math.Ceiling(Math.Sqrt(sprites.Count));
        int minSide = _spritesPerRow * SpriteData.Width;
        _atlasWidth  = NextPow2(minSide);
        _atlasHeight = _atlasWidth; // keep it square / power-of-two

        // Safety guard: if sprites spill into more rows than fit, double height
        int rowsNeeded = (int)Math.Ceiling((double)sprites.Count / _spritesPerRow);
        if (rowsNeeded * SpriteData.Height > _atlasHeight)
            _atlasHeight = NextPow2(rowsNeeded * SpriteData.Height);

        int rowBytes  = _atlasWidth * SpriteData.BytesPerPixel;
        var atlasPixels = new byte[_atlasWidth * _atlasHeight * SpriteData.BytesPerPixel];

        // ── Blit each sprite into the atlas buffer ────────────────────────────
        int slot = 0;
        foreach (var (id, sprite) in sprites)
        {
            int col  = slot % _spritesPerRow;
            int row  = slot / _spritesPerRow;
            int destX = col * SpriteData.Width;
            int destY = row * SpriteData.Height;

            ReadOnlySpan<byte> src = sprite.AsSpan();
            for (int y = 0; y < SpriteData.Height; y++)
            {
                int srcOffset  = y * SpriteData.Width * SpriteData.BytesPerPixel;
                int destOffset = (destY + y) * rowBytes + destX * SpriteData.BytesPerPixel;
                src.Slice(srcOffset, SpriteData.Width * SpriteData.BytesPerPixel)
                   .CopyTo(atlasPixels.AsSpan(destOffset));
            }

            _slotMap[id] = slot;
            slot++;
        }

        // ── Upload to GPU ─────────────────────────────────────────────────────
        _textureHandle = _gl.GenTexture();
        _gl.BindTexture(TextureTarget.Texture2D, _textureHandle);

        fixed (byte* pPix = atlasPixels)
        {
            _gl.TexImage2D(TextureTarget.Texture2D, 0, InternalFormat.Rgba8,
                (uint)_atlasWidth, (uint)_atlasHeight, 0,
                PixelFormat.Rgba, PixelType.UnsignedByte, pPix);
        }

        _gl.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.Nearest);
        _gl.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Nearest);
        _gl.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureWrapS, (int)TextureWrapMode.ClampToEdge);
        _gl.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureWrapT, (int)TextureWrapMode.ClampToEdge);
        _gl.GenerateMipmap(TextureTarget.Texture2D);

        _gl.BindTexture(TextureTarget.Texture2D, 0);
        _built = true;
    }

    // ── UV lookup ────────────────────────────────────────────────────────────

    /// <summary>
    /// Returns the normalised UV rectangle for the given 1-based sprite ID,
    /// or <c>null</c> if that sprite is not in this atlas.
    /// </summary>
    public SpriteUV? GetUV(uint spriteId)
    {
        if (!_slotMap.TryGetValue(spriteId, out int slot)) return null;

        float cellW = (float)SpriteData.Width  / _atlasWidth;
        float cellH = (float)SpriteData.Height / _atlasHeight;
        int col     = slot % _spritesPerRow;
        int row     = slot / _spritesPerRow;

        return new SpriteUV(col * cellW, row * cellH,
                            (col + 1) * cellW, (row + 1) * cellH);
    }

    // ── Cleanup ───────────────────────────────────────────────────────────────

    private void Destroy()
    {
        if (_textureHandle != 0)
        {
            _gl.DeleteTexture(_textureHandle);
            _textureHandle = 0;
        }
        _slotMap.Clear();
        _built = false;
    }

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;
        Destroy();
    }

    // ── Helpers ───────────────────────────────────────────────────────────────

    /// <summary>Returns the smallest power of two ≥ <paramref name="v"/>.</summary>
    public static int NextPow2(int v)
    {
        if (v <= 0) return 1;
        v--;
        v |= v >> 1; v |= v >> 2; v |= v >> 4; v |= v >> 8; v |= v >> 16;
        return v + 1;
    }
}
