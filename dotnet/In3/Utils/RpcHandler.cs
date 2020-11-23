using System.Collections.Generic;
using System.Text.Json;
using In3.Error;
using In3.Rpc;

namespace In3.Utils
{
    internal static class RpcHandler
    {
        // TODO: Checkout this file when the server is fully json-rpc compliant as far as errors go.
        public static T From<T>(string json)
        {
            var stjSample = JsonDocument.Parse(json);
            bool isError = stjSample.RootElement.TryGetProperty("error", out JsonElement value);

            if (!isError)
            {
                Response<T, Rpc.Error> response = JsonSerializer.Deserialize<Response<T, Rpc.Error>>(json);
                return response.Result;
            }

            if (value.ValueKind != JsonValueKind.String)
            {
                Response<T, Rpc.Error> response = JsonSerializer.Deserialize<Response<T, Rpc.Error>>(json);
                throw new RpcException(response.Error.Code, response.Error.Message);
            }
            else
            {
                Response<T, string> response = JsonSerializer.Deserialize<Response<T, string>>(json);
                throw new RpcException(response.Error);
            }
        }

        public static string To(string method, object[] parameters, Dictionary<string, object> in3)
        {
            JsonSerializerOptions options = new JsonSerializerOptions
            {
                IgnoreNullValues = true
            };

            Request rpc = new Request();
            rpc.Method = method;
            rpc.Params = parameters;
            rpc.In3 = in3;

            return JsonSerializer.Serialize(rpc, options);
        }
    }
}