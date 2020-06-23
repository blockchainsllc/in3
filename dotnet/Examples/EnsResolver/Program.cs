using System;
using In3;

namespace EnsResolver
{
    public class Program
    {
        static void Main()
        {
            IN3 in3 = IN3.ForChain(Chain.Mainnet);

            string cryptoKittiesDomain = "cryptokitties.eth";
            string resolver = in3.Eth1.Ens(cryptoKittiesDomain, ENSParameter.Resolver);
            string owner = in3.Eth1.Ens(cryptoKittiesDomain, ENSParameter.Owner);

            Console.Out.WriteLine($"The owner of {cryptoKittiesDomain} is {owner}, resolver is {resolver}.");
        }
    }
}