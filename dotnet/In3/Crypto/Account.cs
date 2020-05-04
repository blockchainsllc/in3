using System.Text.Json.Serialization;

namespace In3.Crypto
{
    public class Account
    {
        [JsonPropertyName("address")] public string Address { get; set; }
        [JsonPropertyName("publicKey")] public string PublicKey { get; set; }
    }
}