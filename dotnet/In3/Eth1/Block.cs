using System.Numerics;
using System.Text.Json.Serialization;
using In3.Utils;

namespace In3.Eth1
{
    /// <summary>
    /// Class that represents as Ethereum block.
    /// </summary>
    public abstract class Block
    {
        /// <summary>
        /// Total Difficulty as a sum of all difficulties starting from genesis.
        /// </summary>
        [JsonPropertyName("totalDifficulty")] public string TotalDifficulty { get; set; }

        /// <summary>
        /// Gas limit.
        /// </summary>
        [JsonPropertyName("gasLimit"), JsonConverter(typeof(BigIntegerFromHexConverter))] public BigInteger GasLimit { get; set; }

        /// <summary>
        /// Extra data.
        /// </summary>
        [JsonPropertyName("extraData")] public string ExtraData { get; set; }
        /// <summary>
        /// Dificulty of the block.
        /// </summary>
        [JsonPropertyName("difficulty"), JsonConverter(typeof(BigIntegerFromHexConverter))] public BigInteger Difficulty { get; set; }

        /// <summary>
        /// The miner of the block.
        /// </summary>
        [JsonPropertyName("author")] public string Author { get; set; }

        /// <summary>
        /// The roothash of the merkletree containing all transaction of the block.
        /// </summary>
        [JsonPropertyName("transactionsRoot")] public string TransactionsRoot { get; set; }

        /// <summary>
        /// The roothash of the merkletree containing all transaction receipts of the block.
        /// </summary>
        [JsonPropertyName("receiptsRoot")] public string ReceiptsRoot { get; set; }

        /// <summary>
        /// The roothash of the merkletree containing the complete state.
        /// </summary>
        [JsonPropertyName("stateRoot")] public string StateRoot { get; set; }

        /// <summary>
        /// Epoch timestamp when the block was created.
        /// </summary>
        [JsonPropertyName("timestamp"), JsonConverter(typeof(LongFromHexConverter))] public long Timestamp { get; set; }

        /// <summary>
        /// The roothash of the merkletree containing all uncles of the block.
        /// </summary>
        [JsonPropertyName("sha3Uncles")] public string Sha3Uncles { get; set; }

        /// <summary>
        /// Size of the block.
        /// </summary>
        [JsonPropertyName("size"), JsonConverter(typeof(LongFromHexConverter))] public long Size { get; set; }

        /// <summary>
        /// The block hash.
        /// </summary>
        [JsonPropertyName("hash")] public string Hash { get; set; }

        /// <summary>
        /// The logsBloom data of the block.
        /// </summary>
        [JsonPropertyName("logsBloom")] public string LogsBloom { get; set; }

        /// <summary>
        /// The  mix hash of the block. (only valid of proof of work).
        /// </summary>
        [JsonPropertyName("mixHash")] public string MixHash { get; set; }

        /// <summary>
        /// The nonce.
        /// </summary>
        [JsonPropertyName("nonce")] public string Nonce { get; set; }

        /// <summary>
        /// The index of the block.
        /// </summary>
        [JsonPropertyName("number"), JsonConverter(typeof(BigIntegerFromHexConverter))] public BigInteger Number { get; set; }

        /// <summary>
        /// The parent block`s hash.
        /// </summary>
        [JsonPropertyName("parentHash")] public string ParentHash { get; set; }

        /// <summary>
        /// List of uncle hashes.
        /// </summary>
        [JsonPropertyName("uncles")] public string[] Uncles { get; set; }
    }
}