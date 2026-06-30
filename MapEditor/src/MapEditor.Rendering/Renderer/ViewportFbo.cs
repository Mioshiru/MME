using System;
using Silk.NET.OpenGL;

namespace MapEditor.Rendering.Renderer;

/// <summary>
/// Encapsulates an OpenGL Framebuffer Object (FBO) with color and depth/stencil attachments.
/// Used to render the map viewport to a texture that can be displayed inside ImGui.
/// </summary>
public sealed class ViewportFbo : IDisposable
{
    private readonly GL _gl;
    private uint _fbo;
    private uint _colorTexture;
    private uint _rboDepthStencil;
    private int _width;
    private int _height;
    private bool _disposed;

    public ViewportFbo(GL gl)
    {
        _gl = gl;
    }

    public uint ColorTexture => _colorTexture;
    public int Width => _width;
    public int Height => _height;

    /// <summary>
    /// Resizes the FBO color texture and depth/stencil buffers if dimensions have changed.
    /// Must be called from the OpenGL render thread.
    /// </summary>
    public void Resize(int width, int height)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);

        // Clamp minimum size to 1x1
        width = Math.Max(1, width);
        height = Math.Max(1, height);

        if (_width == width && _height == height && _fbo != 0)
        {
            return;
        }

        _width = width;
        _height = height;

        // Clean up previous buffers
        CleanUp();

        // 1. Create Framebuffer
        _fbo = _gl.GenFramebuffer();
        _gl.BindFramebuffer(FramebufferTarget.Framebuffer, _fbo);

        // 2. Create Color Texture attachment
        _colorTexture = _gl.GenTexture();
        _gl.BindTexture(TextureTarget.Texture2D, _colorTexture);
        
        unsafe
        {
            _gl.TexImage2D(
                TextureTarget.Texture2D,
                0,
                InternalFormat.Rgba8,
                (uint)_width,
                (uint)_height,
                0,
                PixelFormat.Rgba,
                PixelType.UnsignedByte,
                null
            );
        }

        _gl.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.Linear);
        _gl.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Linear);
        _gl.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureWrapS, (int)TextureWrapMode.ClampToEdge);
        _gl.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureWrapT, (int)TextureWrapMode.ClampToEdge);

        // Attach texture to framebuffer
        _gl.FramebufferTexture2D(
            FramebufferTarget.Framebuffer,
            FramebufferAttachment.ColorAttachment0,
            TextureTarget.Texture2D,
            _colorTexture,
            0
        );

        // 3. Create Renderbuffer for Depth & Stencil attachments
        _rboDepthStencil = _gl.GenRenderbuffer();
        _gl.BindRenderbuffer(RenderbufferTarget.Renderbuffer, _rboDepthStencil);
        _gl.RenderbufferStorage(
            RenderbufferTarget.Renderbuffer,
            InternalFormat.Depth24Stencil8,
            (uint)_width,
            (uint)_height
        );

        // Attach renderbuffer to framebuffer
        _gl.FramebufferRenderbuffer(
            FramebufferTarget.Framebuffer,
            FramebufferAttachment.DepthStencilAttachment,
            RenderbufferTarget.Renderbuffer,
            _rboDepthStencil
        );

        // 4. Validate Framebuffer integrity
        var status = _gl.CheckFramebufferStatus(FramebufferTarget.Framebuffer);
        if ((int)status != 0x8CD5)
        {
            throw new Exception($"FBO creation failed: status {status}");
        }

        _gl.BindFramebuffer(FramebufferTarget.Framebuffer, 0);
    }

    /// <summary>
    /// Binds this Framebuffer for rendering. Subsequent draw calls will render onto the FBO texture.
    /// </summary>
    public void Bind()
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        _gl.BindFramebuffer(FramebufferTarget.Framebuffer, _fbo);
        _gl.Viewport(0, 0, (uint)_width, (uint)_height);
    }

    /// <summary>
    /// Unbinds this Framebuffer, returning rendering target back to the default window screen context.
    /// </summary>
    public void Unbind()
    {
        _gl.BindFramebuffer(FramebufferTarget.Framebuffer, 0);
    }

    private void CleanUp()
    {
        if (_fbo != 0)
        {
            _gl.DeleteFramebuffer(_fbo);
            _fbo = 0;
        }
        if (_colorTexture != 0)
        {
            _gl.DeleteTexture(_colorTexture);
            _colorTexture = 0;
        }
        if (_rboDepthStencil != 0)
        {
            _gl.DeleteRenderbuffer(_rboDepthStencil);
            _rboDepthStencil = 0;
        }
    }

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;
        CleanUp();
    }
}
