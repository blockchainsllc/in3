using System.Text.Json.Serialization;

namespace In3.Btc
{
    /// <summary>
    /// A Block.
    /// </summary>
    public class Block<T> : BlockHeader
    {
        /// <summary>
        /// Transactions or Transaction ids of a block. <see cref="Api.GetBlockWithTxData"/> or <see cref="Api.GetBlockWithTxIds"/>.
        /// </summary>
        [JsonPropertyName("tx")] public T[] Tx { get; set; }
        /// <summary>
        /// Size of this block in bytes.
        /// </summary>
        [JsonPropertyName("size")] public uint Size { get; set; }
        /// <summary>
        /// Weight of this block in bytes.
        /// </summary>
        [JsonPropertyName("weight")] public int Weight { get; set; }
    }
}