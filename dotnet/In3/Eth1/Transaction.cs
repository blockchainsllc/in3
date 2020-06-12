using System.Numerics;
using System.Text.Json.Serialization;
using In3.Utils;

namespace In3.Eth1
{
    /// <summary>
    /// Class representing a transaction that was accepted by the Ethereum chain.
    /// </summary>
    public class Transaction
    {
        /// <summary>
        /// Hash of the block that this transaction belongs to.
        /// </summary>
        [JsonPropertyName("blockHash")] public string BlockHash { get; set; }

        /// <summary>
        /// Number of the block that this transaction belongs to.
        /// </summary>
        [JsonPropertyName("blockNumber"), JsonConverter(typeof(BigIntegerFromHexConverter))] public BigInteger BlockNumber { get; set; }

        /// <summary>
        /// Chain id that this transaction belongs to.
        /// </summary>
        // TODO: swap with a proper chain enum.
        [JsonPropertyName("chainId")] public string ChainId { get; set; }

        /// <summary>
        /// Address of the deployed contract (if successfull).
        /// </summary>
        [JsonPropertyName("creates")] public string Creates { get; set; }


        /// <summary>
        /// Address whose private key signed this transaction with.
        /// </summary>
        [JsonPropertyName("from")] public string From { get; set; }

        /// <summary>
        /// Gas for the transaction.
        /// </summary>
        [JsonPropertyName("gas"), JsonConverter(typeof(LongFromHexConverter))] public long Gas { get; set; }


        /// <summary>
        /// Gas price (in wei) for each unit of gas.
        /// </summary>
        [JsonPropertyName("gasPrice"), JsonConverter(typeof(LongFromHexConverter))] public long GasPrice { get; set; }

        /// <summary>
        /// Transaction hash.
        /// </summary>
        [JsonPropertyName("hash")] public string Hash { get; set; }

        /// <summary>
        /// Transaction data.
        /// </summary>
        [JsonPropertyName("input")] public string Input { get; set; }

        /// <summary>
        /// Nonce for this transaction.
        /// </summary>
        [JsonPropertyName("nonce"), JsonConverter(typeof(LongFromHexConverter))] public long Nonce { get; set; }

        /// <summary>
        /// Public key.
        /// </summary>
        [JsonPropertyName("publicKey")] public string PublicKey { get; set; }

        /// <summary>
        /// Part of the transaction signature.
        /// </summary>
        [JsonPropertyName("r")] public string R { get; set; }
        /// <summary>
        /// Transaction as rlp encoded data.
        /// </summary>
        [JsonPropertyName("raw")] public string Raw { get; set; }

        /// <summary>
        /// Part of the transaction signature.
        /// </summary>
        [JsonPropertyName("s")] public string S { get; set; }

        /// <summary>
        /// Part of the transaction signature. V is parity set by v = 27 + (r % 2).
        /// </summary>
        /// <value>Either <see langword="0" /> or <see langword="1" />.</value>
        [JsonPropertyName("standardV")] public string StandardV { get; set; }

        /// <summary>
        /// To address of the transaction.
        /// </summary>
        [JsonPropertyName("to")] public string To { get; set; }

        /// <summary>
        /// Transaction index.
        /// </summary>
        // TODO: Check if this doesnt make more sense as a long.
        [JsonPropertyName("transactionIndex")] public string TransactionIndex { get; set; }

        /// <summary>
        /// The <see cref="StandardV" /> plus the chain.
        /// </summary>
        [JsonPropertyName("v")] public string V { get; set; }

        /// <summary>
        /// Value of the transaction.
        /// </summary>
        [JsonPropertyName("value"), JsonConverter(typeof(BigIntegerFromHexConverter))] public BigInteger Value { get; set; }
    }
}