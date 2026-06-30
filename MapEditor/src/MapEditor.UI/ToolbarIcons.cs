using System;
using System.IO;
using Silk.NET.OpenGL;
using MapEditor.Rendering.Textures;

namespace MapEditor.UI;

/// <summary>
/// Container holding OpenGL texture handles for all toolbar button icons.
/// </summary>
public sealed class ToolbarIcons : IDisposable
{
    private readonly GL _gl;
    private bool _disposed;

    public uint DrawIcon { get; private set; }
    public uint EraseIcon { get; private set; }
    public uint SelectIcon { get; private set; }
    public uint FillIcon { get; private set; }
    public uint SpawnIcon { get; private set; }
    public uint WaypointIcon { get; private set; }

    public ToolbarIcons(GL gl)
    {
        _gl = gl;
    }

    /// <summary>
    /// Loads all toolbar PNG icons from the specified icons folder.
    /// </summary>
    public void Load(string iconsDir)
    {
        if (!Directory.Exists(iconsDir))
        {
            Console.WriteLine($"[ToolbarIcons] Icons directory not found: {iconsDir}");
            return;
        }

        DrawIcon = IconLoader.LoadIconTexture(_gl, Path.Combine(iconsDir, "rectangular_1.png"));
        EraseIcon = IconLoader.LoadIconTexture(_gl, Path.Combine(iconsDir, "rectangular_7.png"));
        SelectIcon = IconLoader.LoadIconTexture(_gl, Path.Combine(iconsDir, "position_go.png"));
        FillIcon = IconLoader.LoadIconTexture(_gl, Path.Combine(iconsDir, "circular_1.png"));
        SpawnIcon = IconLoader.LoadIconTexture(_gl, Path.Combine(iconsDir, "circular_3.png"));
        WaypointIcon = IconLoader.LoadIconTexture(_gl, Path.Combine(iconsDir, "circular_5.png"));
    }

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;

        DeleteTexture(DrawIcon);
        DeleteTexture(EraseIcon);
        DeleteTexture(SelectIcon);
        DeleteTexture(FillIcon);
        DeleteTexture(SpawnIcon);
        DeleteTexture(WaypointIcon);
    }

    private void DeleteTexture(uint handle)
    {
        if (handle != 0)
        {
            _gl.DeleteTexture(handle);
        }
    }
}
