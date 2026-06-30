using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Numerics;
using System.Reflection;
using System.Runtime.InteropServices;
using Silk.NET.OpenGL;
using MapEditor.Core.Map;
using MapEditor.Core.State;
using MapEditor.Assets.Dat;
using MapEditor.Rendering.Textures;
using MapEditor.Rendering;

namespace MapEditor.Rendering.Renderer;

/// <summary>
/// Per-instance data uploaded to the GPU for each visible tile.
/// Must match the layout declared in tile.vert.
/// </summary>
[StructLayout(LayoutKind.Sequential, Pack = 1)]
internal struct TileInstance
{
    public Vector2 GridPos;    // location 2 — tile grid position (x, y)
    public Vector4 UV;         // location 3 — atlas UV rect (u0, v0, u1, v1)
    public float   SliceIndex; // location 4 — texture array slice index
    public float   Depth;      // location 5 — z-order depth
}

/// <summary>
/// Instanced OpenGL renderer for the tile map.
///
/// Architecture:
///   • One VAO + static quad VBO (6 vertices for a unit quad).
///   • One dynamic instance VBO updated each frame with visible tiles.
///   • One <c>glDrawElementsInstanced</c> call per floor layer.
///   • Viewport-culling system: only tiles within the camera frustum are added to the instance buffer.
///   • Utilizes OpenGL 2D Texture Arrays to bind all sheets at once and render in a single batch.
/// </summary>
public sealed class MapRenderer : IDisposable
{
    // ── GL handles ────────────────────────────────────────────────────────────
    private readonly GL _gl;
    private uint _vao;
    private uint _quadVbo;       // static quad vertices
    private uint _instanceVbo;   // dynamic per-tile instance data
    private uint _shaderProgram;

    // ── Uniform locations ────────────────────────────────────────────────────
    private int _uProjection;
    private int _uTileSize;
    private int _uAtlasArray;
    private int _uAlpha;

    // ── Instance buffer (CPU side) ────────────────────────────────────────────
    private TileInstance[] _instances = new TileInstance[65536];

    public const float TileSize = 32f;

    private bool _disposed;

    public MapRenderer(GL gl)
    {
        _gl = gl;
        Initialize();
    }

    // ── Initialisation ────────────────────────────────────────────────────────

    private void Initialize()
    {
        CreateShaders();
        CreateGeometry();
    }

    private void CreateShaders()
    {
        // Try to load shaders from the data/shaders directory at runtime,
        // with fallback to embedded resources.
        string vertPath = Path.Combine(AppContext.BaseDirectory, "data", "shaders", "tile.vert");
        string fragPath = Path.Combine(AppContext.BaseDirectory, "data", "shaders", "tile.frag");

        if (!File.Exists(vertPath)) vertPath = "data/shaders/tile.vert";
        if (!File.Exists(fragPath)) fragPath = "data/shaders/tile.frag";

        string vertSrc = File.Exists(vertPath) ? File.ReadAllText(vertPath) : LoadEmbeddedShader("tile.vert");
        string fragSrc = File.Exists(fragPath) ? File.ReadAllText(fragPath) : LoadEmbeddedShader("tile.frag");

        uint vert = CompileShader(ShaderType.VertexShader,   vertSrc);
        uint frag = CompileShader(ShaderType.FragmentShader, fragSrc);

        _shaderProgram = _gl.CreateProgram();
        _gl.AttachShader(_shaderProgram, vert);
        _gl.AttachShader(_shaderProgram, frag);
        _gl.LinkProgram(_shaderProgram);

        _gl.GetProgram(_shaderProgram, ProgramPropertyARB.LinkStatus, out int linked);
        if (linked == 0)
        {
            string log = _gl.GetProgramInfoLog(_shaderProgram);
            throw new Exception($"Shader link failed: {log}");
        }

        _gl.DeleteShader(vert);
        _gl.DeleteShader(frag);

        _uProjection = _gl.GetUniformLocation(_shaderProgram, "uProjection");
        _uTileSize   = _gl.GetUniformLocation(_shaderProgram, "uTileSize");
        _uAtlasArray = _gl.GetUniformLocation(_shaderProgram, "uAtlasArray");
        _uAlpha      = _gl.GetUniformLocation(_shaderProgram, "uAlpha");
    }

