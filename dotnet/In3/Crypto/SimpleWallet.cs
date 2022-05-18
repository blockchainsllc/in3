using System.Collections.Generic;
using System.Threading.Tasks;
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
        /// <returns>The signer id derived from the <paramref name="privateKey" /></returns>
        public string AddRawKey(string privateKey)
        {
            string signerId = GetAddress(privateKey);
            PrivateKeys.Add(signerId.ToLower(), privateKey);
            return signerId;
        }

        /// <summary>
        /// Check if this signer id is managed by this wallet.
        /// </summary>
        /// <param name="signerId">The signer id. Value returned by <see cref="SimpleWallet.AddRawKey" />.</param>
        /// <returns><see langword="true" /> if the signer id is managed by this wallet, <see langword="false" /> if not.</returns>
        public bool CanSign(string signerId)
        {
            return PrivateKeys.ContainsKey(signerId);
        }

        /// <summary>
        /// Signs the transaction data by invoking <see cref="Crypto.Api.SignData" />.
        /// </summary>
        /// <param name="data">Data to be signed.</param>
        /// <param name="signerId">Signer Id managed by the wallet, see <see cref="SimpleWallet.AddRawKey" /></param>
        /// <param name="digestType">The digest type used for signing.</param>
        /// <param name="payloadType">The payload type, used for deserialization.</param>
        /// <param name="curveType">The type of the curve.</param>
        /// <returns>The signed transaction data.</returns>
        public Task<string> Sign(string data, string signerId, DigestType digestType, PayloadType payloadType, CurveType curveType)
        {
            string key = PrivateKeys[signerId.ToLower()];
            return Task.Run(() => NativeWallet.Sign(key, data, digestType));
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

        private string GetAddress(string key)
        {
            Task<string> addressTask = In3.Crypto.Pk2Address(key);
            addressTask.Wait();
            return addressTask.Result;
        }
    }
}