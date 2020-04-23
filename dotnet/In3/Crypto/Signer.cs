using In3.Eth1;

namespace In3.Crypto
{
    public interface Signer
    {
        bool CanSign(string address);
        string Sign(string data, string address);
        public TransactionRequest PrepareTransaction(TransactionRequest tx);
    }
}