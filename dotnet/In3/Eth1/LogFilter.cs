using System.Numerics;
namespace In3.Eth1
{
    /// <summary>
    /// Filter configuration for search logs. To be used along with the <see cref="Eth1.Api" /> filter and methods.
    /// </summary>
    public class LogFilter
    {

        /// <summary>
        /// Starting block for the filter.
        /// </summary>
        public BigInteger? FromBlock { get; set; }

        /// <summary>
        /// End block for the filter.
        /// </summary>
        public BigInteger? ToBlock { get; set; }

        /// <summary>
        /// Address for the filter.
        /// </summary>
        public string Address { get; set; }

        /// <summary>
        /// Array of 32 Bytes Data topics. Topics are order-dependent. It's  possible to pass in null to match any topic, or a subarray of multiple topics  of which one should be matching.
        /// </summary>
        public object[] Topics { get; set; }

        /// <summary>
        /// Blcok hash of the filtered blocks.
        /// </summary>
        /// <remarks>If present, <see cref="FromBlock" /> and <see cref="ToBlock" /> will be ignored.</remarks>
        public string BlockHash { get; set; }

        /// <summary>
        /// Standard constructor.
        /// </summary>
        public LogFilter() { }

        internal Rpc.LogFilter ToRPc()
        {
            Rpc.LogFilter result = new Rpc.LogFilter();
            if (FromBlock != null) result.FromBlock = BlockParameter.AsString(FromBlock.Value);
            if (ToBlock != null) result.ToBlock = BlockParameter.AsString(ToBlock.Value);
            result.Address = Address;
            result.Topics = Topics;
            result.BlockHash = BlockHash;

            return result;
        }
    }
}