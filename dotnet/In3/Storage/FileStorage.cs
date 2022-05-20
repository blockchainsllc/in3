using System;
using System.IO;

namespace In3.Storage
{
    /// <summary>
    /// File implementation of <see cref="Storage" />. It caches all in a directory.
    /// </summary>
    public class FileStorage : Storage
    {
        private static string StorageDirectory = ".in3";
        /// <summary>
        /// The method needed to create an instance of <see cref="Storage" />. It also creates the root directory for the cache.
        /// </summary>
        public static Storage Create()
        {
            if (!Directory.Exists(StorageDirectory)) Directory.CreateDirectory(StorageDirectory);
            return new FileStorage();
        }
        private FileStorage() {}
        
        /// <summary>
        /// Fetches the data from disk.
        /// </summary>
        /// <param name="key">Key </param>
        /// <returns>The cached value as a <see langword="byte[]" />.</returns>
        public byte[] GetItem(string key)
        {
            try
            {
                return File.ReadAllBytes(Path.Combine(StorageDirectory, key));
            }
            catch (Exception e)
            {
                return null;
            }
        }

        /// <summary>
        /// Stores an item to cache. Writes synchronously to disk.
        /// </summary>
        /// <param name="key">The key for the item.</param>
        /// <param name="content">The value to store.</param>
        public void SetItem(string key, byte[] content)
        {
            File.WriteAllBytes(Path.Combine(StorageDirectory, key), content);
        }

        /// <summary>
        /// Empty the disk cache.
        /// </summary>
        /// <returns>Result for the clear operation.</returns>
        public bool Clear()
        {
            if (Directory.Exists(StorageDirectory))
            {
                Directory.Delete(StorageDirectory, true);
                Directory.CreateDirectory(StorageDirectory);
                return true;
            }

            return false;
        }
    }
}