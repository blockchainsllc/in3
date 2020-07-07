using System.Threading.Tasks;
using In3.Utils;

namespace In3.Btc
{
    /// <summary>
    /// API for handling BitCoin data. Use it when connected to <see cref="In3.Chain.Btc" />.
    /// </summary>
    public class Api
    {
        private readonly IN3 _in3;

        private const string BtcGetBlock = "getblock";
        private const string BtcGetBlockHeader = "getblockheader";
        private const string BtcGetRawTransaction = "getrawtransaction";

        internal Api(IN3 in3)
        {
            this._in3 = in3;
        }

        /// <summary>
        /// Retrieves the transaction and returns the data as json.
        /// </summary>
        /// <param name="txid">The transaction Id.</param>
        /// <returns>The transaction object.</returns>
        /// <example>
        ///   <code>
        ///   Transaction desiredTransaction = in3.Btc.GetTransaction("1427c7d1698e61afe061950226f1c149990b8c1e1b157320b0c4acf7d6b5605d");
        ///   </code>
        /// </example>
        public async Task<Transaction> GetTransaction(string txid)
        {
            string jsonResponse = await _in3.SendRpc(BtcGetRawTransaction, new object[] { txid, true });
            return RpcHandler.From<Transaction>(jsonResponse);
        }

        /// <summary>
        /// Retrieves the serialized transaction (bytes).
        /// </summary>
        /// <param name="txid">The transaction Id.</param>
        /// <returns>The byte array for the Transaction.</returns>
        /// <example>
        ///   <code>
        ///   byte[] serializedTransaction = in3.Btc.GetTransactionBytes("1427c7d1698e61afe061950226f1c149990b8c1e1b157320b0c4acf7d6b5605d");
        ///   </code>
        /// </example>
        public async Task<byte[]> GetTransactionBytes(string txid)
        {
            string jsonResponse = await _in3.SendRpc(BtcGetRawTransaction, new object[] { txid, false });
            return DataTypeConverter.HexStringToByteArray(RpcHandler.From<string>(jsonResponse));
        }

        /// <summary>
        /// Retrieves the blockheader.
        /// </summary>
        /// <param name="blockHash">The hash of the Block.</param>
        /// <returns>The Block header.</returns>
        /// <example>
        ///   <code>
        ///   BlockHeader header = in3.Btc.GetBlockHeader("0000000000000000000cd3c5d7638014e78a5fba33be5fa5cb10ef9f03d99e60");
        ///   </code>
        /// </example>
        public async Task<BlockHeader> GetBlockHeader(string blockHash)
        {
            string jsonResponse = await _in3.SendRpc(BtcGetBlockHeader, new object[] { blockHash, true });
            return RpcHandler.From<BlockHeader>(jsonResponse);
        }

        /// <summary>
        /// Retrieves the byte array representing teh serialized blockheader data.
        /// </summary>
        /// <param name="blockHash">The hash of the Block.</param>
        /// <returns>The Block header in bytes.</returns>
        /// <example>
        ///   <code>
        ///   byte[] header = in3.Btc.GetBlockHeaderBytes("0000000000000000000cd3c5d7638014e78a5fba33be5fa5cb10ef9f03d99e60");
        ///   </code>
        /// </example>
        public async Task<byte[]> GetBlockHeaderBytes(string blockHash)
        {
            string jsonResponse = await _in3.SendRpc(BtcGetBlockHeader, new object[] { blockHash, false });
            return DataTypeConverter.HexStringToByteArray(RpcHandler.From<string>(jsonResponse));
        }

        /// <summary>
        /// Retrieves the block including the full transaction data. Use <see cref="Api.GetBlockWithTxIds" /> for only the transaction ids.
        /// </summary>
        /// <param name="blockHash">The hash of the Block.</param>
        /// <returns>The block of type <see cref="Block{Transaction}"/>.</returns>
        /// <example>
        ///   <code>
        ///   Block{Transaction} block = in3.Btc.GetBlockWithTxData("000000000000000000064ba7512ecc70cabd7ed17e31c06f2205d5ecdadd6d22");
        ///   Transaction t1 = block.Tx[0];
        ///   </code>
        /// </example>
        public async Task<Block<Transaction>> GetBlockWithTxData(string blockHash)
        {
            string jsonResponse = await _in3.SendRpc(BtcGetBlock, new object[] { blockHash, 2 });
            return RpcHandler.From<Block<Transaction>>(jsonResponse);
        }

        /// <summary>
        /// Retrieves the block including only transaction ids. Use <see cref="Api.GetBlockWithTxData" /> for the full transaction data.
        /// </summary>
        /// <param name="blockHash">The hash of the Block.</param>
        /// <returns>The block of type <see cref="Block{String}"/>.</returns>
        /// <example>
        ///   <code>
        ///   Block{string} block = in3.Btc.GetBlockWithTxIds("000000000000000000064ba7512ecc70cabd7ed17e31c06f2205d5ecdadd6d22");
        ///   string t1 = block.Tx[0];
        ///   </code>
        /// </example>
        public async Task<Block<string>> GetBlockWithTxIds(string blockHash)
        {
            string jsonResponse = await _in3.SendRpc(BtcGetBlock, new object[] { blockHash, 1 });
            return RpcHandler.From<Block<string>>(jsonResponse);
        }

        /// <summary>
        /// Retrieves the serialized block in bytes.
        /// </summary>
        /// <param name="blockHash">The hash of the Block.</param>
        /// <returns>The bytes of the block.</returns>
        /// <example>
        ///   <code>
        ///   byte[] blockBytes = in3.Btc.GetBlockBytes("000000000000000000064ba7512ecc70cabd7ed17e31c06f2205d5ecdadd6d22");
        ///   </code>
        /// </example>
        public async Task<byte[]> GetBlockBytes(string blockHash)
        {
            string jsonResponse = await _in3.SendRpc(BtcGetBlock, new object[] { blockHash, false });
            return DataTypeConverter.HexStringToByteArray(RpcHandler.From<string>(jsonResponse));
        }
    }
}