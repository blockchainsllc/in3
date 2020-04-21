using System.Numerics;
using In3.Rpc;
using In3.Utils;

namespace In3.Eth1
{
    public class LogFilter
    {
        public BigInteger? FromBlock { get; set; }
        public BigInteger? ToBlock { get; set; }
        public string Address { get; set; }
        public object[] Topics { get; set; }
        public string BlockHash { get; set; }

        public LogFilter() {}

        public Rpc.LogFilter ToRPc()
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