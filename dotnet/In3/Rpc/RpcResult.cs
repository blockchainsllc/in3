using System.Text.Json.Serialization;

namespace In3.Rpc
{
    public class RpcResult<T>
    {
        [JsonPropertyName("id")] public long Id { get; set; }
        [JsonPropertyName("result")] public T Result { get; set; }
        [JsonPropertyName("response")] public string Response { get; set; }
    }
}