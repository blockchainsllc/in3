using System.Numerics;
using System.Text.Json.Serialization;
using In3.Utils;

namespace In3.Eth1
{
    /// <summary>
    /// Logs marks changes of state on the blockchain for events. The <see cref="Log" /> is a data object with information from logs.
    /// </summary>
    public class Log
    {
        /// <summary>
        /// Flags log removal (due to chain reorganization).
        /// </summary>
        /// <value>If <see langword="true"/> log has been removed, <see langword="false"/> its still a valid log.</value>
        [JsonPropertyName("removed")] public bool Removed { get; set; }

        /// <summary>
        /// Index position in the block.
        /// </summary>
        // TODO: Check if this can be null, i believe it would be converted to 0 on the json converter which is misleading.
        [JsonPropertyName("logIndex"), JsonConverter(typeof(IntFromHexConverter))] public int LogIndex { get; set; }

        /// <summary>
        /// index position log was created from.
        /// </summary>
        // TODO: Same of the above.
        [JsonPropertyName("transactionIndex"), JsonConverter(typeof(IntFromHexConverter))] public int TransactionIndex { get; set; }

        /// <summary>
        /// Hash of the transactions this log was created from. null when its pending log.
        /// </summary>
        [JsonPropertyName("transactionHash")] public string TransactionHash { get; set; }

        /// <summary>
        /// Hash of the block this log was in. null when its pending log.
        /// </summary>
        [JsonPropertyName("blockHash")] public string BlockHash { get; set; }

        /// <summary>
        /// Number of the block this log was in.
        /// </summary>
        // TODO: Verify for nulls
        [JsonPropertyName("blockNumber"), JsonConverter(typeof(BigIntegerFromHexConverter))] public BigInteger BlockNumber { get; set; }

        /// <summary>
        /// Address from which this log originated.
        /// </summary>
        [JsonPropertyName("address")] public string Address { get; set; }

        /// <summary>
        /// Array of 0 to 4 32 Bytes DATA of indexed log arguments. (In solidity: The first topic is the hash of the signature of the event (e.g. Deposit(address,bytes32,uint256)), except you declared the event with the anonymous specifier).
        /// </summary>
        [JsonPropertyName("topics")] public string[] Topics { get; set; }

        /// <summary>
        /// Data associated with the log.
        /// </summary>
        [JsonPropertyName("data")] public string Data { get; set; }

        /// <summary>
        /// Address from which this log originated.
        /// </summary>
        [JsonPropertyName("type")] public string Type { get; set; }
    }
}