using System.Numerics;

namespace In3.Eth1
{
    public class TransactionRequest
    {
        public string From { get; set; }
        public string To { get; set; }
        public string Data { get; set; }
        public BigInteger? Value { get; set; }
        public long? Nonce { get; set; }
        public long? Gas { get; set; }
        public long? GasPrice { get; set; }
        public string Function { get; set; }
        public object[] Params { get; set; }

        public bool IsFunctionInvocation() => string.IsNullOrEmpty(Function);
    }
}