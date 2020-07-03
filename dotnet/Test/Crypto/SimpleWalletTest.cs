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
            ClientBuilder builder = new ClientBuilder(Chain.Mainnet);
            IN3 in3 = builder.ConstructClient(new string[][] { });
            _wallet = (SimpleWallet) in3.Signer;
        }

        [Test]
        public async Task Sign()
        {
            string pk = "0x889dbed9450f7a4b68e0732ccb7cd016dab158e6946d16158f2736fda1143ca6";
            string address = _wallet.AddRawKey(pk);
            string dataToSign = "1e194c68360307cfb715bf17878791ad1ced8da7d2e5f42b691074c577f41eac";

            string expectedSignerData =
                "0xf16dcaa830a3f710e28444df7df85fa927d8a66f789196fc2a3b934c829dbcaa5329be0711daba3b0c85ab23f1adb32c4e88fd8cb42b951d3be40af1bbd92e7400";

            string signedData = await _wallet.Sign(dataToSign, address);

            Assert.That(address, Is.EqualTo("0x082977959d0C5A1bA627720ac753Ec2ADB5Bd7d0".ToLower()));
            Assert.That(signedData, Is.EqualTo(expectedSignerData));
        }
    }
}