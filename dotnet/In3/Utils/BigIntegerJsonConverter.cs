using System;
using System.Numerics;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace In3.Utils
{
    internal class BigIntegerFromHexConverter : JsonConverter<BigInteger>
    {
        public override BigInteger Read(
            ref Utf8JsonReader reader,
            Type typeToConvert,
            JsonSerializerOptions options) => DataTypeConverter.HexStringToBigint(reader.GetString());

        public override void Write(
            Utf8JsonWriter writer,
            BigInteger intVal,
            JsonSerializerOptions options) =>
            writer.WriteStringValue(DataTypeConverter.BigIntToPrefixedHex(intVal));
    }
}