    private unsafe void CreateGeometry()
    {
        // Unit quad: two triangles covering [0,1]×[0,1]
        // layout: vec2 pos, vec2 uv
        float[] quadVerts =
        {
            // pos         uv
            0f, 0f,        0f, 0f,
            1f, 0f,        1f, 0f,
            1f, 1f,        1f, 1f,
            0f, 0f,        0f, 0f,
            1f, 1f,        1f, 1f,
            0f, 1f,        0f, 1f,
        };

        _vao = _gl.GenVertexArray();
        _gl.BindVertexArray(_vao);

        // Quad VBO (static)
        _quadVbo = _gl.GenBuffer();
        _gl.BindBuffer(BufferTargetARB.ArrayBuffer, _quadVbo);
        fixed (float* pVerts = quadVerts)
            _gl.BufferData(BufferTargetARB.ArrayBuffer,
                (nuint)(quadVerts.Length * sizeof(float)), pVerts, BufferUsageARB.StaticDraw);

        uint stride = (uint)(4 * sizeof(float));
        _gl.VertexAttribPointer(0, 2, VertexAttribPointerType.Float, false, stride, (void*)0);
        _gl.EnableVertexAttribArray(0);
        _gl.VertexAttribPointer(1, 2, VertexAttribPointerType.Float, false, stride, (void*)(2 * sizeof(float)));
        _gl.EnableVertexAttribArray(1);

        // Instance VBO (dynamic)
        _instanceVbo = _gl.GenBuffer();
        _gl.BindBuffer(BufferTargetARB.ArrayBuffer, _instanceVbo);
        _gl.BufferData(BufferTargetARB.ArrayBuffer,
            (nuint)(_instances.Length * sizeof(TileInstance)), null, BufferUsageARB.DynamicDraw);

        uint instStride = (uint)sizeof(TileInstance);
        int baseOffset = 0;

        // location 2: GridPos (vec2)
        _gl.VertexAttribPointer(2, 2, VertexAttribPointerType.Float, false, instStride, (void*)baseOffset);
        _gl.EnableVertexAttribArray(2);
        _gl.VertexAttribDivisor(2, 1);
        baseOffset += 2 * sizeof(float);

        // location 3: UV (vec4)
        _gl.VertexAttribPointer(3, 4, VertexAttribPointerType.Float, false, instStride, (void*)baseOffset);
        _gl.EnableVertexAttribArray(3);
        _gl.VertexAttribDivisor(3, 1);
        baseOffset += 4 * sizeof(float);

        // location 4: SliceIndex (float)
        _gl.VertexAttribPointer(4, 1, VertexAttribPointerType.Float, false, instStride, (void*)baseOffset);
        _gl.EnableVertexAttribArray(4);
        _gl.VertexAttribDivisor(4, 1);
        baseOffset += 1 * sizeof(float);

        // location 5: Depth (float)
        _gl.VertexAttribPointer(5, 1, VertexAttribPointerType.Float, false, instStride, (void*)baseOffset);
        _gl.EnableVertexAttribArray(5);
        _gl.VertexAttribDivisor(5, 1);

        _gl.BindVertexArray(0);
    }

    // ── Render ────────────────────────────────────────────────────────────────

    public bool IsDirty { get; set; } = true;
    private ushort _lastSelectedItemId;

