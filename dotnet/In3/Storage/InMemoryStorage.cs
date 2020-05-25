using System;
using System.Collections.Generic;

namespace In3.Storage
{
    public class InMemoryStorage : Storage
    {
        private Dictionary<string, byte[]> cache;

        public InMemoryStorage()
        {
            cache = new Dictionary<string, byte[]>();
        }

        public byte[] GetItem(string key)
        {
            return cache.ContainsKey(key) ? cache[key] : null;
        }

        public void SetItem(string key, byte[] content)
        {
            if (!String.IsNullOrEmpty(key))
            {
                cache[key] = content;
            }
        }

        public bool Clear()
        {
            cache.Clear();
            return true;
        }
    }
}