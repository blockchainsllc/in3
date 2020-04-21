using System.Collections.Generic;
using In3.Crypto;
using In3.Transport;
using In3.Storage;
using In3.Native;
using In3.Utils;

namespace In3
{
    public class IN3
    {
        private NativeWrapper Native { get; }
        public Eth1.Api Eth1 { get; }
        public Transport.Transport Transport { get; set; }
        public Storage.Storage Storage { get; set; }
        public Signer Signer { get; set; }
        public Crypto.Api Crypto { get; set; }

        private IN3(Chain chainId)
        {
            // Starting to get convoluted. Need to think of a better way.
            Native = new DefaultNativeWrapper(this, chainId);
            Eth1 = new Eth1.Api(this);
            Crypto = new Crypto.Api(this);
            Transport = new DefaultTransport();
            Storage = new InMemoryStorage();
            Signer = new SimpleWallet(this);
        }
        private IN3() { }

        public static IN3 ForChain(Chain chainId)
        {
            return new IN3(chainId);
        }

        public string SendRpc(string method, object[] args, Dictionary<string, object> in3 = null)
        {
            return Native.Send(RpcHandler.To(method, args, in3));
        }
    }
}
