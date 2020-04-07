using System.Collections.Generic;
using System.Text.Json;
using In3.Transport;
using In3.Rpc;
using In3.Storage;
using In3.Eth1;
using In3.Native;

namespace In3
{
    public class IN3
    {
        public Api Eth1 { get; }
        public NativeWrapper Native { get; }
        public Transport.Transport Transport { get; set;  }
        public Storage.Storage Storage { get; set; }
        private IN3(Chain chainId)
        {
            Native = new DefaultNativeWrapper(this, chainId);
            Eth1 = new Api(this);
            Transport = new DefaultTransport();
            Storage = new InMemoryStorage();
        }
        private IN3() {}

        public static IN3 ForChain(Chain chainId)
        {
            return new IN3(chainId);
        }

        public string SendRPCasObject(string method, object[] args, Dictionary<string, object> in3 = null)
        {
            return Native.Send(ToRpc(method, args, in3));
        }

        private string ToRpc(string method, object[] parameters, Dictionary<string, object> in3)
        {
            JsonSerializerOptions options = new JsonSerializerOptions
            {
                IgnoreNullValues = true
            };

            In3Rpc rpc = new In3Rpc();
            rpc.Method = method;
            rpc.Params = parameters;
            rpc.In3 = in3;

            return JsonSerializer.Serialize(rpc, options);
        }
    }
}
