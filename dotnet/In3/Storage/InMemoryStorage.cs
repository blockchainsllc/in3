using System;
using System.Collections.Generic;

namespace In3.Storage
{
    /// <summary>
    /// Default implementation of <see cref="Storage" />. It caches all cacheable data in memory.
    /// </summary>
    public class InMemoryStorage : Storage
    {
        private Dictionary<string, byte[]> cache;

        /// <summary>
        /// Standard constructor.
        /// </summary>
        public InMemoryStorage()
        {
            cache = new Dictionary<string, byte[]>();
        }

        /// <summary>
        /// Fetches the data from memory.
        /// </summary>
        /// <param name="key">Key </param>
        /// <returns>The cached value as a <see langword="byte[]" />.</returns>
        public byte[] GetItem(string key)
        {
            return cache.ContainsKey(key) ? cache[key] : null;
        }

        /// <summary>
        /// Stores a value in memory for a given key.
        /// </summary>
        /// <param name="key">A unique identifier for the data that is being cached.</param>
        /// <param name="content">The value that is being cached.</param>
        public void SetItem(string key, byte[] content)
        {
            if (!String.IsNullOrEmpty(key))
            {
                cache[key] = content;
            }
        }

        /// <summary>
        /// Empty the in-memory cache.
        /// </summary>
        /// <returns>Result for the clear operation.</returns>
        public bool Clear()
        {
            cache.Clear();
            return true;
        }
    }
}