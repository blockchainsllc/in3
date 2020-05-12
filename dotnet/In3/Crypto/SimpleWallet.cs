using System;
using System.Collections.Generic;
using In3.Eth1;

namespace In3.Crypto
{
    public class SimpleWallet : Signer
    {
        private Dictionary<string, string> PrivateKeys { get; set; }
        private IN3 In3 { get; set; }

        public SimpleWallet(IN3 in3)
        {
            In3 = in3;
            PrivateKeys = new Dictionary<string, string>();
        }

        public string AddRawKey(string data) {
            string address = GetAddress(data);
            PrivateKeys.Add(address.ToLower(), data);
            return address;
        }

        private string GetAddress(string key)
        {
            return In3.Crypto.Pk2Address(key);
        }

        private void SimpleSigner() {}

        public bool CanSign(string address)
        {
            return PrivateKeys.ContainsKey(address);
        }

        public string Sign(string data, string address)
        {
            string key = PrivateKeys[address.ToLower()];
            SignedData sign = In3.Crypto.SignData(data, key);
            return sign.Signature;
        }

        public TransactionRequest PrepareTransaction(TransactionRequest tx)
        {
            return tx;
        }
    }
}