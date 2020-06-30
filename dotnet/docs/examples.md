## Examples

### CallSmartContractFunction

source : [in3-c/dotnet/Examples/CallSmartContractFunction](https://github.com/slockit/in3-c/blob/master/dotnet/Examples/CallSmartContractFunction/Program.cs)



```c#
using System;
using System.Numerics;
using In3;
using In3.Eth1;

namespace CallSmartContractFunction
{
    public class Program
    {
        public static void Main()
        {
            // Set it to mainnet
            IN3 mainnetClient = IN3.ForChain(Chain.Mainnet);
            ClientConfiguration cfg = mainnetClient.Configuration;
            cfg.Proof = Proof.None;

            string contractAddress = "0x2736D225f85740f42D17987100dc8d58e9e16252";

            // Create the query transaction
            TransactionRequest serverCountQuery = new TransactionRequest();
            serverCountQuery.To = contractAddress;

            // Define the function and the parameters to query the total in3 servers
            serverCountQuery.Function = "totalServers():uint256";
            serverCountQuery.Params = new object[0];

            string[] serverCountResult = (string[])mainnetClient.Eth1.Call(serverCountQuery, BlockParameter.Latest);
            BigInteger servers = DataTypeConverter.HexStringToBigint(serverCountResult[0]);

            for (int i = 0; i < servers; i++)
            {
                TransactionRequest serverDetailQuery = new TransactionRequest();
                serverDetailQuery.To = contractAddress;

                // Define the function and the parameters to query the in3 servers detail
                serverDetailQuery.Function = "servers(uint256):(string,address,uint32,uint256,uint256,address)";
                serverDetailQuery.Params = new object[] { i }; // index of the server (uint256) as per solidity function signature

                string[] serverDetailResult = (string[])mainnetClient.Eth1.Call(serverDetailQuery, BlockParameter.Latest);
                Console.Out.WriteLine($"Server url: {serverDetailResult[0]}");
            }
        }
    }
```

### ConnectToEthereum

source : [in3-c/dotnet/Examples/ConnectToEthereum](https://github.com/slockit/in3-c/blob/master/dotnet/Examples/ConnectToEthereum/Program.cs)



```c#
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
```

### EnsResolver

source : [in3-c/dotnet/Examples/EnsResolver](https://github.com/slockit/in3-c/blob/master/dotnet/Examples/EnsResolver/Program.cs)



```c#
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
```

### SendTransaction

source : [in3-c/dotnet/Examples/SendTransaction](https://github.com/slockit/in3-c/blob/master/dotnet/Examples/SendTransaction/Program.cs)



```c#
using System;
using System.Threading;
using In3;
using In3.Crypto;
using In3.Eth1;

namespace SendTransaction
{
    public class Program
    {
        static void Main()
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
            long currentGasPrice = goerliClient.Eth1.GetGasPrice();
            transferWei.GasPrice = currentGasPrice;

            long estimatedSpentGas = goerliClient.Eth1.EstimateGas(transferWei, BlockParameter.Latest);
            Console.Out.WriteLine($"Estimated gas to spend: {estimatedSpentGas}");

            string transactionHash = goerliClient.Eth1.SendTransaction(transferWei);
            Console.Out.WriteLine($"Transaction {transactionHash} sent.");
            Thread.Sleep(30000);

            TransactionReceipt receipt = goerliClient.Eth1.GetTransactionReceipt(transactionHash);
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

