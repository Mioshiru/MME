using System;
using System.IO;
using Silk.NET.OpenGL;
using StbImageSharp;

namespace MapEditor.Rendering.Textures;

/// <summary>
/// Utility class to decode PNG icon files into OpenGL textures using StbImageSharp.
/// </summary>
public static class IconLoader
{
    /// <summary>
    /// Decodes a PNG image and uploads it to OpenGL, returning the texture ID.
    /// </summary>
    public static unsafe uint LoadIconTexture(GL gl, string path)
    {
        if (!File.Exists(path))
        {
            Console.WriteLine($"[IconLoader] Icon file not found: {path}");
            return 0;
        }

        try
        {
            // Reset to prevent top-to-bottom flipping if required, or keep it standard
            StbImage.stbi_set_flip_vertically_on_load(1);

            using var stream = File.OpenRead(path);
            var image = ImageResult.FromStream(stream, ColorComponents.RedGreenBlueAlpha);

            uint handle = gl.GenTexture();
            gl.BindTexture(TextureTarget.Texture2D, handle);

            fixed (byte* ptr = image.Data)
            {
                gl.TexImage2D(
                    TextureTarget.Texture2D,
                    0,
                    InternalFormat.Rgba8,
                    (uint)image.Width,
                    (uint)image.Height,
                    0,
                    PixelFormat.Rgba,
                    PixelType.UnsignedByte,
                    ptr
                );
            }

            // Set simple nearest filtering for small icons
            gl.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.Linear);
            gl.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Linear);
            gl.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureWrapS, (int)TextureWrapMode.ClampToEdge);
            gl.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureWrapT, (int)TextureWrapMode.ClampToEdge);

            gl.BindTexture(TextureTarget.Texture2D, 0);

            return handle;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[IconLoader] Failed loading icon at {path}: {ex.Message}");
            return 0;
        }
    }
}
