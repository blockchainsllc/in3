using System;
using System.Threading.Tasks;
using In3;

namespace EnsResolver
{
    public class Program
    {
        static async Task Main()
        {
            IN3 in3 = IN3.ForChain(Chain.Mainnet);

            string cryptoKittiesDomain = "cryptokitties.eth";
            string resolver = await in3.Eth1.Ens(cryptoKittiesDomain, ENSParameter.Resolver);
            string owner = await in3.Eth1.Ens(cryptoKittiesDomain, ENSParameter.Owner);

            Console.Out.WriteLine($"The owner of {cryptoKittiesDomain} is {owner}, resolver is {resolver}.");
        }
    }
}