using System;
using System.Text;
using In3.Utils;

namespace In3.Ipfs
{
    /// <summary>
    /// API for ipfs realted methods. To be used along with <see cref="Chain.Ipfs" /> on <see cref="IN3" />. Ipfs stands for <b>InterPlanetary File System</b> and is a peer-to-peer hypermedia protocol designed to make the web faster, safer, and more open.
    /// </summary>
    public class Api
    {
        private IN3 in3 { get; set; }

        private const string IpfsPut = "ipfs_put";
        private const string IpfsGet = "ipfs_get";

        internal Api(IN3 in3)
        {
            this.in3 = in3;
        }

        /// <summary>
        /// Returns the content associated with specified multihash on success OR <lang cref="Chain.Ipfs" /> on error.
        /// </summary>
        /// <param name="multihash">The multihash.</param>
        /// <returns>The content that was stored by <see cref="Ipfs.Api.Put(byte[])" /> or <see cref="Ipfs.Api.Put(string)" />.</returns>
        public byte[] Get(string multihash)
        {
            if (multihash == null) throw new ArgumentException();

            string result = in3.SendRpc(IpfsGet, new object[] { multihash, IpfsEncoding.Base64.Value });
            return RpcHandler.From<byte[]>(result);
        }

        /// <summary>
        /// Stores content on ipfs.
        /// </summary>
        /// <param name="content">The content that will be stored via ipfs.</param>
        /// <returns>The multihash.</returns>
        public string Put(string content)
        {
            return Put(content == null ? null : Encoding.UTF8.GetBytes(content));
        }

        /// <summary>
        /// Stores content on ipfs. The content is encoded as base64 before storing.
        /// </summary>
        /// <param name="content">The content that will be stored via ipfs.</param>
        /// <returns>The multihash.</returns>
        public string Put(byte[] content)
        {
            if (content == null) throw new ArgumentException();

            string encodedContent = Convert.ToBase64String(content);
            string result = in3.SendRpc(IpfsPut, new object[] { encodedContent });
            return RpcHandler.From<string>(result);
        }
    }
}