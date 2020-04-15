using System.Numerics;
using System.Text.Json.Serialization;
using In3.Utils;

namespace In3.Eth1
{
    public class Log
    {
        [JsonPropertyName("removed")] public bool Removed { get; set; }
        [JsonPropertyName("logIndex"), JsonConverter(typeof(IntFromHexConverter))] public int LogIndex { get; set; }
        [JsonPropertyName("transactionIndex"), JsonConverter(typeof(IntFromHexConverter))] public int TransactionIndex { get; set; }
        [JsonPropertyName("transactionHash")] public string TransactionHash { get; set; }
        [JsonPropertyName("blockHash")] public string BlockHash { get; set; }
        [JsonPropertyName("blockNumber"), JsonConverter(typeof(CustomBigIntegerFromHexConverter))] public long BlockNumber { get; set; }
        [JsonPropertyName("address")] public string Address { get; set; }
        [JsonPropertyName("topics")] public string[] Topics { get; set; }
        [JsonPropertyName("data")] public string Data { get; set; }
        [JsonPropertyName("type")] public string Type { get; set; }
    }
}