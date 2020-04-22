using System;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace In3.Utils
{
    // HOW TO: https://docs.microsoft.com/en-us/dotnet/standard/serialization/system-text-json-converters-how-to
    public class CustomLongFromHexConverter : JsonConverter<long>
    {
        public override long Read(
            ref Utf8JsonReader reader,
            Type typeToConvert,
            JsonSerializerOptions options) => (long) TypesMatcher.HexStringToBigint(reader.GetString());

        public override void Write(
            Utf8JsonWriter writer,
            long intVal,
            JsonSerializerOptions options) =>
            writer.WriteStringValue(TypesMatcher.BigIntToPrefixedHex(intVal));
    }
}