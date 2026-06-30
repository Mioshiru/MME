using System;
using System.IO;
using MapEditor.Core.Map;
using MapEditor.Assets.Dat;
using MapEditor.Assets.Spr;
using MapEditor.Assets.Otbm;

namespace MapEditor.App;

/// <summary>
/// End-to-End Smoke Test verifying:
/// 1. Asset loading for 10.98 client files.
/// 2. Editing tiles in MapDocument.
/// 3. Running structural integrity checks.
/// 4. Exporting to .otbm with verification loops.
/// 5. Re-importing and validating coordinate/item matches.
/// </summary>
public static class IntegrationTest
{
    public static bool Run()
    {
        Console.ForegroundColor = ConsoleColor.Cyan;
        Console.WriteLine("\n==================================================");
        Console.WriteLine("=== Map Editor End-to-End Smoke Integration Test ===");
        Console.WriteLine("==================================================");
        Console.ResetColor();

        string datPath = Path.Combine("data", "1098", "Tibia.dat");
        string sprPath = Path.Combine("data", "1098", "Tibia.spr");
        string testMapPath = Path.Combine(AppContext.BaseDirectory, "integration_test.otbm");

        try
        {
            // Step 1: Client version configuration & verification
            Console.WriteLine("\n[Step 1] Loading Tibia 10.98 client assets...");
            if (!File.Exists(datPath) || !File.Exists(sprPath))
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"Error: Tibia 10.98 files not found in standard paths!");
                Console.ResetColor();
                return false;
            }

            var datDb = DatParser.Parse(datPath);
            Console.WriteLine($"  Successfully parsed Tibia.dat (Items: {datDb.Items.Count})");

            // Step 2: Create a mock MapDocument and edit layout
            Console.WriteLine("\n[Step 2] Creating map document and editing layout...");
            var map = new MapDocument { Name = "E2E Smoke Map" };
            
            // Tile A: Ground (Grass #100) + Wall (#1000)
            var posA = new Position(100, 100, 7);
            var tileA = map.GetOrCreateTile(posA);
            tileA.GroundId = 100;
            tileA.Items.Add(1000);

            // Tile B: Ground (Grass #100) + Stairs (#1385, has FloorChange flag)
            // Note: In 10.98, item 1385 is stairs.
            var posB = new Position(101, 100, 7);
            var tileB = map.GetOrCreateTile(posB);
            tileB.GroundId = 100;
            tileB.Items.Add(1385);

            // Create landing tiles on adjacent floors to satisfy stairs validation
            map.GetOrCreateTile(new Position(101, 100, 6)).GroundId = 100;
            map.GetOrCreateTile(new Position(101, 100, 8)).GroundId = 100;

            Console.WriteLine($"  Modified coordinates (100, 100, 7) and (101, 100, 7).");

            // Step 3: Run structural integrity checks
            Console.WriteLine("\n[Step 3] Running integrity checks...");
            var errors = MapValidator.Validate(map, datDb);
            int warningCount = 0;
            int errorCount = 0;
            foreach (var err in errors)
            {
                if (err.IsWarning) warningCount++;
                else errorCount++;
                Console.WriteLine($"    [Integrity {(err.IsWarning ? "Warning" : "Error")}] at {err.Position.X},{err.Position.Y},{err.Position.Z}: {err.Message}");
            }
            Console.WriteLine($"  Checks finished: {errorCount} critical errors, {warningCount} warnings.");

            if (errorCount > 0)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("Verification failed: Map contains critical structural errors!");
                Console.ResetColor();
                return false;
            }

            // Step 4: Export to .otbm and run header verification
            Console.WriteLine($"\n[Step 4] Exporting to .otbm file at: {testMapPath}");
            OtbmExporter.Export(map, testMapPath);
            Console.WriteLine("  Exported successfully.");

            Console.WriteLine("  Running header verification loop...");
            bool headerOk = OtbmExporter.VerifySavedFile(testMapPath);
            if (!headerOk)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("Error: Exported file failed header structural verification!");
                Console.ResetColor();
                return false;
            }
            Console.WriteLine("  Header verification loop succeeded!");

            // Step 5: Re-import and validate content
            Console.WriteLine("\n[Step 5] Re-importing map to verify layout consistency...");
            var importer = new OtbmImporter();
            var reimportedMap = importer.Import(testMapPath);

            var checkTileA = reimportedMap.GetTile(posA);
            if (checkTileA == null || checkTileA.GroundId != 100 || !checkTileA.Items.Contains(1000))
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("Error: Re-imported tile A data is corrupt!");
                Console.ResetColor();
                return false;
            }

            var checkTileB = reimportedMap.GetTile(posB);
            if (checkTileB == null || checkTileB.GroundId != 100 || !checkTileB.Items.Contains(1385))
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("Error: Re-imported tile B data is corrupt!");
                Console.ResetColor();
                return false;
            }

            Console.ForegroundColor = ConsoleColor.Green;
            Console.WriteLine("\n==================================================");
            Console.WriteLine("=== Integration Test Finished Successfully!      ===");
            Console.WriteLine("==================================================");
            Console.ResetColor();
            return true;
        }
        catch (Exception ex)
        {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine($"\nIntegration Test Failed with Exception: {ex.Message}");
            Console.WriteLine(ex.StackTrace);
            Console.ResetColor();
            return false;
        }
        finally
        {
            if (File.Exists(testMapPath))
            {
                File.Delete(testMapPath);
            }
        }
    }
}
