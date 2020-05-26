## Examples

### test

source : [in3-c/dotnet/examples/test.cs](https://github.com/slockit/in3-c/blob/master/dotnet/examples/test.cs)

example of a simple BlockNumber


```dotnet
/// example of a simple BlockNumber

using System;
using System.Numerics;
using In3;

namespace ConsoleTest
{
    class Program
    {
        static void Main(string[] args)
        {
            IN3 in3 = IN3.ForChain(Chain.Mainnet);
            in3.Configuration.UseHttp = true;
            BigInteger result = in3.Eth1.BlockNumber();
            Console.Out.Write(result);
        }
    }
}
```

### build examples


```sh
dotnet add package Slockit.In3
```

TODO explain how to set up example-projects