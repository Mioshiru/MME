namespace MapEditor.Core.Actions;

/// <summary>
/// Marker interface for all editor actions dispatched to the store.
/// </summary>
public interface IEditorAction { }

// ── Map actions ──────────────────────────────────────────────────────────────

public record OpenMapAction(string FilePath) : IEditorAction;

public record CloseMapAction : IEditorAction;

public record MapDirtyChangedAction(bool IsDirty) : IEditorAction;

// ── Tool selection ────────────────────────────────────────────────────────────

public enum EditorTool
{
    Draw,
    Erase,
    Select,
    Fill,
    SpawnBrush,
    WaypointBrush,
}

public record SelectToolAction(EditorTool Tool) : IEditorAction;

public record SelectItemAction(ushort ItemId) : IEditorAction;

// ── Floor selection ───────────────────────────────────────────────────────────

public record SelectFloorAction(int Floor) : IEditorAction;

// ── Undo / Redo ───────────────────────────────────────────────────────────────

public record UndoAction : IEditorAction;

public record RedoAction : IEditorAction;

public record PushUndoSnapshotAction(string Description, byte[] Snapshot) : IEditorAction;

// ── Settings ──────────────────────────────────────────────────────────────────

public record ToggleSettingsWindowAction : IEditorAction;
