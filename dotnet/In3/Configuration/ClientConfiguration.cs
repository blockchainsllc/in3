using System.Collections.Generic;
using System.Linq;
using System.Text.Json;
using System.Text.Json.Serialization;
using SystemTextJsonSamples;
using csharp.Utils;

namespace In3.Configuration
{
    public class ClientConfiguration : BaseConfiguration
    {
        [JsonPropertyName("nodes"), JsonConverter(typeof(DictionaryChainObjectConverter))]
        public Dictionary<Chain, ChainConfiguration> ChainsConfiguration { get; set; }
        [JsonPropertyName("requestCount")]
        public uint RequestCount
        {
            get => (uint)GetState("requestCount");
            set => SetState("requestCount", value);
        }
        [JsonPropertyName("autoUpdateList")]
        public bool AutoUpdateList
        {
            get => (bool)GetState("autoUpdateList");
            set => SetState("autoUpdateList", value);
        }
        [JsonPropertyName("proof"), JsonConverter(typeof(ProofConverter))]
        public Proof Proof
        {
            get => (Proof)GetState("proof");
            set => SetState("proof", value);
        }
        [JsonPropertyName("maxAttempts")]
        public uint MaxAttempts
        {
            get => (uint)GetState("maxAttempts");
            set => SetState("maxAttempts", value);
        }
        [JsonPropertyName("signatureCount")]
        public uint SignatureCount
        {
            get => (uint)GetState("signatureCount");
            set => SetState("signatureCount", value);
        }
        [JsonPropertyName("finality")]
        public uint Finality
        {
            get => (uint)GetState("finality");
            set => SetState("finality", value);
        }
        [JsonPropertyName("includeCode")]
        public bool IncludeCode
        {
            get => (bool)GetState("includeCode");
            set => SetState("includeCode", value);
        }
        [JsonPropertyName("keepIn3")]
        public bool KeepIn3
        {
            get => (bool)GetState("keepIn3");
            set => SetState("keepIn3", value);
        }
        [JsonPropertyName("useBinary")]
        public bool UseBinary
        {
            get => (bool)GetState("useBinary");
            set => SetState("useBinary", value);
        }
        [JsonPropertyName("useHttp")]
        public bool UseHttp
        {
            get => (bool)GetState("useHttp");
            set => SetState("useHttp", value);
        }
        [JsonPropertyName("maxCodeCache")]
        public ulong MaxCodeCache
        {
            get => (ulong)GetState("maxCodeCache");
            set => SetState("maxCodeCache", value);
        }
        [JsonPropertyName("timeout")]
        public ulong Timeout
        {
            get => (ulong)GetState("timeout");
            set => SetState("timeout", value);
        }
        [JsonPropertyName("minDeposit")]
        public ulong MinDeposit
        {
            get => (ulong)GetState("minDeposit");
            set => SetState("minDeposit", value);
        }
        [JsonPropertyName("nodeProps")]
        public ulong NodeProps
        {
            get => (ulong)GetState("nodeProps");
            set => SetState("nodeProps", value);
        }
        [JsonPropertyName("nodeLimit")]
        public ulong NodeLimit
        {
            get => (ulong)GetState("nodeLimit");
            set => SetState("nodeLimit", value);
        }
        [JsonPropertyName("replaceLatestBlock")]
        public uint? ReplaceLatestBlock
        {
            get => (uint?)GetState("replaceLatestBlock");
            set => SetState("replaceLatestBlock", value);
        }
        [JsonPropertyName("maxBlockCache")]
        public ulong MaxBlockCache
        {
            get => (ulong)GetState("maxBlockCache");
            set => SetState("maxBlockCache", value);
        }
        [JsonPropertyName("rpc")]
        public string? Rpc
        {
            get => (string?)GetState("rpc");
            set => SetState("rpc", value);
        }
        [JsonPropertyName("stats")]
        public bool Stats
        {
            get => (bool)GetState("stats");
            set => SetState("stats", value);
        }

        public ClientConfiguration()
        {
            ChainsConfiguration = new Dictionary<Chain, ChainConfiguration>();
        }

        public string ToJson()
        {
            JsonSerializerOptions options = new JsonSerializerOptions
            {
                IgnoreNullValues = true
            };

            options.Converters.Add(new DictionaryChainObjectConverter());
            options.Converters.Add(new ProofConverter());
            return JsonSerializer.Serialize(this, options);
        }

        public override bool HasChanged()
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