using System.Collections.Generic;
using System.Text.Json;
using In3.Utils;

namespace In3.Crypto
{
    public class Api
    {
        private IN3 in3;

        private const string CryptoPk2Address = "in3_pk2address";
        private const string CryptoSignData = "in3_signData";
        private static string CryptoPk2Public = "in3_pk2public";
        private static string CryptoEcRecover = "in3_ecrecover";
        private static string CryptoDecryptKey = "in3_decryptKey";
        private static string CryptoSha3 = "web3_sha3";

        public Api(IN3 client)
        {
            in3 = client;
        }

        public string Pk2Address(string key)
        {
            string jsonResponse = in3.SendRpc(CryptoPk2Address, new object[] { key });
            return RpcHandler.From<string>(jsonResponse);
        }

        public SignedData SignData(string msg, string key, SignatureType? sigType = null)
        {
            string jsonResponse = in3.SendRpc(CryptoSignData, new object[] { msg, key, sigType?.Value });
            return RpcHandler.From<SignedData>(jsonResponse);
        }

        public string Pk2Public(string key)
        {
            string jsonResponse = in3.SendRpc(CryptoPk2Public, new object[] { key });
            return RpcHandler.From<string>(jsonResponse);
        }

        public Account EcRecover(string msg, string sig, SignatureType sigType)
        {
            string jsonResponse = in3.SendRpc(CryptoEcRecover, new object[] { msg, sig, sigType?.Value });
            return RpcHandler.From<Account>(jsonResponse);
        }

        public string DecryptKey(string key, string passphrase)
        {
            Dictionary<string, object> res = JsonSerializer.Deserialize<Dictionary<string, object>>(key);
            string jsonResponse = in3.SendRpc(CryptoDecryptKey, new object[] { res, passphrase });
            return RpcHandler.From<string>(jsonResponse);
        }

        public string Sha3(string data) {
            string jsonResponse = in3.SendRpc(CryptoSha3, new object[] { data });
            return RpcHandler.From<string>(jsonResponse);
        }
        
    }
}