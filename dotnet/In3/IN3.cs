using System.Collections.Generic;
using System.Threading.Tasks;
using In3.Configuration;
using In3.Context;
using In3.Crypto;
using In3.Transport;
using In3.Storage;
using In3.Native;
using In3.Utils;

namespace In3
{
    /// <summary>
    /// Incubed network client. Connect to the blockchain via a list of bootnodes, then gets the latest list of nodes in
    /// the network and ask a certain number of the to sign the block header of given list, putting their deposit at stake.
    /// Once with the latest list at hand, the client can request any other on-chain information using the same scheme.
    /// </summary>
    public class IN3
    {
        private NativeClient NativeClient { get; }

        /// <summary>Gets <see cref="In3.Eth1.Api"/> object.</summary>
        public Eth1.Api Eth1 { get; }

        /// <summary>Gets or sets <see cref="In3.Transport.Transport"/> object. If not set <see cref="DefaultTransport"/> will be used.</summary>
        public Transport.Transport Transport { get; set; }

        /// <summary>Get or Sets <see cref="In3.Storage.Storage"/> object. If not set <see cref="InMemoryStorage"/> will be used.</summary>
        public Storage.Storage Storage { get; set; }

        /// <summary>Get or Sets <see cref="Signer"/> object. If not set <see cref="SimpleWallet"/> will be used.</summary>
        public Crypto.Signer Signer { get; set; }

        /// <summary>Gets <see cref="In3.Btc.Api"/> object.</summary>
        public Btc.Api Btc { get; }

        /// <summary>Gets <see cref="In3.Crypto.Api"/> object.</summary>
        public Crypto.Api Crypto { get; }

        /// <summary>Gets <see cref="In3.Ipfs.Api"/> object.</summary>
        public Ipfs.Api Ipfs { get; }

        /// <summary>Gets <see cref="ClientConfiguration"/> object. Any changes in the object will be automaticaly applied to the client before each method invocation.</summary>
        public ClientConfiguration Configuration { get; }

        private IN3(Chain chainId)
        {
            // Starting to get convoluted. Need to think of a better way.
            Transport = new DefaultTransport();
            Storage = new InMemoryStorage();
            Signer = new Crypto.SimpleWallet(this);
            NativeClient = new NativeClient(this, chainId);
            Btc = new Btc.Api(this);
            Eth1 = new Eth1.Api(this);
            Crypto = new Crypto.Api(this);
            Ipfs = new Ipfs.Api(this);
            Configuration = NativeClient.ReadConfiguration();
        }

        /// <summary>
        /// Creates a new instance of <c>IN3</c>.
        /// </summary>
        ///
        /// <param name="chain"><see cref="Chain"/> that Incubed will connect to.</param>
        /// <returns>
        /// An Incubed instance.
        /// </returns>
        /// <example>
        /// <code>
        /// IN3 client = IN3.ForChain(Chain.Mainnet);
        /// </code>
        /// </example>
        public static IN3 ForChain(Chain chain)
        {
            return new IN3(chain);
        }

        internal Task<string> SendRpc(string method, object[] args, Dictionary<string, object> in3 = null)
        {
            if (Configuration.HasChanged())
            {
                NativeClient.ApplyConfiguration(Configuration);
            }

            return new Runner(NativeClient).Run(RpcHandler.To(method, args, in3));
        }

        /// <summary>
        /// Finalizer for the client.
        /// </summary>
        ~IN3() => NativeClient?.Free();
    }
}
