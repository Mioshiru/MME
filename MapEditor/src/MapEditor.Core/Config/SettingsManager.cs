using System;
using System.IO;
using System.Text.Json;

namespace MapEditor.Core.Config;

/// <summary>
/// Handles loading, saving, and managing configuration files for the map editor.
/// </summary>
public static class SettingsManager
{
    private static readonly string ConfigPath = Path.Combine(AppContext.BaseDirectory, "config.json");
    private static readonly JsonSerializerOptions JsonOptions = new() { WriteIndented = true };

    private static AppSettings _current = null!;

    public static AppSettings Current
    {
        get
        {
            if (_current == null)
            {
                Load();
            }
            return _current;
        }
    }

    /// <summary>
    /// Loads settings from config.json if it exists, otherwise initializes default settings.
    /// </summary>
    public static void Load()
    {
        try
        {
            if (File.Exists(ConfigPath))
            {
                string json = File.ReadAllText(ConfigPath);
                var loaded = JsonSerializer.Deserialize<AppSettings>(json, JsonOptions);
                if (loaded != null)
                {
                    _current = loaded;
                    return;
                }
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error loading config.json: {ex.Message}");
        }

        // Initialize with default settings
        _current = new AppSettings();
    }

    /// <summary>
    /// Saves current settings model to config.json.
    /// </summary>
    public static void Save()
    {
        try
        {
            string json = JsonSerializer.Serialize(_current, JsonOptions);
            File.WriteAllText(ConfigPath, json);
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error saving config.json: {ex.Message}");
        }
    }
}
