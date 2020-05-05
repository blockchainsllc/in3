using System;
using System.Text.Json;
using System.Text.Json.Serialization;
using In3.Rpc;
using In3.Utils;

namespace csharp.Utils
{
    public class IdConverter : JsonConverter<Id>
    {
        public override Id Read(
            ref Utf8JsonReader reader,
            Type typeToConvert,
            JsonSerializerOptions options)
        {
            if (reader.TokenType == JsonTokenType.Number)
            {
                return new Id(reader.GetInt64());
            }
            else
            {
                return new Id(reader.GetString());
            };
        }

        public override void Write(
            Utf8JsonWriter writer,
            Id val,
            JsonSerializerOptions options) =>
            writer.WriteStringValue(val.AsString());
    }
}