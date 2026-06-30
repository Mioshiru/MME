using System;
using System.Numerics;
using Silk.NET.OpenGL;
using MapEditor.Assets.Atlas;

namespace MapEditor.Rendering.Textures;

/// <summary>
/// OpenGL texture lookup containing the slice layer and the UV bounds.
/// </summary>
public readonly struct SpriteTextureLookup
{
    public readonly int SliceIndex;
    public readonly float U0;
    public readonly float V0;
    public readonly float U1;
    public readonly float V1;

    public SpriteTextureLookup(int sliceIndex, float u0, float v0, float u1, float v1)
    {
        SliceIndex = sliceIndex;
        U0 = u0;
        V0 = v0;
        U1 = u1;
        V1 = v1;
    }
}

/// <summary>
/// Manages uploading texture atlas sheets to the GPU using OpenGL 2D Texture Arrays.
///
/// Texture arrays allow the shader to bind a single texture handle and select sheets
/// via a slice index coordinate, enabling a single draw call to render tiles from any atlas.
/// </summary>
public sealed class TextureManager : IDisposable
{
    private readonly GL _gl;
    private uint _textureArrayHandle;
    private bool _uploaded;
    private bool _disposed;

    // Sprite mappings copied from the generator result
    private SpriteMapping[] _mappings = Array.Empty<SpriteMapping>();

    public TextureManager(GL gl)
    {
        _gl = gl;
    }

    /// <summary>
    /// Gets the OpenGL texture handle for the 2D Texture Array.
    /// </summary>
    public uint TextureArrayHandle => _textureArrayHandle;

    /// <summary>
    /// True if the textures have been successfully uploaded to the GPU.
    /// </summary>
    public bool IsUploaded => _uploaded;

    /// <summary>
    /// Uploads virtual atlas sheets to the GPU as a 2D Texture Array.
    /// Must be called from the main thread containing the active OpenGL context.
    /// </summary>
    public unsafe void UploadAtlases(AtlasGenerationResult result)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);

        if (_uploaded)
        {
            FreeTextures();
        }

        if (result.Sheets.Count == 0)
        {
            return;
        }

        _mappings = result.Mappings;

        // 1. Generate OpenGL texture array handle
        _textureArrayHandle = _gl.GenTexture();
        _gl.BindTexture(TextureTarget.Texture2DArray, _textureArrayHandle);

        // 2. Allocate storage for the entire texture array
        // glTexImage3D parameters: target, level, internalformat, width, height, depth (slices), border, format, type, data
        _gl.TexImage3D(
            TextureTarget.Texture2DArray,
            0,
            InternalFormat.Rgba8,
            (uint)result.AtlasWidth,
            (uint)result.AtlasHeight,
            (uint)result.Sheets.Count,
            0,
            PixelFormat.Rgba,
            PixelType.UnsignedByte,
            null
        );

        // 3. Upload each virtual sheet slice-by-slice
        for (int i = 0; i < result.Sheets.Count; i++)
        {
            byte[] sheetData = result.Sheets[i];
            fixed (byte* ptr = sheetData)
            {
                // glTexSubImage3D parameters: target, level, xoffset, yoffset, zoffset (slice index), width, height, depth (1 slice), format, type, data
                _gl.TexSubImage3D(
                    TextureTarget.Texture2DArray,
                    0,
                    0,
                    0,
                    i, // zoffset is the slice index in the array
                    (uint)result.AtlasWidth,
                    (uint)result.AtlasHeight,
                    1, // depth is 1 slice
                    PixelFormat.Rgba,
                    PixelType.UnsignedByte,
                    ptr
                );
            }
        }

        // 4. Set texture parameters (nearest filtering for crisp pixel art)
        _gl.TexParameter(TextureTarget.Texture2DArray, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.NearestMipmapLinear);
        _gl.TexParameter(TextureTarget.Texture2DArray, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Nearest);
        _gl.TexParameter(TextureTarget.Texture2DArray, TextureParameterName.TextureWrapS, (int)TextureWrapMode.ClampToEdge);
        _gl.TexParameter(TextureTarget.Texture2DArray, TextureParameterName.TextureWrapT, (int)TextureWrapMode.ClampToEdge);
        
        // Generate mipmaps for all slices
        _gl.GenerateMipmap(TextureTarget.Texture2DArray);

        _gl.BindTexture(TextureTarget.Texture2DArray, 0);
        _uploaded = true;
    }

    /// <summary>
    /// Looks up the UV coords and slice index for a given sprite ID.
    /// Returns false if the sprite has no mapping in the loaded atlases.
    /// </summary>
    public bool TryLookupSprite(uint spriteId, out SpriteTextureLookup lookup)
    {
        if (spriteId >= _mappings.Length)
        {
            lookup = default;
            return false;
        }

        var mapping = _mappings[spriteId];
        if (mapping.AtlasIndex == -1) // Unmapped/empty sprite
        {
            lookup = default;
            return false;
        }

        lookup = new SpriteTextureLookup(mapping.AtlasIndex, mapping.U0, mapping.V0, mapping.U1, mapping.V1);
        return true;
    }

    private void FreeTextures()
    {
        if (_textureArrayHandle != 0)
        {
            _gl.DeleteTexture(_textureArrayHandle);
            _textureArrayHandle = 0;
        }
        _uploaded = false;
        _mappings = Array.Empty<SpriteMapping>();
    }

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;
        FreeTextures();
    }
}
