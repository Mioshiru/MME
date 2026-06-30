using System;
using System.IO;
using MapEditor.Core.State;
using MapEditor.UI.Widgets;
using MapEditor.UI.Windows;
using MapEditor.Rendering;
using MapEditor.Rendering.Renderer;
using MapEditor.Assets;
using Silk.NET.OpenGL;

namespace MapEditor.UI;

/// <summary>
/// Top-level UI orchestrator. Instantiates all ImGui panels and calls their
/// <c>Render</c> methods once per frame, passing the current editor state.
/// </summary>
public sealed class UiRoot : IDisposable
{
    private readonly EditorStore          _store;
    private readonly MainMenuBar          _menuBar;
    private readonly ToolbarFactory       _toolbarFactory;
    private readonly SettingsWindow       _settings;
    private readonly MapViewportWindow    _viewportWindow;
    private readonly PaletteView          _terrainPalette;
    private readonly PaletteViewModel     _terrainPaletteViewModel;
    private readonly ToolbarIcons         _icons;
    private bool _disposed;

    public UiRoot(EditorStore store, GL gl, AssetManager assetManager)
    {
        _store          = store;
        _menuBar        = new MainMenuBar(store);
        _toolbarFactory = new ToolbarFactory(store);
        _settings       = new SettingsWindow(store);
        _viewportWindow = new MapViewportWindow(store);
        
        _terrainPaletteViewModel = new PaletteViewModel(assetManager);
        _terrainPalette          = new PaletteView(store, _terrainPaletteViewModel, assetManager);

        // Load toolbar PNG icons
        _icons = new ToolbarIcons(gl);
        string iconsDir = Path.Combine(AppContext.BaseDirectory, "icons");
        if (!Directory.Exists(iconsDir))
        {
            iconsDir = "icons"; // Fallback to relative execution dir
        }
        _icons.Load(iconsDir);
    }

    /// <summary>
    /// Call once per frame, between ImGuiController.Update() and ImGuiController.Render().
    /// </summary>
    public void Render(EditorState state, Camera camera, ViewportFbo fbo, AssetManager assetManager)
    {
        // ── 1. Render Menu Bar ───────────────────────────────────────────────
        _menuBar.Render(state);

        // ── 2. Begin Main DockSpace ──────────────────────────────────────────
        DockingManager.BeginDockSpace();

        // ── 3. Render Docked Windows ─────────────────────────────────────────
        _toolbarFactory.Render(state, _icons);
        _terrainPalette.Render(state);
        _viewportWindow.Render(state, camera, fbo, assetManager.DatDb);

        // ── 4. Render Floating or Dialog Windows ─────────────────────────────
        _settings.Render(state);

        // ── 5. End Main DockSpace ────────────────────────────────────────────
        DockingManager.EndDockSpace();
    }

    public void PopulatePalette()
    {
        if (!_terrainPaletteViewModel.IsDataLoaded)
        {
            _terrainPaletteViewModel.LoadData();
        }
    }

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;
        
        _icons.Dispose();
    }
}
