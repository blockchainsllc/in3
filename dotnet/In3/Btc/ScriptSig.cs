using System.Text.Json.Serialization;

namespace In3.Btc
{
    /// <summary>
    /// Script on a transaction input.
    /// </summary>
    public class ScriptSig
    {
        /// <summary>
        /// The asm data.
        /// </summary>
        [JsonPropertyName("asm")] public string Asm { get; set; }

        /// <summary>
        /// The raw hex data.
        /// </summary>
        [JsonPropertyName("hex")] public string Hex { get; set; }
    }
}