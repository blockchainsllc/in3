using System.Collections.Generic;
using System.Linq;
using System.Text.Json;
using System.Text.Json.Serialization;
using SystemTextJsonSamples;
using csharp.Utils;

namespace In3.Configuration
{
    /// <summary>
    /// Class that represents the configuration to be applied on <see cref="IN3" />.
    /// Due to the 1-to-1 relationship with the client, this class should never be instantiated. To obtain a reference of the client configuration use <see cref="IN3.Configuration" /> instead.
    /// </summary>
    /// <remarks>
    /// Use in conjunction with <see cref="ChainConfiguration" /> and <see cref="NodeConfiguration" />.
    /// </remarks>
    public class ClientConfiguration : BaseConfiguration
    {

        /// <summary>
        /// Configuration for the chains. Read-only attribute.
        /// </summary>
        [JsonPropertyName("nodes"), JsonConverter(typeof(DictionaryChainObjectConverter))]
        public Dictionary<Chain, ChainConfiguration> ChainsConfiguration { get; }

        /// <summary>
        /// Useful when <see cref="SignatureCount" /> is less then <see langword="1" />. The client will check for consensus in responses.
        /// </summary>
        /// <value><see langword="10" />, for example, will ask for 10 different nodes and compare results looking for a consensus in the responses.</value>
        [JsonPropertyName("requestCount")]
        public uint RequestCount
        {
            get => (uint)GetState("requestCount");
            set => SetState("requestCount", value);
        }

        /// <summary>
        /// If <see langword="true"/> the nodelist will be automatically updated. False may compromise data security.
        /// </summary>
        [JsonPropertyName("autoUpdateList")]
        public bool AutoUpdateList
        {
            get => (bool)GetState("autoUpdateList");
            set => SetState("autoUpdateList", value);
        }

        /// <summary>
        /// One of <see cref="Proof" />. <see cref="Proof.Full" /> gets the whole block Patricia-Merkle-Tree, <see cref="Proof.Standard" /> only verifies the specific tree branch concerning the request, <see cref="Proof.None" /> only verifies the root hashes, like a light-client does.
        /// </summary>
        [JsonPropertyName("proof"), JsonConverter(typeof(ProofConverter))]
        public Proof Proof
        {
            get => (Proof)GetState("proof");
            set => SetState("proof", value);
        }

        /// <summary>
        /// Maximum times the client will retry to contact a certain node.
        /// </summary>
        [JsonPropertyName("maxAttempts")]
        public uint MaxAttempts
        {
            get => (uint)GetState("maxAttempts");
            set => SetState("maxAttempts", value);
        }

        /// <summary>
        /// Node signatures attesting the response to your request. Will send a separate request for each.
        /// </summary>
        /// <example>
        /// When set to <see langword="3"/>, 3 nodes will have to sign the response.
        /// </example>
        [JsonPropertyName("signatureCount")]
        public uint SignatureCount
        {
            get => (uint)GetState("signatureCount");
            set => SetState("signatureCount", value);
        }

        /// <sumary>
        /// Numerical value associated with finality. Behavior depends on the chain consensus algorithm: for Proof of Work (the one implemented on all Etherem 1.0 chains at the moment), this means: mined blocks on top of the requested one.
        /// </sumary>
        /// <remarks>
        /// Beware that the semantics of the values change greatly from chain to chain. The value of <see langword="8"/> would mean 8 blocks mined on top of the requested one while with the POW algorithm while, for POA, it would mean 8% of validators.
        /// </remarks>
        [JsonPropertyName("finality")]
        public uint Finality
        {
            get => (uint)GetState("finality");
            set => SetState("finality", value);
        }

        /// <summary>
        /// Code is included when sending eth_call-requests.
        /// </summary>
        /// <value>If <see langword="true"/>, every request with the address field will include the data, if existent, that is stored in that wallet/smart-contract. If <see langword="false"/>, only the code digest is included.</value>
        [JsonPropertyName("includeCode")]
        public bool IncludeCode
        {
            get => (bool)GetState("includeCode");
            set => SetState("includeCode", value);
        }

        /// <summary>
        /// Tthe in3-section (custom node on the RPC call) with the proof will also returned.
        /// </summary>
        /// <value>If <see langword="true"/>, proof data will be kept in every rpc response while <see langword="false"/> will remove this data after using it to verify the responses. Useful for debugging and manually verifying the proofs.</value>
        [JsonPropertyName("keepIn3")]
        public bool KeepIn3
        {
            get => (bool)GetState("keepIn3");
            set => SetState("keepIn3", value);
        }

