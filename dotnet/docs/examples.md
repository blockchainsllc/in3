## Examples

### CallSmartContractFunction

source : [in3-c/dotnet/Examples/CallSmartContractFunction//CallSmartContractFunction](https://github.com/blockchainsllc/in3/blob/master/dotnet/Examples/CallSmartContractFunction//CallSmartContractFunction/Program.cs)



```c#
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
            // Set it to mainnet
            IN3 mainnetClient = IN3.ForChain(Chain.Mainnet);
            ClientConfiguration cfg = mainnetClient.Configuration;
            cfg.Proof = Proof.Standard;

            string contractAddress = "0x2736D225f85740f42D17987100dc8d58e9e16252";

            // Create the query transaction
            TransactionRequest serverCountQuery = new TransactionRequest();
            serverCountQuery.To = contractAddress;

            // Define the function and the parameters to query the total in3 servers
            serverCountQuery.Function = "totalServers():uint256";
            serverCountQuery.Params = new object[0];

            string[] serverCountResult = (string[])await mainnetClient.Eth1.Call(serverCountQuery, BlockParameter.Latest);
            BigInteger servers = DataTypeConverter.HexStringToBigint(serverCountResult[0]);

            for (int i = 0; i < servers; i++)
            {
                TransactionRequest serverDetailQuery = new TransactionRequest();
                serverDetailQuery.To = contractAddress;

                // Define the function and the parameters to query the in3 servers detail
                serverDetailQuery.Function = "servers(uint256):(string,address,uint32,uint256,uint256,address)";
                serverDetailQuery.Params = new object[] { i }; // index of the server (uint256) as per solidity function signature

                string[] serverDetailResult = (string[])await mainnetClient.Eth1.Call(serverDetailQuery, BlockParameter.Latest);
                Console.Out.WriteLine($"Server url: {serverDetailResult[0]}");
            }
        }
    }
}

```

### ConnectToEthereum

source : [in3-c/dotnet/Examples/ConnectToEthereum//ConnectToEthereum](https://github.com/blockchainsllc/in3/blob/master/dotnet/Examples/ConnectToEthereum//ConnectToEthereum/Program.cs)



```c#
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

            Console.Out.WriteLine("Ethereum EWC Network");
            IN3 ewcClient = IN3.ForChain(Chain.Ewc);
            BigInteger ewcLatest = await ewcClient.Eth1.BlockNumber();
            BigInteger ewcCurrentGasPrice = await ewcClient.Eth1.GetGasPrice();
            Console.Out.WriteLine($"Latest Block Number: {ewcLatest}");
            Console.Out.WriteLine($"Gas Price: {ewcCurrentGasPrice} Wei");

            Console.Out.WriteLine("Ethereum Goerli Test Network");
            IN3 goerliClient = IN3.ForChain(Chain.Goerli);
            BigInteger goerliLatest = await goerliClient.Eth1.BlockNumber();
            BigInteger clientCurrentGasPrice = await goerliClient.Eth1.GetGasPrice();
            Console.Out.WriteLine($"Latest Block Number: {goerliLatest}");
            Console.Out.WriteLine($"Gas Price: {clientCurrentGasPrice} Wei");
        }
    }
}
```

### EnsResolver

source : [in3-c/dotnet/Examples/EnsResolver//EnsResolver](https://github.com/blockchainsllc/in3/blob/master/dotnet/Examples/EnsResolver//EnsResolver/Program.cs)



```c#
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
```

### Ipfs

source : [in3-c/dotnet/Examples/Ipfs//Ipfs](https://github.com/blockchainsllc/in3/blob/master/dotnet/Examples/Ipfs//Ipfs/Program.cs)



```c#
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

```

### Logs

source : [in3-c/dotnet/Examples/Logs//Logs](https://github.com/blockchainsllc/in3/blob/master/dotnet/Examples/Logs//Logs/Program.cs)



```c#
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
            // Define an upper limit for poll since we dont want our application potentially running forever.
            int maxIterations = 500;
            int oneSecond = 1000; // in ms

            // Connect to mainnet.
            IN3 mainnetClient = IN3.ForChain(Chain.Mainnet);

            // Create a filter object pointing, in this case, to an "eventful" contract address.
            LogFilter tetherUsFilter = new LogFilter {Address = "0xdAC17F958D2ee523a2206206994597C13D831ec7"};

            // Create the filter to be polled for logs.
            long filterId = await mainnetClient.Eth1.NewLogFilter(tetherUsFilter);

            // Loop to initiate the poll for the logs.
            for (int i = 0; i < maxIterations; i++)
            {
                // Query for the log events since the creation of the filter or the previous poll (this method in NOT idempotent as it retrieves a diff).
                Log[] tetherLogs = await mainnetClient.Eth1.GetFilterChangesFromLogs(filterId);
                if (tetherLogs.Length > 0)
                {
                    Console.Out.WriteLine("Logs found: " + tetherLogs.Length);
                    break;
                }

                // Wait before next query.
                Thread.Sleep(oneSecond);
            }
        }
    }
}

```

### SendTransaction

source : [in3-c/dotnet/Examples/SendTransaction//SendTransaction](https://github.com/blockchainsllc/in3/blob/master/dotnet/Examples/SendTransaction//SendTransaction/Program.cs)



```c#
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

            // Get the wallet, which is the default signer.
            SimpleWallet myAccountWallet = (SimpleWallet)goerliClient.Signer;

            string myAccount = myAccountWallet.AddRawKey(myPrivateKey);

            // Create the transaction request
            TransactionRequest transferWei = new TransactionRequest();
            transferWei.To = receivingAddress;
            transferWei.From = myAccount;
            transferWei.Value = 300;

            // Get the current gas prices
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
```

### Build Examples


To setup and run the example projects, simply run on the respective project folder:

```sh
dotnet run
```

To build all of them, on the solution folder, run:

```sh
dotnet build
```

