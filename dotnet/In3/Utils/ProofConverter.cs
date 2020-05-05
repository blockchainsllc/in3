using System;
using System.Text.Json;
using System.Text.Json.Serialization;
using In3.Configuration;

namespace csharp.Utils
{
    internal class ProofConverter : JsonConverter<Proof>
    {
        public override Proof Read(
            ref Utf8JsonReader reader,
            Type typeToConvert,
            JsonSerializerOptions options) => Proof.FromValue(reader.GetString());

        public override void Write(
            Utf8JsonWriter writer,
            Proof proofVal,
            JsonSerializerOptions options) =>
            writer.WriteStringValue(proofVal.Value);
    }
}