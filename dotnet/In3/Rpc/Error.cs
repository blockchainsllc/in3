using System.Text.Json.Serialization;

namespace In3.Rpc
{
    public class Error
    {
        [JsonPropertyName("code")] public int Code { get; set; }
        [JsonPropertyName("message")] public string Message { get; set; }
    }
}