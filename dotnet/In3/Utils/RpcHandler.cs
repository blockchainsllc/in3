using System;
using System.Collections.Generic;
using System.Text.Json;
using In3.Error;
using In3.Rpc;

namespace In3.Utils
{
    internal static class RpcHandler
    {
        public static T From<T>(string json)
        {
            Response<T> response = JsonSerializer.Deserialize<Response<T>>(json);
            if (response.Error != null)
            {
                throw new RpcException(response.Error.Code, response.Error.Message);
            }
            return response.Result;
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