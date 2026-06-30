using Silk.NET.Input;
using Silk.NET.Maths;
using Silk.NET.OpenGL;
using Silk.NET.OpenGL.Extensions.ImGui;
using Silk.NET.Windowing;
using MapEditor.Assets.Atlas;
using MapEditor.Assets.Dat;
using MapEditor.Assets.Spr;
using MapEditor.Core.State;
using MapEditor.Core.Config;
using MapEditor.Core.Map;
using MapEditor.Core.Actions;
using MapEditor.Assets;
using MapEditor.Assets.Otbm;
using MapEditor.Rendering;
using MapEditor.Rendering.Renderer;
using MapEditor.Rendering.Textures;
using MapEditor.UI;
using System;
using System.IO;
using System.Linq;

// ─── Command Line Arguments / Validation ────────────────────────────────────
if (args.Contains("--test"))
{
    bool success = MapEditor.App.IntegrationTest.Run();
    Environment.Exit(success ? 0 : 1);
}

if (args.Contains("--validate"))
{
    Console.WriteLine("=== MapEditor Asset Parser Validation (Tibia 10.98) ===");
    
    string datPath = Path.Combine(AppContext.BaseDirectory, "data", "1098", "Tibia.dat");
    string sprPath = Path.Combine(AppContext.BaseDirectory, "data", "1098", "Tibia.spr");
    
    // Fallback to relative path if run from workspace directory
    if (!File.Exists(datPath)) datPath = "data/1098/Tibia.dat";
    if (!File.Exists(sprPath)) sprPath = "data/1098/Tibia.spr";
    
    Console.WriteLine($"Looking for DAT at: {Path.GetFullPath(datPath)}");
    Console.WriteLine($"Looking for SPR at: {Path.GetFullPath(sprPath)}");
    
    try
    {
        if (!File.Exists(datPath) || !File.Exists(sprPath))
        {
            throw new FileNotFoundException("Tibia.dat or Tibia.spr not found in data/1098 folder.");
        }
        
        // 1. Validate DatParser
        Console.WriteLine("\n[1/2] Parsing Tibia.dat...");
        
        DatDatabase validationDatDb;
        try
        {
            validationDatDb = DatParser.Parse(datPath);
        }
        catch (Exception ex)
        {
            Console.WriteLine($"\nFailed parsing Tibia.dat!");
            Console.WriteLine(ex.Message);
            if (ex.InnerException != null)
            {
                Console.WriteLine($"Inner Exception: {ex.InnerException.Message}");
            }
            throw;
        }
        
        Console.WriteLine("Successfully parsed Tibia.dat!");
        Console.WriteLine($"  Signature     : 0x{validationDatDb.Signature:X8}");
        Console.WriteLine($"  Item Count    : {validationDatDb.ItemCount}");
        Console.WriteLine($"  Outfit Count  : {validationDatDb.OutfitCount}");
        Console.WriteLine($"  Effect Count  : {validationDatDb.EffectCount}");
        Console.WriteLine($"  Missile Count : {validationDatDb.MissileCount}");
        Console.WriteLine($"  Total Objects : {validationDatDb.TotalObjects}");
        
        // Inspect an item (first item starts at 100)
        var item100 = validationDatDb.GetItem(100);
        if (item100 != null)
        {
            Console.WriteLine($"\n  Inspecting Item #100:");
            Console.WriteLine($"    Width×Height : {item100.Width}×{item100.Height}");
            Console.WriteLine($"    Layers       : {item100.Layers}");
            Console.WriteLine($"    Frames       : {item100.Frames}");
            Console.WriteLine($"    Sprite Count : {item100.SpriteCount}");
            if (item100.SpriteIds.Length > 0)
            {
                Console.WriteLine($"    First Sprite : {item100.SpriteIds[0]}");
            }
        }
        
        // 2. Validate SprParser
        Console.WriteLine("\n[2/2] Parsing Tibia.spr...");
        var sprDb = SprParser.ParseOffsetTable(sprPath);
        Console.WriteLine("Successfully parsed Tibia.spr Offset Table!");
        Console.WriteLine($"  Signature         : 0x{sprDb.Signature:X8}");
        Console.WriteLine($"  Total Sprite Count: {sprDb.SpriteCount}");
        Console.WriteLine($"  Non-Empty Sprites : {sprDb.NonEmptyCount}");
        
        // Decode a sprite (e.g. sprite ID 1 or first non-empty)
        uint testSpriteId = 1;
        while (testSpriteId <= sprDb.SpriteCount && !sprDb.HasSprite(testSpriteId))
        {
            testSpriteId++;
        }
        
        if (testSpriteId <= sprDb.SpriteCount)
        {
            Console.WriteLine($"\n  Decoding first non-empty sprite starting from #{testSpriteId}...");
            SpriteData? sprite = null;
            uint decodedId = testSpriteId;
            while (decodedId <= sprDb.SpriteCount)
            {
                if (sprDb.HasSprite(decodedId))
                {
                    sprite = SprParser.DecodeSprite(sprDb, sprPath, decodedId);
                    if (sprite != null)
                    {
                        testSpriteId = decodedId;
                        break;
                    }
                }
                decodedId++;
            }
            
            if (sprite != null)
            {
                Console.WriteLine($"  Successfully decoded sprite #{testSpriteId}!");
                Console.WriteLine($"    Dimensions : {SpriteData.Width}×{SpriteData.Height} px");
                Console.WriteLine($"    Format     : RGBA8 ({SpriteData.BytesPerPixel} bytes per pixel)");
                Console.WriteLine($"    Total Bytes: {sprite.Pixels.Length}");
            }
            else
            {
                Console.WriteLine("  Failed to decode any non-empty sprite.");
            }
        }

        // 3. Validate OtbmExporter and Importer Pipeline Integrity
        Console.WriteLine("\n[3/3] Testing OTBM Export/Import Pipeline...");
        string testMapPath = Path.Combine(AppContext.BaseDirectory, "world_temp.otbm");
        try
        {
            var testMap = new MapDocument { Name = "IntegrationTest" };
            
            // Create a tile with Ground ID 100 (grass) and a wall item 1000
            var testPos = new Position(100, 200, 7);
            var testTile = testMap.GetOrCreateTile(testPos);
            testTile.GroundId = 100;
            testTile.Items.Add(1000);

            // Run integrity check
            Console.WriteLine("  Running pre-export integrity check...");
            var errors = MapValidator.Validate(testMap, validationDatDb);
            Console.WriteLine($"    Found {errors.Count} validation errors/warnings.");

            // Export map to temp .otbm file
            Console.WriteLine($"  Exporting map to: {testMapPath}");
            OtbmExporter.Export(testMap, testMapPath);

            // Re-import the temp map
            Console.WriteLine("  Re-importing exported map...");
            var importer = new OtbmImporter();
            var importedMap = importer.Import(testMapPath);

            // Verify integrity of the imported data
            var importedTile = importedMap.GetTile(testPos);
            if (importedTile == null)
            {
                throw new InvalidDataException("Verification Failed: Tile not found in imported map!");
            }
            if (importedTile.GroundId != 100)
            {
                throw new InvalidDataException($"Verification Failed: GroundId is {importedTile.GroundId}, expected 100!");
            }
            if (importedTile.Items.Count != 1 || importedTile.Items[0] != 1000)
            {
                throw new InvalidDataException($"Verification Failed: Items count is {importedTile.Items.Count}, expected 1 containing item 1000!");
            }

            Console.WriteLine("  Verification Succeeded: Data integrity verified successfully!");
        }
        finally
        {
            // Clean up the temporary file
            if (File.Exists(testMapPath))
            {
                File.Delete(testMapPath);
            }
        }
        
        Console.WriteLine("\n=== Validation Succeeded! ===");
        return;
    }
    catch (Exception ex)
    {
        Console.ForegroundColor = ConsoleColor.Red;
        Console.WriteLine($"\nValidation Failed: {ex.Message}");
        Console.WriteLine(ex.StackTrace);
        Console.ResetColor();
        Environment.Exit(1);
    }
}

