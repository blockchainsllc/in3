using System.Numerics;
using System.Text.Json;
using In3.Rpc;
using In3.Utils;

namespace In3.Eth1
{
    public class Api
    {
        private IN3 in3;

        private const string EthBlockNumber = "eth_blockNumber";

        public Api(IN3 in3)
        {
            this.in3 = in3;
        }

        public BigInteger BlockNumber()
        {
            string jsonResponse = in3.SendRPCasObject(EthBlockNumber, new object[] { });
            RpcResult<string> response = JsonSerializer.Deserialize<RpcResult<string>>(jsonResponse);
            return TypesMatcher.HexStringToBigint(response.Result);
        }
    }
}