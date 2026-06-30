using System;
using System.Numerics;

namespace MapEditor.Rendering;

/// <summary>
/// Manages viewport dimensions, panning, zooming, and projection matrix calculations.
/// Coordinates are represented in grid tile units.
/// </summary>
public sealed class Camera
{
    private float _cameraX;
    private float _cameraY;
    private float _zoom = 1.0f;
    private int _viewportWidth = 1280;
    private int _viewportHeight = 800;

    public const float TileSize = 32f;
    public const float MinZoom = 0.1f;
    public const float MaxZoom = 10f;

    /// <summary>Camera X scroll offset in grid tile units.</summary>
    public float CameraX
    {
        get => _cameraX;
        set => _cameraX = value;
    }

    /// <summary>Camera Y scroll offset in grid tile units.</summary>
    public float CameraY
    {
        get => _cameraY;
        set => _cameraY = value;
    }

    /// <summary>Current camera zoom level (clamped between 0.1 and 10.0).</summary>
    public float Zoom
    {
        get => _zoom;
        set => _zoom = Math.Clamp(value, MinZoom, MaxZoom);
    }

    public int ViewportWidth
    {
        get => _viewportWidth;
        set => _viewportWidth = Math.Max(1, value);
    }

    public int ViewportHeight
    {
        get => _viewportHeight;
        set => _viewportHeight = Math.Max(1, value);
    }

    /// <summary>
    /// Computes the orthographic projection matrix for the camera's viewport.
    /// </summary>
    public Matrix4x4 GetProjectionMatrix()
    {
        float left = _cameraX * TileSize;
        float right = left + _viewportWidth / _zoom;
        float top = _cameraY * TileSize;
        float bottom = top + _viewportHeight / _zoom;

        // Create an orthographic matrix mapping the view boundaries to OpenGL clip space [-1, 1].
        // Depth range is set to [-1, 1].
        return Matrix4x4.CreateOrthographicOffCenter(left, right, bottom, top, -1f, 1f);
    }

    /// <summary>
    /// Pans the camera scroll offsets by a delta in screen pixels.
    /// </summary>
    public void Pan(Vector2 pixelDelta)
    {
        // Adjust pixel delta by zoom and tile size to convert back to grid tile units
        _cameraX -= pixelDelta.X / (_zoom * TileSize);
        _cameraY -= pixelDelta.Y / (_zoom * TileSize);
    }

    /// <summary>
    /// Zooms the camera focusing on a specific screen pixel coordinate,
    /// so the mouse cursor points to the same tile position post-zoom.
    /// </summary>
    public void ZoomAt(Vector2 screenCursorPos, float zoomFactor)
    {
        Vector2 targetTileBefore = ScreenToTile(screenCursorPos);

        // Update zoom
        Zoom *= zoomFactor;

        Vector2 targetTileAfter = ScreenToTile(screenCursorPos);

        // Adjust position so that the target point remains stationary
        _cameraX += (targetTileBefore.X - targetTileAfter.X);
        _cameraY += (targetTileBefore.Y - targetTileAfter.Y);
    }

    /// <summary>
    /// Translates a screen pixel coordinate to grid tile coordinates.
    /// </summary>
    public Vector2 ScreenToTile(Vector2 screenPos)
    {
        float tileX = _cameraX + (screenPos.X / (_zoom * TileSize));
        float tileY = _cameraY + (screenPos.Y / (_zoom * TileSize));
        return new Vector2(tileX, tileY);
    }

    /// <summary>
    /// Gets the bounding box of grid coordinates currently visible in the camera view.
    /// Used for frustum culling.
    /// </summary>
    public void GetVisibleBounds(out int minX, out int minY, out int maxX, out int maxY)
    {
        float visLeft = _cameraX;
        float visTop = _cameraY;
        float visRight = _cameraX + _viewportWidth / (_zoom * TileSize);
        float visBottom = _cameraY + _viewportHeight / (_zoom * TileSize);

        // Add 1 tile margin defensively
        minX = (int)Math.Floor(visLeft) - 1;
        minY = (int)Math.Floor(visTop) - 1;
        maxX = (int)Math.Ceiling(visRight) + 1;
        maxY = (int)Math.Ceiling(visBottom) + 1;
    }
}
