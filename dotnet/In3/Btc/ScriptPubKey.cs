using System.Text.Json.Serialization;

namespace In3.Btc
{
    /// <summary>
    /// Script on a transaction output.
    /// </summary>
    public class ScriptPubKey
    {
        /// <summary>
        /// The asm data,
        /// </summary>
        [JsonPropertyName("asm")] public string Asm { get; set; }

        /// <summary>
        /// The raw hex data.
        /// </summary>
        [JsonPropertyName("hex")] public string Hex { get; set; }

        /// <summary>
        /// The required sigs.
        /// </summary>
        [JsonPropertyName("reqSigs")] public uint ReqSigs { get; set; }

        /// <summary>
        /// The type.
        /// </summary>
        /// <example>pubkeyhash</example>
        [JsonPropertyName("type")] public string Type { get; set; }

        /// <summary>
        /// List of addresses.
        /// </summary>
        [JsonPropertyName("addresses")] public string[] Addresses { get; set; }
    }
}