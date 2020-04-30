using System;
using System.Text;
using In3.Utils;

namespace In3.Ipfs
{
    public class Api
    {
        private IN3 in3 { get; set; }

        private const string IpfsPut = "ipfs_put";
        private const string IpfsGet = "ipfs_get";

        public Api(IN3 in3)
        {
            this.in3 = in3;
        }

        public byte[] Get(string multihash)
        {
            if (multihash == null) throw new ArgumentException();

            string result = in3.SendRpc(IpfsGet, new object[] { multihash, IpfsEncoding.Base64.Value });
            return RpcHandler.From<byte[]>(result);
        }

        public string Put(string content)
        {
            return Put(content == null ? null : Encoding.UTF8.GetBytes(content));
        }

        public string Put(byte[] content)
        {
            if (content == null) throw new ArgumentException();

            string encodedContent = Convert.ToBase64String(content);
            string result = in3.SendRpc(IpfsPut, new object[] { encodedContent });
            return RpcHandler.From<string>(result);
        }
    }
}