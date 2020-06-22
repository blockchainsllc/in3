using System;
using System.Numerics;
using In3;

namespace ConnectToEthereum
{
    class Program
    {
        static void Main()
        {
            Console.Out.WriteLine("Ethereum Main Network");
            IN3 mainnetClient = IN3.ForChain(Chain.Mainnet);
            BigInteger mainnetLatest = mainnetClient.Eth1.BlockNumber();
            BigInteger mainnetCurrentGasPrice = mainnetClient.Eth1.GetGasPrice();
            Console.Out.WriteLine($"Latest Block Number: {mainnetLatest}");
            Console.Out.WriteLine($"Gas Price: {mainnetCurrentGasPrice} Wei");

            Console.Out.WriteLine("Ethereum Kovan Test Network");
            IN3 kovanClient = IN3.ForChain(Chain.Kovan);
            BigInteger kovanLatest = kovanClient.Eth1.BlockNumber();
            BigInteger kovanCurrentGasPrice = kovanClient.Eth1.GetGasPrice();
            Console.Out.WriteLine($"Latest Block Number: {kovanLatest}");
            Console.Out.WriteLine($"Gas Price: {kovanCurrentGasPrice} Wei");

            Console.Out.WriteLine("Ethereum Goerli Test Network");
            IN3 goerliClient = IN3.ForChain(Chain.Goerli);
            BigInteger goerliLatest = goerliClient.Eth1.BlockNumber();
            BigInteger clientCurrentGasPrice = goerliClient.Eth1.GetGasPrice();
            Console.Out.WriteLine($"Latest Block Number: {goerliLatest}");
            Console.Out.WriteLine($"Gas Price: {clientCurrentGasPrice} Wei");
        }
    }
}