    /// <summary>
    /// Renders all visible tiles for the given state, texture manager, camera, and item database.
    /// Call once per frame from the main render loop.
    /// </summary>
    public unsafe void Render(EditorState state, TextureManager textureManager, Camera camera, DatDatabase datDb)
    {
        if (!textureManager.IsUploaded || state.ActiveMap is null) return;

        // Explicit Dirty-Flag check: force canvas redraw / cache update on TerrainPalette selection change
        if (state.SelectedItemId != _lastSelectedItemId)
        {
            _lastSelectedItemId = state.SelectedItemId;
            IsDirty = true;
        }

        if (IsDirty)
        {
            // Force redraw/re-sync
            IsDirty = false;
        }

        _gl.UseProgram(_shaderProgram);

        // Upload orthographic projection matrix from camera
        var proj = camera.GetProjectionMatrix();
        _gl.UniformMatrix4(_uProjection, 1, false, (float*)&proj);
        _gl.Uniform2(_uTileSize, TileSize, TileSize);
        _gl.Uniform1(_uAtlasArray, 0);

        _gl.ActiveTexture(TextureUnit.Texture0);
        _gl.BindTexture(TextureTarget.Texture2DArray, textureManager.TextureArrayHandle);

        _gl.Enable(EnableCap.Blend);
        _gl.BlendFunc(BlendingFactor.SrcAlpha, BlendingFactor.OneMinusSrcAlpha);

        // Draw active floor at full alpha, adjacent floors faded
        int activeFloor = state.ActiveFloor;
        for (int fl = MapDocument.MaxFloor; fl >= MapDocument.MinFloor; fl--)
        {
            var layer = state.ActiveMap.GetLayer(fl);
            
            // Build the instanced buffer for the current floor
            RenderVisibleMap(layer, textureManager, camera, datDb, out int count);
            if (count == 0) continue;

            float alpha = fl == activeFloor ? 1.0f : 0.4f;
            _gl.Uniform1(_uAlpha, alpha);

            UploadAndDraw(count);
        }

        _gl.BindVertexArray(0);
        _gl.UseProgram(0);
    }

    /// <summary>
    /// Computes and packs the instance rendering attributes for all visible tiles inside the current viewport.
    /// </summary>
    public void RenderVisibleMap(
        MapLayer layer, 
        TextureManager textureManager, 
        Camera camera, 
        DatDatabase datDb, 
        out int count)
    {
        count = 0;

        // Perform frustum culling using camera visible bounds
        camera.GetVisibleBounds(out int minX, out int minY, out int maxX, out int maxY);

        // Pre-allocated local lists to group items by their layering category
        var grounds = new List<TileInstance>(1024);
        var bottoms = new List<TileInstance>(1024);
        var commons = new List<TileInstance>(2048);
        var tops    = new List<TileInstance>(512);

        // Traverse in vertical-first order (rows then columns) to ensure natural back-to-front layering
        for (int y = minY; y <= maxY; y++)
        {
            for (int x = minX; x <= maxX; x++)
            {
                var tile = layer.GetTile(x, y);
                if (tile == null || tile.IsEmpty) continue;

                // 1. Process Ground/Terrain
                if (tile.GroundId != 0)
                {
                    var itemInfo = datDb.GetItem(tile.GroundId);
                    if (itemInfo != null)
                    {
                        AddObjectInstances(grounds, itemInfo, x, y, textureManager);
                    }
                }

                // 2. Process Stacked items (creatures, borders, walls, items)
                foreach (ushort itemId in tile.Items)
                {
                    var itemInfo = datDb.GetItem(itemId);
                    if (itemInfo == null) continue;

                    if (itemInfo.HasFlag(DatFlags.OnBottom))
                    {
                        AddObjectInstances(bottoms, itemInfo, x, y, textureManager);
                    }
                    else if (itemInfo.HasFlag(DatFlags.OnTop))
                    {
                        AddObjectInstances(tops, itemInfo, x, y, textureManager);
                    }
                    else
                    {
                        AddObjectInstances(commons, itemInfo, x, y, textureManager);
                    }
                }
            }
        }

        // Copy batches sequentially to instance buffer to enforce painter's algorithm
        int totalCount = grounds.Count + bottoms.Count + commons.Count + tops.Count;
        if (totalCount > _instances.Length)
        {
            int newSize = _instances.Length;
            while (newSize < totalCount) newSize *= 2;
            Array.Resize(ref _instances, newSize);
        }

        foreach (var inst in grounds) _instances[count++] = inst;
        foreach (var inst in bottoms) _instances[count++] = inst;
        foreach (var inst in commons) _instances[count++] = inst;
        foreach (var inst in tops)    _instances[count++] = inst;
    }

