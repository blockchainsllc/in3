using System.Numerics;
using System.Text.Json.Serialization;
using In3.Utils;

namespace In3.Eth1
{
    public abstract class Block
    {
        [JsonPropertyName("totalDifficulty")] public string TotalDifficulty { get; set; }
        [JsonPropertyName("gasLimit"), JsonConverter(typeof(BigIntegerFromHexConverter))] public BigInteger GasLimit { get; set; }
        [JsonPropertyName("extraData")] public string ExtraData { get; set; }
        [JsonPropertyName("difficulty"), JsonConverter(typeof(BigIntegerFromHexConverter))] public BigInteger Difficulty { get; set; }
        [JsonPropertyName("author")] public string Author { get; set; }
        [JsonPropertyName("transactionsRoot")] public string TransactionsRoot { get; set; }
        [JsonPropertyName("receiptsRoot")] public string ReceiptsRoot { get; set; }
        [JsonPropertyName("stateRoot")] public string StateRoot { get; set; }
        [JsonPropertyName("timestamp"), JsonConverter(typeof(LongFromHexConverter))] public long Timestamp { get; set; }
        [JsonPropertyName("sha3Uncles")] public string Sha3Uncles { get; set; }
        [JsonPropertyName("size"), JsonConverter(typeof(LongFromHexConverter))] public long Size { get; set; }
        // [JsonPropertyName("sealFields")] public string SealFields { get; set; }
        [JsonPropertyName("hash")] public string Hash { get; set; }
        [JsonPropertyName("logsBloom")] public string LogsBloom { get; set; }
        [JsonPropertyName("mixHash")] public string MixHash { get; set; }
        [JsonPropertyName("nonce")] public string Nonce { get; set; }
        [JsonPropertyName("number"), JsonConverter(typeof(BigIntegerFromHexConverter))] public BigInteger Number { get; set; }
        [JsonPropertyName("parentHash")] public string ParentHash { get; set; }
        [JsonPropertyName("uncles")] public string[] Uncles { get; set; }
    }
}