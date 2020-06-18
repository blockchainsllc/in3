using System.Text.Json.Serialization;

namespace In3.Eth1
{
    /// <summary>
    /// Class that holds a block with its transaction hash array.
    /// </summary>
    public class TransactionHashBlock : Block
    {
        /// <summary>
        /// Array with the full transactions containing on this block.
        /// </summary>
        /// <remarks>
        /// Returned when <see langword="shouldIncludeTransactions" /> on <see cref="Eth1.Api" /> get block methods are set to <see langword="false"/>.
        /// </remarks>
        [JsonPropertyName("transactions")] public string[] Transactions { get; set; }
    }
}