// Load persisted configuration on startup
SettingsManager.Load();
var settings = SettingsManager.Current;

var options = WindowOptions.Default with
{
    Title             = "OT Map Editor",
    Size              = new Vector2D<int>(settings.Width, settings.Height),
    PreferredDepthBufferBits   = 24,
    PreferredStencilBufferBits = 8,
    VSync             = settings.VSync,    // VSync parameter loaded from settings
    API               = new GraphicsAPI(
        ContextAPI.OpenGL,
        ContextProfile.Core,
        ContextFlags.ForwardCompatible,
        new APIVersion(3, 3)),
};

IWindow window = Window.Create(options);

// ── Resources (initialised inside Load) ────────────────────────────────────

GL?              gl             = null;
IInputContext?   input          = null;
ImGuiController? imGui          = null;
EditorStore?     store          = null;
UiRoot?          uiRoot         = null;
MapRenderer?     mapRenderer    = null;
Camera?          camera         = null;
ViewportFbo?     viewportFbo    = null;
AssetManager?    assetManager   = null;

// ── Window callbacks ─────────────────────────────────────────────────────────

window.Load += () =>
{
    gl    = window.CreateOpenGL();
    input = window.CreateInput();
    store = new EditorStore();

    // Setup ImGui with the Silk.NET controller
    imGui = new ImGuiController(gl, window, input, onConfigureIO: () =>
    {
        ImGuiNET.ImGui.GetIO().ConfigFlags |= ImGuiNET.ImGuiConfigFlags.DockingEnable;
    });

    // Apply a sleek dark theme
    ImGuiNET.ImGui.StyleColorsDark();
    StyleEditorTheme();

    // ── Asset Loading ────────────────────────────────────────────────────────
    assetManager = new AssetManager(gl);
    assetManager.LoadAssets(AppContext.BaseDirectory);

    uiRoot         = new UiRoot(store, gl, assetManager);
    uiRoot.PopulatePalette();
    
    mapRenderer    = new MapRenderer(gl);
    camera         = new Camera();
    viewportFbo    = new ViewportFbo(gl);

    // Initial GL state
    gl.ClearColor(0.12f, 0.12f, 0.14f, 1.0f);  // charcoal background
    gl.Enable(EnableCap.Blend);
    gl.BlendFunc(BlendingFactor.SrcAlpha, BlendingFactor.OneMinusSrcAlpha);
};

