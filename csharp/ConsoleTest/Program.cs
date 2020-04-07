using System;
using In3;

namespace ConsoleTest
{
    class Program
    {
        static void Main(string[] args)
        {
            IN3 in3 = IN3.ForChain(Chain.Mainnet);
            object result = in3.Eth1.BlockNumber();
            Console.Out.Write(result);
        }
    }
}