    private void AddObjectInstances(
        List<TileInstance> list, 
        DatObjectInfo itemInfo, 
        int gridX, 
        int gridY, 
        TextureManager textureManager)
    {
        // Calculate displacement offset (negative direction in screen pixels)
        // Convert pixels to relative tile units
        float dx = -itemInfo.OffsetX / TileSize;
        float dy = -itemInfo.OffsetY / TileSize;

        // Apply elevation displacement if applicable
        if (itemInfo.HasFlag(DatFlags.HasElevation))
        {
            dy -= itemInfo.ElevationHeight / TileSize;
        }

        // Select pattern indices based on grid position for variation
        int px = itemInfo.PatternX > 1 ? Math.Abs(gridX % itemInfo.PatternX) : 0;
        int py = itemInfo.PatternY > 1 ? Math.Abs(gridY % itemInfo.PatternY) : 0;

        // Loop over the width and height of the item (multi-tile structures)
        for (int h = 0; h < itemInfo.Height; h++)
        {
            for (int w = 0; w < itemInfo.Width; w++)
            {
                // Retrieve sprite ID from the flat layout array
                // w and h represent offsets. In Tibia, sprites are rendered bottom-right anchored.
                uint spriteId = itemInfo.GetSpriteId(w, h, layer: 0, px, py, pz: 0, frame: 0);
                if (spriteId == 0) continue;

                if (textureManager.TryLookupSprite(spriteId, out var lookup))
                {
                    // Compute drawing coordinates: bottom-right anchored
                    float drawX = gridX + dx - (itemInfo.Width - 1 - w);
                    float drawY = gridY + dy - (itemInfo.Height - 1 - h);

                    list.Add(new TileInstance
                    {
                        GridPos    = new Vector2(drawX, drawY),
                        UV         = new Vector4(lookup.U0, lookup.V0, lookup.U1, lookup.V1),
                        SliceIndex = lookup.SliceIndex,
                        Depth      = 0f // Z-sorting is handled by painter's list order
                    });
                }
            }
        }
    }

    private unsafe void UploadAndDraw(int count)
    {
        _gl.BindVertexArray(_vao);
        _gl.BindBuffer(BufferTargetARB.ArrayBuffer, _instanceVbo);

        fixed (TileInstance* pInst = _instances)
            _gl.BufferSubData(BufferTargetARB.ArrayBuffer, 0,
                (nuint)(count * sizeof(TileInstance)), pInst);

        // 6 vertices per quad, `count` instances
        _gl.DrawArraysInstanced(PrimitiveType.Triangles, 0, 6, (uint)count);
    }

    // ── Shader helpers ────────────────────────────────────────────────────────

    private uint CompileShader(ShaderType type, string src)
    {
        uint shader = _gl.CreateShader(type);
        _gl.ShaderSource(shader, src);
        _gl.CompileShader(shader);
        _gl.GetShader(shader, ShaderParameterName.CompileStatus, out int compiled);
        if (compiled == 0)
        {
            string log = _gl.GetShaderInfoLog(shader);
            throw new Exception($"Shader compile failed ({type}): {log}");
        }
        return shader;
    }

    private static string LoadEmbeddedShader(string filename)
    {
        var asm = Assembly.GetExecutingAssembly();
        string resourceName = asm.GetManifestResourceNames()
            .FirstOrDefault(n => n.EndsWith(filename, StringComparison.OrdinalIgnoreCase))
            ?? throw new FileNotFoundException($"Embedded shader not found: {filename}");

        using var stream = asm.GetManifestResourceStream(resourceName)!;
        using var reader = new StreamReader(stream);
        return reader.ReadToEnd();
    }

    // ── Cleanup ───────────────────────────────────────────────────────────────

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;
        _gl.DeleteVertexArray(_vao);
        _gl.DeleteBuffer(_quadVbo);
        _gl.DeleteBuffer(_instanceVbo);
        _gl.DeleteProgram(_shaderProgram);
    }
}
