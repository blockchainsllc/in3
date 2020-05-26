using System.Collections.Generic;
using System.Text.Json;
using In3.Utils;

namespace In3.Crypto
{
    /// <summary>
    /// Class that exposes utility methods for cryptographic utilities. Relies on <see cref="IN3" /> functionality.
    /// </summary>
    public class Api
    {
        private IN3 in3;

        private const string CryptoPk2Address = "in3_pk2address";
        private const string CryptoSignData = "in3_signData";
        private static string CryptoPk2Public = "in3_pk2public";
        private static string CryptoEcRecover = "in3_ecrecover";
        private static string CryptoDecryptKey = "in3_decryptKey";
        private static string CryptoSha3 = "web3_sha3";

        internal Api(IN3 in3)
        {
            this.in3 = in3;
        }

        /// <summary>
        /// Derives an address from the given private (<paramref name="pk"/>) key using SHA-3 algorithm.
        /// </summary>
        /// <param name="pk">Private key.</param>
        /// <returns>The address.</returns>
        public string Pk2Address(string pk)
        {
            string jsonResponse = in3.SendRpc(CryptoPk2Address, new object[] { pk });
            return RpcHandler.From<string>(jsonResponse);
        }

        /// <summary>
        /// Signs the data <paramref name="msg" /> with a given private key. Refer to <see cref="SignedData" /> for more information.
        /// </summary>
        /// <param name="msg">Data to be signed.</param>
        /// <param name="pk">Private key.</param>
        /// <param name="sigType">Type of signature, one of <see cref="SignatureType" />.</param>
        /// <returns>The signed data.</returns>
        public SignedData SignData(string msg, string pk, SignatureType? sigType = null)
        {
            string jsonResponse = in3.SendRpc(CryptoSignData, new object[] { msg, pk, sigType?.Value });
            return RpcHandler.From<SignedData>(jsonResponse);
        }

        /// <summary>
        /// Derives public key from the given private (<paramref name="pk"/>) key using SHA-3 algorithm.
        /// </summary>
        /// <param name="pk">Private key.</param>
        /// <returns>The public key.</returns>
        public string Pk2Public(string pk)
        {
            string jsonResponse = in3.SendRpc(CryptoPk2Public, new object[] { pk });
            return RpcHandler.From<string>(jsonResponse);
        }

        /// <summary>
        /// Recovers the account associated with the signed data.
        /// </summary>
        /// <param name="signedData">Data that was signed with.</param>
        /// <param name="signature">The signature.</param>
        /// <param name="signatureType">One of <see cref="SignatureType" />.</param>
        /// <returns>The account.</returns>
        public Account EcRecover(string signedData, string signature, SignatureType signatureType)
        {
            string jsonResponse = in3.SendRpc(CryptoEcRecover, new object[] { signedData, signature, signatureType?.Value });
            return RpcHandler.From<Account>(jsonResponse);
        }

        /// <summary>
        /// Decryot an encrypted private key.
        /// </summary>
        /// <param name="pk">Private key.</param>
        /// <param name="passphrase">Passphrase whose <paramref name="pk" />.</param>
        /// <returns>Decrypted key.</returns>
        public string DecryptKey(string pk, string passphrase)
        {
            Dictionary<string, object> res = JsonSerializer.Deserialize<Dictionary<string, object>>(pk);
            string jsonResponse = in3.SendRpc(CryptoDecryptKey, new object[] { res, passphrase });
            return RpcHandler.From<string>(jsonResponse);
        }

        /// <summary>
        /// Hash the input data using sha3 algorithm.
        /// </summary>
        /// <param name="data">Content to be hashed.</param>
        /// <returns>Hashed output.</returns>
        public string Sha3(string data)
        {
            string jsonResponse = in3.SendRpc(CryptoSha3, new object[] { data });
            return RpcHandler.From<string>(jsonResponse);
        }

    }
}