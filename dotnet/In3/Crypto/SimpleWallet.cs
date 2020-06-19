using System.Collections.Generic;
using In3.Eth1;
using In3.Native;

namespace In3.Crypto
{
    /// <summary>
    /// Default implementation of the <see cref="Signer" />. Works as an orchestration of the <see cref="In3.Crypto" /> in order to manage multiple accounts.
    /// </summary>
    public class SimpleWallet : Signer
    {
        private Dictionary<string, string> PrivateKeys { get; set; }
        private IN3 In3 { get; set; }

        /// <summary>
        /// Basic constructor.
        /// </summary>
        /// <param name="in3">A client instance.</param>
        public SimpleWallet(IN3 in3)
        {
            In3 = in3;
            PrivateKeys = new Dictionary<string, string>();
        }

        /// <summary>
        /// Adds a private key to be managed by the wallet and sign transactions.
        /// </summary>
        /// <param name="privateKey">The private key to be stored by the wallet.</param>
        /// <returns>The address derived from the <paramref name="privateKey" /></returns>
        public string AddRawKey(string privateKey)
        {
            string address = GetAddress(privateKey);
            PrivateKeys.Add(address.ToLower(), privateKey);
            return address;
        }

        private string GetAddress(string key)
        {
            return In3.Crypto.Pk2Address(key);
        }

        private void SimpleSigner() { }

        /// <summary>
        /// Check if this address is managed by this wallet.
        /// </summary>
        /// <param name="address">The address. Value returned by <see cref="SimpleWallet.AddRawKey" />.</param>
        /// <returns><see langword="true" /> if the address is managed by this wallter, <see langword="false" /> if not.</returns>
        public bool CanSign(string address)
        {
            return PrivateKeys.ContainsKey(address);
        }

        /// <summary>
        /// Signs the transaction data by invoking <see cref="Crypto.Api.SignData" />.
        /// </summary>
        /// <param name="data">Data to be signed.</param>
        /// <param name="address">Address managed by the wallet, see <see cref="SimpleWallet.AddRawKey" /></param>
        /// <returns>Signed transaction data.</returns>
        public string Sign(string data, string address)
        {
            string key = PrivateKeys[address.ToLower()];
            return NativeWallet.Sign(key, data);
        }

        /// <summary>
        /// Identity function-like method.
        /// </summary>
        /// <param name="tx">A transaction object.</param>
        /// <returns><paramref name="tx" /></returns>
        public TransactionRequest PrepareTransaction(TransactionRequest tx)
        {
            return tx;
        }
    }
}