using In3.Native;
using NUnit.Framework;
using System.Text.Json;

namespace Test.Eth1
{
    public class TransactionRequestTest
    {
        [Test]
        public void AbiEncode()
        {
            string signature = "getBalance(address)";
            string[] args = {
                "0x1234567890123456789012345678901234567890"
            };

            string expected  = "0xf8b2cb4f0000000000000000000000001234567890123456789012345678901234567890";
            Assert.That(result, Is.EqualTo(expected));
        }
    }
}