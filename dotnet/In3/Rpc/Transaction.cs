using System.Text.Json.Serialization;

namespace In3.Rpc
{
    public class Transaction
    {
        [JsonPropertyName("to")] public string To { get; set; }
        [JsonPropertyName("from")] public string From { get; set; }
        [JsonPropertyName("gas")] public string Gas { get; set; }
        [JsonPropertyName("gasPrice")] public string GasPrice { get; set; }
        [JsonPropertyName("value")] public string Value { get; set; }
        [JsonPropertyName("nonce")] public string Nonce { get; set; }
        [JsonPropertyName("data")] public string Data { get; set; }
    }
}