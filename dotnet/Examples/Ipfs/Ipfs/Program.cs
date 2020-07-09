using System;
using System.Text;
using System.Threading.Tasks;
using In3;

namespace Ipfs
{
    class Program
    {
        static async Task Main()
        {
            // Content to be stored
            string toStore = "LOREM_IPSUM";

            // Connect to ipfs.
            IN3 ipfsClient = IN3.ForChain(Chain.Ipfs);

            // Store the hash since it will be needed to fetch the content back.
            string hash = await ipfsClient.Ipfs.Put(toStore);

            //
            byte[] storedBytes = await ipfsClient.Ipfs.Get(hash);
            string storedStging = Encoding.UTF8.GetString(storedBytes, 0, storedBytes.Length);
            Console.Out.WriteLine($"The stored string is: {storedStging}");
        }
    }
}
