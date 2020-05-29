using System.Collections.Generic;
using System.Linq;
using System.Text.Json.Serialization;

namespace In3.Configuration
{
    /// <summary>
    /// Class that represents part of the configuration to be applied on the <see cref="IN3" /> (in particular to each chain).
    /// This is a child of <see cref="ClientConfiguration" /> and have many <see cref="NodeConfiguration" />.
    /// </summary>
    public class ChainConfiguration : BaseConfiguration
    {
        private List<NodeConfiguration> _nodesConfig { get; set; }

        /// <summary>
        /// Getter for the list of elements that represent the configuration for each node.
        /// </summary>
        /// <remarks>
        /// This is a read-only property. To add configuration for nodes, Use <see cref="NodeConfiguration" /> constructor.
        /// </remarks>
        [JsonPropertyName("nodeList")]
        public List<NodeConfiguration> NodesConfiguration
        {
            get => _nodesConfig;
        }

        /// <summary>
        /// Preemptively update the node list.
        /// </summary>
        [JsonPropertyName("needsUpdate")]
        public bool? NeedsUpdate
        {
            get => (bool?)GetState("needsUpdate");
            set => SetState("needsUpdate", value);
        }

        /// <summary>
        /// Incubed registry contract from which the list was taken.
        /// </summary>
        [JsonPropertyName("contract")]
        public string Contract
        {
            get => (string)GetState("contract");
            set => SetState("contract", value);
        }

        /// <summary>
        /// Uuid of this incubed network. one chain could contain more than one incubed networks.
        /// </summary>
        [JsonPropertyName("registryId")]
        public string RegistryId
        {
            get => (string?)GetState("registryId");
            set => SetState("registryId", value);
        }

        /// <summary>
        /// Address of whiteList contract.
        /// </summary>
        /// <value>If specified, whiteList is always auto-updated and manual <see cref="WhiteList" /> is overridden.</value>
        [JsonPropertyName("whiteListContract")]
        public string WhiteListContract
        {
            get => (string?)GetState("whiteListContract");
            set => SetState("whiteListContract", value);
        }

        /// <summary>
        /// Node addresses that constitute the white list of nodes.
        /// </summary>
        [JsonPropertyName("whiteList")]
        public string[] WhiteList
        {
            get => (string[])GetState("whiteList");
            set => SetState("whiteList", value);
        }


        internal ChainConfiguration() { }

        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="chain">One of <see cref="Chain" />. The chain that this configuration is related to.</param>
        /// <param name="clientConfiguration">The configuration for the client whose the chain configuration belongs to.</param>
        /// <example>
        /// <code>
        /// ChainConfiguration goerliConfiguration = new ChainConfiguration(Chain.Goerli, in3Client.Configuration);
        /// </code>
        /// </example>
        public ChainConfiguration(Chain chain, ClientConfiguration clientConfiguration)
        {
            clientConfiguration.AddChainConfiguration(chain, this);
        }

        internal void AddNodeConfiguration(NodeConfiguration nodeConfiguration)
        {
            if (_nodesConfig == null)
            {
                _nodesConfig = new List<NodeConfiguration>();
            }
            _nodesConfig.Add(nodeConfiguration);
        }

        internal override bool HasChanged()
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