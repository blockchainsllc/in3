using System.Collections.Generic;
using System.Runtime.CompilerServices;

[assembly: InternalsVisibleToAttribute("Test")]

namespace In3.Configuration
{
    /// <summary>
    /// Base class for all configuration classes.
    /// </summary>
    public abstract class BaseConfiguration
    {
        // Consider using composite pattern
        private readonly Dictionary<string, object> _state;
        private bool _isDirty;

        internal BaseConfiguration()
        {
            _state = new Dictionary<string, object>();
        }

        internal abstract bool HasChanged();

        internal object GetState(string key)
        {
            return _state.ContainsKey(key) ? _state[key] : null;
        }

        internal void SetState(string key, object value)
        {
            MarkAsDirty(!_state.ContainsKey(key) || _state[key] != value);
            _state[key] = value;
        }

        internal bool IsDirty()
        {
            return _isDirty;
        }

        internal void Clean()
        {
            _isDirty = false;
        }

        private void MarkAsDirty(bool becameDirty)
        {
            _isDirty = _isDirty || becameDirty;
        }
    }
}