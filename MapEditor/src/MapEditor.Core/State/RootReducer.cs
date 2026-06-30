using System;
using System.IO;
using System.Linq;
using MapEditor.Core.Actions;
using MapEditor.Core.Map;

namespace MapEditor.Core.State;

/// <summary>
/// Root reducer — dispatches each action to the appropriate sub-reducer and
/// assembles a new immutable <see cref="EditorState"/>.
/// </summary>
internal static class RootReducer
{
    private const int MaxUndoDepth = 100;

    public static EditorState Reduce(EditorState state, IEditorAction action) => action switch
    {
        // ── Map ───────────────────────────────────────────────────────────────
        OpenMapAction open => state with
        {
            MapPath = open.FilePath,
            ActiveMap = new MapDocument { FilePath = open.FilePath, Name = Path.GetFileName(open.FilePath) },
        },
        CloseMapAction => state with { ActiveMap = null, MapPath = null },
        MapDirtyChangedAction dirty when state.ActiveMap is not null =>
            Mutate(() => state.ActiveMap!.IsDirty = dirty.IsDirty, state),

        // ── Tool ─────────────────────────────────────────────────────────────
        SelectToolAction tool => state with { ActiveTool = tool.Tool },
        SelectItemAction select => state with { SelectedItemId = select.ItemId },

        // ── Floor ────────────────────────────────────────────────────────────
        SelectFloorAction floor when floor.Floor is >= MapDocument.MinFloor and <= MapDocument.MaxFloor =>
            state with { ActiveFloor = floor.Floor },

        // ── Undo/Redo ─────────────────────────────────────────────────────────
        PushUndoSnapshotAction push => state with
        {
            UndoStack = state.UndoStack
                .Append((push.Description, push.Snapshot))
                .TakeLast(MaxUndoDepth)
                .ToList(),
            RedoStack = Array.Empty<(string, byte[])>(),
        },
        UndoAction when state.CanUndo && state.ActiveMap is not null => ApplyUndo(state),
        RedoAction when state.CanRedo && state.ActiveMap is not null => ApplyRedo(state),

        // ── UI ────────────────────────────────────────────────────────────────
        ToggleSettingsWindowAction => state with { SettingsWindowOpen = !state.SettingsWindowOpen },

        _ => state, // no-op for unhandled actions
    };

    private static EditorState ApplyUndo(EditorState state)
    {
        var undoStack = state.UndoStack.ToList();
        var entry = undoStack[^1];
        undoStack.RemoveAt(undoStack.Count - 1);

        // Capture current state of the map as a redo snapshot before restoring
        byte[] redoSnapshot = MapSerializer.Serialize(state.ActiveMap!);
        var redoStack = state.RedoStack.Prepend((entry.Item1, redoSnapshot)).ToList();

        // Restore map state from the undo snapshot
        MapSerializer.Deserialize(entry.Item2, state.ActiveMap!);

        // Return state with updated stacks and trigger a mutation notify
        return state with { UndoStack = undoStack, RedoStack = redoStack, IsLoading = state.IsLoading };
    }

    private static EditorState ApplyRedo(EditorState state)
    {
        var redoStack = state.RedoStack.ToList();
        var entry = redoStack[0];
        redoStack.RemoveAt(0);

        // Capture current state of the map as an undo snapshot before restoring
        byte[] undoSnapshot = MapSerializer.Serialize(state.ActiveMap!);
        var undoStack = state.UndoStack.Append((entry.Item1, undoSnapshot)).ToList();

        // Restore map state from the redo snapshot
        MapSerializer.Deserialize(entry.Item2, state.ActiveMap!);

        // Return state with updated stacks and trigger a mutation notify
        return state with { UndoStack = undoStack, RedoStack = redoStack, IsLoading = state.IsLoading };
    }

    /// <summary>
    /// Applies a side-effecting mutation to a mutable object and returns a NEW state record
    /// so that subscribers are still notified. Used only for mutable reference-type members
    /// like <see cref="MapDocument"/> that don't support <c>with</c> expressions.
    /// </summary>
    private static EditorState Mutate(Action mutation, EditorState state)
    {
        mutation();
        // Force a new record instance so ReferenceEquals check in the store triggers notifications
        return state with { IsLoading = state.IsLoading };
    }
}
