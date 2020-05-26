using System;

namespace In3.Storage
{
    /// <summary>
    /// Provider methods to cache data.
    /// These data could be nodelists, contract codes or validator changes.
    /// Any form of cache should implement <see cref="Storage" /> and be set with <see cref="IN3.Storage" />.
    /// </summary>
    public interface Storage
    {
        /// <summary>
        /// returns a item from cache.
        /// </summary>
        /// <param name="key">The key for the item.</param>
        /// <returns>The bytes or <see langword="null" /> if not found.</returns>
        byte[] GetItem(string key);

        /// <summary>
        /// Stores an item to cache.
        /// </summary>
        /// <param name="key">The key for the item.</param>
        /// <param name="content">The value to store.</param>
        void SetItem(string key, byte[] content);

        /// <summary>
        /// Clear the cache.
        /// </summary>
        /// <returns>The result of the operation: <see langword="true" /> for success and <see langword="false" /> for failure.</returns>
        bool Clear();
    }
}