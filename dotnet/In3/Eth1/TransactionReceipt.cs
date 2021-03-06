using System.Numerics;
using System.Text.Json.Serialization;
using In3.Utils;

namespace In3.Eth1
{
    /// <summary>
    /// Class that represents a transaction receipt. See <see cref="Eth1.Api.GetTransactionReceipt" />.
    /// </summary>
    public class TransactionReceipt
    {
        /// <summary>
        /// Hash of the block with the transaction which this receipt is associated with.
        /// </summary>
        [JsonPropertyName("blockHash")] public string BlockHash { get; set; }

        /// <summary>
        /// Number of the block with the transaction which this receipt is associated with.
        /// </summary>
        [JsonPropertyName("blockNumber"), JsonConverter(typeof(BigIntegerFromHexConverter))] public BigInteger BlockNumber { get; set; }

        /// <summary>
        /// Address of the smart contract invoked in the transaction (if any).
        /// </summary>
        [JsonPropertyName("contractAddress")] public string ContractAddress { get; set; }

        /// <summary>
        /// Address of the account that signed the transaction.
        /// </summary>
        [JsonPropertyName("from")] public string From { get; set; }

        /// <summary>
        /// Hash of the transaction.
        /// </summary>
        [JsonPropertyName("transactionHash")] public string TransactionHash { get; set; }

        /// <summary>
        /// Number of the transaction on the block.
        /// </summary>
        [JsonPropertyName("transactionIndex"), JsonConverter(typeof(IntFromHexConverter))] public int TransactionIndex { get; set; }

        /// <summary>
        /// Address whose value will be transfered to.
        /// </summary>
        [JsonPropertyName("to")] public string To { get; set; }


        /// <summary>
        /// Gas used on this transaction.
        /// </summary>
        [JsonPropertyName("gasUsed"), JsonConverter(typeof(LongFromHexConverter))] public long GasUsed { get; set; }

        /// <summary>
        /// Logs/events for this transaction.
        /// </summary>
        [JsonPropertyName("logs")] public Log[] Logs { get; set; }

        /// <summary>
        /// A bloom filter of logs/events generated by contracts during transaction execution. Used to efficiently rule out transactions without expected logs.
        /// </summary>
        [JsonPropertyName("logsBloom")] public string LogsBloom { get; set; }

        /// <summary>
        /// Merkle root of the state trie after the transaction has been  executed (optional after Byzantium hard fork EIP609).
        /// </summary>
        [JsonPropertyName("root")] public string Root { get; set; }

        /// <summary>
        /// Status of the transaction.
        /// </summary>
        [JsonPropertyName("status"), JsonConverter(typeof(BoolFromHexConverter))] public bool Status { get; set; }
    }
}