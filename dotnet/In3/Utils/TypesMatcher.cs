using System;
using System.Globalization;
using System.Numerics;

namespace In3.Utils
{
    public class TypesMatcher
    {
        public static BigInteger HexStringToBigint(string source)
        {
            if (String.IsNullOrWhiteSpace(source)) return BigInteger.Zero;
            if (source.StartsWith("0x") || source.StartsWith("-0x"))
            {
                bool isNegative = source.StartsWith("-");
                int startsAt = isNegative ? 3 : 2;
                BigInteger converted = BigInteger.Parse(source.Substring(startsAt), NumberStyles.AllowHexSpecifier);
                return isNegative ? converted * -1 : converted;
            }
            return BigInteger.Parse(source, NumberStyles.AllowHexSpecifier);
        }
    }
}