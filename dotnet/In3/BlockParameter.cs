using System.Numerics;
using In3.Utils;

namespace In3
{
    public class BlockParameter
    {
        public static BigInteger Latest {
            get { return -1; }
        }
        public static BigInteger Earliest {
            get { return 0; }
        }
        public static string AsString(BigInteger block)
        {
            if (block == Latest) return "latest";
            if (block == Earliest || block < -1) return "earliest";
            return TypesMatcher.BigIntToPrefixedHex(block);
        }
    }
}