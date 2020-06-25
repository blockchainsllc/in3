using System.Numerics;
using In3.Utils;

namespace In3
{
    /// <summary>
    /// Enum-like class that defines constants to be used with <see cref="Eth1.Api" />.
    /// </summary>
    public class BlockParameter
    {
        /// <summary>
        /// Constant associated with the latest mined block in the chain.
        /// </summary>
        /// <remarks>While the parameter itself is constant the current "latest" block changes everytime a new block is mined. The result of the operations are also related to <see langword="ReplaceLatestBlock" /> on <see cref="Configuration.ClientConfiguration" />.</remarks>
        public static BigInteger Latest
        {
            get { return -1; }
        }

        /// <summary>
        /// Genesis block.
        /// </summary>
        public static BigInteger Earliest
        {
            get { return 0; }
        }
        internal static string AsString(BigInteger block)
        {
            if (block == Latest) return "latest";
            if (block == Earliest || block < -1) return "earliest";
            return DataTypeConverter.BigIntToPrefixedHex(block);
        }
    }
}