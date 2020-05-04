using System.Text.Json.Serialization;

namespace In3.Rpc
{
    public class LogFilter
    {
        [JsonPropertyName("fromBlock")] public string FromBlock { get; set; }
        [JsonPropertyName("toBlock")] public string ToBlock { get; set; }
        [JsonPropertyName("address")] public string Address { get; set; }
        [JsonPropertyName("blockhash")] public string BlockHash { get; set; }
        [JsonPropertyName("topics")] public object[] Topics { get; set; }
    }
}