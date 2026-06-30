using MapEditor.Core.Actions;

namespace MapEditor.Core.State;

/// <summary>
/// Thread-safe Redux-inspired store. Maintains the single source of truth (<see cref="EditorState"/>),
/// processes actions through the root reducer, and notifies subscribers on every state change.
/// </summary>
public sealed class EditorStore
{
    private EditorState _state = EditorState.Initial;
    private readonly object _lock = new();
    private readonly List<Action<EditorState>> _subscribers = new();

    /// <summary>Current state snapshot — safe to read from any thread.</summary>
    public EditorState State
    {
        get { lock (_lock) return _state; }
    }

    /// <summary>
    /// Dispatches an action, runs it through the root reducer to produce a new state,
    /// and notifies all subscribers if the state changed.
    /// </summary>
    public void Dispatch(IEditorAction action)
    {
        EditorState newState;
        lock (_lock)
        {
            newState = RootReducer.Reduce(_state, action);
            if (ReferenceEquals(newState, _state)) return; // no change
            _state = newState;
        }
        NotifySubscribers(newState);
    }

    /// <summary>
    /// Registers a callback that fires every time the state changes.
    /// Returns an <see cref="IDisposable"/> that unsubscribes when disposed.
    /// </summary>
    public IDisposable Subscribe(Action<EditorState> subscriber)
    {
        lock (_lock) _subscribers.Add(subscriber);
        return new Subscription(this, subscriber);
    }

    private void NotifySubscribers(EditorState state)
    {
        List<Action<EditorState>> snapshot;
        lock (_lock) snapshot = [.. _subscribers];
        foreach (var sub in snapshot)
        {
            try { sub(state); }
            catch { /* individual subscriber errors must not break the loop */ }
        }
    }

    private void Unsubscribe(Action<EditorState> subscriber)
    {
        lock (_lock) _subscribers.Remove(subscriber);
    }

    private sealed class Subscription(EditorStore store, Action<EditorState> subscriber) : IDisposable
    {
        private bool _disposed;
        public void Dispose()
        {
            if (_disposed) return;
            _disposed = true;
            store.Unsubscribe(subscriber);
        }
    }
}
