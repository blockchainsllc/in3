using System.Collections.Generic;
using System.Linq;
using System.Text.Json.Serialization;

namespace In3.Configuration
{
    public class ChainConfiguration : BaseConfiguration
    {
        private List<NodeConfiguration> _nodesConfig { get; set; }

        [JsonPropertyName("nodeList")]
        public List<NodeConfiguration> NodesConfiguration
        {
            get => _nodesConfig;
        }
        [JsonPropertyName("needsUpdate")]
        public bool? NeedsUpdate
        {
            get => (bool?)GetState("needsUpdate");
            set => SetState("needsUpdate", value);
        }
        [JsonPropertyName("contract")]
        public string? Contract
        {
            get => (string?)GetState("contract");
            set => SetState("contract", value);
        }
        [JsonPropertyName("registryId")]
        public string RegistryId
        {
            get => (string?)GetState("registryId");
            set => SetState("registryId", value);
        }
        [JsonPropertyName("whiteListContract")]
        public string WhiteListContract
        {
            get => (string?)GetState("whiteListContract");
            set => SetState("whiteListContract", value);
        }
        [JsonPropertyName("whiteList")]
        public string[] WhiteList
        {
            get => (string[]) GetState("whiteList");
            set => SetState("whiteList", value);
        }

        
        internal ChainConfiguration() {}
        
        public ChainConfiguration(Chain chain, ClientConfiguration clientConfiguration)
        {
            clientConfiguration.AddChainConfiguration(chain, this);
        }

        public void AddNodeConfiguration(NodeConfiguration nodeConfiguration)
        {
            if (_nodesConfig == null)
            {
                _nodesConfig = new List<NodeConfiguration>();   
            }
            _nodesConfig.Add(nodeConfiguration);
        }

        public override bool HasChanged()
        {
            bool hasChildrenChanged = NodesConfiguration != null && NodesConfiguration.Any(configuration => configuration.HasChanged());
            return IsDirty() || hasChildrenChanged;
        }

        internal void MarkSynced()
        {
            Clean();
            if (NodesConfiguration != null)
            {
                foreach (var nodeConfiguration in NodesConfiguration)
                {
                    nodeConfiguration.MarkSynced();
                }
            }
        }
    }
}