using System.Text.Json.Serialization;
using In3.Utils;

namespace In3.Rpc
{
    internal class Response<T1, T2>
    {
        [JsonPropertyName("jsonrpc")] public string JsonRpc { get; set; }
        [JsonPropertyName("id"), JsonConverter(typeof(IdConverter))] public Id Id { get; set; }
        [JsonPropertyName("result")] public T1 Result { get; set; }
        [JsonPropertyName("error")] public T2 Error { get; set; }
    }
}