window.Update += delta =>
{
    imGui!.Update((float)delta);

    // Update window title with FPS
    float fps = 1.0f / (float)delta;
    window.Title = $"OT Map Editor  —  {fps:F0} FPS  |  Floor {store!.State.ActiveFloor}";

    // Handle global Ctrl+S keyboard shortcut for saving
    var activeMap = store!.State.ActiveMap;
    if (activeMap != null)
    {
        var io = ImGuiNET.ImGui.GetIO();
        if (io.KeyCtrl && ImGuiNET.ImGui.IsKeyPressed(ImGuiNET.ImGuiKey.S))
        {
            string path = activeMap.FilePath ?? "world.otbm";
            
            var validationErrors = MapValidator.Validate(activeMap, assetManager?.DatDb);
            foreach (var err in validationErrors)
            {
                Console.WriteLine($"[Validation {(err.IsWarning ? "Warning" : "Error")}] at {err.Position.X},{err.Position.Y},{err.Position.Z}: {err.Message}");
            }

            try
            {
                OtbmExporter.Export(activeMap, path);
                activeMap.FilePath = path;
                store.Dispatch(new MapDirtyChangedAction(false));
                Console.WriteLine($"[Shortcut] Map saved successfully to {Path.GetFullPath(path)}");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[Shortcut] Error exporting map: {ex.Message}");
            }
        }
    }
};

