using System.Numerics;
using In3;
using In3.Crypto;
using In3.Eth1;
using In3.Utils;
using NUnit.Framework;

namespace Test.Eth1
{
    public class ApiTest
    {
        private ClientBuilder _builder;

        [SetUp]
        public void Setup()
        {
            _builder = new ClientBuilder(Chain.Mainnet);
        }

        [Test]
        public void BlockNumber()
        {
            string[][] mockedResponses =
            {
                new[] {"eth_blockNumber", "eth_blockNumber.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            BigInteger result = in3.Eth1.BlockNumber();

            Assert.That(result, Is.EqualTo(new BigInteger(3220)));
        }

        [Test]
        public void BlockByNumber()
        {
            string[][] mockedResponses = {
                new [] {"eth_getBlockByNumber", "eth_getBlockByNumber.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            Block latest = in3.Eth1.GetBlockByNumber(BlockParameter.Earliest);

            Assert.That(latest.Number, Is.EqualTo(new BigInteger(0)));
            Assert.That(latest.Size, Is.EqualTo(540));
        }

        [Test]
        public void BlockByHash()
        {
            string[][] mockedResponses = {
                new [] {"eth_getBlockByHash", "eth_getBlockByHash.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            Block latest = in3.Eth1.GetBlockByHash("0xd4e56740f876aef8c010b86a40d5f56745a118d0906a34e69aec8c0db1cb8fa3");

            Assert.That(latest.Number, Is.EqualTo(new BigInteger(0)));
            Assert.That(latest.Size, Is.EqualTo(540));
        }

        [Test]
        public void AbiEncode()
        {
            IN3 in3 = _builder.ConstructClient(new string[][] { });

            string signature = "getBalance(address)";
            string[] args = { "0x1234567890123456789012345678901234567890" };
            string expectedEncoded = "0xf8b2cb4f0000000000000000000000001234567890123456789012345678901234567890";

            string result = in3.Eth1.AbiEncode(signature, args);

            Assert.That(result, Is.EqualTo(expectedEncoded));
        }

        [Test]
        public void AbiDecode()
        {
            IN3 in3 = _builder.ConstructClient(new string[][] { });

            string signature = "(address,uint256)";
            string encoded = "0x00000000000000000000000012345678901234567890123456789012345678900000000000000000000000000000000000000000000000000000000000000005";
            string[] expectedDecode = { "0x1234567890123456789012345678901234567890", "0x05" };

            string[] result = in3.Eth1.AbiDecode(signature, encoded);

            Assert.That(result, Is.EqualTo(expectedDecode));
        }

        [Test]
        public void GetGasPrice()
        {
            string[][] mockedResponses = {
                new [] {"eth_gasPrice", "eth_gasPrice.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            BigInteger currentGasPrice = in3.Eth1.GetGasPrice();

            Assert.That(currentGasPrice, Is.EqualTo(new BigInteger(2100000000)));
        }

        [Test]
        public void GetChainId()
        {
            string[][] mockedResponses = {
                new [] {"eth_estimateGas", "eth_estimateGas.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            Chain currentChain = in3.Eth1.GetChainId();
            Assert.That(currentChain, Is.EqualTo(Chain.Mainnet));
        }

        [Test]
        public void GetCode()
        {
            string[][] mockedResponses = {
                new [] {"eth_getCode", "eth_getCode.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            string response = in3.Eth1.GetCode("0xdAC17F958D2ee523a2206206994597C13D831ec7", BlockParameter.Latest);

            Assert.NotNull(response);
        }

        [Test]
        public void GetStorageAt()
        {
            string[][] mockedResponses = {
                new [] {"eth_getStorageAt", "eth_getStorageAt.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            string storage = in3.Eth1.GetStorageAt("0x862174623bc39e57de552538f424806b947d3d05", 0, BlockParameter.Latest);

            Assert.That(storage, Is.EqualTo("0x0000000000000000000000000000000000000000000000000000000000000000"));
        }

        [Test]
        public void GetBlockTransactionCountByHash()
        {
            string[][] mockedResponses = {
                new [] {"eth_getBlockTransactionCountByHash", "eth_getBlockTransactionCountByHash.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            long transactionCount = in3.Eth1
                .GetBlockTransactionCountByHash("0xd4e56740f876aef8c010b86a40d5f56745a118d0906a34e69aec8c0db1cb8fa3");

            Assert.That(transactionCount, Is.EqualTo(0));
        }

        [Test]
        public void GetBlockTransactionCountByNumber()
        {
            string[][] mockedResponses = {
                new [] {"eth_getBlockTransactionCountByNumber", "eth_getBlockTransactionCountByNumber.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            long transactionCount = in3.Eth1
                .GetBlockTransactionCountByNumber(9298869);

            Assert.That(transactionCount, Is.EqualTo(121));
        }

        [Test]
        public void GetTransactionByBlockHashAndIndex()
        {
            string[][] mockedResponses = {
                new [] {"eth_getTransactionByBlockHashAndIndex", "eth_getTransactionByBlockHashAndIndex.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            Transaction tx = in3.Eth1.GetTransactionByBlockHashAndIndex(
                "0xd03f4a0ce830ce568be08aa37bc0a72374e92da5b388e839b35f24a144a5085d", 1);
            Assert.That(tx.Value, Is.EqualTo(new BigInteger(48958690000000000)));
        }

        [Test]
        public void GetTransactionByBlockNumberAndIndex()
        {
            string[][] mockedResponses = {
                new [] {"eth_getTransactionByBlockNumberAndIndex", "eth_getTransactionByBlockNumberAndIndex.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            Transaction tx = in3.Eth1.GetTransactionByBlockNumberAndIndex(9319093, 1);

            Assert.That(tx.Value, Is.EqualTo(new BigInteger(48958690000000000)));
            Assert.That(tx.From, Is.EqualTo("0xe3649077ce21a48caf34041e983b92e332e80fd9"));
        }

        [Test]
        public void GetTransactionByHash()
        {
            string[][] mockedResponses = {
                new [] {"eth_getTransactionByHash", "eth_getTransactionByHash.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            Transaction tx = in3.Eth1.GetTransactionByHash("0x6188bf0672c005e30ad7c2542f2f048521662e30c91539d976408adf379bdae2");

            Assert.That(tx.BlockHash, Is.EqualTo("0x8220e66456e40636bff3a440832c9f179e4811d4e28269c7ab70142c3e5f9be2"));
            Assert.That(tx.From, Is.EqualTo("0x3a9e354dee60df25c0389badafec8457e36ebfd2"));
        }

        [Test]
        public void GetTransactionCount()
        {
            string[][] mockedResponses = {
                new [] {"eth_getTransactionCount", "eth_getTransactionCount.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            string from = "0x7fc7032e731f5bcbd4843406945acaf917087fde";

            long transactionCount = in3.Eth1.GetTransactionCount(from, BlockParameter.Latest);

            Assert.That(transactionCount, Is.EqualTo(19));
        }

        [Test]
        public void SendRawTransaction()
        {
            string[][] mockedResponses = {
                new [] {"eth_sendRawTransaction", "eth_sendRawTransaction_1.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            string rawTransaction = "0xf8671b8477359400825208943940256b93c4be0b1d5931a6a036608c25706b0c8405f5e100802da0278d2c010a59688fc12a55563d81239b1dc7e3d9c6a535b34600329b0c640ad8a03894daf8d7c25b56caec71b695c5f6b1b6fd903ecfa441b2c4e15fd1c72c54a9";

            string hash = in3.Eth1.SendRawTransaction(rawTransaction);

            Assert.That(hash, Is.EqualTo("0xd55a8b0cf4896ffbbb10b125bf20d89c8006f42cc327a9859c59ac54e439b388"));
        }

        [Test]
        public void SendTransaction()
        {
            string[][] mockedResponses = {
                new[] {"eth_gasPrice", "eth_gasPrice.json"},
                new[] {"eth_estimateGas", "eth_estimateGas.json"},
                new[] {"eth_getTransactionCount", "eth_getTransactionCount.json"},
                new[] {"eth_sendRawTransaction", "eth_sendRawTransaction.json"}
            };

            IN3 in3 = _builder.ConstructClient(mockedResponses);
            string expectedHash = "0xd5651b7c0b396c16ad9dc44ef0770aa215ca795702158395713facfbc9b55f38";
            string pk = "0x0829B3C639A3A8F2226C8057F100128D4F7AE8102C92048BA6DE38CF4D3BC6F1";
            SimpleWallet sw = (SimpleWallet)in3.Signer;
            string from = sw.AddRawKey(pk);

            TransactionRequest request = new TransactionRequest();
            request.From = from;
            request.To = "0x3940256B93c4BE0B1d5931A6A036608c25706B0c";
            request.Gas = 21000;
            request.Value = 100000000;

            object txHash = in3.Eth1.SendTransaction(request);
            Assert.That(txHash, Is.EqualTo(expectedHash));
        }

        [Test]
        public void ChecksumAddress()
        {
            IN3 in3 = _builder.ConstructClient(new string[][] { });

            string expectedAddress = "0xBc0ea09C1651A3D5D40Bacb4356FB59159A99564";
            string address = in3.Eth1.ChecksumAddress("0xbc0ea09c1651a3d5d40bacb4356fb59159a99564");

            Assert.That(address, Is.EqualTo(expectedAddress));
        }

        [Test]
        public void GetUncleCountByBlockHash()
        {
            string[][] mockedResponses = {
                new[] {"eth_getUncleCountByBlockHash", "eth_getUncleCountByBlockHash.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            long uncleCount = in3.Eth1.GetUncleCountByBlockHash("0x884aaab2f9116742e693ced034a2dff840b45f21709025f7d69cde26d071068b");

            Assert.That(uncleCount, Is.EqualTo(1));
        }

        [Test]
        public void GetUncleCountByBlockNumber()
        {
            string[][] mockedResponses = {
                new[] {"eth_getUncleCountByBlockNumber", "eth_getUncleCountByBlockNumber.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            long uncleCount = in3.Eth1.GetUncleCountByBlockNumber(9830239);

            Assert.That(uncleCount, Is.EqualTo(2));
        }

        [Test]
        public void NewBlockFilter()
        {
            string[][] mockedResponses = {
                new[] {"eth_blockNumber", "eth_blockNumber_1.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            long filterId = in3.Eth1.NewBlockFilter();

            Assert.That(filterId > 0);

            // Given the same mock response, this should increment the filter_id
            long filterId2 = in3.Eth1.NewBlockFilter();

            Assert.That(filterId + 1, Is.EqualTo(filterId2));
        }

        [Test]
        public void UninstallFilter()
        {
            string[][] mockedResponses = {
                new[] {"eth_blockNumber", "eth_blockNumber_1.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            LogFilter filter = new LogFilter();
            filter.Address = "0xF0AD5cAd05e10572EfcEB849f6Ff0c68f9700455";
            long filterId = in3.Eth1.NewLogFilter(filter);

            bool success1 = in3.Eth1.UninstallFilter(filterId);
            Assert.That(success1);
            bool success2 = in3.Eth1.UninstallFilter(filterId);
            Assert.That(!success2);
        }

        [Test]
        public void Call()
        {
            string[][] mockedResponses = {
                new[] {"eth_call", "eth_call_1.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            TransactionRequest request = new TransactionRequest();
            request.To = "0x2736D225f85740f42D17987100dc8d58e9e16252"; ;
            request.Function = "servers(uint256):(string,address,uint32,uint256,uint256,address)";
            request.Params = new object[] { 1 };

            string[] res1 = (string[])in3.Eth1.Call(request, BlockParameter.Latest);

            Assert.That(res1.Length, Is.EqualTo(6));
            Assert.That(res1[0], Is.EqualTo("https://in3.slock.it/mainnet/nd-4"));
            Assert.That(res1[1], Is.EqualTo("0xbc0ea09c1651a3d5d40bacb4356fb59159a99564"));
            Assert.That(res1[2], Is.EqualTo("0xffff"));
            Assert.That(res1[3], Is.EqualTo("0xffff"));
        }

        [Test]
        public void GetBalance()
        {
            string[][] mockedResponses = {
                new[] {"eth_getBalance", "eth_getBalance.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            BigInteger expectedBalance = 3646260000000000000;
            string address = "0x4144FFD5430a8518fa2d84ef5606Fd7e1921cE27";
            BigInteger balance = in3.Eth1.GetBalance(address, BlockParameter.Latest);

            Assert.That(balance, Is.EqualTo(expectedBalance));
        }

        [Test]
        public void EstimateGas()
        {
            string[][] mockedResponses = {
                new[] {"eth_estimateGas", "eth_estimateGas.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);
            string pk = "0x0829B3C639A3A8F2226C8057F100128D4F7AE8102C92048BA6DE38CF4D3BC6F1";
            SimpleWallet wallet = (SimpleWallet)in3.Signer;
            string from = wallet.AddRawKey(pk);

            long expectedGasEstimate = 21000;
            TransactionRequest tx = new TransactionRequest();
            tx.GasPrice = 1;
            tx.From = from;
            tx.To = "0xEA674fdDe714fd979de3EdF0F56AA9716B898ec8";

            long gasEstimate = in3.Eth1.EstimateGas(tx, BlockParameter.Latest);
            Assert.That(gasEstimate, Is.EqualTo(expectedGasEstimate));
        }

        [Test]
        public void NewLogFilter()
        {
            string[][] mockedResponses = {
                new[] {"eth_blockNumber", "eth_blockNumber_1.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            LogFilter filter = new LogFilter();
            filter.Address = "0xF0AD5cAd05e10572EfcEB849f6Ff0c68f9700455";

            long filterId = in3.Eth1.NewLogFilter(filter);

            Assert.That(filterId > 0);

            // Given the same mock response, this should increment the filter_id
            long filterId2 = in3.Eth1.NewLogFilter(filter);

            Assert.That(filterId2, Is.EqualTo(filterId + 1));
        }

        [Test]
        public void GetFilterChanges()
        {
            string[][] mockedResponses = {
                new[] {"eth_blockNumber", "eth_blockNumber_2.json"},
                new[] {"eth_getLogs", "eth_getLogs_1.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            LogFilter filter = new LogFilter();
            filter.Address = "0x1c81079b0752881f3231318a1355e21de26bbeb5";
            filter.FromBlock = 2050343;
            long filterId = in3.Eth1.NewLogFilter(filter);

            Log[] logs1 = in3.Eth1.GetFilterChangesFromLogs(filterId);

            Assert.That(logs1.Length, Is.GreaterThan(0));

            Log[] logs2 = in3.Eth1.GetFilterChangesFromLogs(filterId);

            Assert.That(logs2.Length, Is.EqualTo(0));
        }

        [Test]
        public void GetFilterLogs()
        {
            string[][] mockedResponses = {
                new[] {"eth_blockNumber", "eth_blockNumber_2.json"},
                new[] {"eth_getLogs", "eth_getLogs_1.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            LogFilter filter = new LogFilter();
            filter.Address = "0x1c81079b0752881f3231318a1355e21de26bbeb5";
            filter.FromBlock = 2050343;
            long filterId = in3.Eth1.NewLogFilter(filter);

            Log[] logs1 = in3.Eth1.GetFilterChangesFromLogs(filterId);

            Assert.That(logs1.Length, Is.GreaterThan(0));

            Log[] logs2 = in3.Eth1.GetFilterChangesFromLogs(filterId);

            Assert.That(logs2.Length, Is.EqualTo(0));
        }

        [Test]
        public void GetLogs()
        {
            string[][] mockedResponses = {
                new[] {"eth_getLogs", "eth_getLogs.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            LogFilter filter = new LogFilter();
            filter.FromBlock = TypesMatcher.HexStringToBigint("0x834B77");
            filter.ToBlock = TypesMatcher.HexStringToBigint("0x834B77");
            filter.Address = "0xdac17f958d2ee523a2206206994597c13d831ec7";

            Log[] response = in3.Eth1.GetLogs(filter);

            Assert.That(response[0].TransactionHash, Is.EqualTo("0x20be6d27ed6a4c99c5dbeeb9081e114a9b400c52b80c4d10096c94ad7d3c1af6"));
        }

        [Test]
        public void GetTransactionReceipt()
        {
            string[][] mockedResponses = {
                new[] {"eth_getTransactionReceipt", "eth_getTransactionReceipt.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            TransactionReceipt receipt = in3.Eth1.GetTransactionReceipt("0x6188bf0672c005e30ad7c2542f2f048521662e30c91539d976408adf379bdae2");
            Assert.That(receipt.To, Is.EqualTo("0x5b8174e20996ec743f01d3b55a35dd376429c596"));
            Assert.That(receipt.Status);
            Assert.That(receipt.Logs[0].Address, Is.EqualTo("0x5b8174e20996ec743f01d3b55a35dd376429c596"));
        }

        [Test]
        public void GetUncleByBlockNumberAndIndex()
        {
            string[][] mockedResponses = {
                new[] {"eth_getUncleByBlockNumberAndIndex", "eth_getUncleByBlockNumberAndIndex.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            Block uncle = in3.Eth1.GetUncleByBlockNumberAndIndex(9317999, 0);

            Assert.That(uncle.Number, Is.EqualTo(new BigInteger(9317998)));
            Assert.That(uncle.Size, Is.EqualTo(37088));
        }
    }
}