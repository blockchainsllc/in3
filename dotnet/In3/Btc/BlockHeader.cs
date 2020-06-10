using System.Text.Json.Serialization;

namespace In3.Btc
{
    /// <summary>
    /// A Block header.
    /// </summary>
    public class BlockHeader
    {
        /// <summary>
        /// The hash of the blockheader.
        /// </summary>
        [JsonPropertyName("hash")] public string Hash { get; set; }

        /// <summary>
        /// Number of confirmations or blocks mined on top of the containing block.
        /// </summary>
        [JsonPropertyName("confirmations")] public uint Confirmations { get; set; }

        /// <summary>
        /// Block number.
        /// </summary>
        [JsonPropertyName("height")] public int Height { get; set; }

        /// <summary>
        /// Used version.
        /// </summary>
        [JsonPropertyName("version")] public int Version { get; set; }

        /// <summary>
        /// Version as hex.
        /// </summary>
        [JsonPropertyName("versionHex")] public string VersionHex { get; set; }

        /// <summary>
        /// Merkle root of the trie of all transactions in the block.
        /// </summary>
        [JsonPropertyName("merkleroot")] public string Merkleroot { get; set; }

        /// <summary>
        /// Unix timestamp in seconds since 1970.
        /// </summary>
        [JsonPropertyName("time")] public uint Time { get; set; }

        /// <summary>
        /// Unix timestamp in seconds since 1970.
        /// </summary>
        [JsonPropertyName("mediantime")] public uint Mediantime { get; set; }

        /// <summary>
        /// Nonce-field of the block.
        /// </summary>
        [JsonPropertyName("nonce")] public uint Nonce { get; set; }

        /// <summary>
        /// Bits (target) for the block as hex.
        /// </summary>
        [JsonPropertyName("bits")] public string Bits { get; set; }

        /// <summary>
        /// Difficulty of the block.
        /// </summary>
        [JsonPropertyName("difficulty")] public float Difficulty { get; set; }

        /// <summary>
        /// Total amount of work since genesis.
        /// </summary>
        [JsonPropertyName("chainwork")] public string Chainwork { get; set; }

        /// <summary>
        /// Number of transactions in the block.
        /// </summary>
        [JsonPropertyName("nTx")] public long NTx { get; set; }

        /// <summary>
        /// Hash of the parent blockheader.
        /// </summary>
        [JsonPropertyName("previousblockhash")] public string Previousblockhash { get; set; }

        /// <summary>
        /// Hash of the next blockheader.
        /// </summary>
        [JsonPropertyName("nextblockhash")] public string Nextblockhash { get; set; }
    }
}