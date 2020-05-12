using System.Collections.Generic;
using System.Text.Json.Serialization;

namespace In3.Rpc
{
    public class Request
    {
        [JsonPropertyName("method")] public string Method { get; set; }
        [JsonPropertyName("params")] public object[] Params { get; set; }
        [JsonPropertyName("in3")] public Dictionary<string, object> In3 { get; set;  }
    }
}