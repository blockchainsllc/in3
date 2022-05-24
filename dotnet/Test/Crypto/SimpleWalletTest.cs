using System.Threading.Tasks;
using In3;
using In3.Crypto;
using NUnit.Framework;

namespace Test.Crypto
{
    public class SimpleWalletTest
    {
        private SimpleWallet _wallet;

        [SetUp]
        public void Setup()
        {
            ClientFactory factory = new ProoflessClientFactory(Chain.Mainnet);
            IN3 in3 = factory.CreateIn3(new string[][] { });
            _wallet = (SimpleWallet) in3.Signer;
        }

        [Test]
        public async Task Sign()
        {;
            string signerId = _wallet.AddRawKey("0x889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6");
            string dataToSign = "0xdeadbeaf";

            string expectedSignerData =
                "0xb5abef88f0f895ccfcd1f2eb6cf77b417e64600ea5a4c97f9d77056345a4a68578bc41ad6066fdfa95b26c91d458f17f2c304796a21a3f496ba6334198aa920b00";

            string signedData = await _wallet.Sign(dataToSign, signerId, DigestType.Hash, PayloadType.EthTx, CurveType.Ecdsa);

            Assert.That(signerId, Is.EqualTo("0x082977959d0C5A1bA627720ac753Ec2ADB5Bd7d0".ToLower()));
            Assert.That(signedData, Is.EqualTo(expectedSignerData));
        }
    }
}