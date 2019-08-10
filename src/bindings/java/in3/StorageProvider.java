package in3;


/**
 * Provider methods to cache data.
 * These data could be nodelists, contract codes or validator changes.
 */
public interface StorageProvider {
    /**
     * returns a item from cache ()
     * @return the bytes or null if not found.
     */
    byte[] getItem(String key /** the key for the item */);

    /**
     * stores a item in the cache.
     */
    void setItem(String key/** the key for the item */, byte[] content /** the value to store */);
}
