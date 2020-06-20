using System;
using System.Globalization;
using System.Numerics;

namespace In3.Utils
{
    /// <summary>
    /// General util class for conversion between blockchain types.
    /// </summary>
    public static class DataTypeConverter
    {
        private static readonly char[] charsToTrim = new char[] { '0' };

        internal static string BytesToHexString(byte[] input, int len)
        {
            char[] result = new char[len * 2];
            char[] hex = "0123456789abcdef".ToCharArray();

            int i = 0, j = 0;
            while (j < len)
            {
                result[i++] = hex[(input[j] >> 4) & 0xF];
                result[i++] = hex[input[j] & 0xF];
                j++;
            }

            string zeroSufixedString = new string(result);
            return $"0x{zeroSufixedString}";
        }
        /// <summary>
        /// Converts a zero-prefixed hex (e.g.: 0x05) to <see cref="BigInteger"/>
        /// </summary>
        /// <param name="source">The hex number string.</param>
        /// <returns>The number representation of <paramref name="source"></paramref>.</returns>
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

        internal static string BigIntToPrefixedHex(BigInteger source)
        {
            // This initial if is to overcome the weird behavior of ".ToString("X")" and the corner case of trimming leading zeroes. 
            if (source == 0) return "0x0";
            bool isPositive = source >= 0;
            BigInteger toConvert = isPositive ? source : source * -1;
            return isPositive ? $"0x{toConvert.ToString("X").TrimStart(charsToTrim)}" : $"-0x{toConvert.ToString("X").TrimStart(charsToTrim)}";
        }

        internal static string AddHexPrefixer(string integerString)
        {
            if (integerString.StartsWith("0x") || integerString.StartsWith("-0x")) return integerString;
            return BigIntToPrefixedHex(BigInteger.Parse(integerString));
        }

        internal static byte[] HexStringToByteArray(string hexString)
        {
            if (String.IsNullOrEmpty(hexString)) return new byte[] { };

            byte[] a = new byte[hexString.Length / 2];
            for (int i = 0, h = 0; h < hexString.Length; i++, h += 2)
            {
                a[i] = (byte)Int32.Parse(hexString.Substring(h, 2), System.Globalization.NumberStyles.HexNumber);
            }

            return a;
        }

    }
}