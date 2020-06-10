using System.Text.Json.Serialization;

namespace In3.Btc
{
    /// <summary>
    /// A BitCoin Transaction.
    /// </summary>
    public class Transaction
    {
        /// <summary>
        /// Transaction Id.
        /// </summary>
        [JsonPropertyName("txid")] public string Txid { get; set; }

        /// <summary>
        /// The transaction hash (differs from txid for witness transactions).
        /// </summary>
        [JsonPropertyName("hash")] public string Hash { get; set; }

        /// <summary>
        /// The version.
        /// </summary>
        [JsonPropertyName("version")] public int Version { get; set; }

        /// <summary>
        /// The serialized transaction size.
        /// </summary>
        [JsonPropertyName("size")] public int Size { get; set; }

        /// <summary>
        /// The virtual transaction size (differs from size for witness transactions).
        /// </summary>
        [JsonPropertyName("vsize")] public int Vsize { get; set; }

        /// <summary>
        /// The transaction’s weight (between vsize4-3 and vsize4).
        /// </summary>
        [JsonPropertyName("weight")] public int Weight { get; set; }

        /// <summary>
        /// The locktime.
        /// </summary>
        [JsonPropertyName("locktime")] public uint Locktime { get; set; }

        /// <summary>
        /// The hex representation of raw data.
        /// </summary>
        [JsonPropertyName("hex")] public string Hex { get; set; }

        /// <summary>
        /// The block hash of the block containing this transaction.
        /// </summary>
        [JsonPropertyName("blockhash")] public string Blockhash { get; set; }

        /// <summary>
        /// The confirmations. 
        /// </summary>
        [JsonPropertyName("confirmations")] public uint Confirmations { get; set; }

        /// <summary>
        /// The transaction time in seconds since epoch (Jan 1 1970 GMT).
        /// </summary>
        [JsonPropertyName("time")] public uint Time { get; set; }

        /// <summary>
        /// The block time in seconds since epoch (Jan 1 1970 GMT).
        /// </summary>
        [JsonPropertyName("blocktime")] public uint Blocktime { get; set; }

        /// <summary>
        /// The transaction inputs.
        /// </summary>
        [JsonPropertyName("vin")] public object[] Vin { get; set; }

        /// <summary>
        /// The transaction outputs.
        /// </summary>
        [JsonPropertyName("vout")] public object[] Vout { get; set; }
    }
}