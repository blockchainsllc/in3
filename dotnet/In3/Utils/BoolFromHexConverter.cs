using System;
using System.Linq;
using System.Text.Json;
using System.Text.Json.Serialization;
using In3.Utils;

namespace In3.Utils
{
    internal class BoolFromHexConverter : JsonConverter<bool>
    {
        // https://developer.mozilla.org/en-US/docs/Glossary/Falsy
        private static readonly string[] FalsyValues = {
            "false",
            "0",
            "-0",
            "0n",
            "",
            "null",
            "undefined",
            "NaN"
        };
        
        private static readonly string[] HexsFalsyValues = {
            "0x",
            "0x0",
            "0x00",
        };

        public override bool Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
        {
            string boolString = reader.GetString();
            return !(HexsFalsyValues.Contains(boolString) || HexsFalsyValues.Contains(boolString));
        }

        public override void Write(
            Utf8JsonWriter writer,
            bool boolVal,
            JsonSerializerOptions options) =>
            writer.WriteStringValue(boolVal.ToString());
    }
}