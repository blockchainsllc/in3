# Examples


-  [CallSmartContractFunction](./CallSmartContractFunction//CallSmartContractFunction/Program.cs)
using System;
using System.Numerics;
using System.Threading.Tasks;
using In3;
using In3.Configuration;
using In3.Eth1;
using In3.Utils;

namespace CallSmartContractFunction
{
    public class Program
    {
        public static async Task Main()
        {
            IN3 mainnetClient = IN3.ForChain(Chain.Mainnet);
            ClientConfiguration cfg = mainnetClient.Configuration;
            cfg.Proof = Proof.Standard;

            string contractAddress = "0x2736D225f85740f42D17987100dc8d58e9e16252";

            TransactionRequest serverCountQuery = new TransactionRequest();
            serverCountQuery.To = contractAddress;

            serverCountQuery.Function = "totalServers():uint256";
            serverCountQuery.Params = new object[0];

            string[] serverCountResult = (string[])await mainnetClient.Eth1.Call(serverCountQuery, BlockParameter.Latest);
            BigInteger servers = DataTypeConverter.HexStringToBigint(serverCountResult[0]);

            for (int i = 0; i < servers; i++)
            {
                TransactionRequest serverDetailQuery = new TransactionRequest();
                serverDetailQuery.To = contractAddress;

                serverDetailQuery.Function = "servers(uint256):(string,address,uint32,uint256,uint256,address)";

                string[] serverDetailResult = (string[])await mainnetClient.Eth1.Call(serverDetailQuery, BlockParameter.Latest);
                Console.Out.WriteLine($"Server url: {serverDetailResult[0]}");
            }
        }
    }
}

-  [ConnectToEthereum](./ConnectToEthereum//ConnectToEthereum/Program.cs)
using System;
using System.Numerics;
using System.Threading.Tasks;
using In3;

namespace ConnectToEthereum
{
    class Program
    {
        static async Task Main()
        {
            Console.Out.WriteLine("Ethereum Main Network");
            IN3 mainnetClient = IN3.ForChain(Chain.Mainnet);
            BigInteger mainnetLatest = await mainnetClient.Eth1.BlockNumber();
            BigInteger mainnetCurrentGasPrice = await mainnetClient.Eth1.GetGasPrice();
            Console.Out.WriteLine($"Latest Block Number: {mainnetLatest}");
            Console.Out.WriteLine($"Gas Price: {mainnetCurrentGasPrice} Wei");

            Console.Out.WriteLine("Ethereum Kovan Test Network");
            IN3 kovanClient = IN3.ForChain(Chain.Kovan);
            BigInteger kovanLatest = await kovanClient.Eth1.BlockNumber();
            BigInteger kovanCurrentGasPrice = await kovanClient.Eth1.GetGasPrice();
            Console.Out.WriteLine($"Latest Block Number: {kovanLatest}");
            Console.Out.WriteLine($"Gas Price: {kovanCurrentGasPrice} Wei");

            Console.Out.WriteLine("Ethereum Goerli Test Network");
            IN3 goerliClient = IN3.ForChain(Chain.Goerli);
            BigInteger goerliLatest = await goerliClient.Eth1.BlockNumber();
            BigInteger clientCurrentGasPrice = await goerliClient.Eth1.GetGasPrice();
            Console.Out.WriteLine($"Latest Block Number: {goerliLatest}");
            Console.Out.WriteLine($"Gas Price: {clientCurrentGasPrice} Wei");
        }
    }
}

-  [EnsResolver](./EnsResolver//EnsResolver/Program.cs)
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

-  [Ipfs](./Ipfs//Ipfs/Program.cs)
﻿using System;
using System.Text;
using System.Threading.Tasks;
using In3;

namespace Ipfs
{
    class Program
    {
        static async Task Main()
        {
            string toStore = "LOREM_IPSUM";

            IN3 ipfsClient = IN3.ForChain(Chain.Ipfs);

            string hash = await ipfsClient.Ipfs.Put(toStore);

            byte[] storedBytes = await ipfsClient.Ipfs.Get(hash);
            string storedStging = Encoding.UTF8.GetString(storedBytes, 0, storedBytes.Length);
            Console.Out.WriteLine($"The stored string is: {storedStging}");
        }
    }
}

-  [Logs](./Logs//Logs/Program.cs)
﻿using System;
using System.Threading;
using System.Threading.Tasks;
using In3;
using In3.Eth1;

namespace Logs
{
    class Program
    {
        static async Task Main()
        {
            int maxIterations = 500;

            IN3 mainnetClient = IN3.ForChain(Chain.Mainnet);

            LogFilter tetherUsFilter = new LogFilter {Address = "0xdAC17F958D2ee523a2206206994597C13D831ec7"};

            long filterId = await mainnetClient.Eth1.NewLogFilter(tetherUsFilter);

            for (int i = 0; i < maxIterations; i++)
            {
                Log[] tetherLogs = await mainnetClient.Eth1.GetFilterChangesFromLogs(filterId);
                if (tetherLogs.Length > 0)
                {
                    Console.Out.WriteLine("Logs found: " + tetherLogs.Length);
                    break;
                }

                Thread.Sleep(oneSecond);
            }
        }
    }
}

-  [SendTransaction](./SendTransaction//SendTransaction/Program.cs)
using System;
using System.Threading;
using System.Threading.Tasks;
using In3;
using In3.Crypto;
using In3.Eth1;

namespace SendTransaction
{
    public class Program
    {
        static async Task Main()
        {
            IN3 goerliClient = IN3.ForChain(Chain.Goerli);

            string myPrivateKey = "0x0829B3C639A3A8F2226C8057F100128D4F7AE8102C92048BA6DE38CF4D3BC6F1";
            string receivingAddress = "0x6FA33809667A99A805b610C49EE2042863b1bb83";

            SimpleWallet myAccountWallet = (SimpleWallet)goerliClient.Signer;

            string myAccount = myAccountWallet.AddRawKey(myPrivateKey);

            TransactionRequest transferWei = new TransactionRequest();
            transferWei.To = receivingAddress;
            transferWei.From = myAccount;
            transferWei.Value = 300;

            long currentGasPrice = await goerliClient.Eth1.GetGasPrice();
            transferWei.GasPrice = currentGasPrice;

            long estimatedSpentGas = await goerliClient.Eth1.EstimateGas(transferWei, BlockParameter.Latest);
            Console.Out.WriteLine($"Estimated gas to spend: {estimatedSpentGas}");

            string transactionHash = await goerliClient.Eth1.SendTransaction(transferWei);
            Console.Out.WriteLine($"Transaction {transactionHash} sent.");
            Thread.Sleep(30000);

            TransactionReceipt receipt = await goerliClient.Eth1.GetTransactionReceipt(transactionHash);
            Console.Out.WriteLine($"Transaction {transactionHash} mined on block {receipt.BlockNumber}.");
        }
    }
}
### Build Examples


To setup and run the example projects, simply run on the respective project folder:

```sh
dotnet run
```

To build all of them, on the solution folder, run:

```sh
dotnet build
```

