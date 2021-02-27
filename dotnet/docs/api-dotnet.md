# API Reference Dotnet

Dotnet bindings and library for in3. Go to our [readthedocs](https://in3.readthedocs.io/en/develop/api-dotnet.html) page for more on usage.

This library is based on the [C version of Incubed](http://github.com/blockchainsllc/in3).

## Runtimes

Since this is built on top of the native library, it is limited to the followin runtimes (RID)

- osx-x64
- linux-x86
- linux-x64
- win-x64
- linux-arm64

For more information, see [Rid Catalog](https://docs.microsoft.com/en-us/dotnet/core/rid-catalog).

## Quickstart

### Install with nuget

```sh
dotnet add package Blockchains.In3
```

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

\n## Index\n

- [Account](#T-In3-Crypto-Account "In3.Crypto.Account")
  - [Address](#P-In3-Crypto-Account-Address "In3.Crypto.Account.Address")
  - [PublicKey](#P-In3-Crypto-Account-PublicKey "In3.Crypto.Account.PublicKey")
- [Api](#T-In3-Btc-Api "In3.Btc.Api")
- [Api](#T-In3-Crypto-Api "In3.Crypto.Api")
- [Api](#T-In3-Eth1-Api "In3.Eth1.Api")
- [Api](#T-In3-Ipfs-Api "In3.Ipfs.Api")
  - [GetBlockBytes(blockHash)](#M-In3-Btc-Api-GetBlockBytes-System-String- "In3.Btc.Api.GetBlockBytes(System.String)")
  - [GetBlockHeader(blockHash)](#M-In3-Btc-Api-GetBlockHeader-System-String- "In3.Btc.Api.GetBlockHeader(System.String)")
  - [GetBlockHeaderBytes(blockHash)](#M-In3-Btc-Api-GetBlockHeaderBytes-System-String- "In3.Btc.Api.GetBlockHeaderBytes(System.String)")
  - [GetBlockWithTxData(blockHash)](#M-In3-Btc-Api-GetBlockWithTxData-System-String- "In3.Btc.Api.GetBlockWithTxData(System.String)")
  - [GetBlockWithTxIds(blockHash)](#M-In3-Btc-Api-GetBlockWithTxIds-System-String- "In3.Btc.Api.GetBlockWithTxIds(System.String)")
  - [GetTransaction(txid)](#M-In3-Btc-Api-GetTransaction-System-String- "In3.Btc.Api.GetTransaction(System.String)")
  - [GetTransactionBytes(txid)](#M-In3-Btc-Api-GetTransactionBytes-System-String- "In3.Btc.Api.GetTransactionBytes(System.String)")
  - [DecryptKey(pk,passphrase)](#M-In3-Crypto-Api-DecryptKey-System-String,System-String- "In3.Crypto.Api.DecryptKey(System.String,System.String)")
  - [EcRecover(signedData,signature,signatureType)](#M-In3-Crypto-Api-EcRecover-System-String,System-String,In3-Crypto-SignatureType- "In3.Crypto.Api.EcRecover(System.String,System.String,In3.Crypto.SignatureType)")
  - [Pk2Address(pk)](#M-In3-Crypto-Api-Pk2Address-System-String- "In3.Crypto.Api.Pk2Address(System.String)")
  - [Pk2Public(pk)](#M-In3-Crypto-Api-Pk2Public-System-String- "In3.Crypto.Api.Pk2Public(System.String)")
  - [Sha3(data)](#M-In3-Crypto-Api-Sha3-System-String- "In3.Crypto.Api.Sha3(System.String)")
  - [SignData(msg,pk,sigType)](#M-In3-Crypto-Api-SignData-System-String,System-String,In3-Crypto-SignatureType- "In3.Crypto.Api.SignData(System.String,System.String,In3.Crypto.SignatureType)")
  - [AbiDecode(signature,encodedData)](#M-In3-Eth1-Api-AbiDecode-System-String,System-String- "In3.Eth1.Api.AbiDecode(System.String,System.String)")
  - [AbiEncode(signature,args)](#M-In3-Eth1-Api-AbiEncode-System-String,System-Object[]- "In3.Eth1.Api.AbiEncode(System.String,System.Object[])")
  - [BlockNumber()](#M-In3-Eth1-Api-BlockNumber "In3.Eth1.Api.BlockNumber")
  - [Call(request,blockNumber)](#M-In3-Eth1-Api-Call-In3-Eth1-TransactionRequest,System-Numerics-BigInteger- "In3.Eth1.Api.Call(In3.Eth1.TransactionRequest,System.Numerics.BigInteger)")
  - [ChecksumAddress(address,shouldUseChainId)](#M-In3-Eth1-Api-ChecksumAddress-System-String,System-Nullable{System-Boolean}- "In3.Eth1.Api.ChecksumAddress(System.String,System.Nullable{System.Boolean})")
  - [Ens(name,type)](#M-In3-Eth1-Api-Ens-System-String,In3-ENSParameter- "In3.Eth1.Api.Ens(System.String,In3.ENSParameter)")
  - [EstimateGas(request,blockNumber)](#M-In3-Eth1-Api-EstimateGas-In3-Eth1-TransactionRequest,System-Numerics-BigInteger- "In3.Eth1.Api.EstimateGas(In3.Eth1.TransactionRequest,System.Numerics.BigInteger)")
  - [GetBalance(address,blockNumber)](#M-In3-Eth1-Api-GetBalance-System-String,System-Numerics-BigInteger- "In3.Eth1.Api.GetBalance(System.String,System.Numerics.BigInteger)")
  - [GetBlockByHash(blockHash,shouldIncludeTransactions)](#M-In3-Eth1-Api-GetBlockByHash-System-String,System-Boolean- "In3.Eth1.Api.GetBlockByHash(System.String,System.Boolean)")
  - [GetBlockByNumber(blockNumber,shouldIncludeTransactions)](#M-In3-Eth1-Api-GetBlockByNumber-System-Numerics-BigInteger,System-Boolean- "In3.Eth1.Api.GetBlockByNumber(System.Numerics.BigInteger,System.Boolean)")
  - [GetBlockTransactionCountByHash(blockHash)](#M-In3-Eth1-Api-GetBlockTransactionCountByHash-System-String- "In3.Eth1.Api.GetBlockTransactionCountByHash(System.String)")
  - [GetBlockTransactionCountByNumber(blockNumber)](#M-In3-Eth1-Api-GetBlockTransactionCountByNumber-System-Numerics-BigInteger- "In3.Eth1.Api.GetBlockTransactionCountByNumber(System.Numerics.BigInteger)")
  - [GetChainId()](#M-In3-Eth1-Api-GetChainId "In3.Eth1.Api.GetChainId")
  - [GetCode(address,blockNumber)](#M-In3-Eth1-Api-GetCode-System-String,System-Numerics-BigInteger- "In3.Eth1.Api.GetCode(System.String,System.Numerics.BigInteger)")
  - [GetFilterChangesFromLogs(filterId)](#M-In3-Eth1-Api-GetFilterChangesFromLogs-System-Int64- "In3.Eth1.Api.GetFilterChangesFromLogs(System.Int64)")
  - [GetFilterLogs(filterId)](#M-In3-Eth1-Api-GetFilterLogs-System-Int64- "In3.Eth1.Api.GetFilterLogs(System.Int64)")
  - [GetGasPrice()](#M-In3-Eth1-Api-GetGasPrice "In3.Eth1.Api.GetGasPrice")
  - [GetLogs(filter)](#M-In3-Eth1-Api-GetLogs-In3-Eth1-LogFilter- "In3.Eth1.Api.GetLogs(In3.Eth1.LogFilter)")
  - [GetStorageAt(address,position,blockNumber)](#M-In3-Eth1-Api-GetStorageAt-System-String,System-Numerics-BigInteger,System-Numerics-BigInteger- "In3.Eth1.Api.GetStorageAt(System.String,System.Numerics.BigInteger,System.Numerics.BigInteger)")
  - [GetTransactionByBlockHashAndIndex(blockHash,index)](#M-In3-Eth1-Api-GetTransactionByBlockHashAndIndex-System-String,System-Int32- "In3.Eth1.Api.GetTransactionByBlockHashAndIndex(System.String,System.Int32)")
  - [GetTransactionByBlockNumberAndIndex(blockNumber,index)](#M-In3-Eth1-Api-GetTransactionByBlockNumberAndIndex-System-Numerics-BigInteger,System-Int32- "In3.Eth1.Api.GetTransactionByBlockNumberAndIndex(System.Numerics.BigInteger,System.Int32)")
  - [GetTransactionByHash(transactionHash)](#M-In3-Eth1-Api-GetTransactionByHash-System-String- "In3.Eth1.Api.GetTransactionByHash(System.String)")
  - [GetTransactionCount(address,blockNumber)](#M-In3-Eth1-Api-GetTransactionCount-System-String,System-Numerics-BigInteger- "In3.Eth1.Api.GetTransactionCount(System.String,System.Numerics.BigInteger)")
  - [GetTransactionReceipt(transactionHash)](#M-In3-Eth1-Api-GetTransactionReceipt-System-String- "In3.Eth1.Api.GetTransactionReceipt(System.String)")
  - [GetUncleByBlockNumberAndIndex(blockNumber,position)](#M-In3-Eth1-Api-GetUncleByBlockNumberAndIndex-System-Numerics-BigInteger,System-Int32- "In3.Eth1.Api.GetUncleByBlockNumberAndIndex(System.Numerics.BigInteger,System.Int32)")
  - [GetUncleCountByBlockHash(blockHash)](#M-In3-Eth1-Api-GetUncleCountByBlockHash-System-String- "In3.Eth1.Api.GetUncleCountByBlockHash(System.String)")
  - [GetUncleCountByBlockNumber(blockNumber)](#M-In3-Eth1-Api-GetUncleCountByBlockNumber-System-Numerics-BigInteger- "In3.Eth1.Api.GetUncleCountByBlockNumber(System.Numerics.BigInteger)")
  - [NewBlockFilter()](#M-In3-Eth1-Api-NewBlockFilter "In3.Eth1.Api.NewBlockFilter")
  - [NewLogFilter(filter)](#M-In3-Eth1-Api-NewLogFilter-In3-Eth1-LogFilter- "In3.Eth1.Api.NewLogFilter(In3.Eth1.LogFilter)")
  - [SendRawTransaction(transactionData)](#M-In3-Eth1-Api-SendRawTransaction-System-String- "In3.Eth1.Api.SendRawTransaction(System.String)")
  - [SendTransaction(tx)](#M-In3-Eth1-Api-SendTransaction-In3-Eth1-TransactionRequest- "In3.Eth1.Api.SendTransaction(In3.Eth1.TransactionRequest)")
  - [SendTransactionAndWait(tx)](#M-In3-Eth1-Api-SendTransactionAndWait-In3-Eth1-TransactionRequest- "In3.Eth1.Api.SendTransactionAndWait(In3.Eth1.TransactionRequest)")
  - [UninstallFilter(filterId)](#M-In3-Eth1-Api-UninstallFilter-System-Int64- "In3.Eth1.Api.UninstallFilter(System.Int64)")
  - [Get(multihash)](#M-In3-Ipfs-Api-Get-System-String- "In3.Ipfs.Api.Get(System.String)")
  - [Put(content)](#M-In3-Ipfs-Api-Put-System-String- "In3.Ipfs.Api.Put(System.String)")
  - [Put(content)](#M-In3-Ipfs-Api-Put-System-Byte[]- "In3.Ipfs.Api.Put(System.Byte[])")
- [BaseConfiguration](#T-In3-Configuration-BaseConfiguration "In3.Configuration.BaseConfiguration")
- [Block](#T-In3-Eth1-Block "In3.Eth1.Block")
  - [Author](#P-In3-Eth1-Block-Author "In3.Eth1.Block.Author")
  - [Difficulty](#P-In3-Eth1-Block-Difficulty "In3.Eth1.Block.Difficulty")
  - [ExtraData](#P-In3-Eth1-Block-ExtraData "In3.Eth1.Block.ExtraData")
  - [GasLimit](#P-In3-Eth1-Block-GasLimit "In3.Eth1.Block.GasLimit")
  - [Hash](#P-In3-Eth1-Block-Hash "In3.Eth1.Block.Hash")
  - [LogsBloom](#P-In3-Eth1-Block-LogsBloom "In3.Eth1.Block.LogsBloom")
  - [MixHash](#P-In3-Eth1-Block-MixHash "In3.Eth1.Block.MixHash")
  - [Nonce](#P-In3-Eth1-Block-Nonce "In3.Eth1.Block.Nonce")
  - [Number](#P-In3-Eth1-Block-Number "In3.Eth1.Block.Number")
  - [ParentHash](#P-In3-Eth1-Block-ParentHash "In3.Eth1.Block.ParentHash")
  - [ReceiptsRoot](#P-In3-Eth1-Block-ReceiptsRoot "In3.Eth1.Block.ReceiptsRoot")
  - [Sha3Uncles](#P-In3-Eth1-Block-Sha3Uncles "In3.Eth1.Block.Sha3Uncles")
  - [Size](#P-In3-Eth1-Block-Size "In3.Eth1.Block.Size")
  - [StateRoot](#P-In3-Eth1-Block-StateRoot "In3.Eth1.Block.StateRoot")
  - [Timestamp](#P-In3-Eth1-Block-Timestamp "In3.Eth1.Block.Timestamp")
  - [TotalDifficulty](#P-In3-Eth1-Block-TotalDifficulty "In3.Eth1.Block.TotalDifficulty")
  - [TransactionsRoot](#P-In3-Eth1-Block-TransactionsRoot "In3.Eth1.Block.TransactionsRoot")
  - [Uncles](#P-In3-Eth1-Block-Uncles "In3.Eth1.Block.Uncles")
- [BlockHeader](#T-In3-Btc-BlockHeader "In3.Btc.BlockHeader")
  - [Bits](#P-In3-Btc-BlockHeader-Bits "In3.Btc.BlockHeader.Bits")
  - [Chainwork](#P-In3-Btc-BlockHeader-Chainwork "In3.Btc.BlockHeader.Chainwork")
  - [Confirmations](#P-In3-Btc-BlockHeader-Confirmations "In3.Btc.BlockHeader.Confirmations")
  - [Difficulty](#P-In3-Btc-BlockHeader-Difficulty "In3.Btc.BlockHeader.Difficulty")
  - [Hash](#P-In3-Btc-BlockHeader-Hash "In3.Btc.BlockHeader.Hash")
  - [Height](#P-In3-Btc-BlockHeader-Height "In3.Btc.BlockHeader.Height")
  - [Mediantime](#P-In3-Btc-BlockHeader-Mediantime "In3.Btc.BlockHeader.Mediantime")
  - [Merkleroot](#P-In3-Btc-BlockHeader-Merkleroot "In3.Btc.BlockHeader.Merkleroot")
  - [NTx](#P-In3-Btc-BlockHeader-NTx "In3.Btc.BlockHeader.NTx")
  - [Nextblockhash](#P-In3-Btc-BlockHeader-Nextblockhash "In3.Btc.BlockHeader.Nextblockhash")
  - [Nonce](#P-In3-Btc-BlockHeader-Nonce "In3.Btc.BlockHeader.Nonce")
  - [Previousblockhash](#P-In3-Btc-BlockHeader-Previousblockhash "In3.Btc.BlockHeader.Previousblockhash")
  - [Time](#P-In3-Btc-BlockHeader-Time "In3.Btc.BlockHeader.Time")
  - [Version](#P-In3-Btc-BlockHeader-Version "In3.Btc.BlockHeader.Version")
  - [VersionHex](#P-In3-Btc-BlockHeader-VersionHex "In3.Btc.BlockHeader.VersionHex")
- [BlockParameter](#T-In3-BlockParameter "In3.BlockParameter")
  - [Earliest](#P-In3-BlockParameter-Earliest "In3.BlockParameter.Earliest")
  - [Latest](#P-In3-BlockParameter-Latest "In3.BlockParameter.Latest")
- [Block\`1](#T-In3-Btc-Block`1 "In3.Btc.Block`1")
  - [Size](#P-In3-Btc-Block`1-Size "In3.Btc.Block`1.Size")
  - [Tx](#P-In3-Btc-Block`1-Tx "In3.Btc.Block`1.Tx")
  - [Weight](#P-In3-Btc-Block`1-Weight "In3.Btc.Block`1.Weight")
- [Chain](#T-In3-Chain "In3.Chain")
  - [Btc](#F-In3-Chain-Btc "In3.Chain.Btc")
  - [Evan](#F-In3-Chain-Evan "In3.Chain.Evan")
  - [Ewc](#F-In3-Chain-Ewc "In3.Chain.Ewc")
  - [Goerli](#F-In3-Chain-Goerli "In3.Chain.Goerli")
  - [Ipfs](#F-In3-Chain-Ipfs "In3.Chain.Ipfs")
  - [Local](#F-In3-Chain-Local "In3.Chain.Local")
  - [Mainnet](#F-In3-Chain-Mainnet "In3.Chain.Mainnet")
  - [Multichain](#F-In3-Chain-Multichain "In3.Chain.Multichain")
  - [Tobalaba](#F-In3-Chain-Tobalaba "In3.Chain.Tobalaba")
  - [Volta](#F-In3-Chain-Volta "In3.Chain.Volta")
- [ChainConfiguration](#T-In3-Configuration-ChainConfiguration "In3.Configuration.ChainConfiguration")
  - [#ctor(chain,clientConfiguration)](#M-In3-Configuration-ChainConfiguration-#ctor-In3-Chain,In3-Configuration-ClientConfiguration- "In3.Configuration.ChainConfiguration.#ctor(In3.Chain,In3.Configuration.ClientConfiguration)")
  - [Contract](#P-In3-Configuration-ChainConfiguration-Contract "In3.Configuration.ChainConfiguration.Contract")
  - [NeedsUpdate](#P-In3-Configuration-ChainConfiguration-NeedsUpdate "In3.Configuration.ChainConfiguration.NeedsUpdate")
  - [NodesConfiguration](#P-In3-Configuration-ChainConfiguration-NodesConfiguration "In3.Configuration.ChainConfiguration.NodesConfiguration")
  - [RegistryId](#P-In3-Configuration-ChainConfiguration-RegistryId "In3.Configuration.ChainConfiguration.RegistryId")
  - [WhiteList](#P-In3-Configuration-ChainConfiguration-WhiteList "In3.Configuration.ChainConfiguration.WhiteList")
  - [WhiteListContract](#P-In3-Configuration-ChainConfiguration-WhiteListContract "In3.Configuration.ChainConfiguration.WhiteListContract")
- [ClientConfiguration](#T-In3-Configuration-ClientConfiguration "In3.Configuration.ClientConfiguration")
  - [AutoUpdateList](#P-In3-Configuration-ClientConfiguration-AutoUpdateList "In3.Configuration.ClientConfiguration.AutoUpdateList")
  - [BootWeights](#P-In3-Configuration-ClientConfiguration-BootWeights "In3.Configuration.ClientConfiguration.BootWeights")
  - [ChainsConfiguration](#P-In3-Configuration-ClientConfiguration-ChainsConfiguration "In3.Configuration.ClientConfiguration.ChainsConfiguration")
  - [Finality](#P-In3-Configuration-ClientConfiguration-Finality "In3.Configuration.ClientConfiguration.Finality")
  - [IncludeCode](#P-In3-Configuration-ClientConfiguration-IncludeCode "In3.Configuration.ClientConfiguration.IncludeCode")
  - [KeepIn3](#P-In3-Configuration-ClientConfiguration-KeepIn3 "In3.Configuration.ClientConfiguration.KeepIn3")
  - [MaxAttempts](#P-In3-Configuration-ClientConfiguration-MaxAttempts "In3.Configuration.ClientConfiguration.MaxAttempts")
  - [MinDeposit](#P-In3-Configuration-ClientConfiguration-MinDeposit "In3.Configuration.ClientConfiguration.MinDeposit")
  - [NodeLimit](#P-In3-Configuration-ClientConfiguration-NodeLimit "In3.Configuration.ClientConfiguration.NodeLimit")
  - [NodeProps](#P-In3-Configuration-ClientConfiguration-NodeProps "In3.Configuration.ClientConfiguration.NodeProps")
  - [Proof](#P-In3-Configuration-ClientConfiguration-Proof "In3.Configuration.ClientConfiguration.Proof")
  - [ReplaceLatestBlock](#P-In3-Configuration-ClientConfiguration-ReplaceLatestBlock "In3.Configuration.ClientConfiguration.ReplaceLatestBlock")
  - [RequestCount](#P-In3-Configuration-ClientConfiguration-RequestCount "In3.Configuration.ClientConfiguration.RequestCount")
  - [Rpc](#P-In3-Configuration-ClientConfiguration-Rpc "In3.Configuration.ClientConfiguration.Rpc")
  - [SignatureCount](#P-In3-Configuration-ClientConfiguration-SignatureCount "In3.Configuration.ClientConfiguration.SignatureCount")
  - [Timeout](#P-In3-Configuration-ClientConfiguration-Timeout "In3.Configuration.ClientConfiguration.Timeout")
  - [UseHttp](#P-In3-Configuration-ClientConfiguration-UseHttp "In3.Configuration.ClientConfiguration.UseHttp")
- [Context](#T-In3-Context-Context "In3.Context.Context")
  - [#ctor(ctx,nativeClient)](#M-In3-Context-Context-#ctor-System-IntPtr,In3-Native-NativeClient- "In3.Context.Context.#ctor(System.IntPtr,In3.Native.NativeClient)")
  - [CreateNativeCtx(nativeIn3Ptr,rpc)](#M-In3-Context-Context-CreateNativeCtx-System-IntPtr,System-String- "In3.Context.Context.CreateNativeCtx(System.IntPtr,System.String)")
  - [Dispose()](#M-In3-Context-Context-Dispose "In3.Context.Context.Dispose")
  - [Execute()](#M-In3-Context-Context-Execute "In3.Context.Context.Execute")
  - [FromRpc(wrapper,rpc)](#M-In3-Context-Context-FromRpc-In3-Native-NativeClient,System-String- "In3.Context.Context.FromRpc(In3.Native.NativeClient,System.String)")
  - [GetErrorMessage()](#M-In3-Context-Context-GetErrorMessage "In3.Context.Context.GetErrorMessage")
  - [GetLastWaiting()](#M-In3-Context-Context-GetLastWaiting "In3.Context.Context.GetLastWaiting")
  - [GetResponse()](#M-In3-Context-Context-GetResponse "In3.Context.Context.GetResponse")
  - [GetType()](#M-In3-Context-Context-GetType "In3.Context.Context.GetType")
  - [HandleRequest()](#M-In3-Context-Context-HandleRequest "In3.Context.Context.HandleRequest")
  - [HandleSign()](#M-In3-Context-Context-HandleSign "In3.Context.Context.HandleSign")
  - [IsValid()](#M-In3-Context-Context-IsValid "In3.Context.Context.IsValid")
  - [ReportError()](#M-In3-Context-Context-ReportError-System-String- "In3.Context.Context.ReportError(System.String)")
- [ContextException](#T-In3-Error-ContextException "In3.Error.ContextException")
- [DataTypeConverter](#T-In3-Utils-DataTypeConverter "In3.Utils.DataTypeConverter")
  - [HexStringToBigint(source)](#M-In3-Utils-DataTypeConverter-HexStringToBigint-System-String- "In3.Utils.DataTypeConverter.HexStringToBigint(System.String)")
- [DefaultTransport](#T-In3-Transport-DefaultTransport "In3.Transport.DefaultTransport")
  - [#ctor()](#M-In3-Transport-DefaultTransport-#ctor "In3.Transport.DefaultTransport.#ctor")
  - [Handle(url,payload)](#M-In3-Transport-DefaultTransport-Handle-System-String,System-String- "In3.Transport.DefaultTransport.Handle(System.String,System.String)")
- [ENSParameter](#T-In3-ENSParameter "In3.ENSParameter")
  - [Addr](#P-In3-ENSParameter-Addr "In3.ENSParameter.Addr")
  - [Hash](#P-In3-ENSParameter-Hash "In3.ENSParameter.Hash")
  - [Owner](#P-In3-ENSParameter-Owner "In3.ENSParameter.Owner")
  - [Resolver](#P-In3-ENSParameter-Resolver "In3.ENSParameter.Resolver")
- [IN3](#T-In3-IN3 "In3.IN3")
  - [#ctor(chainId)](#M-In3-IN3-#ctor-In3-Chain- "In3.IN3.#ctor(In3.Chain)")
  - [Btc](#P-In3-IN3-Btc "In3.IN3.Btc")
  - [Configuration](#P-In3-IN3-Configuration "In3.IN3.Configuration")
  - [Crypto](#P-In3-IN3-Crypto "In3.IN3.Crypto")
  - [Eth1](#P-In3-IN3-Eth1 "In3.IN3.Eth1")
  - [Ipfs](#P-In3-IN3-Ipfs "In3.IN3.Ipfs")
  - [Signer](#P-In3-IN3-Signer "In3.IN3.Signer")
  - [Storage](#P-In3-IN3-Storage "In3.IN3.Storage")
  - [Transport](#P-In3-IN3-Transport "In3.IN3.Transport")
  - [Finalize()](#M-In3-IN3-Finalize "In3.IN3.Finalize")
  - [ForChain(chain)](#M-In3-IN3-ForChain-In3-Chain- "In3.IN3.ForChain(In3.Chain)")
  - [SendRpc(method,args,in3)](#M-In3-IN3-SendRpc-System-String,System-Object[],System-Collections-Generic-Dictionary{System-String,System-Object}- "In3.IN3.SendRpc(System.String,System.Object[],System.Collections.Generic.Dictionary{System.String,System.Object})")
- [In3Exception](#T-In3-Error-In3Exception "In3.Error.In3Exception")
- [InMemoryStorage](#T-In3-Storage-InMemoryStorage "In3.Storage.InMemoryStorage")
  - [#ctor()](#M-In3-Storage-InMemoryStorage-#ctor "In3.Storage.InMemoryStorage.#ctor")
  - [Clear()](#M-In3-Storage-InMemoryStorage-Clear "In3.Storage.InMemoryStorage.Clear")
  - [GetItem(key)](#M-In3-Storage-InMemoryStorage-GetItem-System-String- "In3.Storage.InMemoryStorage.GetItem(System.String)")
  - [SetItem(key,content)](#M-In3-Storage-InMemoryStorage-SetItem-System-String,System-Byte[]- "In3.Storage.InMemoryStorage.SetItem(System.String,System.Byte[])")
- [InvalidSignerException](#T-In3-Error-InvalidSignerException "In3.Error.InvalidSignerException")
- [InvalidTransactionRequestException](#T-In3-Error-InvalidTransactionRequestException "In3.Error.InvalidTransactionRequestException")
- [Log](#T-In3-Eth1-Log "In3.Eth1.Log")
  - [Address](#P-In3-Eth1-Log-Address "In3.Eth1.Log.Address")
  - [BlockHash](#P-In3-Eth1-Log-BlockHash "In3.Eth1.Log.BlockHash")
  - [BlockNumber](#P-In3-Eth1-Log-BlockNumber "In3.Eth1.Log.BlockNumber")
  - [Data](#P-In3-Eth1-Log-Data "In3.Eth1.Log.Data")
  - [LogIndex](#P-In3-Eth1-Log-LogIndex "In3.Eth1.Log.LogIndex")
  - [Removed](#P-In3-Eth1-Log-Removed "In3.Eth1.Log.Removed")
  - [Topics](#P-In3-Eth1-Log-Topics "In3.Eth1.Log.Topics")
  - [TransactionHash](#P-In3-Eth1-Log-TransactionHash "In3.Eth1.Log.TransactionHash")
  - [TransactionIndex](#P-In3-Eth1-Log-TransactionIndex "In3.Eth1.Log.TransactionIndex")
  - [Type](#P-In3-Eth1-Log-Type "In3.Eth1.Log.Type")
- [LogFilter](#T-In3-Eth1-LogFilter "In3.Eth1.LogFilter")
  - [#ctor()](#M-In3-Eth1-LogFilter-#ctor "In3.Eth1.LogFilter.#ctor")
  - [Address](#P-In3-Eth1-LogFilter-Address "In3.Eth1.LogFilter.Address")
  - [BlockHash](#P-In3-Eth1-LogFilter-BlockHash "In3.Eth1.LogFilter.BlockHash")
  - [FromBlock](#P-In3-Eth1-LogFilter-FromBlock "In3.Eth1.LogFilter.FromBlock")
  - [ToBlock](#P-In3-Eth1-LogFilter-ToBlock "In3.Eth1.LogFilter.ToBlock")
  - [Topics](#P-In3-Eth1-LogFilter-Topics "In3.Eth1.LogFilter.Topics")
- [NodeConfiguration](#T-In3-Configuration-NodeConfiguration "In3.Configuration.NodeConfiguration")
  - [#ctor(config)](#M-In3-Configuration-NodeConfiguration-#ctor-In3-Configuration-ChainConfiguration- "In3.Configuration.NodeConfiguration.#ctor(In3.Configuration.ChainConfiguration)")
  - [Address](#P-In3-Configuration-NodeConfiguration-Address "In3.Configuration.NodeConfiguration.Address")
  - [Props](#P-In3-Configuration-NodeConfiguration-Props "In3.Configuration.NodeConfiguration.Props")
  - [Url](#P-In3-Configuration-NodeConfiguration-Url "In3.Configuration.NodeConfiguration.Url")
- [Proof](#T-In3-Configuration-Proof "In3.Configuration.Proof")
  - [Full](#P-In3-Configuration-Proof-Full "In3.Configuration.Proof.Full")
  - [None](#P-In3-Configuration-Proof-None "In3.Configuration.Proof.None")
  - [Standard](#P-In3-Configuration-Proof-Standard "In3.Configuration.Proof.Standard")
- [Props](#T-In3-Configuration-Props "In3.Configuration.Props")
  - [NodePropArchive](#F-In3-Configuration-Props-NodePropArchive "In3.Configuration.Props.NodePropArchive")
  - [NodePropBinary](#F-In3-Configuration-Props-NodePropBinary "In3.Configuration.Props.NodePropBinary")
  - [NodePropData](#F-In3-Configuration-Props-NodePropData "In3.Configuration.Props.NodePropData")
  - [NodePropHttp](#F-In3-Configuration-Props-NodePropHttp "In3.Configuration.Props.NodePropHttp")
  - [NodePropMinblockheight](#F-In3-Configuration-Props-NodePropMinblockheight "In3.Configuration.Props.NodePropMinblockheight")
  - [NodePropMultichain](#F-In3-Configuration-Props-NodePropMultichain "In3.Configuration.Props.NodePropMultichain")
  - [NodePropOnion](#F-In3-Configuration-Props-NodePropOnion "In3.Configuration.Props.NodePropOnion")
  - [NodePropProof](#F-In3-Configuration-Props-NodePropProof "In3.Configuration.Props.NodePropProof")
  - [NodePropSigner](#F-In3-Configuration-Props-NodePropSigner "In3.Configuration.Props.NodePropSigner")
  - [NodePropStats](#F-In3-Configuration-Props-NodePropStats "In3.Configuration.Props.NodePropStats")
- [RpcException](#T-In3-Error-RpcException "In3.Error.RpcException")
  - [Code](#P-In3-Error-RpcException-Code "In3.Error.RpcException.Code")
- [ScriptPubKey](#T-In3-Btc-ScriptPubKey "In3.Btc.ScriptPubKey")
  - [Addresses](#P-In3-Btc-ScriptPubKey-Addresses "In3.Btc.ScriptPubKey.Addresses")
  - [Asm](#P-In3-Btc-ScriptPubKey-Asm "In3.Btc.ScriptPubKey.Asm")
  - [Hex](#P-In3-Btc-ScriptPubKey-Hex "In3.Btc.ScriptPubKey.Hex")
  - [ReqSigs](#P-In3-Btc-ScriptPubKey-ReqSigs "In3.Btc.ScriptPubKey.ReqSigs")
  - [Type](#P-In3-Btc-ScriptPubKey-Type "In3.Btc.ScriptPubKey.Type")
- [ScriptSig](#T-In3-Btc-ScriptSig "In3.Btc.ScriptSig")
  - [Asm](#P-In3-Btc-ScriptSig-Asm "In3.Btc.ScriptSig.Asm")
  - [Hex](#P-In3-Btc-ScriptSig-Hex "In3.Btc.ScriptSig.Hex")
- [SignatureType](#T-In3-Crypto-SignatureType "In3.Crypto.SignatureType")
  - [EthSign](#P-In3-Crypto-SignatureType-EthSign "In3.Crypto.SignatureType.EthSign")
  - [Hash](#P-In3-Crypto-SignatureType-Hash "In3.Crypto.SignatureType.Hash")
  - [Raw](#P-In3-Crypto-SignatureType-Raw "In3.Crypto.SignatureType.Raw")
- [SignedData](#T-In3-Crypto-SignedData "In3.Crypto.SignedData")
  - [Message](#P-In3-Crypto-SignedData-Message "In3.Crypto.SignedData.Message")
  - [MessageHash](#P-In3-Crypto-SignedData-MessageHash "In3.Crypto.SignedData.MessageHash")
  - [R](#P-In3-Crypto-SignedData-R "In3.Crypto.SignedData.R")
  - [S](#P-In3-Crypto-SignedData-S "In3.Crypto.SignedData.S")
  - [Signature](#P-In3-Crypto-SignedData-Signature "In3.Crypto.SignedData.Signature")
  - [V](#P-In3-Crypto-SignedData-V "In3.Crypto.SignedData.V")
- [Signer](#T-In3-Crypto-Signer "In3.Crypto.Signer")
  - [CanSign(account)](#M-In3-Crypto-Signer-CanSign-System-String- "In3.Crypto.Signer.CanSign(System.String)")
  - [PrepareTransaction()](#M-In3-Crypto-Signer-PrepareTransaction-In3-Eth1-TransactionRequest- "In3.Crypto.Signer.PrepareTransaction(In3.Eth1.TransactionRequest)")
  - [Sign(data,account)](#M-In3-Crypto-Signer-Sign-System-String,System-String- "In3.Crypto.Signer.Sign(System.String,System.String)")
- [SimpleWallet](#T-In3-Crypto-SimpleWallet "In3.Crypto.SimpleWallet")
  - [#ctor(in3)](#M-In3-Crypto-SimpleWallet-#ctor-In3-IN3- "In3.Crypto.SimpleWallet.#ctor(In3.IN3)")
  - [AddRawKey(privateKey)](#M-In3-Crypto-SimpleWallet-AddRawKey-System-String- "In3.Crypto.SimpleWallet.AddRawKey(System.String)")
  - [CanSign(address)](#M-In3-Crypto-SimpleWallet-CanSign-System-String- "In3.Crypto.SimpleWallet.CanSign(System.String)")
  - [PrepareTransaction(tx)](#M-In3-Crypto-SimpleWallet-PrepareTransaction-In3-Eth1-TransactionRequest- "In3.Crypto.SimpleWallet.PrepareTransaction(In3.Eth1.TransactionRequest)")
  - [Sign(data,address)](#M-In3-Crypto-SimpleWallet-Sign-System-String,System-String- "In3.Crypto.SimpleWallet.Sign(System.String,System.String)")
- [Storage](#T-In3-Storage-Storage "In3.Storage.Storage")
  - [Clear()](#M-In3-Storage-Storage-Clear "In3.Storage.Storage.Clear")
  - [GetItem(key)](#M-In3-Storage-Storage-GetItem-System-String- "In3.Storage.Storage.GetItem(System.String)")
  - [SetItem(key,content)](#M-In3-Storage-Storage-SetItem-System-String,System-Byte[]- "In3.Storage.Storage.SetItem(System.String,System.Byte[])")
- [Transaction](#T-In3-Btc-Transaction "In3.Btc.Transaction")
- [Transaction](#T-In3-Eth1-Transaction "In3.Eth1.Transaction")
  - [Blockhash](#P-In3-Btc-Transaction-Blockhash "In3.Btc.Transaction.Blockhash")
  - [Blocktime](#P-In3-Btc-Transaction-Blocktime "In3.Btc.Transaction.Blocktime")
  - [Confirmations](#P-In3-Btc-Transaction-Confirmations "In3.Btc.Transaction.Confirmations")
  - [Hash](#P-In3-Btc-Transaction-Hash "In3.Btc.Transaction.Hash")
  - [Hex](#P-In3-Btc-Transaction-Hex "In3.Btc.Transaction.Hex")
  - [Locktime](#P-In3-Btc-Transaction-Locktime "In3.Btc.Transaction.Locktime")
  - [Size](#P-In3-Btc-Transaction-Size "In3.Btc.Transaction.Size")
  - [Time](#P-In3-Btc-Transaction-Time "In3.Btc.Transaction.Time")
  - [Txid](#P-In3-Btc-Transaction-Txid "In3.Btc.Transaction.Txid")
  - [Version](#P-In3-Btc-Transaction-Version "In3.Btc.Transaction.Version")
  - [Vin](#P-In3-Btc-Transaction-Vin "In3.Btc.Transaction.Vin")
  - [Vout](#P-In3-Btc-Transaction-Vout "In3.Btc.Transaction.Vout")
  - [Vsize](#P-In3-Btc-Transaction-Vsize "In3.Btc.Transaction.Vsize")
  - [Weight](#P-In3-Btc-Transaction-Weight "In3.Btc.Transaction.Weight")
  - [BlockHash](#P-In3-Eth1-Transaction-BlockHash "In3.Eth1.Transaction.BlockHash")
  - [BlockNumber](#P-In3-Eth1-Transaction-BlockNumber "In3.Eth1.Transaction.BlockNumber")
  - [ChainId](#P-In3-Eth1-Transaction-ChainId "In3.Eth1.Transaction.ChainId")
  - [Creates](#P-In3-Eth1-Transaction-Creates "In3.Eth1.Transaction.Creates")
  - [From](#P-In3-Eth1-Transaction-From "In3.Eth1.Transaction.From")
  - [Gas](#P-In3-Eth1-Transaction-Gas "In3.Eth1.Transaction.Gas")
  - [GasPrice](#P-In3-Eth1-Transaction-GasPrice "In3.Eth1.Transaction.GasPrice")
  - [Hash](#P-In3-Eth1-Transaction-Hash "In3.Eth1.Transaction.Hash")
  - [Input](#P-In3-Eth1-Transaction-Input "In3.Eth1.Transaction.Input")
  - [Nonce](#P-In3-Eth1-Transaction-Nonce "In3.Eth1.Transaction.Nonce")
  - [PublicKey](#P-In3-Eth1-Transaction-PublicKey "In3.Eth1.Transaction.PublicKey")
  - [R](#P-In3-Eth1-Transaction-R "In3.Eth1.Transaction.R")
  - [Raw](#P-In3-Eth1-Transaction-Raw "In3.Eth1.Transaction.Raw")
  - [S](#P-In3-Eth1-Transaction-S "In3.Eth1.Transaction.S")
  - [StandardV](#P-In3-Eth1-Transaction-StandardV "In3.Eth1.Transaction.StandardV")
  - [To](#P-In3-Eth1-Transaction-To "In3.Eth1.Transaction.To")
  - [TransactionIndex](#P-In3-Eth1-Transaction-TransactionIndex "In3.Eth1.Transaction.TransactionIndex")
  - [V](#P-In3-Eth1-Transaction-V "In3.Eth1.Transaction.V")
  - [Value](#P-In3-Eth1-Transaction-Value "In3.Eth1.Transaction.Value")
- [TransactionBlock](#T-In3-Eth1-TransactionBlock "In3.Eth1.TransactionBlock")
  - [Transactions](#P-In3-Eth1-TransactionBlock-Transactions "In3.Eth1.TransactionBlock.Transactions")
- [TransactionHashBlock](#T-In3-Eth1-TransactionHashBlock "In3.Eth1.TransactionHashBlock")
  - [Transactions](#P-In3-Eth1-TransactionHashBlock-Transactions "In3.Eth1.TransactionHashBlock.Transactions")
- [TransactionInput](#T-In3-Btc-TransactionInput "In3.Btc.TransactionInput")
  - [ScriptSig](#P-In3-Btc-TransactionInput-ScriptSig "In3.Btc.TransactionInput.ScriptSig")
  - [Sequence](#P-In3-Btc-TransactionInput-Sequence "In3.Btc.TransactionInput.Sequence")
  - [Txid](#P-In3-Btc-TransactionInput-Txid "In3.Btc.TransactionInput.Txid")
  - [Txinwitness](#P-In3-Btc-TransactionInput-Txinwitness "In3.Btc.TransactionInput.Txinwitness")
  - [Yout](#P-In3-Btc-TransactionInput-Yout "In3.Btc.TransactionInput.Yout")
- [TransactionOutput](#T-In3-Btc-TransactionOutput "In3.Btc.TransactionOutput")
  - [N](#P-In3-Btc-TransactionOutput-N "In3.Btc.TransactionOutput.N")
  - [ScriptPubKey](#P-In3-Btc-TransactionOutput-ScriptPubKey "In3.Btc.TransactionOutput.ScriptPubKey")
  - [Value](#P-In3-Btc-TransactionOutput-Value "In3.Btc.TransactionOutput.Value")
- [TransactionReceipt](#T-In3-Eth1-TransactionReceipt "In3.Eth1.TransactionReceipt")
  - [BlockHash](#P-In3-Eth1-TransactionReceipt-BlockHash "In3.Eth1.TransactionReceipt.BlockHash")
  - [BlockNumber](#P-In3-Eth1-TransactionReceipt-BlockNumber "In3.Eth1.TransactionReceipt.BlockNumber")
  - [ContractAddress](#P-In3-Eth1-TransactionReceipt-ContractAddress "In3.Eth1.TransactionReceipt.ContractAddress")
  - [From](#P-In3-Eth1-TransactionReceipt-From "In3.Eth1.TransactionReceipt.From")
  - [GasUsed](#P-In3-Eth1-TransactionReceipt-GasUsed "In3.Eth1.TransactionReceipt.GasUsed")
  - [Logs](#P-In3-Eth1-TransactionReceipt-Logs "In3.Eth1.TransactionReceipt.Logs")
  - [LogsBloom](#P-In3-Eth1-TransactionReceipt-LogsBloom "In3.Eth1.TransactionReceipt.LogsBloom")
  - [Root](#P-In3-Eth1-TransactionReceipt-Root "In3.Eth1.TransactionReceipt.Root")
  - [Status](#P-In3-Eth1-TransactionReceipt-Status "In3.Eth1.TransactionReceipt.Status")
  - [To](#P-In3-Eth1-TransactionReceipt-To "In3.Eth1.TransactionReceipt.To")
  - [TransactionHash](#P-In3-Eth1-TransactionReceipt-TransactionHash "In3.Eth1.TransactionReceipt.TransactionHash")
  - [TransactionIndex](#P-In3-Eth1-TransactionReceipt-TransactionIndex "In3.Eth1.TransactionReceipt.TransactionIndex")
- [TransactionRequest](#T-In3-Eth1-TransactionRequest "In3.Eth1.TransactionRequest")
  - [Data](#P-In3-Eth1-TransactionRequest-Data "In3.Eth1.TransactionRequest.Data")
  - [From](#P-In3-Eth1-TransactionRequest-From "In3.Eth1.TransactionRequest.From")
  - [Function](#P-In3-Eth1-TransactionRequest-Function "In3.Eth1.TransactionRequest.Function")
  - [Gas](#P-In3-Eth1-TransactionRequest-Gas "In3.Eth1.TransactionRequest.Gas")
  - [GasPrice](#P-In3-Eth1-TransactionRequest-GasPrice "In3.Eth1.TransactionRequest.GasPrice")
  - [Nonce](#P-In3-Eth1-TransactionRequest-Nonce "In3.Eth1.TransactionRequest.Nonce")
  - [Params](#P-In3-Eth1-TransactionRequest-Params "In3.Eth1.TransactionRequest.Params")
  - [To](#P-In3-Eth1-TransactionRequest-To "In3.Eth1.TransactionRequest.To")
  - [Value](#P-In3-Eth1-TransactionRequest-Value "In3.Eth1.TransactionRequest.Value")
- [Transport](#T-In3-Transport-Transport "In3.Transport.Transport")
  - [Handle(url,payload)](#M-In3-Transport-Transport-Handle-System-String,System-String- "In3.Transport.Transport.Handle(System.String,System.String)")

<a name='T-In3-Crypto-Account'></a>

### Account `type`

In3.Crypto

Composite entity that holds address and public key. It represents and Ethereum acount. Entity returned from [EcRecover](#M-In3-Crypto-Api-EcRecover-System-String,System-String,In3-Crypto-SignatureType- "In3.Crypto.Api.EcRecover(System.String,System.String,In3.Crypto.SignatureType)").

<a name='P-In3-Crypto-Account-Address'></a>

#### Address `property`

The address.

<a name='P-In3-Crypto-Account-PublicKey'></a>

#### PublicKey `property`

The public key.

<a name='T-In3-Btc-Api'></a>

### Api `type`

In3.Btc

API for handling BitCoin data. Use it when connected to [Btc](#F-In3-Chain-Btc "In3.Chain.Btc").

<a name='T-In3-Crypto-Api'></a>

### Api `type`

In3.Crypto

Class that exposes utility methods for cryptographic utilities. Relies on [IN3](#T-In3-IN3 "In3.IN3") functionality.

<a name='T-In3-Eth1-Api'></a>

### Api `type`

In3.Eth1

Module based on Ethereum's api and web3. Works as a general parent for all Ethereum-specific operations.

<a name='T-In3-Ipfs-Api'></a>

### Api `type`

In3.Ipfs

API for ipfs realted methods. To be used along with [Ipfs](#F-In3-Chain-Ipfs "In3.Chain.Ipfs") on [IN3](#T-In3-IN3 "In3.IN3"). Ipfs stands for and is a peer-to-peer hypermedia protocol designed to make the web faster, safer, and more open.

<a name='M-In3-Btc-Api-GetBlockBytes-System-String-'></a>

#### GetBlockBytes(blockHash) `method`

Retrieves the serialized block in bytes.

###### Returns

The bytes of the block.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **blockHash** - The hash of the Block.

###### Example

```
byte[] blockBytes = in3.Btc.GetBlockBytes("000000000000000000064ba7512ecc70cabd7ed17e31c06f2205d5ecdadd6d22");
```

<a name='M-In3-Btc-Api-GetBlockHeader-System-String-'></a>

#### GetBlockHeader(blockHash) `method`

Retrieves the blockheader.

###### Returns

The Block header.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **blockHash** - The hash of the Block.

###### Example

```
BlockHeader header = in3.Btc.GetBlockHeader("0000000000000000000cd3c5d7638014e78a5fba33be5fa5cb10ef9f03d99e60");
```

<a name='M-In3-Btc-Api-GetBlockHeaderBytes-System-String-'></a>

#### GetBlockHeaderBytes(blockHash) `method`

Retrieves the byte array representing teh serialized blockheader data.

###### Returns

The Block header in bytes.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **blockHash** - The hash of the Block.

###### Example

```
byte[] header = in3.Btc.GetBlockHeaderBytes("0000000000000000000cd3c5d7638014e78a5fba33be5fa5cb10ef9f03d99e60");
```

<a name='M-In3-Btc-Api-GetBlockWithTxData-System-String-'></a>

#### GetBlockWithTxData(blockHash) `method`

Retrieves the block including the full transaction data. Use [GetBlockWithTxIds](#M-In3-Btc-Api-GetBlockWithTxIds-System-String- "In3.Btc.Api.GetBlockWithTxIds(System.String)") for only the transaction ids.

###### Returns

The block of type [Block\`1](#T-In3-Btc-Block`1 "In3.Btc.Block`1").

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **blockHash** - The hash of the Block.

###### Example

```
Block{Transaction} block = in3.Btc.GetBlockWithTxData("000000000000000000064ba7512ecc70cabd7ed17e31c06f2205d5ecdadd6d22");
Transaction t1 = block.Tx[0];
```

<a name='M-In3-Btc-Api-GetBlockWithTxIds-System-String-'></a>

#### GetBlockWithTxIds(blockHash) `method`

Retrieves the block including only transaction ids. Use [GetBlockWithTxData](#M-In3-Btc-Api-GetBlockWithTxData-System-String- "In3.Btc.Api.GetBlockWithTxData(System.String)") for the full transaction data.

###### Returns

The block of type [Block\`1](#T-In3-Btc-Block`1 "In3.Btc.Block`1").

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **blockHash** - The hash of the Block.

###### Example

```
Block{string} block = in3.Btc.GetBlockWithTxIds("000000000000000000064ba7512ecc70cabd7ed17e31c06f2205d5ecdadd6d22");
string t1 = block.Tx[0];
```

<a name='M-In3-Btc-Api-GetTransaction-System-String-'></a>

#### GetTransaction(txid) `method`

Retrieves the transaction and returns the data as json.

###### Returns

The transaction object.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **txid** - The transaction Id.

###### Example

```
Transaction desiredTransaction = in3.Btc.GetTransaction("1427c7d1698e61afe061950226f1c149990b8c1e1b157320b0c4acf7d6b5605d");
```

<a name='M-In3-Btc-Api-GetTransactionBytes-System-String-'></a>

#### GetTransactionBytes(txid) `method`

Retrieves the serialized transaction (bytes).

###### Returns

The byte array for the Transaction.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **txid** - The transaction Id.

###### Example

```
byte[] serializedTransaction = in3.Btc.GetTransactionBytes("1427c7d1698e61afe061950226f1c149990b8c1e1b157320b0c4acf7d6b5605d");
```

<a name='M-In3-Crypto-Api-DecryptKey-System-String,System-String-'></a>

#### DecryptKey(pk,passphrase) `method`

Decryot an encrypted private key.

###### Returns

Decrypted key.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **pk** - Private key.
- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **passphrase** - Passphrase whose `pk`.

<a name='M-In3-Crypto-Api-EcRecover-System-String,System-String,In3-Crypto-SignatureType-'></a>

#### EcRecover(signedData,signature,signatureType) `method`

Recovers the account associated with the signed data.

###### Returns

The account.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **signedData** - Data that was signed with.
- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **signature** - The signature.
- [In3.Crypto.SignatureType](#T-In3-Crypto-SignatureType "In3.Crypto.SignatureType") **signatureType** - One of [SignatureType](#T-In3-Crypto-SignatureType "In3.Crypto.SignatureType").

<a name='M-In3-Crypto-Api-Pk2Address-System-String-'></a>

#### Pk2Address(pk) `method`

Derives an address from the given private (`pk`) key using SHA-3 algorithm.

###### Returns

The address.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **pk** - Private key.

<a name='M-In3-Crypto-Api-Pk2Public-System-String-'></a>

#### Pk2Public(pk) `method`

Derives public key from the given private (`pk`) key using SHA-3 algorithm.

###### Returns

The public key.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **pk** - Private key.

<a name='M-In3-Crypto-Api-Sha3-System-String-'></a>

#### Sha3(data) `method`

Hash the input data using sha3 algorithm.

###### Returns

Hashed output.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **data** - Content to be hashed.

<a name='M-In3-Crypto-Api-SignData-System-String,System-String,In3-Crypto-SignatureType-'></a>

#### SignData(msg,pk,sigType) `method`

Signs the data `msg` with a given private key. Refer to [SignedData](#T-In3-Crypto-SignedData "In3.Crypto.SignedData") for more information.

###### Returns

The signed data.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **msg** - Data to be signed.
- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **pk** - Private key.
- [In3.Crypto.SignatureType](#T-In3-Crypto-SignatureType "In3.Crypto.SignatureType") **sigType** - Type of signature, one of [SignatureType](#T-In3-Crypto-SignatureType "In3.Crypto.SignatureType").

<a name='M-In3-Eth1-Api-AbiDecode-System-String,System-String-'></a>

#### AbiDecode(signature,encodedData) `method`

ABI decoder. Used to parse rpc responses from the EVM.
Based on the Solidity specification .

###### Returns

The decoded argugments for the function call given the encded data.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **signature** - Function signature i.e. or . In case of the latter, the function signature will be ignored and only the return types will be parsed.
- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **encodedData** - Abi encoded values. Usually the string returned from a rpc to the EVM.

<a name='M-In3-Eth1-Api-AbiEncode-System-String,System-Object[]-'></a>

#### AbiEncode(signature,args) `method`

ABI encoder. Used to serialize a rpc to the EVM.
Based on the Solidity specification .
Note: Parameters refers to the list of variables in a method declaration.
Arguments are the actual values that are passed in when the method is invoked.
When you invoke a method, the arguments used must match the declaration's parameters in type and order.

###### Returns

The encoded data.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **signature** - Function signature, with parameters. i.e. , can contain the return types but will be ignored.
- [System.Object[]](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Object[] "System.Object[]") **args** - Function parameters, in the same order as in passed on to .

<a name='M-In3-Eth1-Api-BlockNumber'></a>

#### BlockNumber() `method`

Returns the number of the most recent block the in3 network can collect signatures to verify.
Can be changed by [ReplaceLatestBlock](#P-In3-Configuration-ClientConfiguration-ReplaceLatestBlock "In3.Configuration.ClientConfiguration.ReplaceLatestBlock").
If you need the very latest block, change [SignatureCount](#P-In3-Configuration-ClientConfiguration-SignatureCount "In3.Configuration.ClientConfiguration.SignatureCount") to `0`.

###### Returns

The number of the block.

###### Parameters

This method has no parameters.

<a name='M-In3-Eth1-Api-Call-In3-Eth1-TransactionRequest,System-Numerics-BigInteger-'></a>

#### Call(request,blockNumber) `method`

Calls a smart-contract method. Will be executed locally by Incubed's EVM or signed and sent over to save the state changes.
Check https://ethereum.stackexchange.com/questions/3514/how-to-call-a-contract-method-using-the-eth-call-json-rpc-api for more.

###### Returns

Ddecoded result. If only one return value is expected the Object will be returned, if not an array of objects will be the result.

###### Parameters

- [In3.Eth1.TransactionRequest](#T-In3-Eth1-TransactionRequest "In3.Eth1.TransactionRequest") **request** - The transaction request to be processed.
- [System.Numerics.BigInteger](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Numerics.BigInteger "System.Numerics.BigInteger") **blockNumber** - Block number or [Latest](#P-In3-BlockParameter-Latest "In3.BlockParameter.Latest") or [Earliest](#P-In3-BlockParameter-Earliest "In3.BlockParameter.Earliest").

<a name='M-In3-Eth1-Api-ChecksumAddress-System-String,System-Nullable{System-Boolean}-'></a>

#### ChecksumAddress(address,shouldUseChainId) `method`

Will convert an upper or lowercase Ethereum `address` to a checksum address, that uses case to encode values.
See [EIP55](https://github.com/ethereum/EIPs/blob/master/EIPS/eip-55.md).

###### Returns

EIP-55 compliant, mixed-case address.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **address** - Ethereum address.
- [System.Nullable{System.Boolean}](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Nullable "System.Nullable{System.Boolean}") **shouldUseChainId** - If `true`, the chain id is integrated as well. Default being `false`.

<a name='M-In3-Eth1-Api-Ens-System-String,In3-ENSParameter-'></a>

#### Ens(name,type) `method`

Resolves ENS domain name.

###### Returns

The resolved entity for the domain.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **name** - ENS domain name.
- [In3.ENSParameter](#T-In3-ENSParameter "In3.ENSParameter") **type** - One of [ENSParameter](#T-In3-ENSParameter "In3.ENSParameter").

###### Remarks

The actual semantics of the returning value changes according to `type`.

<a name='M-In3-Eth1-Api-EstimateGas-In3-Eth1-TransactionRequest,System-Numerics-BigInteger-'></a>

#### EstimateGas(request,blockNumber) `method`

Gas estimation for transaction. Used to fill transaction.gas field. Check RawTransaction docs for more on gas.

###### Returns

Estimated gas in Wei.

###### Parameters

- [In3.Eth1.TransactionRequest](#T-In3-Eth1-TransactionRequest "In3.Eth1.TransactionRequest") **request** - The transaction request whose cost will be estimated.
- [System.Numerics.BigInteger](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Numerics.BigInteger "System.Numerics.BigInteger") **blockNumber** - Block number or [Latest](#P-In3-BlockParameter-Latest "In3.BlockParameter.Latest") or [Earliest](#P-In3-BlockParameter-Earliest "In3.BlockParameter.Earliest").

<a name='M-In3-Eth1-Api-GetBalance-System-String,System-Numerics-BigInteger-'></a>

#### GetBalance(address,blockNumber) `method`

Returns the balance of the account of given `address`.

###### Returns

The current balance in wei.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **address** - Address to check for balance.
- [System.Numerics.BigInteger](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Numerics.BigInteger "System.Numerics.BigInteger") **blockNumber** - Block number or [Latest](#P-In3-BlockParameter-Latest "In3.BlockParameter.Latest") or [Earliest](#P-In3-BlockParameter-Earliest "In3.BlockParameter.Earliest").

<a name='M-In3-Eth1-Api-GetBlockByHash-System-String,System-Boolean-'></a>

#### GetBlockByHash(blockHash,shouldIncludeTransactions) `method`

Blocks can be identified by root hash of the block merkle tree (this), or sequential number in which it was mined [GetBlockByNumber](#M-In3-Eth1-Api-GetBlockByNumber-System-Numerics-BigInteger,System-Boolean- "In3.Eth1.Api.GetBlockByNumber(System.Numerics.BigInteger,System.Boolean)").

###### Returns

The [Block](#T-In3-Eth1-Block "In3.Eth1.Block") of the requested (if exists).

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **blockHash** - Desired block hash.
- [System.Boolean](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Boolean "System.Boolean") **shouldIncludeTransactions** - If true, returns the full transaction objects, otherwise only its hashes. The default value is `false`.

###### Remarks

Returning [Block](#T-In3-Eth1-Block "In3.Eth1.Block") must be cast to [TransactionBlock](#T-In3-Eth1-TransactionBlock "In3.Eth1.TransactionBlock") or [TransactionHashBlock](#T-In3-Eth1-TransactionHashBlock "In3.Eth1.TransactionHashBlock") to access the transaction data.

<a name='M-In3-Eth1-Api-GetBlockByNumber-System-Numerics-BigInteger,System-Boolean-'></a>

#### GetBlockByNumber(blockNumber,shouldIncludeTransactions) `method`

Blocks can be identified by sequential number in which it was mined, or root hash of the block merkle tree [GetBlockByHash](#M-In3-Eth1-Api-GetBlockByHash-System-String,System-Boolean- "In3.Eth1.Api.GetBlockByHash(System.String,System.Boolean)").

###### Returns

The [Block](#T-In3-Eth1-Block "In3.Eth1.Block") of the requested (if exists).

###### Parameters

- [System.Numerics.BigInteger](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Numerics.BigInteger "System.Numerics.BigInteger") **blockNumber** - Desired block number or [Latest](#P-In3-BlockParameter-Latest "In3.BlockParameter.Latest") or [Earliest](#P-In3-BlockParameter-Earliest "In3.BlockParameter.Earliest").
- [System.Boolean](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Boolean "System.Boolean") **shouldIncludeTransactions** - If `true`, returns the full transaction objects, otherwise only its hashes. The default value is `true`.

###### Example

```
TransactionBlock latest = (TransactionBlock) _client.Eth1.GetBlockByNumber(BlockParameter.Latest, true);
TransactionHashBlock earliest = (TransactionHashBlock) _client.Eth1.GetBlockByNumber(BlockParameter.Earliest, false);
```

###### Remarks

Returning [Block](#T-In3-Eth1-Block "In3.Eth1.Block") must be cast to [TransactionBlock](#T-In3-Eth1-TransactionBlock "In3.Eth1.TransactionBlock") or [TransactionHashBlock](#T-In3-Eth1-TransactionHashBlock "In3.Eth1.TransactionHashBlock") to access the transaction data.

<a name='M-In3-Eth1-Api-GetBlockTransactionCountByHash-System-String-'></a>

#### GetBlockTransactionCountByHash(blockHash) `method`

The total transactions on a block. See also [GetBlockTransactionCountByNumber](#M-In3-Eth1-Api-GetBlockTransactionCountByNumber-System-Numerics-BigInteger- "In3.Eth1.Api.GetBlockTransactionCountByNumber(System.Numerics.BigInteger)").

###### Returns

The number (count) of [Transaction](#T-In3-Eth1-Transaction "In3.Eth1.Transaction").

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **blockHash** - Desired block hash.

<a name='M-In3-Eth1-Api-GetBlockTransactionCountByNumber-System-Numerics-BigInteger-'></a>

#### GetBlockTransactionCountByNumber(blockNumber) `method`

The total transactions on a block. See also [GetBlockTransactionCountByHash](#M-In3-Eth1-Api-GetBlockTransactionCountByHash-System-String- "In3.Eth1.Api.GetBlockTransactionCountByHash(System.String)").

###### Returns

The number (count) of [Transaction](#T-In3-Eth1-Transaction "In3.Eth1.Transaction").

###### Parameters

- [System.Numerics.BigInteger](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Numerics.BigInteger "System.Numerics.BigInteger") **blockNumber** - Block number or [Latest](#P-In3-BlockParameter-Latest "In3.BlockParameter.Latest") or [Earliest](#P-In3-BlockParameter-Earliest "In3.BlockParameter.Earliest").

<a name='M-In3-Eth1-Api-GetChainId'></a>

#### GetChainId() `method`

Get the [Chain](#T-In3-Chain "In3.Chain") which the client is currently connected to.

###### Returns

The [Chain](#T-In3-Chain "In3.Chain").

###### Parameters

This method has no parameters.

<a name='M-In3-Eth1-Api-GetCode-System-String,System-Numerics-BigInteger-'></a>

#### GetCode(address,blockNumber) `method`

Smart-Contract bytecode in hexadecimal. If the account is a simple wallet the function will return '0x'.

###### Returns

Smart-Contract bytecode in hexadecimal.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **address** - Ethereum address.
- [System.Numerics.BigInteger](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Numerics.BigInteger "System.Numerics.BigInteger") **blockNumber** - Block number or [Latest](#P-In3-BlockParameter-Latest "In3.BlockParameter.Latest") or [Earliest](#P-In3-BlockParameter-Earliest "In3.BlockParameter.Earliest").

<a name='M-In3-Eth1-Api-GetFilterChangesFromLogs-System-Int64-'></a>

#### GetFilterChangesFromLogs(filterId) `method`

Retrieve the logs for a certain filter. Logs marks changes of state on the chan for events. Equivalent to [GetFilterLogs](#M-In3-Eth1-Api-GetFilterLogs-System-Int64- "In3.Eth1.Api.GetFilterLogs(System.Int64)").

###### Returns

Array of logs which occurred since last poll.

###### Parameters

- [System.Int64](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Int64 "System.Int64") **filterId** - Id returned during the filter creation.

###### Remarks

Since the return is the since last poll, executing this multiple times changes the state making this a "non-idempotent" getter.

<a name='M-In3-Eth1-Api-GetFilterLogs-System-Int64-'></a>

#### GetFilterLogs(filterId) `method`

Retrieve the logs for a certain filter. Logs marks changes of state on the blockchain for events. Equivalent to [GetFilterChangesFromLogs](#M-In3-Eth1-Api-GetFilterChangesFromLogs-System-Int64- "In3.Eth1.Api.GetFilterChangesFromLogs(System.Int64)").

###### Returns

Array of logs which occurred since last poll.

###### Parameters

- [System.Int64](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Int64 "System.Int64") **filterId** - Id returned during the filter creation.

###### Remarks

Since the return is the `Log[]` since last poll, executing this multiple times changes the state making this a "non-idempotent" getter.

<a name='M-In3-Eth1-Api-GetGasPrice'></a>

#### GetGasPrice() `method`

The current gas price in Wei (1 ETH equals 1000000000000000000 Wei ).

###### Returns

The gas price.

###### Parameters

This method has no parameters.

<a name='M-In3-Eth1-Api-GetLogs-In3-Eth1-LogFilter-'></a>

#### GetLogs(filter) `method`

Retrieve the logs for a certain filter. Logs marks changes of state on the blockchain for events. Unlike [GetFilterChangesFromLogs](#M-In3-Eth1-Api-GetFilterChangesFromLogs-System-Int64- "In3.Eth1.Api.GetFilterChangesFromLogs(System.Int64)") or [GetFilterLogs](#M-In3-Eth1-Api-GetFilterLogs-System-Int64- "In3.Eth1.Api.GetFilterLogs(System.Int64)") this is made to be used in a non-incremental manner (aka no poll) and will return the Logs that satisfy the filter condition.

###### Returns

Logs that satisfy the `filter`.

###### Parameters

- [In3.Eth1.LogFilter](#T-In3-Eth1-LogFilter "In3.Eth1.LogFilter") **filter** - Filter conditions.

<a name='M-In3-Eth1-Api-GetStorageAt-System-String,System-Numerics-BigInteger,System-Numerics-BigInteger-'></a>

#### GetStorageAt(address,position,blockNumber) `method`

Stored value in designed position at a given `address`. Storage can be used to store a smart contract state, constructor or just any data.
Each contract consists of a EVM bytecode handling the execution and a storage to save the state of the contract.

###### Returns

Stored value in designed position.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **address** - Ethereum account address.
- [System.Numerics.BigInteger](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Numerics.BigInteger "System.Numerics.BigInteger") **position** - Position index, 0x0 up to 100.
- [System.Numerics.BigInteger](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Numerics.BigInteger "System.Numerics.BigInteger") **blockNumber** - Block number or [Latest](#P-In3-BlockParameter-Latest "In3.BlockParameter.Latest") or [Earliest](#P-In3-BlockParameter-Earliest "In3.BlockParameter.Earliest").

<a name='M-In3-Eth1-Api-GetTransactionByBlockHashAndIndex-System-String,System-Int32-'></a>

#### GetTransactionByBlockHashAndIndex(blockHash,index) `method`

Transactions can be identified by root hash of the transaction merkle tree (this) or by its position in the block transactions merkle tree.
Every transaction hash is unique for the whole chain. Collision could in theory happen, chances are 67148E-63%.
See also [GetTransactionByBlockNumberAndIndex](#M-In3-Eth1-Api-GetTransactionByBlockNumberAndIndex-System-Numerics-BigInteger,System-Int32- "In3.Eth1.Api.GetTransactionByBlockNumberAndIndex(System.Numerics.BigInteger,System.Int32)").

###### Returns

The [Transaction](#T-In3-Eth1-Transaction "In3.Eth1.Transaction") (if it exists).

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **blockHash** - Desired block hash.
- [System.Int32](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Int32 "System.Int32") **index** - The index of the [Transaction](#T-In3-Eth1-Transaction "In3.Eth1.Transaction") in a [Block](#T-In3-Eth1-Block "In3.Eth1.Block")

<a name='M-In3-Eth1-Api-GetTransactionByBlockNumberAndIndex-System-Numerics-BigInteger,System-Int32-'></a>

#### GetTransactionByBlockNumberAndIndex(blockNumber,index) `method`

Transactions can be identified by root hash of the transaction merkle tree (this) or by its position in the block transactions merkle tree.
Every transaction hash is unique for the whole chain. Collision could in theory happen, chances are 67148E-63%.

###### Returns

The [Transaction](#T-In3-Eth1-Transaction "In3.Eth1.Transaction") (if it exists).

###### Parameters

- [System.Numerics.BigInteger](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Numerics.BigInteger "System.Numerics.BigInteger") **blockNumber** - Block number or [Latest](#P-In3-BlockParameter-Latest "In3.BlockParameter.Latest") or [Earliest](#P-In3-BlockParameter-Earliest "In3.BlockParameter.Earliest").
- [System.Int32](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Int32 "System.Int32") **index** - The index of the [Transaction](#T-In3-Eth1-Transaction "In3.Eth1.Transaction") in a [Block](#T-In3-Eth1-Block "In3.Eth1.Block")

<a name='M-In3-Eth1-Api-GetTransactionByHash-System-String-'></a>

#### GetTransactionByHash(transactionHash) `method`

Transactions can be identified by root hash of the transaction merkle tree (this) or by its position in the block transactions merkle tree.
Every transaction hash is unique for the whole chain. Collision could in theory happen, chances are 67148E-63%.

###### Returns

The [Transaction](#T-In3-Eth1-Transaction "In3.Eth1.Transaction") (if it exists).

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **transactionHash** - Desired transaction hash.

<a name='M-In3-Eth1-Api-GetTransactionCount-System-String,System-Numerics-BigInteger-'></a>

#### GetTransactionCount(address,blockNumber) `method`

Number of transactions mined from this `address`. Used to set transaction nonce.
Nonce is a value that will make a transaction fail in case it is different from (transaction count + 1).
It exists to mitigate replay attacks.

###### Returns

Number of transactions mined from this address.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **address** - Ethereum account address.
- [System.Numerics.BigInteger](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Numerics.BigInteger "System.Numerics.BigInteger") **blockNumber** - Block number or [Latest](#P-In3-BlockParameter-Latest "In3.BlockParameter.Latest") or [Earliest](#P-In3-BlockParameter-Earliest "In3.BlockParameter.Earliest").

<a name='M-In3-Eth1-Api-GetTransactionReceipt-System-String-'></a>

#### GetTransactionReceipt(transactionHash) `method`

After a transaction is received the by the client, it returns the transaction hash. With it, it is possible to gather the receipt, once a miner has mined and it is part of an acknowledged block. Because how it is possible, in distributed systems, that data is asymmetric in different parts of the system, the transaction is only "final" once a certain number of blocks was mined after it, and still it can be possible that the transaction is discarded after some time. But, in general terms, it is accepted that after 6 to 8 blocks from latest, that it is very likely that the transaction will stay in the chain.

###### Returns

The mined transaction data including event logs.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **transactionHash** - Desired transaction hash.

<a name='M-In3-Eth1-Api-GetUncleByBlockNumberAndIndex-System-Numerics-BigInteger,System-Int32-'></a>

#### GetUncleByBlockNumberAndIndex(blockNumber,position) `method`

Retrieve the of uncle of a block for the given `blockNumber` and a position. Uncle blocks are valid blocks and are mined in a genuine manner, but get rejected from the main blockchain.

###### Returns

The uncle block.

###### Parameters

- [System.Numerics.BigInteger](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Numerics.BigInteger "System.Numerics.BigInteger") **blockNumber** - Block number or [Latest](#P-In3-BlockParameter-Latest "In3.BlockParameter.Latest") or [Earliest](#P-In3-BlockParameter-Earliest "In3.BlockParameter.Earliest").
- [System.Int32](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Int32 "System.Int32") **position** - Position of the block.

<a name='M-In3-Eth1-Api-GetUncleCountByBlockHash-System-String-'></a>

#### GetUncleCountByBlockHash(blockHash) `method`

Retrieve the total of uncles of a block for the given `blockHash`. Uncle blocks are valid blocks and are mined in a genuine manner, but get rejected from the main blockchain.
See [GetUncleCountByBlockNumber](#M-In3-Eth1-Api-GetUncleCountByBlockNumber-System-Numerics-BigInteger- "In3.Eth1.Api.GetUncleCountByBlockNumber(System.Numerics.BigInteger)").

###### Returns

The number of uncles in a block.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **blockHash** - Desired block hash.

<a name='M-In3-Eth1-Api-GetUncleCountByBlockNumber-System-Numerics-BigInteger-'></a>

#### GetUncleCountByBlockNumber(blockNumber) `method`

Retrieve the total of uncles of a block for the given `blockNumber`. Uncle blocks are valid and are mined in a genuine manner, but get rejected from the main blockchain.
See [GetUncleCountByBlockHash](#M-In3-Eth1-Api-GetUncleCountByBlockHash-System-String- "In3.Eth1.Api.GetUncleCountByBlockHash(System.String)").

###### Returns

The number of uncles in a block.

###### Parameters

- [System.Numerics.BigInteger](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Numerics.BigInteger "System.Numerics.BigInteger") **blockNumber** - Block number or [Latest](#P-In3-BlockParameter-Latest "In3.BlockParameter.Latest") or [Earliest](#P-In3-BlockParameter-Earliest "In3.BlockParameter.Earliest").

<a name='M-In3-Eth1-Api-NewBlockFilter'></a>

#### NewBlockFilter() `method`

Creates a filter in the node, to notify when a new block arrives. To check if the state has changed, call [GetFilterChangesFromLogs](#M-In3-Eth1-Api-GetFilterChangesFromLogs-System-Int64- "In3.Eth1.Api.GetFilterChangesFromLogs(System.Int64)").
Filters are event catchers running on the Ethereum Client. Incubed has a client-side implementation.
An event will be stored in case it is within to and from blocks, or in the block of blockhash, contains a
transaction to the designed address, and has a word listed on topics.

###### Returns

The filter id.

###### Parameters

This method has no parameters.

###### Remarks

Use the returned filter id to perform other filter operations.

<a name='M-In3-Eth1-Api-NewLogFilter-In3-Eth1-LogFilter-'></a>

#### NewLogFilter(filter) `method`

Creates a filter object, based on filter options, to notify when the state changes (logs). To check if the state has changed, call [GetFilterChangesFromLogs](#M-In3-Eth1-Api-GetFilterChangesFromLogs-System-Int64- "In3.Eth1.Api.GetFilterChangesFromLogs(System.Int64)").
Filters are event catchers running on the Ethereum Client. Incubed has a client-side implementation.
An event will be stored in case it is within to and from blocks, or in the block of blockhash, contains a
transaction to the designed address, and has a word listed on topics.

###### Returns

The filter id.

###### Parameters

- [In3.Eth1.LogFilter](#T-In3-Eth1-LogFilter "In3.Eth1.LogFilter") **filter** - Model that holds the data for the filter creation.

###### Remarks

Use the returned filter id to perform other filter operations.

<a name='M-In3-Eth1-Api-SendRawTransaction-System-String-'></a>

#### SendRawTransaction(transactionData) `method`

Sends a signed and encoded transaction.

###### Returns

Transaction hash, used to get the receipt and check if the transaction was mined.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **transactionData** - Signed keccak hash of the serialized transaction.

###### Remarks

Client will add the other required fields, gas and chaind id.

<a name='M-In3-Eth1-Api-SendTransaction-In3-Eth1-TransactionRequest-'></a>

#### SendTransaction(tx) `method`

Signs and sends the assigned transaction. The [Signer](#T-In3-Crypto-Signer "In3.Crypto.Signer") used to sign the transaction is the one set by [Signer](#P-In3-IN3-Signer "In3.IN3.Signer").
Transactions change the state of an account, just the balance, or additionally, the storage and the code.
Every transaction has a cost, gas, paid in Wei. The transaction gas is calculated over estimated gas times the gas cost, plus an additional miner fee, if the sender wants to be sure that the transaction will be mined in the latest block.
See [SendTransactionAndWait](#M-In3-Eth1-Api-SendTransactionAndWait-In3-Eth1-TransactionRequest- "In3.Eth1.Api.SendTransactionAndWait(In3.Eth1.TransactionRequest)") to wait for the [TransactionReceipt](#T-In3-Eth1-TransactionReceipt "In3.Eth1.TransactionReceipt") in the same call.

###### Returns

Transaction hash, used to get the receipt and check if the transaction was mined.

###### Parameters

- [In3.Eth1.TransactionRequest](#T-In3-Eth1-TransactionRequest "In3.Eth1.TransactionRequest") **tx** - All information needed to perform a transaction.

###### Example

```
 SimpleWallet wallet = (SimpleWallet) client.Signer;
 TransactionRequest tx = new TransactionRequest();
 tx.From = wallet.AddRawKey(pk);;
 tx.To = "0x3940256B93c4BE0B1d5931A6A036608c25706B0c";
 tx.Gas = 21000;
 tx.Value = 100000000;
 client.Eth1.SendTransaction(tx);
```

<a name='M-In3-Eth1-Api-SendTransactionAndWait-In3-Eth1-TransactionRequest-'></a>

#### SendTransactionAndWait(tx) `method`

Signs and sends the assigned transaction. The [Signer](#T-In3-Crypto-Signer "In3.Crypto.Signer") used to sign the transaction is the one set by [Signer](#P-In3-IN3-Signer "In3.IN3.Signer").
Transactions change the state of an account, just the balance, or additionally, the storage and the code.
Every transaction has a cost, gas, paid in Wei. The transaction gas is calculated over estimated gas times the gas cost, plus an additional miner fee, if the sender wants to be sure that the transaction will be mined in the latest block.
In this particular case, the transaction will wait the receipt. See [SendTransaction](#M-In3-Eth1-Api-SendTransaction-In3-Eth1-TransactionRequest- "In3.Eth1.Api.SendTransaction(In3.Eth1.TransactionRequest)") if that is not desirable.

###### Returns

The receipt associated with the performed transaction.

###### Parameters

- [In3.Eth1.TransactionRequest](#T-In3-Eth1-TransactionRequest "In3.Eth1.TransactionRequest") **tx** - All information needed to perform a transaction.

###### Example

```
 SimpleWallet wallet = (SimpleWallet) client.Signer;
 TransactionRequest tx = new TransactionRequest();
 tx.From = wallet.AddRawKey(pk);;
 tx.To = "0x3940256B93c4BE0B1d5931A6A036608c25706B0c";
 tx.Gas = 21000;
 tx.Value = 100000000;
 client.Eth1.SendTransactionAndWait(tx);
```

<a name='M-In3-Eth1-Api-UninstallFilter-System-Int64-'></a>

#### UninstallFilter(filterId) `method`

Uninstalls a previously created filter.

###### Returns

The result of the operation, `true` on success or `false` on failure.

###### Parameters

- [System.Int64](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Int64 "System.Int64") **filterId** - The filter id returned by [NewBlockFilter](#M-In3-Eth1-Api-NewBlockFilter "In3.Eth1.Api.NewBlockFilter").

<a name='M-In3-Ipfs-Api-Get-System-String-'></a>

#### Get(multihash) `method`

Returns the content associated with specified multihash on success OR on error.

###### Returns

The content that was stored by [Put](#M-In3-Ipfs-Api-Put-System-Byte[]- "In3.Ipfs.Api.Put(System.Byte[])") or [Put](#M-In3-Ipfs-Api-Put-System-String- "In3.Ipfs.Api.Put(System.String)").

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **multihash** - The multihash.

<a name='M-In3-Ipfs-Api-Put-System-String-'></a>

#### Put(content) `method`

Stores content on ipfs.

###### Returns

The multihash.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **content** - The content that will be stored via ipfs.

<a name='M-In3-Ipfs-Api-Put-System-Byte[]-'></a>

#### Put(content) `method`

Stores content on ipfs. The content is encoded as base64 before storing.

###### Returns

The multihash.

###### Parameters

- [System.Byte[]](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Byte[] "System.Byte[]") **content** - The content that will be stored via ipfs.

<a name='T-In3-Configuration-BaseConfiguration'></a>

### BaseConfiguration `type`

In3.Configuration

Base class for all configuration classes.

<a name='T-In3-Eth1-Block'></a>

### Block `type`

In3.Eth1

Class that represents as Ethereum block.

<a name='P-In3-Eth1-Block-Author'></a>

#### Author `property`

The miner of the block.

<a name='P-In3-Eth1-Block-Difficulty'></a>

#### Difficulty `property`

Dificulty of the block.

<a name='P-In3-Eth1-Block-ExtraData'></a>

#### ExtraData `property`

Extra data.

<a name='P-In3-Eth1-Block-GasLimit'></a>

#### GasLimit `property`

Gas limit.

<a name='P-In3-Eth1-Block-Hash'></a>

#### Hash `property`

The block hash.

<a name='P-In3-Eth1-Block-LogsBloom'></a>

#### LogsBloom `property`

The logsBloom data of the block.

<a name='P-In3-Eth1-Block-MixHash'></a>

#### MixHash `property`

The mix hash of the block. (only valid of proof of work).

<a name='P-In3-Eth1-Block-Nonce'></a>

#### Nonce `property`

The nonce.

<a name='P-In3-Eth1-Block-Number'></a>

#### Number `property`

The index of the block.

<a name='P-In3-Eth1-Block-ParentHash'></a>

#### ParentHash `property`

The parent block\`s hash.

<a name='P-In3-Eth1-Block-ReceiptsRoot'></a>

#### ReceiptsRoot `property`

The roothash of the merkletree containing all transaction receipts of the block.

<a name='P-In3-Eth1-Block-Sha3Uncles'></a>

#### Sha3Uncles `property`

The roothash of the merkletree containing all uncles of the block.

<a name='P-In3-Eth1-Block-Size'></a>

#### Size `property`

Size of the block.

<a name='P-In3-Eth1-Block-StateRoot'></a>

#### StateRoot `property`

The roothash of the merkletree containing the complete state.

<a name='P-In3-Eth1-Block-Timestamp'></a>

#### Timestamp `property`

Epoch timestamp when the block was created.

<a name='P-In3-Eth1-Block-TotalDifficulty'></a>

#### TotalDifficulty `property`

Total Difficulty as a sum of all difficulties starting from genesis.

<a name='P-In3-Eth1-Block-TransactionsRoot'></a>

#### TransactionsRoot `property`

The roothash of the merkletree containing all transaction of the block.

<a name='P-In3-Eth1-Block-Uncles'></a>

#### Uncles `property`

List of uncle hashes.

<a name='T-In3-Btc-BlockHeader'></a>

### BlockHeader `type`

In3.Btc

A Block header.

<a name='P-In3-Btc-BlockHeader-Bits'></a>

#### Bits `property`

Bits (target) for the block as hex.

<a name='P-In3-Btc-BlockHeader-Chainwork'></a>

#### Chainwork `property`

Total amount of work since genesis.

<a name='P-In3-Btc-BlockHeader-Confirmations'></a>

#### Confirmations `property`

Number of confirmations or blocks mined on top of the containing block.

<a name='P-In3-Btc-BlockHeader-Difficulty'></a>

#### Difficulty `property`

Difficulty of the block.

<a name='P-In3-Btc-BlockHeader-Hash'></a>

#### Hash `property`

The hash of the blockheader.

<a name='P-In3-Btc-BlockHeader-Height'></a>

#### Height `property`

Block number.

<a name='P-In3-Btc-BlockHeader-Mediantime'></a>

#### Mediantime `property`

Unix timestamp in seconds since 1970.

<a name='P-In3-Btc-BlockHeader-Merkleroot'></a>

#### Merkleroot `property`

Merkle root of the trie of all transactions in the block.

<a name='P-In3-Btc-BlockHeader-NTx'></a>

#### NTx `property`

Number of transactions in the block.

<a name='P-In3-Btc-BlockHeader-Nextblockhash'></a>

#### Nextblockhash `property`

Hash of the next blockheader.

<a name='P-In3-Btc-BlockHeader-Nonce'></a>

#### Nonce `property`

Nonce-field of the block.

<a name='P-In3-Btc-BlockHeader-Previousblockhash'></a>

#### Previousblockhash `property`

Hash of the parent blockheader.

<a name='P-In3-Btc-BlockHeader-Time'></a>

#### Time `property`

Unix timestamp in seconds since 1970.

<a name='P-In3-Btc-BlockHeader-Version'></a>

#### Version `property`

Used version.

<a name='P-In3-Btc-BlockHeader-VersionHex'></a>

#### VersionHex `property`

Version as hex.

<a name='T-In3-BlockParameter'></a>

### BlockParameter `type`

In3

Enum-like class that defines constants to be used with [Api](#T-In3-Eth1-Api "In3.Eth1.Api").

<a name='P-In3-BlockParameter-Earliest'></a>

#### Earliest `property`

Genesis block.

<a name='P-In3-BlockParameter-Latest'></a>

#### Latest `property`

Constant associated with the latest mined block in the chain.

###### Remarks

While the parameter itself is constant the current "latest" block changes everytime a new block is mined. The result of the operations are also related to `ReplaceLatestBlock` on [ClientConfiguration](#T-In3-Configuration-ClientConfiguration "In3.Configuration.ClientConfiguration").

<a name='T-In3-Btc-Block`1'></a>

### Block\`1 `type`

In3.Btc

A Block.

<a name='P-In3-Btc-Block`1-Size'></a>

#### Size `property`

Size of this block in bytes.

<a name='P-In3-Btc-Block`1-Tx'></a>

#### Tx `property`

Transactions or Transaction ids of a block. [GetBlockWithTxData](#M-In3-Btc-Api-GetBlockWithTxData-System-String- "In3.Btc.Api.GetBlockWithTxData(System.String)") or [GetBlockWithTxIds](#M-In3-Btc-Api-GetBlockWithTxIds-System-String- "In3.Btc.Api.GetBlockWithTxIds(System.String)").

<a name='P-In3-Btc-Block`1-Weight'></a>

#### Weight `property`

Weight of this block in bytes.

<a name='T-In3-Chain'></a>

### Chain `type`

In3

Represents the multiple chains supported by Incubed.

<a name='F-In3-Chain-Btc'></a>

#### Btc `constants`

Bitcoin chain.

<a name='F-In3-Chain-Evan'></a>

#### Evan `constants`

Evan testnet.

<a name='F-In3-Chain-Ewc'></a>

#### Ewc `constants`

Ewf chain.

<a name='F-In3-Chain-Goerli'></a>

#### Goerli `constants`

Goerli testnet.

<a name='F-In3-Chain-Ipfs'></a>

#### Ipfs `constants`

Ipfs (InterPlanetary File System).

<a name='F-In3-Chain-Local'></a>

#### Local `constants`

Local client.

<a name='F-In3-Chain-Mainnet'></a>

#### Mainnet `constants`

Ethereum mainnet.

<a name='F-In3-Chain-Multichain'></a>

#### Multichain `constants`

Support for multiple chains, a client can then switch between different chains (but consumes more memory).

<a name='F-In3-Chain-Tobalaba'></a>

#### Tobalaba `constants`

Tobalaba testnet.

<a name='F-In3-Chain-Volta'></a>

#### Volta `constants`

Volta testnet.

<a name='T-In3-Configuration-ChainConfiguration'></a>

### ChainConfiguration `type`

In3.Configuration

Class that represents part of the configuration to be applied on the [IN3](#T-In3-IN3 "In3.IN3") (in particular to each chain).
This is a child of [ClientConfiguration](#T-In3-Configuration-ClientConfiguration "In3.Configuration.ClientConfiguration") and have many [NodeConfiguration](#T-In3-Configuration-NodeConfiguration "In3.Configuration.NodeConfiguration").

<a name='M-In3-Configuration-ChainConfiguration-#ctor-In3-Chain,In3-Configuration-ClientConfiguration-'></a>

#### #ctor(chain,clientConfiguration) `constructor`

Constructor.

###### Parameters

- [In3.Chain](#T-In3-Chain "In3.Chain") **chain** - One of [Chain](#T-In3-Chain "In3.Chain"). The chain that this configuration is related to.
- [In3.Configuration.ClientConfiguration](#T-In3-Configuration-ClientConfiguration "In3.Configuration.ClientConfiguration") **clientConfiguration** - The configuration for the client whose the chain configuration belongs to.

###### Example

```
ChainConfiguration goerliConfiguration = new ChainConfiguration(Chain.Goerli, in3Client.Configuration);
```

<a name='P-In3-Configuration-ChainConfiguration-Contract'></a>

#### Contract `property`

Incubed registry contract from which the list was taken.

<a name='P-In3-Configuration-ChainConfiguration-NeedsUpdate'></a>

#### NeedsUpdate `property`

Preemptively update the node list.

<a name='P-In3-Configuration-ChainConfiguration-NodesConfiguration'></a>

#### NodesConfiguration `property`

Getter for the list of elements that represent the configuration for each node.

###### Remarks

This is a read-only property. To add configuration for nodes, Use [NodeConfiguration](#T-In3-Configuration-NodeConfiguration "In3.Configuration.NodeConfiguration") constructor.

<a name='P-In3-Configuration-ChainConfiguration-RegistryId'></a>

#### RegistryId `property`

Uuid of this incubed network. one chain could contain more than one incubed networks.

<a name='P-In3-Configuration-ChainConfiguration-WhiteList'></a>

#### WhiteList `property`

Node addresses that constitute the white list of nodes.

<a name='P-In3-Configuration-ChainConfiguration-WhiteListContract'></a>

#### WhiteListContract `property`

Address of whiteList contract.

<a name='T-In3-Configuration-ClientConfiguration'></a>

### ClientConfiguration `type`

In3.Configuration

Class that represents the configuration to be applied on [IN3](#T-In3-IN3 "In3.IN3").
Due to the 1-to-1 relationship with the client, this class should never be instantiated. To obtain a reference of the client configuration use [Configuration](#P-In3-IN3-Configuration "In3.IN3.Configuration") instead.

###### Remarks

Use in conjunction with [ChainConfiguration](#T-In3-Configuration-ChainConfiguration "In3.Configuration.ChainConfiguration") and [NodeConfiguration](#T-In3-Configuration-NodeConfiguration "In3.Configuration.NodeConfiguration").

<a name='P-In3-Configuration-ClientConfiguration-AutoUpdateList'></a>

#### AutoUpdateList `property`

If `true` the nodelist will be automatically updated. False may compromise data security.

<a name='P-In3-Configuration-ClientConfiguration-BootWeights'></a>

#### BootWeights `property`

if true, the first request (updating the nodelist) will also fetch the current health status
and use it for blacklisting unhealthy nodes. This is used only if no nodelist is availabkle from cache.

<a name='P-In3-Configuration-ClientConfiguration-ChainsConfiguration'></a>

#### ChainsConfiguration `property`

Configuration for the chains. Read-only attribute.

<a name='P-In3-Configuration-ClientConfiguration-Finality'></a>

#### Finality `property`

###### Remarks

Beware that the semantics of the values change greatly from chain to chain. The value of `8` would mean 8 blocks mined on top of the requested one while with the POW algorithm while, for POA, it would mean 8% of validators.

<a name='P-In3-Configuration-ClientConfiguration-IncludeCode'></a>

#### IncludeCode `property`

Code is included when sending eth_call-requests.

<a name='P-In3-Configuration-ClientConfiguration-KeepIn3'></a>

#### KeepIn3 `property`

Tthe in3-section (custom node on the RPC call) with the proof will also returned.

<a name='P-In3-Configuration-ClientConfiguration-MaxAttempts'></a>

#### MaxAttempts `property`

Maximum times the client will retry to contact a certain node.

<a name='P-In3-Configuration-ClientConfiguration-MinDeposit'></a>

#### MinDeposit `property`

Only nodes owning at least this amount will be chosen to sign responses to your requests.

<a name='P-In3-Configuration-ClientConfiguration-NodeLimit'></a>

#### NodeLimit `property`

Limit nodes stored in the client.

<a name='P-In3-Configuration-ClientConfiguration-NodeProps'></a>

#### NodeProps `property`

Props define the capabilities of the nodes. Accepts a combination of values.

###### Example

```
clientConfiguration.NodeProps = Props.NodePropProof | Props.NodePropArchive;
```

<a name='P-In3-Configuration-ClientConfiguration-Proof'></a>

#### Proof `property`

One of [Proof](#P-In3-Configuration-ClientConfiguration-Proof "In3.Configuration.ClientConfiguration.Proof"). [Full](#P-In3-Configuration-Proof-Full "In3.Configuration.Proof.Full") gets the whole block Patricia-Merkle-Tree, [Standard](#P-In3-Configuration-Proof-Standard "In3.Configuration.Proof.Standard") only verifies the specific tree branch concerning the request, [None](#P-In3-Configuration-Proof-None "In3.Configuration.Proof.None") only verifies the root hashes, like a light-client does.

<a name='P-In3-Configuration-ClientConfiguration-ReplaceLatestBlock'></a>

#### ReplaceLatestBlock `property`

Distance considered safe, consensus wise, from the very latest block. Higher values exponentially increases state finality, and therefore data security, as well guaranteeded responses from in3 nodes.

<a name='P-In3-Configuration-ClientConfiguration-RequestCount'></a>

#### RequestCount `property`

Useful when [SignatureCount](#P-In3-Configuration-ClientConfiguration-SignatureCount "In3.Configuration.ClientConfiguration.SignatureCount") is less then `1`. The client will check for consensus in responses.

<a name='P-In3-Configuration-ClientConfiguration-Rpc'></a>

#### Rpc `property`

Setup an custom rpc source for requests by setting chain to [Local](#F-In3-Chain-Local "In3.Chain.Local") and proof to [None](#P-In3-Configuration-Proof-None "In3.Configuration.Proof.None").

<a name='P-In3-Configuration-ClientConfiguration-SignatureCount'></a>

#### SignatureCount `property`

Node signatures attesting the response to your request. Will send a separate request for each.

###### Example

When set to `3`, 3 nodes will have to sign the response.

<a name='P-In3-Configuration-ClientConfiguration-Timeout'></a>

#### Timeout `property`

Milliseconds before a request times out.

<a name='P-In3-Configuration-ClientConfiguration-UseHttp'></a>

#### UseHttp `property`

Disable ssl on the Http connection.

<a name='T-In3-Context-Context'></a>

### Context `type`

In3.Context

Acts as the main orchestrator for the execution of an rpc. Holds a reference to the native context (ctx) and wraps behavior around it.

<a name='M-In3-Context-Context-#ctor-System-IntPtr,In3-Native-NativeClient-'></a>

#### #ctor(ctx,nativeClient) `constructor`

Standard constructor, private so people use [FromRpc](#M-In3-Context-Context-FromRpc-In3-Native-NativeClient,System-String- "In3.Context.Context.FromRpc(In3.Native.NativeClient,System.String)").

###### Parameters

- [System.IntPtr](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.IntPtr "System.IntPtr") **ctx** - The native rpc context.
- [In3.Native.NativeClient](#T-In3-Native-NativeClient "In3.Native.NativeClient") **nativeClient** - Object that encapsulates the native client.

<a name='M-In3-Context-Context-CreateNativeCtx-System-IntPtr,System-String-'></a>

#### CreateNativeCtx(nativeIn3Ptr,rpc) `method`

Method to manage the creation of the native ctx request.

###### Returns

Native rpc pointer

###### Parameters

- [System.IntPtr](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.IntPtr "System.IntPtr") **nativeIn3Ptr** - Native client pointer.
- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **rpc** - The rpc request

###### Exceptions

| Name | Description |

| [In3.Error.ContextException](#T-In3-Error-ContextException "In3.Error.ContextException") | |

<a name='M-In3-Context-Context-Dispose'></a>

#### Dispose() `method`

Destructor method for the native ctx encapsulated by the [Context](#T-In3-Context-Context "In3.Context.Context") object.

###### Parameters

This method has no parameters.

<a name='M-In3-Context-Context-Execute'></a>

#### Execute() `method`

Proxy to in3_ctx_execute, every invocation generates a new state.

###### Returns

The state as computed by in3_ctx_execute.

###### Parameters

This method has no parameters.

<a name='M-In3-Context-Context-FromRpc-In3-Native-NativeClient,System-String-'></a>

#### FromRpc(wrapper,rpc) `method`

Factory-like method to build a Context object from an rpc request.

###### Returns

An instance of context.

###### Parameters

- [In3.Native.NativeClient](#T-In3-Native-NativeClient "In3.Native.NativeClient") **wrapper** - The object that encapsulates the native client pointer.
- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **rpc** - The rpc request

<a name='M-In3-Context-Context-GetErrorMessage'></a>

#### GetErrorMessage() `method`

Retrieve the error result on the context.

###### Returns

A string describing the encountered error.

###### Parameters

This method has no parameters.

<a name='M-In3-Context-Context-GetLastWaiting'></a>

#### GetLastWaiting() `method`

Method responsible to fetch the pending context references in the current context.

###### Returns

A context object.

###### Parameters

This method has no parameters.

<a name='M-In3-Context-Context-GetResponse'></a>

#### GetResponse() `method`

Method to get the consolidated response of a request.

###### Returns

The final result.

###### Parameters

This method has no parameters.

<a name='M-In3-Context-Context-GetType'></a>

#### GetType() `method`

Method to get the consolidated response of a request.

###### Returns

The final result.

###### Parameters

This method has no parameters.

<a name='M-In3-Context-Context-HandleRequest'></a>

#### HandleRequest() `method`

Handle rpc request in an asynchronous manner.

###### Parameters

This method has no parameters.

<a name='M-In3-Context-Context-HandleSign'></a>

#### HandleSign() `method`

Handle signing request in an asynchronous manner.

###### Parameters

This method has no parameters.

<a name='M-In3-Context-Context-IsValid'></a>

#### IsValid() `method`

Conditional to verify if the encapsulated pointer actually points to something.

###### Returns

if its valid, `false` if it is not.

###### Parameters

This method has no parameters.

<a name='M-In3-Context-Context-ReportError-System-String-'></a>

#### ReportError() `method`

Setter for the error on the current context. Proxies it to the native context.

###### Parameters

This method has no parameters.

<a name='T-In3-Error-ContextException'></a>

### ContextException `type`

In3.Error

Exception thrown when there is an issue with the Context engine.

<a name='T-In3-Utils-DataTypeConverter'></a>

### DataTypeConverter `type`

In3.Utils

General util class for conversion between blockchain types.

<a name='M-In3-Utils-DataTypeConverter-HexStringToBigint-System-String-'></a>

#### HexStringToBigint(source) `method`

Converts a zero-prefixed hex (e.g.: 0x05) to [BigInteger](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Numerics.BigInteger "System.Numerics.BigInteger")

###### Returns

The number representation of `source`.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **source** - The hex number string.

<a name='T-In3-Transport-DefaultTransport'></a>

### DefaultTransport `type`

In3.Transport

Basic implementation for synchronous http transport for Incubed client.

<a name='M-In3-Transport-DefaultTransport-#ctor'></a>

#### #ctor() `constructor`

Standard construction.

###### Parameters

This constructor has no parameters.

<a name='M-In3-Transport-DefaultTransport-Handle-System-String,System-String-'></a>

#### Handle(url,payload) `method`

Method that handles, sychronously the http requests.

###### Returns

The http json response.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **url** - The url of the node.
- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **payload** - Json for the body of the POST request to the node.

<a name='T-In3-ENSParameter'></a>

### ENSParameter `type`

In3

Defines the kind of entity associated with the ENS Resolved. Used along with [Ens](#M-In3-Eth1-Api-Ens-System-String,In3-ENSParameter- "In3.Eth1.Api.Ens(System.String,In3.ENSParameter)").

<a name='P-In3-ENSParameter-Addr'></a>

#### Addr `property`

Address.

<a name='P-In3-ENSParameter-Hash'></a>

#### Hash `property`

Hash.

<a name='P-In3-ENSParameter-Owner'></a>

#### Owner `property`

Owner.

<a name='P-In3-ENSParameter-Resolver'></a>

#### Resolver `property`

Resolver.

<a name='T-In3-IN3'></a>

### IN3 `type`

In3

Incubed network client. Connect to the blockchain via a list of bootnodes, then gets the latest list of nodes in
the network and ask a certain number of the to sign the block header of given list, putting their deposit at stake.
Once with the latest list at hand, the client can request any other on-chain information using the same scheme.

<a name='M-In3-IN3-#ctor-In3-Chain-'></a>

#### #ctor(chainId) `constructor`

Standard constructor, use [ForChain](#M-In3-IN3-ForChain-In3-Chain- "In3.IN3.ForChain(In3.Chain)") instead.

###### Parameters

- [In3.Chain](#T-In3-Chain "In3.Chain") **chainId** - The chainId to connect to.

<a name='P-In3-IN3-Btc'></a>

#### Btc `property`

Gets [Api](#T-In3-Btc-Api "In3.Btc.Api") object.

<a name='P-In3-IN3-Configuration'></a>

#### Configuration `property`

Gets [ClientConfiguration](#T-In3-Configuration-ClientConfiguration "In3.Configuration.ClientConfiguration") object. Any changes in the object will be automaticaly applied to the client before each method invocation.

<a name='P-In3-IN3-Crypto'></a>

#### Crypto `property`

Gets [Api](#T-In3-Crypto-Api "In3.Crypto.Api") object.

<a name='P-In3-IN3-Eth1'></a>

#### Eth1 `property`

Gets [Api](#T-In3-Eth1-Api "In3.Eth1.Api") object.

<a name='P-In3-IN3-Ipfs'></a>

#### Ipfs `property`

Gets [Api](#T-In3-Ipfs-Api "In3.Ipfs.Api") object.

<a name='P-In3-IN3-Signer'></a>

#### Signer `property`

Get or Sets [Signer](#P-In3-IN3-Signer "In3.IN3.Signer") object. If not set [SimpleWallet](#T-In3-Crypto-SimpleWallet "In3.Crypto.SimpleWallet") will be used.

<a name='P-In3-IN3-Storage'></a>

#### Storage `property`

Get or Sets [Storage](#T-In3-Storage-Storage "In3.Storage.Storage") object. If not set [InMemoryStorage](#T-In3-Storage-InMemoryStorage "In3.Storage.InMemoryStorage") will be used.

<a name='P-In3-IN3-Transport'></a>

#### Transport `property`

Gets or sets [Transport](#T-In3-Transport-Transport "In3.Transport.Transport") object. If not set [DefaultTransport](#T-In3-Transport-DefaultTransport "In3.Transport.DefaultTransport") will be used.

<a name='M-In3-IN3-Finalize'></a>

#### Finalize() `method`

Finalizer for the client.

###### Parameters

This method has no parameters.

<a name='M-In3-IN3-ForChain-In3-Chain-'></a>

#### ForChain(chain) `method`

Creates a new instance of `IN3`.

###### Returns

An Incubed instance.

###### Parameters

- [In3.Chain](#T-In3-Chain "In3.Chain") **chain** - [Chain](#T-In3-Chain "In3.Chain") that Incubed will connect to.

###### Example

```
IN3 client = IN3.ForChain(Chain.Mainnet);
```

<a name='M-In3-IN3-SendRpc-System-String,System-Object[],System-Collections-Generic-Dictionary{System-String,System-Object}-'></a>

#### SendRpc(method,args,in3) `method`

Method used to communicate with the client. In general, its preferably to use the API.

###### Returns

The result of the Rpc operation as JSON.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **method** - Rpc method.
- [System.Object[]](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Object[] "System.Object[]") **args** - Arguments to the operation.
- [System.Collections.Generic.Dictionary{System.String,System.Object}](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Collections.Generic.Dictionary "System.Collections.Generic.Dictionary{System.String,System.Object}") **in3** - Internal parameters to be repassed to the server or to change the client behavior.

<a name='T-In3-Error-In3Exception'></a>

### In3Exception `type`

In3.Error

General purpose, all-encompassing, Incubed specific Exception class.

<a name='T-In3-Storage-InMemoryStorage'></a>

### InMemoryStorage `type`

In3.Storage

Default implementation of [Storage](#T-In3-Storage-Storage "In3.Storage.Storage"). It caches all cacheable data in memory.

<a name='M-In3-Storage-InMemoryStorage-#ctor'></a>

#### #ctor() `constructor`

Standard constructor.

###### Parameters

This constructor has no parameters.

<a name='M-In3-Storage-InMemoryStorage-Clear'></a>

#### Clear() `method`

Empty the in-memory cache.

###### Returns

Result for the clear operation.

###### Parameters

This method has no parameters.

<a name='M-In3-Storage-InMemoryStorage-GetItem-System-String-'></a>

#### GetItem(key) `method`

Fetches the data from memory.

###### Returns

The cached value as a `byte[]`.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **key** - Key

<a name='M-In3-Storage-InMemoryStorage-SetItem-System-String,System-Byte[]-'></a>

#### SetItem(key,content) `method`

Stores a value in memory for a given key.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **key** - A unique identifier for the data that is being cached.
- [System.Byte[]](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Byte[] "System.Byte[]") **content** - The value that is being cached.

<a name='T-In3-Error-InvalidSignerException'></a>

### InvalidSignerException `type`

In3.Error

Exception thrown when there is no available [Signer](#T-In3-Crypto-Signer "In3.Crypto.Signer") to sign.

<a name='T-In3-Error-InvalidTransactionRequestException'></a>

### InvalidTransactionRequestException `type`

In3.Error

Exception thrown when the state of [TransactionRequest](#T-In3-Eth1-TransactionRequest "In3.Eth1.TransactionRequest") is invalid.

<a name='T-In3-Eth1-Log'></a>

### Log `type`

In3.Eth1

Logs marks changes of state on the blockchain for events. The [Log](#T-In3-Eth1-Log "In3.Eth1.Log") is a data object with information from logs.

<a name='P-In3-Eth1-Log-Address'></a>

#### Address `property`

Address from which this log originated.

<a name='P-In3-Eth1-Log-BlockHash'></a>

#### BlockHash `property`

Hash of the block this log was in. null when its pending log.

<a name='P-In3-Eth1-Log-BlockNumber'></a>

#### BlockNumber `property`

Number of the block this log was in.

<a name='P-In3-Eth1-Log-Data'></a>

#### Data `property`

Data associated with the log.

<a name='P-In3-Eth1-Log-LogIndex'></a>

#### LogIndex `property`

Index position in the block.

<a name='P-In3-Eth1-Log-Removed'></a>

#### Removed `property`

Flags log removal (due to chain reorganization).

<a name='P-In3-Eth1-Log-Topics'></a>

#### Topics `property`

Array of 0 to 4 32 Bytes DATA of indexed log arguments. (In solidity: The first topic is the hash of the signature of the event (e.g. Deposit(address,bytes32,uint256)), except you declared the event with the anonymous specifier).

<a name='P-In3-Eth1-Log-TransactionHash'></a>

#### TransactionHash `property`

Hash of the transactions this log was created from. null when its pending log.

<a name='P-In3-Eth1-Log-TransactionIndex'></a>

#### TransactionIndex `property`

index position log was created from.

<a name='P-In3-Eth1-Log-Type'></a>

#### Type `property`

Address from which this log originated.

<a name='T-In3-Eth1-LogFilter'></a>

### LogFilter `type`

In3.Eth1

Filter configuration for search logs. To be used along with the [Api](#T-In3-Eth1-Api "In3.Eth1.Api") filter and methods.

<a name='M-In3-Eth1-LogFilter-#ctor'></a>

#### #ctor() `constructor`

Standard constructor.

###### Parameters

This constructor has no parameters.

<a name='P-In3-Eth1-LogFilter-Address'></a>

#### Address `property`

Address for the filter.

<a name='P-In3-Eth1-LogFilter-BlockHash'></a>

#### BlockHash `property`

Blcok hash of the filtered blocks.

###### Remarks

If present, [FromBlock](#P-In3-Eth1-LogFilter-FromBlock "In3.Eth1.LogFilter.FromBlock") and [ToBlock](#P-In3-Eth1-LogFilter-ToBlock "In3.Eth1.LogFilter.ToBlock") will be ignored.

<a name='P-In3-Eth1-LogFilter-FromBlock'></a>

#### FromBlock `property`

Starting block for the filter.

<a name='P-In3-Eth1-LogFilter-ToBlock'></a>

#### ToBlock `property`

End block for the filter.

<a name='P-In3-Eth1-LogFilter-Topics'></a>

#### Topics `property`

Array of 32 Bytes Data topics. Topics are order-dependent. It's possible to pass in null to match any topic, or a subarray of multiple topics of which one should be matching.

<a name='T-In3-Configuration-NodeConfiguration'></a>

### NodeConfiguration `type`

In3.Configuration

Class that represents part of the configuration to be applied on the [IN3](#T-In3-IN3 "In3.IN3") (in particular to each boot node).
This is a child of [ChainConfiguration](#T-In3-Configuration-ChainConfiguration "In3.Configuration.ChainConfiguration").

<a name='M-In3-Configuration-NodeConfiguration-#ctor-In3-Configuration-ChainConfiguration-'></a>

#### #ctor(config) `constructor`

Constructor for the node configuration.

###### Parameters

- [In3.Configuration.ChainConfiguration](#T-In3-Configuration-ChainConfiguration "In3.Configuration.ChainConfiguration") **config** - The [ChainConfiguration](#T-In3-Configuration-ChainConfiguration "In3.Configuration.ChainConfiguration") of which this node belongs to.

###### Example

```
NodeConfiguration myDeployedNode = new NodeConfiguration(mainnetChainConfiguration);
```

<a name='P-In3-Configuration-NodeConfiguration-Address'></a>

#### Address `property`

Address of the node, which is the public address it is signing with.

<a name='P-In3-Configuration-NodeConfiguration-Props'></a>

#### Props `property`

Props define the capabilities of the node. Accepts a combination of values.

###### Example

```
nodeConfiguration.Props = Props.NodePropProof | Props.NodePropArchive;
```

<a name='P-In3-Configuration-NodeConfiguration-Url'></a>

#### Url `property`

Url of the bootnode which the client can connect to.

<a name='T-In3-Configuration-Proof'></a>

### Proof `type`

In3.Configuration

Alias for verification levels. Verification is done by calculating Ethereum Trie states requested by the Incubed network ans signed as proofs of a certain state.

<a name='P-In3-Configuration-Proof-Full'></a>

#### Full `property`

All fields will be validated (including uncles).

<a name='P-In3-Configuration-Proof-None'></a>

#### None `property`

No Verification.

<a name='P-In3-Configuration-Proof-Standard'></a>

#### Standard `property`

Standard Verification of the important properties.

<a name='T-In3-Configuration-Props'></a>

### Props `type`

In3.Configuration

`Enum` that defines the capabilities an incubed node.

<a name='F-In3-Configuration-Props-NodePropArchive'></a>

#### NodePropArchive `constants`

filter out non-archive supporting nodes.

<a name='F-In3-Configuration-Props-NodePropBinary'></a>

#### NodePropBinary `constants`

filter out nodes that don't support binary encoding.

<a name='F-In3-Configuration-Props-NodePropData'></a>

#### NodePropData `constants`

filter out non-data provider nodes.

<a name='F-In3-Configuration-Props-NodePropHttp'></a>

#### NodePropHttp `constants`

filter out non-http nodes.

<a name='F-In3-Configuration-Props-NodePropMinblockheight'></a>

#### NodePropMinblockheight `constants`

filter out nodes that will sign blocks with lower min block height than specified.

<a name='F-In3-Configuration-Props-NodePropMultichain'></a>

#### NodePropMultichain `constants`

filter out nodes other then which have capability of the same RPC endpoint may also accept requests for different chains.

<a name='F-In3-Configuration-Props-NodePropOnion'></a>

#### NodePropOnion `constants`

filter out non-onion nodes.

<a name='F-In3-Configuration-Props-NodePropProof'></a>

#### NodePropProof `constants`

filter out nodes which are providing no proof.

<a name='F-In3-Configuration-Props-NodePropSigner'></a>

#### NodePropSigner `constants`

filter out non-signer nodes.

<a name='F-In3-Configuration-Props-NodePropStats'></a>

#### NodePropStats `constants`

filter out nodes that do not provide stats.

<a name='T-In3-Error-RpcException'></a>

### RpcException `type`

In3.Error

Custom Exception to be thrown in case of Rpc errors.

<a name='P-In3-Error-RpcException-Code'></a>

#### Code `property`

Json-rpc code for the error.

<a name='T-In3-Btc-ScriptPubKey'></a>

### ScriptPubKey `type`

In3.Btc

Script on a transaction output.

<a name='P-In3-Btc-ScriptPubKey-Addresses'></a>

#### Addresses `property`

List of addresses.

<a name='P-In3-Btc-ScriptPubKey-Asm'></a>

#### Asm `property`

The asm data,

<a name='P-In3-Btc-ScriptPubKey-Hex'></a>

#### Hex `property`

The raw hex data.

<a name='P-In3-Btc-ScriptPubKey-ReqSigs'></a>

#### ReqSigs `property`

The required sigs.

<a name='P-In3-Btc-ScriptPubKey-Type'></a>

#### Type `property`

The type.

###### Example

pubkeyhash

<a name='T-In3-Btc-ScriptSig'></a>

### ScriptSig `type`

In3.Btc

Script on a transaction input.

<a name='P-In3-Btc-ScriptSig-Asm'></a>

#### Asm `property`

The asm data.

<a name='P-In3-Btc-ScriptSig-Hex'></a>

#### Hex `property`

The raw hex data.

<a name='T-In3-Crypto-SignatureType'></a>

### SignatureType `type`

In3.Crypto

Group of constants to be used along with the methods of [Api](#T-In3-Crypto-Api "In3.Crypto.Api").

<a name='P-In3-Crypto-SignatureType-EthSign'></a>

#### EthSign `property`

For hashes of the RLP prefixed.

<a name='P-In3-Crypto-SignatureType-Hash'></a>

#### Hash `property`

For data that was hashed and then signed.

<a name='P-In3-Crypto-SignatureType-Raw'></a>

#### Raw `property`

For data that was signed directly.

<a name='T-In3-Crypto-SignedData'></a>

### SignedData `type`

In3.Crypto

Output of [SignData](#M-In3-Crypto-Api-SignData-System-String,System-String,In3-Crypto-SignatureType- "In3.Crypto.Api.SignData(System.String,System.String,In3.Crypto.SignatureType)").

<a name='P-In3-Crypto-SignedData-Message'></a>

#### Message `property`

Signed message.

<a name='P-In3-Crypto-SignedData-MessageHash'></a>

#### MessageHash `property`

Hash of ([Message](#P-In3-Crypto-SignedData-Message "In3.Crypto.SignedData.Message").

<a name='P-In3-Crypto-SignedData-R'></a>

#### R `property`

Part of the ECDSA signature.

<a name='P-In3-Crypto-SignedData-S'></a>

#### S `property`

Part of the ECDSA signature.

<a name='P-In3-Crypto-SignedData-Signature'></a>

#### Signature `property`

ECDSA calculated r, s, and parity v, concatenated.

<a name='P-In3-Crypto-SignedData-V'></a>

#### V `property`

27 + ([R](#P-In3-Crypto-SignedData-R "In3.Crypto.SignedData.R") % 2).

<a name='T-In3-Crypto-Signer'></a>

### Signer `type`

In3.Crypto

Minimum interface to be implemented by a kind of signer. Used by [SendTransaction](#M-In3-Eth1-Api-SendTransaction-In3-Eth1-TransactionRequest- "In3.Eth1.Api.SendTransaction(In3.Eth1.TransactionRequest)"). Set it with [Signer](#P-In3-IN3-Signer "In3.IN3.Signer").

<a name='M-In3-Crypto-Signer-CanSign-System-String-'></a>

#### CanSign(account) `method`

Queries the Signer if it can sign for a certain key.

###### Returns

`true` if it can sign, `false` if it cant.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **account** - The account derived from the private key used to sign transactions.

###### Remarks

This method is invoked internaly by [SendTransaction](#M-In3-Eth1-Api-SendTransaction-In3-Eth1-TransactionRequest- "In3.Eth1.Api.SendTransaction(In3.Eth1.TransactionRequest)") using [From](#P-In3-Eth1-TransactionRequest-From "In3.Eth1.TransactionRequest.From") and will throw a `SystemException` in case `false` is returned.

<a name='M-In3-Crypto-Signer-PrepareTransaction-In3-Eth1-TransactionRequest-'></a>

#### PrepareTransaction() `method`

Optional method which allows to change the transaction-data before sending it. This can be used for redirecting it through a multisig. Invoked just before sending a transaction through [SendTransaction](#M-In3-Eth1-Api-SendTransaction-In3-Eth1-TransactionRequest- "In3.Eth1.Api.SendTransaction(In3.Eth1.TransactionRequest)").

###### Returns

Modified transaction request.

###### Parameters

This method has no parameters.

<a name='M-In3-Crypto-Signer-Sign-System-String,System-String-'></a>

#### Sign(data,account) `method`

Signs the transaction data with the private key associated with the invoked account. Both arguments are automaticaly passed by Incubed client base on [TransactionRequest](#T-In3-Eth1-TransactionRequest "In3.Eth1.TransactionRequest") data during a [SendTransaction](#M-In3-Eth1-Api-SendTransaction-In3-Eth1-TransactionRequest- "In3.Eth1.Api.SendTransaction(In3.Eth1.TransactionRequest)").

###### Returns

The signed transaction data.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **data** - Data to be signed.
- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **account** - The account that will sign the transaction.

<a name='T-In3-Crypto-SimpleWallet'></a>

### SimpleWallet `type`

In3.Crypto

Default implementation of the [Signer](#T-In3-Crypto-Signer "In3.Crypto.Signer"). Works as an orchestration of the [](#N-In3-Crypto "In3.Crypto") in order to manage multiple accounts.

<a name='M-In3-Crypto-SimpleWallet-#ctor-In3-IN3-'></a>

#### #ctor(in3) `constructor`

Basic constructor.

###### Parameters

- [In3.IN3](#T-In3-IN3 "In3.IN3") **in3** - A client instance.

<a name='M-In3-Crypto-SimpleWallet-AddRawKey-System-String-'></a>

#### AddRawKey(privateKey) `method`

Adds a private key to be managed by the wallet and sign transactions.

###### Returns

The address derived from the `privateKey`

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **privateKey** - The private key to be stored by the wallet.

<a name='M-In3-Crypto-SimpleWallet-CanSign-System-String-'></a>

#### CanSign(address) `method`

Check if this address is managed by this wallet.

###### Returns

`true` if the address is managed by this wallter, `false` if not.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **address** - The address. Value returned by [AddRawKey](#M-In3-Crypto-SimpleWallet-AddRawKey-System-String- "In3.Crypto.SimpleWallet.AddRawKey(System.String)").

<a name='M-In3-Crypto-SimpleWallet-PrepareTransaction-In3-Eth1-TransactionRequest-'></a>

#### PrepareTransaction(tx) `method`

Identity function-like method.

###### Returns

`tx`

###### Parameters

- [In3.Eth1.TransactionRequest](#T-In3-Eth1-TransactionRequest "In3.Eth1.TransactionRequest") **tx** - A transaction object.

<a name='M-In3-Crypto-SimpleWallet-Sign-System-String,System-String-'></a>

#### Sign(data,address) `method`

Signs the transaction data by invoking [SignData](#M-In3-Crypto-Api-SignData-System-String,System-String,In3-Crypto-SignatureType- "In3.Crypto.Api.SignData(System.String,System.String,In3.Crypto.SignatureType)").

###### Returns

Signed transaction data.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **data** - Data to be signed.
- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **address** - Address managed by the wallet, see [AddRawKey](#M-In3-Crypto-SimpleWallet-AddRawKey-System-String- "In3.Crypto.SimpleWallet.AddRawKey(System.String)")

<a name='T-In3-Storage-Storage'></a>

### Storage `type`

In3.Storage

Provider methods to cache data.
These data could be nodelists, contract codes or validator changes.
Any form of cache should implement [Storage](#T-In3-Storage-Storage "In3.Storage.Storage") and be set with [Storage](#P-In3-IN3-Storage "In3.IN3.Storage").

<a name='M-In3-Storage-Storage-Clear'></a>

#### Clear() `method`

Clear the cache.

###### Returns

The result of the operation: `true` for success and `false` for failure.

###### Parameters

This method has no parameters.

<a name='M-In3-Storage-Storage-GetItem-System-String-'></a>

#### GetItem(key) `method`

returns a item from cache.

###### Returns

The bytes or `null` if not found.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **key** - The key for the item.

<a name='M-In3-Storage-Storage-SetItem-System-String,System-Byte[]-'></a>

#### SetItem(key,content) `method`

Stores an item to cache.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **key** - The key for the item.
- [System.Byte[]](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.Byte[] "System.Byte[]") **content** - The value to store.

<a name='T-In3-Btc-Transaction'></a>

### Transaction `type`

In3.Btc

A BitCoin Transaction.

<a name='T-In3-Eth1-Transaction'></a>

### Transaction `type`

In3.Eth1

Class representing a transaction that was accepted by the Ethereum chain.

<a name='P-In3-Btc-Transaction-Blockhash'></a>

#### Blockhash `property`

The block hash of the block containing this transaction.

<a name='P-In3-Btc-Transaction-Blocktime'></a>

#### Blocktime `property`

The block time in seconds since epoch (Jan 1 1970 GMT).

<a name='P-In3-Btc-Transaction-Confirmations'></a>

#### Confirmations `property`

The confirmations.

<a name='P-In3-Btc-Transaction-Hash'></a>

#### Hash `property`

The transaction hash (differs from txid for witness transactions).

<a name='P-In3-Btc-Transaction-Hex'></a>

#### Hex `property`

The hex representation of raw data.

<a name='P-In3-Btc-Transaction-Locktime'></a>

#### Locktime `property`

The locktime.

<a name='P-In3-Btc-Transaction-Size'></a>

#### Size `property`

The serialized transaction size.

<a name='P-In3-Btc-Transaction-Time'></a>

#### Time `property`

The transaction time in seconds since epoch (Jan 1 1970 GMT).

<a name='P-In3-Btc-Transaction-Txid'></a>

#### Txid `property`

Transaction Id.

<a name='P-In3-Btc-Transaction-Version'></a>

#### Version `property`

The version.

<a name='P-In3-Btc-Transaction-Vin'></a>

#### Vin `property`

The transaction inputs.

<a name='P-In3-Btc-Transaction-Vout'></a>

#### Vout `property`

The transaction outputs.

<a name='P-In3-Btc-Transaction-Vsize'></a>

#### Vsize `property`

The virtual transaction size (differs from size for witness transactions).

<a name='P-In3-Btc-Transaction-Weight'></a>

#### Weight `property`

The transaction’s weight (between vsize4-3 and vsize4).

<a name='P-In3-Eth1-Transaction-BlockHash'></a>

#### BlockHash `property`

Hash of the block that this transaction belongs to.

<a name='P-In3-Eth1-Transaction-BlockNumber'></a>

#### BlockNumber `property`

Number of the block that this transaction belongs to.

<a name='P-In3-Eth1-Transaction-ChainId'></a>

#### ChainId `property`

Chain id that this transaction belongs to.

<a name='P-In3-Eth1-Transaction-Creates'></a>

#### Creates `property`

Address of the deployed contract (if successfull).

<a name='P-In3-Eth1-Transaction-From'></a>

#### From `property`

Address whose private key signed this transaction with.

<a name='P-In3-Eth1-Transaction-Gas'></a>

#### Gas `property`

Gas for the transaction.

<a name='P-In3-Eth1-Transaction-GasPrice'></a>

#### GasPrice `property`

Gas price (in wei) for each unit of gas.

<a name='P-In3-Eth1-Transaction-Hash'></a>

#### Hash `property`

Transaction hash.

<a name='P-In3-Eth1-Transaction-Input'></a>

#### Input `property`

Transaction data.

<a name='P-In3-Eth1-Transaction-Nonce'></a>

#### Nonce `property`

Nonce for this transaction.

<a name='P-In3-Eth1-Transaction-PublicKey'></a>

#### PublicKey `property`

Public key.

<a name='P-In3-Eth1-Transaction-R'></a>

#### R `property`

Part of the transaction signature.

<a name='P-In3-Eth1-Transaction-Raw'></a>

#### Raw `property`

Transaction as rlp encoded data.

<a name='P-In3-Eth1-Transaction-S'></a>

#### S `property`

Part of the transaction signature.

<a name='P-In3-Eth1-Transaction-StandardV'></a>

#### StandardV `property`

Part of the transaction signature. V is parity set by v = 27 + (r % 2).

<a name='P-In3-Eth1-Transaction-To'></a>

#### To `property`

To address of the transaction.

<a name='P-In3-Eth1-Transaction-TransactionIndex'></a>

#### TransactionIndex `property`

Transaction index.

<a name='P-In3-Eth1-Transaction-V'></a>

#### V `property`

The [StandardV](#P-In3-Eth1-Transaction-StandardV "In3.Eth1.Transaction.StandardV") plus the chain.

<a name='P-In3-Eth1-Transaction-Value'></a>

#### Value `property`

Value of the transaction.

<a name='T-In3-Eth1-TransactionBlock'></a>

### TransactionBlock `type`

In3.Eth1

Class that holds a block with its full transaction array: [Transaction](#T-In3-Eth1-Transaction "In3.Eth1.Transaction").

<a name='P-In3-Eth1-TransactionBlock-Transactions'></a>

#### Transactions `property`

Array with the full transactions containing on this block.

###### Remarks

Returned when `shouldIncludeTransactions` on [Api](#T-In3-Eth1-Api "In3.Eth1.Api") get block methods are set to `true`.

<a name='T-In3-Eth1-TransactionHashBlock'></a>

### TransactionHashBlock `type`

In3.Eth1

Class that holds a block with its transaction hash array.

<a name='P-In3-Eth1-TransactionHashBlock-Transactions'></a>

#### Transactions `property`

Array with the full transactions containing on this block.

###### Remarks

Returned when `shouldIncludeTransactions` on [Api](#T-In3-Eth1-Api "In3.Eth1.Api") get block methods are set to `false`.

<a name='T-In3-Btc-TransactionInput'></a>

### TransactionInput `type`

In3.Btc

Input of a transaction.

<a name='P-In3-Btc-TransactionInput-ScriptSig'></a>

#### ScriptSig `property`

The script.

<a name='P-In3-Btc-TransactionInput-Sequence'></a>

#### Sequence `property`

The script sequence number.

<a name='P-In3-Btc-TransactionInput-Txid'></a>

#### Txid `property`

The transaction id.

<a name='P-In3-Btc-TransactionInput-Txinwitness'></a>

#### Txinwitness `property`

Hex-encoded witness data (if any).

<a name='P-In3-Btc-TransactionInput-Yout'></a>

#### Yout `property`

The index of the transactionoutput.

<a name='T-In3-Btc-TransactionOutput'></a>

### TransactionOutput `type`

In3.Btc

Output of a transaction.

<a name='P-In3-Btc-TransactionOutput-N'></a>

#### N `property`

The index in the transaction.

<a name='P-In3-Btc-TransactionOutput-ScriptPubKey'></a>

#### ScriptPubKey `property`

The script of the transaction.

<a name='P-In3-Btc-TransactionOutput-Value'></a>

#### Value `property`

The value in bitcoins.

<a name='T-In3-Eth1-TransactionReceipt'></a>

### TransactionReceipt `type`

In3.Eth1

Class that represents a transaction receipt. See [GetTransactionReceipt](#M-In3-Eth1-Api-GetTransactionReceipt-System-String- "In3.Eth1.Api.GetTransactionReceipt(System.String)").

<a name='P-In3-Eth1-TransactionReceipt-BlockHash'></a>

#### BlockHash `property`

Hash of the block with the transaction which this receipt is associated with.

<a name='P-In3-Eth1-TransactionReceipt-BlockNumber'></a>

#### BlockNumber `property`

Number of the block with the transaction which this receipt is associated with.

<a name='P-In3-Eth1-TransactionReceipt-ContractAddress'></a>

#### ContractAddress `property`

Address of the smart contract invoked in the transaction (if any).

<a name='P-In3-Eth1-TransactionReceipt-From'></a>

#### From `property`

Address of the account that signed the transaction.

<a name='P-In3-Eth1-TransactionReceipt-GasUsed'></a>

#### GasUsed `property`

Gas used on this transaction.

<a name='P-In3-Eth1-TransactionReceipt-Logs'></a>

#### Logs `property`

Logs/events for this transaction.

<a name='P-In3-Eth1-TransactionReceipt-LogsBloom'></a>

#### LogsBloom `property`

A bloom filter of logs/events generated by contracts during transaction execution. Used to efficiently rule out transactions without expected logs.

<a name='P-In3-Eth1-TransactionReceipt-Root'></a>

#### Root `property`

Merkle root of the state trie after the transaction has been executed (optional after Byzantium hard fork EIP609).

<a name='P-In3-Eth1-TransactionReceipt-Status'></a>

#### Status `property`

Status of the transaction.

<a name='P-In3-Eth1-TransactionReceipt-To'></a>

#### To `property`

Address whose value will be transfered to.

<a name='P-In3-Eth1-TransactionReceipt-TransactionHash'></a>

#### TransactionHash `property`

Hash of the transaction.

<a name='P-In3-Eth1-TransactionReceipt-TransactionIndex'></a>

#### TransactionIndex `property`

Number of the transaction on the block.

<a name='T-In3-Eth1-TransactionRequest'></a>

### TransactionRequest `type`

In3.Eth1

Class that holds the state for the transaction request to be submited via [SendTransaction](#M-In3-Eth1-Api-SendTransaction-In3-Eth1-TransactionRequest- "In3.Eth1.Api.SendTransaction(In3.Eth1.TransactionRequest)").

<a name='P-In3-Eth1-TransactionRequest-Data'></a>

#### Data `property`

Data of the transaction (in the case of a smart contract deployment for exemple).

<a name='P-In3-Eth1-TransactionRequest-From'></a>

#### From `property`

Address derivated from the private key that will sign the transaction. See [Signer](#T-In3-Crypto-Signer "In3.Crypto.Signer").

<a name='P-In3-Eth1-TransactionRequest-Function'></a>

#### Function `property`

Function of the smart contract to be invoked.

<a name='P-In3-Eth1-TransactionRequest-Gas'></a>

#### Gas `property`

Gas cost for the transaction. Can be estimated via [EstimateGas](#M-In3-Eth1-Api-EstimateGas-In3-Eth1-TransactionRequest,System-Numerics-BigInteger- "In3.Eth1.Api.EstimateGas(In3.Eth1.TransactionRequest,System.Numerics.BigInteger)").

<a name='P-In3-Eth1-TransactionRequest-GasPrice'></a>

#### GasPrice `property`

Gas price (in wei). Can be obtained via [GetGasPrice](#M-In3-Eth1-Api-GetGasPrice "In3.Eth1.Api.GetGasPrice").

<a name='P-In3-Eth1-TransactionRequest-Nonce'></a>

#### Nonce `property`

Nonce of the transaction.

<a name='P-In3-Eth1-TransactionRequest-Params'></a>

#### Params `property`

Array of parameters for the function (in the same order of its signature), see [Function](#P-In3-Eth1-TransactionRequest-Function "In3.Eth1.TransactionRequest.Function")

<a name='P-In3-Eth1-TransactionRequest-To'></a>

#### To `property`

Address to whom the transaction value will be transfered to or the smart contract address whose function will be invoked.

<a name='P-In3-Eth1-TransactionRequest-Value'></a>

#### Value `property`

Value of the transaction.

<a name='T-In3-Transport-Transport'></a>

### Transport `type`

In3.Transport

Minimum interface for a custom transport. Transport is a mean of communication with the Incubed server.

<a name='M-In3-Transport-Transport-Handle-System-String,System-String-'></a>

#### Handle(url,payload) `method`

Method to be implemented that will handle the requests to the server. This method may be called once for each url on each batch of requests.

###### Returns

The rpc response.

###### Parameters

- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **url** - Url of the node.
- [System.String](http://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k:System.String "System.String") **payload** - Content for the RPC request.
