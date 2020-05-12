using System.Text.Json.Serialization;
using csharp.Utils;

namespace In3.Rpc
{
    internal class Response<T>
    {
        [JsonPropertyName("jsonrpc")] public string JsonRpc { get; set; }
        [JsonPropertyName("id"), JsonConverter(typeof(IdConverter))] public Id Id { get; set; }
        [JsonPropertyName("result")] public T Result { get; set; }
        [JsonPropertyName("error")] public Error Error { get; set; }
    }
}