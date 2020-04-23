using System.Text.Json.Serialization;

namespace In3.Rpc
{
    internal class Response<T>
    {
        [JsonPropertyName("id")] public long Id { get; set; }
        [JsonPropertyName("result")] public T Result { get; set; }
        [JsonPropertyName("error")] public Error Error { get; set; }
    }
}