        /// <summary>
        /// Disable ssl on the Http connection.
        /// </summary>
        /// <value>If <see langword="true"/> the client will try to use http instead of https.</value>
        [JsonPropertyName("useHttp")]
        public bool UseHttp
        {
            get => (bool)GetState("useHttp");
            set => SetState("useHttp", value);
        }

        /// <summary>
        /// Maximum number of bytes used to cache EVM code in memory.
        /// </summary>
        [JsonPropertyName("maxCodeCache")]
        public ulong MaxCodeCache
        {
            get => (ulong)GetState("maxCodeCache");
            set => SetState("maxCodeCache", value);
        }

        /// <summary>
        /// Milliseconds before a request times out.
        /// </summary>
        [JsonPropertyName("timeout")]
        public ulong Timeout
        {
            get => (ulong)GetState("timeout");
            set => SetState("timeout", value);
        }

        /// <summary>
        /// Only nodes owning at least this amount will be chosen to sign responses to your requests.
        /// </summary>
        [JsonPropertyName("minDeposit")]
        public ulong MinDeposit
        {
            get => (ulong)GetState("minDeposit");
            set => SetState("minDeposit", value);
        }

        /// <summary>
        /// Props define the capabilities of the nodes. Accepts a combination of values.
        /// </summary>
        /// <example>
        /// <code>
        /// clientConfiguration.NodeProps = Props.NodePropProof | Props.NodePropArchive;
        /// </code>
        /// </example>

        [JsonPropertyName("nodeProps")]
        public Props? NodeProps
        {
            get => (Props?)GetState("nodeProps");
            set => SetState("nodeProps", value);
        }

        /// <summary>
        /// Limit nodes stored in the client.
        /// </summary>
        [JsonPropertyName("nodeLimit")]
        public ulong NodeLimit
        {
            get => (ulong)GetState("nodeLimit");
            set => SetState("nodeLimit", value);
        }

        /// <summary>
        /// Distance considered safe, consensus wise, from the very latest block. Higher values exponentially increases state finality, and therefore data security, as well guaranteeded responses from in3 nodes.
        /// </summary>
        /// <value>Setting to <see langword="10"/> will ask for the state from ten blocks ago of the latest block.</value>
        [JsonPropertyName("replaceLatestBlock")]
        public uint? ReplaceLatestBlock
        {
            get => (uint?)GetState("replaceLatestBlock");
            set => SetState("replaceLatestBlock", value);
        }

        /// <summary>
        /// Maximum blocks kept in memory.
        /// </summary>
        [JsonPropertyName("maxBlockCache")]
        public ulong MaxBlockCache
        {
            get => (ulong)GetState("maxBlockCache");
            set => SetState("maxBlockCache", value);
        }

        /// <summary>
        /// Setup an custom rpc source for requests by setting chain to <see cref="Chain.Local" /> and proof to <see cref="Proof.None" />.
        /// </summary>
        [JsonPropertyName("rpc")]
        public string? Rpc
        {
            get => (string?)GetState("rpc");
            set => SetState("rpc", value);
        }

        internal ClientConfiguration()
        {
            ChainsConfiguration = new Dictionary<Chain, ChainConfiguration>();
        }

        internal string ToJson()
        {
            JsonSerializerOptions options = new JsonSerializerOptions
            {
                IgnoreNullValues = true
            };

            options.Converters.Add(new DictionaryChainObjectConverter());
            options.Converters.Add(new ProofConverter());
            return JsonSerializer.Serialize(this, options);
        }

        internal override bool HasChanged()
        {
            bool hasChildrenChanged = ChainsConfiguration.Values.Any(configuration => configuration.HasChanged());
            return IsDirty() || hasChildrenChanged;
        }

        internal void AddChainConfiguration(Chain chain, ChainConfiguration conf)
        {
            if (ChainsConfiguration.ContainsKey(chain))
            {
                ChainsConfiguration[chain] = conf;
            }
            else
            {
                ChainsConfiguration.Add(chain, conf);
            }
        }

        internal void MarkSynced()
        {
            Clean();
            foreach (var chainConfiguration in ChainsConfiguration.Values)
            {
                chainConfiguration.MarkSynced();
            }
        }
    }
}