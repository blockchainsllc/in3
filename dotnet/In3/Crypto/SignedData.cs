using System.Text.Json.Serialization;

namespace In3.Crypto
{
    public class SignedData
    {
        [JsonPropertyName("message")] public string Message { get; set; }
        [JsonPropertyName("messageHash")] public string MessageHash { get; set; }
        [JsonPropertyName("signature")] public string Signature { get; set; }
        [JsonPropertyName("r")] public string R { get; set; }
        [JsonPropertyName("s")] public string S { get; set; }
        [JsonPropertyName("v")] public int V { get; set; }
    }
}