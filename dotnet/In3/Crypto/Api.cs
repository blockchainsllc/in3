using In3.Utils;

namespace In3.Crypto
{
    public class Api
    {
        private IN3 in3;

        private const string CryptoPk2Address  = "in3_pk2address";
        private const string CryptoSignData  = "in3_signData";

        public Api(IN3 client)
        {
            in3 = client;
        }

        public string Pk2Address(string key) {
            string jsonResponse = in3.SendRpc(CryptoPk2Address, new object[] { key });
            return RpcHandler.From<string>(jsonResponse);
        }

        public SignedData SignData(string msg, string key, SignatureType? sigType = null) {
            string jsonResponse = in3.SendRpc(CryptoSignData, new object[] { msg, key, sigType.Value });
            return RpcHandler.From<SignedData>(jsonResponse);
        }
    }
}