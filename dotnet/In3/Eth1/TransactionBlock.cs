using System.Text.Json.Serialization;

namespace In3.Eth1
{
    /// <summary>
    /// Class that holds a block with its full transaction array: <see cref="Transaction" />.
    /// </summary>
    public class TransactionBlock : Block
    {
        /// <summary>
        /// Array with the full transactions containing on this block.
        /// </summary>
        /// <remarks>
        /// Returned when <see langword="shouldIncludeTransactions" /> on <see cref="Eth1.Api" /> get block methods are set to <see langword="true"/>.
        /// </remarks>
        [JsonPropertyName("transactions")] public Transaction[] Transactions { get; set; }
    }
}