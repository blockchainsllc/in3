using System.Text.Json.Serialization;

namespace In3.Rpc
{
    internal class Error
    {
        [JsonPropertyName("code")] public int Code { get; set; }
        [JsonPropertyName("message")] public string Message { get; set; }
    }
}