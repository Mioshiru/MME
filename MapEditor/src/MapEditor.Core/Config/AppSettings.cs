using System;

namespace MapEditor.Core.Config;

public enum PerformancePreset
{
    Low,
    Medium,
    High,
    Custom
}

/// <summary>
/// Serializable configuration schema for Editor Settings.
/// </summary>
public sealed class AppSettings
{
    public PerformancePreset Preset { get; set; } = PerformancePreset.High;
    public bool VSync { get; set; } = true;
    public int BatchSize { get; set; } = 65536;
    public int Width { get; set; } = 1280;
    public int Height { get; set; } = 800;
    
    // UI Display Preferences
    public bool ShowGrid { get; set; } = true;
    public bool ShowMinimap { get; set; } = true;
    public bool EnableAutosave { get; set; } = false;
    public int AutosaveIntervalMinutes { get; set; } = 5;
    public float UiScale { get; set; } = 1.0f;
    public int SelectedTheme { get; set; } = 0;

    // Asset paths
    public string ClientDatPath { get; set; } = "data/1098/Tibia.dat";
    public string ClientSprPath { get; set; } = "data/1098/Tibia.spr";

    /// <summary>
    /// Adjusts GPU configuration parameters automatically based on the chosen Performance Preset.
    /// </summary>
    public void ApplyPreset(PerformancePreset preset)
    {
        Preset = preset;
        if (preset == PerformancePreset.Custom)
        {
            return;
        }

        switch (preset)
        {
            case PerformancePreset.Low:
                VSync = true;
                BatchSize = 8192;     // Low GPU vertex buffer usage
                break;
            case PerformancePreset.Medium:
                VSync = true;
                BatchSize = 32768;    // Balanced buffer size
                break;
            case PerformancePreset.High:
                VSync = true;
                BatchSize = 65536;    // Max buffer allocation for dense tile ranges
                break;
        }
    }
}