window.Render += delta =>
{
    var state = store!.State;

    // ── 1. Render Map onto the Viewport FBO ────────────────────────────────
    if (assetManager != null && assetManager.TextureManager != null && assetManager.TextureManager.IsUploaded && assetManager.DatDb != null)
    {
        viewportFbo!.Bind();
        
        gl!.ClearColor(0.12f, 0.12f, 0.14f, 1.0f); // clear FBO background
        gl!.Clear(ClearBufferMask.ColorBufferBit | ClearBufferMask.DepthBufferBit);
        
        mapRenderer!.Render(state, assetManager.TextureManager, camera!, assetManager.DatDb);
        
        viewportFbo!.Unbind();
    }

    // ── 2. Render default screen target (including ImGui overlay) ─────────
    gl!.ClearColor(0.12f, 0.12f, 0.14f, 1.0f); // clear screen background
    gl!.Clear(ClearBufferMask.ColorBufferBit | ClearBufferMask.DepthBufferBit);

    // Render ImGui overlay (passes camera and viewport FBO to render map canvas window)
    uiRoot!.Render(state, camera!, viewportFbo!, assetManager!);
    imGui!.Render();
};

window.Resize += size =>
{
    gl?.Viewport(0, 0, (uint)size.X, (uint)size.Y);
};

window.Closing += () =>
{
    viewportFbo?.Dispose();
    assetManager?.Dispose();
    mapRenderer?.Dispose();
    uiRoot?.Dispose();
    imGui?.Dispose();
    input?.Dispose();
    gl?.Dispose();
};

window.Run();
window.Dispose();

// ── Theme tweaks ─────────────────────────────────────────────────────────────

static void StyleEditorTheme()
{
    var style = ImGuiNET.ImGui.GetStyle();
    style.WindowRounding    = 6f;
    style.FrameRounding     = 4f;
    style.ScrollbarRounding = 4f;
    style.GrabRounding      = 4f;
    style.TabRounding       = 4f;
    style.FramePadding      = new System.Numerics.Vector2(6, 4);
    style.ItemSpacing       = new System.Numerics.Vector2(8, 6);
    style.WindowBorderSize  = 1f;
    style.ChildBorderSize   = 1f;

    // Accent blue palette
    var colors = style.Colors;
    colors[(int)ImGuiNET.ImGuiCol.Header]        = new System.Numerics.Vector4(0.25f, 0.50f, 0.85f, 0.65f);
    colors[(int)ImGuiNET.ImGuiCol.HeaderHovered] = new System.Numerics.Vector4(0.35f, 0.60f, 0.95f, 0.80f);
    colors[(int)ImGuiNET.ImGuiCol.HeaderActive]  = new System.Numerics.Vector4(0.20f, 0.45f, 0.80f, 1.00f);
    colors[(int)ImGuiNET.ImGuiCol.TitleBgActive] = new System.Numerics.Vector4(0.16f, 0.32f, 0.58f, 1.00f);
    colors[(int)ImGuiNET.ImGuiCol.CheckMark]     = new System.Numerics.Vector4(0.40f, 0.75f, 1.00f, 1.00f);
    colors[(int)ImGuiNET.ImGuiCol.SliderGrab]    = new System.Numerics.Vector4(0.35f, 0.65f, 0.95f, 1.00f);
    colors[(int)ImGuiNET.ImGuiCol.Button]        = new System.Numerics.Vector4(0.20f, 0.42f, 0.72f, 0.80f);
    colors[(int)ImGuiNET.ImGuiCol.ButtonHovered] = new System.Numerics.Vector4(0.30f, 0.55f, 0.90f, 1.00f);
    colors[(int)ImGuiNET.ImGuiCol.ButtonActive]  = new System.Numerics.Vector4(0.15f, 0.38f, 0.70f, 1.00f);
    colors[(int)ImGuiNET.ImGuiCol.Tab]           = new System.Numerics.Vector4(0.16f, 0.30f, 0.50f, 0.90f);
    colors[(int)ImGuiNET.ImGuiCol.TabHovered]    = new System.Numerics.Vector4(0.35f, 0.55f, 0.85f, 1.00f);
    colors[(int)ImGuiNET.ImGuiCol.TabActive]     = new System.Numerics.Vector4(0.24f, 0.46f, 0.78f, 1.00f);
}
