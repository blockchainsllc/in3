using System.Text.Json.Serialization;

namespace In3.Configuration
{
    /// <summary>
    /// Class that represents part of the configuration to be applied on the <see cref="IN3" /> (in particular to each boot node).
    /// This is a child of <see cref="ChainConfiguration" />.
    /// </summary>
    public class NodeConfiguration : BaseConfiguration
    {
        /// <summary>
        /// Url of the bootnode which the client can connect to.
        /// </summary>
        [JsonPropertyName("url")]
        public string Url
        {
            get => (string)GetState("url");
            set => SetState("url", value);
        }

        /// <summary>
        /// Props define the capabilities of the node. Accepts a combination of values.
        /// </summary>
        /// <example>
        /// <code>
        /// nodeConfiguration.Props = Props.NodePropProof | Props.NodePropArchive;
        /// </code>
        /// </example>
        [JsonPropertyName("props")]
        public Props? Props
        {
            get => (Props)GetState("props");
            set => SetState("props", value);
        }

        /// <summary>
        /// Address of the node, which is the public address it is signing with.
        /// </summary>
        [JsonPropertyName("address")]
        public string Address
        {
            get => (string)GetState("address");
            set => SetState("address", value);
        }

        /// <summary>
        /// Constructor for the node configuration.
        /// </summary>
        /// <param name="config">The <see cref="ChainConfiguration" /> of which this node belongs to.</param>
        /// <example>
        /// <code>
        /// NodeConfiguration myDeployedNode = new NodeConfiguration(mainnetChainConfiguration);
        /// </code>
        /// </example>
        public NodeConfiguration(ChainConfiguration config)
        {
            config.AddNodeConfiguration(this);
        }

        internal override bool HasChanged()
        {
            return IsDirty();
        }

        internal void MarkSynced()
        {
            Clean();
        }
    }
}