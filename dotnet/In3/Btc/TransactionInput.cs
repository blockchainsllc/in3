using System.Text.Json.Serialization;

namespace In3.Btc
{
    /// <summary>
    /// Input of a transaction.
    /// </summary>
    public class TransactionInput
    {
        /// <summary>
        /// The transaction id.
        /// </summary>
        [JsonPropertyName("txid")] public string Txid { get; set; }

        /// <summary>
        /// The index of the transactionoutput.
        /// </summary>
        [JsonPropertyName("vout")] public uint Yout { get; set; }

        /// <summary>
        /// The script.
        /// </summary>
        [JsonPropertyName("scriptSig")] public ScriptSig ScriptSig { get; set; }

        /// <summary>
        /// Hex-encoded witness data (if any).
        /// </summary>
        [JsonPropertyName("txinwitness")] public string[] Txinwitness { get; set; }

        /// <summary>
        /// The script sequence number.
        /// </summary>
        [JsonPropertyName("sequence")] public uint Sequence { get; set; }
    }
}