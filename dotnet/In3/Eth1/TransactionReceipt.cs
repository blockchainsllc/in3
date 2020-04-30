using System.Numerics;
using System.Text.Json.Serialization;
using In3.Utils;

namespace In3.Eth1
{
    public class TransactionReceipt
    {
        [JsonPropertyName("blockHash")] public string BlockHash { get; set; }
        [JsonPropertyName("blockNumber"), JsonConverter(typeof(BigIntegerFromHexConverter))] public BigInteger BlockNumber { get; set; }
        [JsonPropertyName("contractAddress")] public string ContractAddress { get; set; }
        [JsonPropertyName("from")] public string From { get; set; }
        [JsonPropertyName("transactionHash")] public string TransactionHash { get; set; }
        [JsonPropertyName("transactionIndex"), JsonConverter(typeof(IntFromHexConverter))] public int TransactionIndex { get; set; }
        [JsonPropertyName("to")] public string To { get; set; }
        [JsonPropertyName("gasUsed"), JsonConverter(typeof(LongFromHexConverter))] public long GasUsed { get; set; }
        [JsonPropertyName("logs")] public Log[] Logs { get; set; }
        [JsonPropertyName("logsBloom")] public string LogsBloom { get; set; }
        [JsonPropertyName("root")] public string Root { get; set; }
        [JsonPropertyName("status"), JsonConverter(typeof(BoolFromHexConverter))] public bool Status { get; set; }
    }
}