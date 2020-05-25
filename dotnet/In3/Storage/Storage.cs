using System;

namespace In3.Storage
{
    /**
     * Provider methods to cache data.
     * These data could be nodelists, contract codes or validator changes.
     */
    public interface Storage
    {
        /**
         * returns a item from cache ()
         * @return the bytes or null if not found.
         */
        byte[] GetItem(string key /** the key for the item */);

        /**
         * stores a item in the cache.
         */
        void SetItem(string key /** the key for the item */, byte[] content /** the value to store */);

        /**
         * clear the cache.
         */
        bool Clear();
    }
}