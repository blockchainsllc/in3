using System;
using System.Globalization;
using System.Numerics;

namespace In3.Utils
{
    public class TypesMatcher
    {
        private static readonly char[] charsToTrim = new char[] { '0' };
        public static BigInteger HexStringToBigint(string source)
        {
            if (String.IsNullOrWhiteSpace(source)) return BigInteger.Zero;
            if (source.StartsWith("0x") || source.StartsWith("-0x"))
            {
                bool isNegative = source.StartsWith("-");
                int startsAt = isNegative ? 3 : 2;

                // Remove 0x prefix. Dotnet doesnt like it
                string toConvert = source.Substring(isNegative ? 3 : 2);

                // If this looks crazy, its because it is: https://docs.microsoft.com/en-us/dotnet/api/system.numerics.biginteger.parse?redirectedfrom=MSDN&view=netframework-4.8#System_Numerics_BigInteger_Parse_System_String_System_Globalization_NumberStyles_
                toConvert = $"0{toConvert}";
                BigInteger converted = BigInteger.Parse(toConvert, NumberStyles.AllowHexSpecifier);
                return isNegative ? 0 - converted : converted;
            }
            return BigInteger.Parse(source, NumberStyles.AllowHexSpecifier);
        }

        public static string BigIntToPrefixedHex(BigInteger source)
        {
            // This initial if is to overcome the weird behavior of ".ToString("X")" and the corner case of trimming leading zeroes. 
            if (source == 0) return "0x0";
            bool isPositive = source >= 0;
            BigInteger toConvert = isPositive ? source : source * -1;
            return isPositive ? $"0x{toConvert.ToString("X").TrimStart(charsToTrim)}" : $"-0x{toConvert.ToString("X").TrimStart(charsToTrim)}";
        }

        public static string AddHexPrefixer(string integerString)
        {
            if (integerString.StartsWith("0x") || integerString.StartsWith("-0x")) return integerString;
            return BigIntToPrefixedHex(BigInteger.Parse(integerString));
        }
    }
}