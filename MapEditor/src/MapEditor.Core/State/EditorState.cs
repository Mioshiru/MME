using MapEditor.Core.Actions;
using MapEditor.Core.Map;

namespace MapEditor.Core.State;

/// <summary>
/// Immutable snapshot of all editor state. Produced by reducers.
/// </summary>
public sealed record EditorState
{
    // ── Map ──────────────────────────────────────────────────────────────────
    public MapDocument? ActiveMap { get; init; }
    public string? MapPath { get; init; }

    // ── View ─────────────────────────────────────────────────────────────────
    public int ActiveFloor { get; init; } = 7;
    public float ZoomLevel { get; init; } = 1.0f;

    // ── Tool ─────────────────────────────────────────────────────────────────
    public EditorTool ActiveTool { get; init; } = EditorTool.Draw;
    public ushort SelectedItemId { get; init; } = 100; // Default to first client ID (100)

    // ── Undo/Redo ────────────────────────────────────────────────────────────
    public IReadOnlyList<(string Description, byte[] Snapshot)> UndoStack { get; init; }
        = Array.Empty<(string, byte[])>();

    public IReadOnlyList<(string Description, byte[] Snapshot)> RedoStack { get; init; }
        = Array.Empty<(string, byte[])>();

    public bool CanUndo => UndoStack.Count > 0;
    public bool CanRedo => RedoStack.Count > 0;

    // ── UI state ─────────────────────────────────────────────────────────────
    public bool SettingsWindowOpen { get; init; }
    public bool IsLoading { get; init; }

    // ── Static initial state ─────────────────────────────────────────────────
    public static EditorState Initial { get; } = new();
}
