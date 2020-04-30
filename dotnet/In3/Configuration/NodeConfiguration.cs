using System.Text.Json.Serialization;

namespace In3.Configuration
{
    public class NodeConfiguration : BaseConfiguration
    {
        [JsonPropertyName("url")] public string Url {
            get => (string) GetState("url");
            set => SetState("url", value);
        }
        [JsonPropertyName("props")] public long? Props {
            get => (long) GetState("props");
            set => SetState("props", value);
        }
        [JsonPropertyName("address")] public string Address {
            get => (string) GetState("address");
            set => SetState("address", value);
        }

        public NodeConfiguration(ChainConfiguration config) {
            config.AddNodeConfiguration(this);
        }

        public override bool HasChanged()
        {
            return IsDirty();
        }
        
        internal void MarkSynced()
        {
            Clean();
        }
        
    }
}