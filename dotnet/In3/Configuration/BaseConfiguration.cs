using System.Collections.Generic;

namespace In3.Configuration
{
    public abstract class BaseConfiguration
    {
        // Consider using composite pattern
        private readonly Dictionary<string, object> _state;
        private bool _isDirty;

        public BaseConfiguration()
        {
            _state = new Dictionary<string, object>();
        }

        public abstract bool HasChanged();

        protected object GetState(string key)
        {
            return _state.ContainsKey(key) ?_state[key] : null;
        }

        protected void SetState(string key, object value)
        {
            MarkAsDirty(!_state.ContainsKey(key) || _state[key] != value);
            _state[key] = value;
        }

        protected bool IsDirty()
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