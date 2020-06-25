using System.Numerics;
using System.Text.Json;
using In3.Eth1;
using In3.Utils;
using NUnit.Framework;

namespace Test.Utils
{
    public class DataTypeConverterTest
    {
        [Test]
        public void BigIntToPrefixedHex()
        {
            BigInteger n1 = -3482395934534543;
            BigInteger n2 = 0;
            BigInteger n3 = 15;
            BigInteger n4 = 17;

            Assert.That(DataTypeConverter.BigIntToPrefixedHex(n1), Is.EqualTo("-0xC5F387CA52B8F"));
            Assert.That(DataTypeConverter.BigIntToPrefixedHex(n2), Is.EqualTo("0x0"));
            Assert.That(DataTypeConverter.BigIntToPrefixedHex(n3), Is.EqualTo("0xF"));
            Assert.That(DataTypeConverter.BigIntToPrefixedHex(n4), Is.EqualTo("0x11"));
        }

        [Test]
        public void HexStringToBigint()
        {
            string n1 = "-0x0C5F387CA52B8F";
            string n2 = "0x0";
            string n3 = "0x0F";
            string n4 = "0x11";
            string n5 = "0x961e2e";
            string n6 = "0x24e160300";

            BigInteger res1 = -3482395934534543;
            BigInteger res2 = 0;
            BigInteger res3 = 15;
            BigInteger res4 = 17;
            BigInteger res5 = 9838126;
            BigInteger res6 = 9900000000;

            Assert.That(DataTypeConverter.HexStringToBigint(n1), Is.EqualTo(res1));
            Assert.That(DataTypeConverter.HexStringToBigint(n2), Is.EqualTo(res2));
            Assert.That(DataTypeConverter.HexStringToBigint(n3), Is.EqualTo(res3));
            Assert.That(DataTypeConverter.HexStringToBigint(n4), Is.EqualTo(res4));
            Assert.That(DataTypeConverter.HexStringToBigint(n5), Is.EqualTo(res5));
            Assert.That(DataTypeConverter.HexStringToBigint(n6), Is.EqualTo(res6));
        }
    }
}