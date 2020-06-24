using System.Text.Json.Serialization;

namespace In3.Btc
{
    /// <summary>
    /// Output of a transaction.
    /// </summary>
    public class TransactionOutput
    {
        /// <summary>
        /// The value in bitcoins.
        /// </summary>
        [JsonPropertyName("value")] public float Value { get; set; }

        /// <summary>
        /// The index in the transaction.
        /// </summary>
        [JsonPropertyName("n")] public uint N { get; set; }

        /// <summary>
        /// The script of the transaction. 
        /// </summary>
        [JsonPropertyName("scriptPubKey")] public ScriptPubKey ScriptPubKey { get; set; }
    }
}