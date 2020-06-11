using In3;
using In3.Btc;
using NUnit.Framework;

namespace Test.Btc
{
    public class ApiTest
    {
        private ClientBuilder _builder;

        [SetUp]
        public void Setup()
        {
            _builder = new ClientBuilder(Chain.Btc);
        }

        [Test]
        public void GetBlockHeader()
        {
            string[][] mockedResponses =
            {
                new[] {"getblockheader", "btc_GetBlockHeader_1.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            BlockHeader result = in3.Btc.GetBlockHeader("0000000000000000000cd3c5d7638014e78a5fba33be5fa5cb10ef9f03d99e60");

            Assert.That(result.Hash, Is.EqualTo("0000000000000000000cd3c5d7638014e78a5fba33be5fa5cb10ef9f03d99e60"));
            Assert.That(result.Confirmations, Is.EqualTo(13));
            Assert.That(result.Height, Is.EqualTo(634026));
            Assert.That(result.Version, Is.EqualTo(1073676288));
            Assert.That(result.VersionHex, Is.EqualTo("3fff0000"));
            Assert.That(result.Merkleroot, Is.EqualTo("0b27b05fb5c9c53f595cf12696daa3ea4fb03ea67e0ffd909a1f2bb6544b63c1"));
            Assert.That(result.Time, Is.EqualTo(1591782412));
            Assert.That(result.Mediantime, Is.EqualTo(1591778995));
            Assert.That(result.Nonce, Is.EqualTo(783981944));
            Assert.That(result.Bits, Is.EqualTo("17147f35"));
            Assert.That(result.Difficulty, Is.EqualTo(13732352106018.34f));
            Assert.That(result.Chainwork, Is.EqualTo("00000000000000000000000000000000000000001038fd1e673c4ff045dbd6c1"));
            Assert.That(result.NTx, Is.EqualTo(1659));
            Assert.That(result.Previousblockhash, Is.EqualTo("00000000000000000013a6ca3921ce63af646ac191c733d9728103d7a9e3236e"));
            Assert.That(result.Nextblockhash, Is.EqualTo("0000000000000000000d1e4cf9f6f0a6fa88aa162e08a966bf3043f7f77e21bb"));
        }

        [Test]
        public void GetBlockHeaderBytes()
        {
            string[][] mockedResponses =
            {
                new[] {"getblockheader", "btc_GetBlockHeaderBytes_1.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            byte[] expectedBytes = new byte[]
            {
                0, 0, 255, 63, 110, 35, 227, 169, 215, 3, 129, 114, 217, 51, 199, 145, 193, 106, 100, 175, 99, 206, 33, 57, 202, 166, 19, 0, 0, 0, 0, 0, 0, 0, 0, 0, 193, 99, 75, 84, 182, 43, 31, 154, 144, 253, 15, 126, 166, 62, 176, 79, 234, 163, 218, 150, 38, 241, 92, 89, 63, 197, 201, 181, 95, 176, 39, 11, 12, 172, 224, 94, 53, 127, 20, 23, 120, 157, 186, 46
            };

            byte[] result = in3.Btc.GetBlockHeaderBytes("0000000000000000000cd3c5d7638014e78a5fba33be5fa5cb10ef9f03d99e60");

            Assert.That(result, Is.EqualTo(expectedBytes));
        }

        [Test]
        public void GetTransaction()
        {
            string[][] mockedResponses =
            {
                new[] {"getrawtransaction", "btc_GetTransaction_1.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            Transaction result = in3.Btc.GetTransaction("1427c7d1698e61afe061950226f1c149990b8c1e1b157320b0c4acf7d6b5605d");

            Assert.That(result.Txid, Is.EqualTo("1427c7d1698e61afe061950226f1c149990b8c1e1b157320b0c4acf7d6b5605d"));
            Assert.That(result.Blocktime, Is.EqualTo(1591782412));
            Assert.That(result.Confirmations, Is.EqualTo(17));
            Assert.That(result.Hash, Is.EqualTo("371d1b497b6a9930b1f40c30ea18df4027bf452bb312c5945648d85d1f56dad5"));
            Assert.That(result.Hex, Is.EqualTo("02000000000101adba296f842330296785f061ca9e152dec63fe235d0ef1d4d38cc4a67f586c7c0000000000feffffff0280f0fa020000000017a914e9f20f1225a9528739495649405861ae5d77ba1e871cc9df050000000017a9141abf9a43d3a56f06930b95b1a8f0161bc0b0c9be8702483045022100a1c74e429c4e40ef90d915556ce4b54a9aa4a83872622d0dbbaca3029f07f2d802204d99cd230b2e1d7378401d502cf589227272173f93b3ccc4aed6f97988067e780121035ad17694971c7dadab72369ab2444e355ae7d17ed8fba67aab80da9a3556d37c7cac0900"));
            Assert.That(result.Locktime, Is.EqualTo(633980));
            Assert.That(result.Size, Is.EqualTo(225));
            Assert.That(result.Time, Is.EqualTo(1591782412));
            Assert.That(result.Version, Is.EqualTo(2));
            Assert.That(result.Vin.Length, Is.EqualTo(1));
            Assert.That(result.Vout.Length, Is.EqualTo(2));
            Assert.That(result.Vsize, Is.EqualTo(143));
            Assert.That(result.Weight, Is.EqualTo(570));
        }

        [Test]
        public void GetTransactionBytes()
        {
            string[][] mockedResponses =
            {
                new[] {"getrawtransaction", "btc_GetTransactionBytes_1.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            byte[] result = in3.Btc.GetTransactionBytes("1427c7d1698e61afe061950226f1c149990b8c1e1b157320b0c4acf7d6b5605d");

            Assert.That(result.Length, Is.EqualTo(225));
        }

        [Test]
        public void GetBlockWithTxIds()
        {
            string[][] mockedResponses =
            {
                new[] {"getblock", "btc_GetBlockWithTxIds_1.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            Block<string> result = in3.Btc.GetBlockWithTxIds("0000000000000000000cd3c5d7638014e78a5fba33be5fa5cb10ef9f03d99e60");

            Assert.That(result, Is.EqualTo(result));
            Assert.That(result.Hash, Is.EqualTo("0000000000000000000cd3c5d7638014e78a5fba33be5fa5cb10ef9f03d99e60"));
            Assert.That(result.Confirmations, Is.EqualTo(51));
            Assert.That(result.Height, Is.EqualTo(634026));
            Assert.That(result.Version, Is.EqualTo(1073676288));
            Assert.That(result.VersionHex, Is.EqualTo("3fff0000"));
            Assert.That(result.Merkleroot, Is.EqualTo("0b27b05fb5c9c53f595cf12696daa3ea4fb03ea67e0ffd909a1f2bb6544b63c1"));
            Assert.That(result.Time, Is.EqualTo(1591782412));
            Assert.That(result.Mediantime, Is.EqualTo(1591778995));
            Assert.That(result.Nonce, Is.EqualTo(783981944));
            Assert.That(result.Bits, Is.EqualTo("17147f35"));
            Assert.That(result.Difficulty, Is.EqualTo(13732352106018.34f));
            Assert.That(result.Chainwork, Is.EqualTo("00000000000000000000000000000000000000001038fd1e673c4ff045dbd6c1"));
            Assert.That(result.NTx, Is.EqualTo(1659));
            Assert.That(result.Previousblockhash, Is.EqualTo("00000000000000000013a6ca3921ce63af646ac191c733d9728103d7a9e3236e"));
            Assert.That(result.Nextblockhash, Is.EqualTo("0000000000000000000d1e4cf9f6f0a6fa88aa162e08a966bf3043f7f77e21bb"));
            Assert.That(result.Tx.Length, Is.EqualTo(1659));
        }

        [Test]
        public void GetBlockWithTxData()
        {
            string[][] mockedResponses =
            {
                new[] {"getblock", "btc_GetBlockWithTxData_1.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            Block<Transaction> result = in3.Btc.GetBlockWithTxData("000000000000000000064ba7512ecc70cabd7ed17e31c06f2205d5ecdadd6d22");

            Assert.That(result.Hash, Is.EqualTo("000000000000000000064ba7512ecc70cabd7ed17e31c06f2205d5ecdadd6d22"));
            Assert.That(result.Confirmations, Is.EqualTo(83));
            Assert.That(result.Height, Is.EqualTo(634007));
            Assert.That(result.Version, Is.EqualTo(536870912));
            Assert.That(result.VersionHex, Is.EqualTo("20000000"));
            Assert.That(result.Merkleroot, Is.EqualTo("22f78daf63c48b582142421ea17e3a989b1421c1a9d07585668962f19fe12558"));
            Assert.That(result.Time, Is.EqualTo(1591770949));
            Assert.That(result.Mediantime, Is.EqualTo(1591768893));
            Assert.That(result.Nonce, Is.EqualTo(3201445374));
            Assert.That(result.Bits, Is.EqualTo("17147f35"));
            Assert.That(result.Difficulty, Is.EqualTo(13732352106018.34f));
            Assert.That(result.Chainwork, Is.EqualTo("000000000000000000000000000000000000000010380fd08a8436abc886cbc8"));
            Assert.That(result.NTx, Is.EqualTo(131));
            Assert.That(result.Previousblockhash, Is.EqualTo("00000000000000000008c486d4d80a3bd24cc33b0011538baa8d1c3fa7d54c76"));
            Assert.That(result.Nextblockhash, Is.EqualTo("00000000000000000001d7fb302a3c72f7cc77d4def5d1d9503195469eb2049f"));
            Assert.That(result.Tx.Length, Is.EqualTo(131));
        }

        [Test]
        public void GetBlockBytes()
        {
            string[][] mockedResponses =
            {
                new[] {"getblock", "btc_GetBlockBytes_1.json"}
            };
            IN3 in3 = _builder.ConstructClient(mockedResponses);

            byte[] result = in3.Btc.GetBlockBytes("000000000000000000064ba7512ecc70cabd7ed17e31c06f2205d5ecdadd6d22");

            Assert.That(result.Length, Is.EqualTo(55812));
        }
    }
}