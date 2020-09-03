using System;
using System.Collections.Generic;
using System.Numerics;
using System.Text.Json;
using System.Threading.Tasks;
using In3.Crypto;
using In3.Error;
using In3.Utils;

namespace In3.Eth1
{
    /// <summary>
    /// Module based on Ethereum's api and web3. Works as a general parent for all Ethereum-specific operations.
    /// </summary>
    public class Api
    {
        private readonly IN3 _in3;

        private const string EthBlockNumber = "eth_blockNumber";
        private const string EthGetBlockByNumber = "eth_getBlockByNumber";
        private const string EthGetBlockByHash = "eth_getBlockByHash";
        private const string In3AbiEncode = "in3_abiEncode";
        private const string In3AbiDecode = "in3_abiDecode";
        private const string EthGasPrice = "eth_gasPrice";
        private const string EthChainId = "eth_chainId";
        private const string EthGetBalance = "eth_getBalance";
        private const string EthGetCode = "eth_getCode";
        private const string EthGetStorageAt = "eth_getStorageAt";
        private const string EthGetBlockTransactionCountByHash = "eth_getBlockTransactionCountByHash";
        private const string EthGetBlockTransactionCountByNumber = "eth_getBlockTransactionCountByNumber";
        private const string EthGetTransactionByBlockHashAndIndex = "eth_getTransactionByBlockHashAndIndex";
        private const string EthGetTransactionByBlockNumberAndIndex = "eth_getTransactionByBlockNumberAndIndex";
        private const string EthGetTransactionByHash = "eth_getTransactionByHash";
        private const string EthGetTransactionCount = "eth_getTransactionCount";
        private const string EthSendRawTransaction = "eth_sendRawTransaction";
        private const string EthSendTransaction = "eth_sendTransaction";
        private const string EthChecksumAddress = "in3_checksumAddress";
        private const string EthGetUncleCountByBlockHash = "eth_getUncleCountByBlockHash";
        private const string EthGetUncleCountByBlockNumber = "eth_getUncleCountByBlockNumber";
        private const string EthNewBlockFilter = "eth_newBlockFilter";
        private const string EthUninstallFilter = "eth_uninstallFilter";
        private const string EthCall = "eth_call";
        private const string EthEstimateGas = "eth_estimateGas";
        private const string EthNewFilter = "eth_newFilter";
        private const string EthGetFilterChanges = "eth_getFilterChanges";
        private const string EthGetFilterLogs = "eth_getFilterLogs";
        private const string EthGetLogs = "eth_getLogs";
        private const string EthGetTransactionReceipt = "eth_getTransactionReceipt";
        private const string EthGetUncleByBlockNumberAndIndex = "eth_getUncleByBlockNumberAndIndex";
        private const string EthENS = "in3_ens";

        internal Api(IN3 in3)
        {
            this._in3 = in3;
        }

        /// <summary>
        /// Returns the number of the most recent block the in3 network can collect signatures to verify.
        /// Can be changed by <see cref="Configuration.ClientConfiguration.ReplaceLatestBlock" />.
        /// If you need the very latest block, change <see cref="Configuration.ClientConfiguration.SignatureCount" /> to <see langword="0"/>.
        /// </summary>
        /// <returns>The number of the block.</returns>
        public async Task<BigInteger> BlockNumber()
        {
            string jsonResponse = await _in3.SendRpc(EthBlockNumber, new object[] { });
            return DataTypeConverter.HexStringToBigint(RpcHandler.From<string>(jsonResponse));
        }

        /// <summary>
        /// Blocks can be identified by sequential number in which it was mined, or root hash of the block merkle tree <see cref="Eth1.Api.GetBlockByHash" />.
        /// </summary>
        /// <param name="blockNumber">Desired block number or <see cref="BlockParameter.Latest" /> or <see cref="BlockParameter.Earliest" />.</param>
        /// <param name="shouldIncludeTransactions">If <see langword="true"/>, returns the full transaction objects, otherwise only its hashes. The default value is <see langword="true"/>.</param>
        /// <returns>The <see cref="Block" /> of the requested <b>blockNumber</b> (if exists).</returns>
        /// <remarks>
        /// <para>
        /// Returning <see cref="Block" /> must be cast to <see cref="TransactionBlock" /> or <see cref="TransactionHashBlock" /> to access the transaction data.
        /// </para>
        /// </remarks>
        /// <example>
        ///   <code>
        ///   TransactionBlock latest = (TransactionBlock) _client.Eth1.GetBlockByNumber(BlockParameter.Latest, true);
        ///   TransactionHashBlock earliest = (TransactionHashBlock) _client.Eth1.GetBlockByNumber(BlockParameter.Earliest, false);
        ///   </code>
        /// </example>
        public async Task<Block> GetBlockByNumber(BigInteger blockNumber, bool shouldIncludeTransactions = true)
        {
            string jsonResponse = await _in3.SendRpc(EthGetBlockByNumber,
                new object[] { BlockParameter.AsString(blockNumber), shouldIncludeTransactions });
            if (shouldIncludeTransactions)
            {
                return RpcHandler.From<TransactionBlock>(jsonResponse);
            }
            else
            {
                return RpcHandler.From<TransactionHashBlock>(jsonResponse);
            }
        }

        /// <summary>
        /// Blocks can be identified by root hash of the block merkle tree (this), or sequential number in which it was mined <see cref="Eth1.Api.GetBlockByNumber" />.
        /// </summary>
        /// <param name="blockHash">Desired block hash.</param>
        /// <param name="shouldIncludeTransactions">If true, returns the full transaction objects, otherwise only its hashes. The default value is <see langword="false"/>.</param>
        /// <returns>The <see cref="Block" /> of the requested <b>blockHash</b> (if exists).</returns>
        /// <remarks>
        /// <para>
        /// Returning <see cref="Block" /> must be cast to <see cref="TransactionBlock" /> or <see cref="TransactionHashBlock" /> to access the transaction data.
        /// </para>
        /// </remarks>
        public async Task<Block> GetBlockByHash(string blockHash, bool shouldIncludeTransactions = true)
        {
            string jsonResponse = await _in3.SendRpc(EthGetBlockByHash,
                new object[] { blockHash, shouldIncludeTransactions });
            if (shouldIncludeTransactions)
            {
                return RpcHandler.From<TransactionBlock>(jsonResponse);
            }
            else
            {
                return RpcHandler.From<TransactionHashBlock>(jsonResponse);
            }
        }

        /// <summary>
        /// ABI encoder. Used to serialize a rpc to the EVM.
        /// Based on the Solidity specification <see href="https://solidity.readthedocs.io/en/v0.5.3/abi-spec.html" />.
        /// Note: Parameters refers to the list of variables in a method declaration.
        /// Arguments are the actual values that are passed in when the method is invoked.
        /// When you invoke a method, the arguments used must match the declaration's parameters in type and order.
        /// </summary>
        /// <param name="signature">Function signature, with parameters. i.e. <i>>getBalance(uint256):uint256</i>, can contain the return types but will be ignored.</param>
        /// <param name="args">Function parameters, in the same order as in passed on to <b>signature</b>.</param>
        /// <returns>The encoded data.</returns>
        public async Task<string> AbiEncode(string signature, object[] args)
        {
            string jsonResponse = await _in3.SendRpc(In3AbiEncode, new object[] {
                signature, args});
            return RpcHandler.From<string>(jsonResponse);
        }

        /// <summary>
        /// ABI decoder. Used to parse rpc responses from the EVM.
        /// Based on the Solidity specification <see href="https://solidity.readthedocs.io/en/v0.5.3/abi-spec.html" />.
        /// </summary>
        /// <param name="signature">Function signature i.e. <i>>(address,string,uint256)</i> or <i>>getBalance(uint256):uint256</i>. In case of the latter, the function signature will be ignored and only the return types will be parsed.</param>
        /// <param name="encodedData">Abi encoded values. Usually the string returned from a rpc to the EVM.</param>
        /// <returns>The decoded argugments for the function call given the encded data.</returns>
        public async Task<string[]> AbiDecode(string signature, string encodedData)
        {
            string jsonResponse = await _in3.SendRpc(In3AbiDecode, new object[] {
                signature, encodedData});
            // This is ugly, unsemantic and error prone and SHOULD be changed.
            JsonElement result = (JsonElement)RpcHandler.From<object>(jsonResponse);

            if (result.ValueKind == JsonValueKind.String)
            {
                string singleResult = result.GetString();
                return new[] { singleResult };
            }

            IEnumerator<JsonElement> arr = result.EnumerateArray();
            string[] arrayResult = new string[result.GetArrayLength()];
            int i = 0;
            while (arr.MoveNext())
            {
                arrayResult[i] = arr.Current.GetString();
                i++;
            }

            return arrayResult;
        }

        /// <summary>
        /// The current gas price in Wei (1 ETH equals 1000000000000000000 Wei ).
        /// </summary>
        /// <returns>The gas price.</returns>
        public async Task<long> GetGasPrice()
        {
            string jsonResponse = await _in3.SendRpc(EthGasPrice, new object[] { });
            return (long)DataTypeConverter.HexStringToBigint(RpcHandler.From<string>(jsonResponse));
        }

        /// <summary>
        /// Get the <see cref="Chain" /> which the client is currently connected to.
        /// </summary>
        /// <returns>The <see cref="Chain" />.</returns>
        public async Task<Chain> GetChainId()
        {
            string jsonResponse = await _in3.SendRpc(EthChainId, new object[] { });
            return (Chain)(long)DataTypeConverter.HexStringToBigint(RpcHandler.From<string>(jsonResponse));
        }

        /// <summary>
        /// Returns the balance of the account of given <paramref name="address" />.
        /// </summary>
        /// <param name="address">Address to check for balance.</param>
        /// <param name="blockNumber">Block number or <see cref="BlockParameter.Latest" /> or <see cref="BlockParameter.Earliest" />.</param>
        /// <returns>The current balance in wei.</returns>
        public async Task<BigInteger> GetBalance(string address, BigInteger blockNumber)
        {
            string jsonResponse = await _in3.SendRpc(EthGetBalance, new object[] { address, BlockParameter.AsString(blockNumber) });
            return DataTypeConverter.HexStringToBigint(RpcHandler.From<string>(jsonResponse));
        }

        /// <summary>
        /// Smart-Contract bytecode in hexadecimal. If the account is a simple wallet the function will return '0x'.
        /// </summary>
        /// <param name="address">Ethereum address.</param>
        /// <param name="blockNumber">Block number or <see cref="BlockParameter.Latest" /> or <see cref="BlockParameter.Earliest" />.</param>
        /// <returns>Smart-Contract bytecode in hexadecimal.</returns>
        public async Task<string> GetCode(string address, BigInteger blockNumber)
        {
            string jsonResponse = await _in3.SendRpc(EthGetCode, new object[] { address, BlockParameter.AsString(blockNumber) });
            return RpcHandler.From<string>(jsonResponse);
        }

        /// <summary>
        /// Stored value in designed position at a given <paramref name="address" />. Storage can be used to store a smart contract state, constructor or just any data.
        /// Each contract consists of a EVM bytecode handling the execution and a storage to save the state of the contract.
        /// </summary>
        /// <param name="address">Ethereum account address.</param>
        /// <param name="position">Position index, 0x0 up to 100.</param>
        /// <param name="blockNumber">Block number or <see cref="BlockParameter.Latest" /> or <see cref="BlockParameter.Earliest" />.</param>
        /// <returns>Stored value in designed position.</returns>
        public async Task<string> GetStorageAt(string address, BigInteger position, BigInteger blockNumber)
        {
            string jsonResponse = await _in3.SendRpc(EthGetStorageAt, new object[] {
                address, DataTypeConverter.BigIntToPrefixedHex(position), BlockParameter.AsString(blockNumber)
            });
            return RpcHandler.From<string>(jsonResponse);
        }

        /// <summary>
        /// The total transactions on a block. See also <see cref="Eth1.Api.GetBlockTransactionCountByNumber" />.
        /// </summary>
        /// <param name="blockHash">Desired block hash.</param>
        /// <returns>The number (count) of <see cref="Transaction" />.</returns>
        public async Task<long> GetBlockTransactionCountByHash(string blockHash)
        {
            string jsonResponse = await _in3.SendRpc(EthGetBlockTransactionCountByHash, new object[] { blockHash });
            return Convert.ToInt64(RpcHandler.From<string>(jsonResponse), 16);
        }

        /// <summary>
        /// The total transactions on a block. See also <see cref="Eth1.Api.GetBlockTransactionCountByHash" />.
        /// </summary>
        /// <param name="blockNumber">Block number or <see cref="BlockParameter.Latest" /> or <see cref="BlockParameter.Earliest" />.</param>
        /// <returns>The number (count) of <see cref="Transaction" />.</returns>
        public async Task<long> GetBlockTransactionCountByNumber(BigInteger blockNumber)
        {
            string jsonResponse = await _in3.SendRpc(EthGetBlockTransactionCountByNumber, new object[] { BlockParameter.AsString(blockNumber) });
            return Convert.ToInt64(RpcHandler.From<string>(jsonResponse), 16);
        }

        /// <summary>
        /// Transactions can be identified by root hash of the transaction merkle tree (this) or by its position in the block transactions merkle tree.
        /// Every transaction hash is unique for the whole chain. Collision could in theory happen, chances are 67148E-63%.
        /// See also <see cref="Eth1.Api.GetTransactionByBlockNumberAndIndex" />.
        /// </summary>
        /// <param name="blockHash">Desired block hash.</param>
        /// <param name="index">The index of the <see cref="Transaction" /> in a <see cref="Block" /></param>
        /// <returns>The <see cref="Transaction" /> (if it exists).</returns>
        public async Task<Transaction> GetTransactionByBlockHashAndIndex(String blockHash, int index)
        {
            string jsonResponse = await _in3.SendRpc(EthGetTransactionByBlockHashAndIndex,
                new object[] { blockHash, BlockParameter.AsString(index) });
            return RpcHandler.From<Transaction>(jsonResponse);
        }

        /// <summary>
        /// Transactions can be identified by root hash of the transaction merkle tree (this) or by its position in the block transactions merkle tree.
        /// Every transaction hash is unique for the whole chain. Collision could in theory happen, chances are 67148E-63%.
        /// </summary>
        /// <param name="blockNumber">Block number or <see cref="BlockParameter.Latest" /> or <see cref="BlockParameter.Earliest" />.</param>
        /// <param name="index">The index of the <see cref="Transaction" /> in a <see cref="Block" /></param>
        /// <returns>The <see cref="Transaction" /> (if it exists).</returns>
        public async Task<Transaction> GetTransactionByBlockNumberAndIndex(BigInteger blockNumber, int index)
        {
            string jsonResponse = await _in3.SendRpc(EthGetTransactionByBlockNumberAndIndex,
                new object[] { BlockParameter.AsString(blockNumber), BlockParameter.AsString(index) });
            return RpcHandler.From<Transaction>(jsonResponse);
        }

        /// <summary>
        /// Transactions can be identified by root hash of the transaction merkle tree (this) or by its position in the block transactions merkle tree.
        /// Every transaction hash is unique for the whole chain. Collision could in theory happen, chances are 67148E-63%.
        /// </summary>
        /// <param name="transactionHash">Desired transaction hash.</param>
        /// <returns>The <see cref="Transaction" /> (if it exists).</returns>
        public async Task<Transaction> GetTransactionByHash(string transactionHash)
        {
            string jsonResponse = await _in3.SendRpc(EthGetTransactionByHash, new object[] { transactionHash });
            return RpcHandler.From<Transaction>(jsonResponse);
        }

        /// <summary>
        /// Number of transactions mined from this <paramref name="address" />. Used to set transaction nonce.
        /// Nonce is a value that will make a transaction fail in case it is different from (transaction count + 1).
        /// It exists to mitigate replay attacks.
        /// </summary>
        /// <param name="address">Ethereum account address.</param>
        /// <param name="blockNumber">Block number or <see cref="BlockParameter.Latest" /> or <see cref="BlockParameter.Earliest" />.</param>
        /// <returns>Number of transactions mined from this address.</returns>
        public async Task<long> GetTransactionCount(string address, BigInteger blockNumber)
        {
            string jsonResponse = await _in3.SendRpc(EthGetTransactionCount, new object[] { address, BlockParameter.AsString(blockNumber) });
            return Convert.ToInt64(RpcHandler.From<string>(jsonResponse), 16);
        }

        /// <summary>
        /// Sends a signed and encoded transaction.
        /// </summary>
        /// <param name="transactionData">Signed keccak hash of the serialized transaction.</param>
        /// <returns>Transaction hash, used to get the receipt and check if the transaction was mined.</returns>
        /// <remarks>
        /// <para>Client will add the other required fields, gas and chaind id.</para>
        /// </remarks>
        public async Task<string> SendRawTransaction(string transactionData)
        {
            string jsonResponse = await _in3.SendRpc(EthSendRawTransaction, new object[] { transactionData });
            return RpcHandler.From<string>(jsonResponse);
        }

        /// <summary>
        /// Will convert an upper or lowercase Ethereum <paramref name="address" /> to a checksum address, that uses case to encode values.
        /// See [EIP55](https://github.com/ethereum/EIPs/blob/master/EIPS/eip-55.md).
        /// </summary>
        /// <param name="address">Ethereum address.</param>
        /// <param name="shouldUseChainId">If <see langword="true" />, the chain id is integrated as well. Default being <see langword="false" />.</param>
        /// <returns>EIP-55 compliant, mixed-case address.</returns>
        public async Task<string> ChecksumAddress(string address, bool? shouldUseChainId = null)
        {
            string jsonResponse = await _in3.SendRpc(EthChecksumAddress, new object[] { address, shouldUseChainId });
            return RpcHandler.From<string>(jsonResponse);
        }

        /// <summary>
        /// Retrieve the total of uncles of a block for the given <paramref name="blockHash" />. Uncle blocks are valid blocks and are mined in a genuine manner, but get rejected from the main blockchain.
        /// See <see cref="Eth1.Api.GetUncleCountByBlockNumber" />.
        /// </summary>
        /// <param name="blockHash">Desired block hash.</param>
        /// <returns>The number of uncles in a block.</returns>
        public async Task<long> GetUncleCountByBlockHash(string blockHash)
        {
            string jsonResponse = await _in3.SendRpc(EthGetUncleCountByBlockHash, new object[] { blockHash });
            return Convert.ToInt64(RpcHandler.From<string>(jsonResponse), 16);
        }

        /// <summary>
        /// Retrieve the total of uncles of a block for the given <paramref name="blockNumber" />. Uncle blocks are valid and are mined in a genuine manner, but get rejected from the main blockchain.
        /// See <see cref="Eth1.Api.GetUncleCountByBlockHash" />.
        /// </summary>
        /// <param name="blockNumber">Block number or <see cref="BlockParameter.Latest" /> or <see cref="BlockParameter.Earliest" />.</param>
        /// <returns>The number of uncles in a block.</returns>
        public async Task<long> GetUncleCountByBlockNumber(BigInteger blockNumber)
        {
            string jsonResponse = await _in3.SendRpc(EthGetUncleCountByBlockNumber, new object[] { BlockParameter.AsString(blockNumber) });
            return Convert.ToInt64(RpcHandler.From<string>(jsonResponse), 16);
        }

        /// <summary>
        /// Creates a filter in the node, to notify when a new block arrives. To check if the state has changed, call <see cref="Eth1.Api.GetFilterChangesFromLogs" />.
        /// Filters are event catchers running on the Ethereum Client. Incubed has a client-side implementation.
        /// An event will be stored in case it is within to and from blocks, or in the block of blockhash, contains a
        /// transaction to the designed address, and has a word listed on topics.
        /// </summary>
        /// <returns>The filter id.</returns>
        /// <remarks>
        /// <para>Use the returned filter id to perform other filter operations.</para>
        /// </remarks>
        public async Task<long> NewBlockFilter()
        {
            string jsonResponse = await _in3.SendRpc(EthNewBlockFilter, new object[] { });
            return (long)DataTypeConverter.HexStringToBigint(RpcHandler.From<string>(jsonResponse));
        }

        /// <summary>
        /// Uninstalls a previously created filter.
        /// </summary>
        /// <param name="filterId">The filter id returned by <see cref="Eth1.Api.NewBlockFilter" />.</param>
        /// <returns>The result of the operation, <see langword="true" /> on success or <see langword="false" /> on failure.</returns>
        public async Task<bool> UninstallFilter(long filterId)
        {
            string jsonResponse = await _in3.SendRpc(EthUninstallFilter, new object[] { DataTypeConverter.BigIntToPrefixedHex(filterId) });
            return RpcHandler.From<bool>(jsonResponse);
        }

        /// <summary>
        /// Calls a smart-contract method. Will be executed locally by Incubed's EVM or signed and sent over to save the state changes.
        /// Check https://ethereum.stackexchange.com/questions/3514/how-to-call-a-contract-method-using-the-eth-call-json-rpc-api for more.
        /// </summary>
        /// <param name="request">The transaction request to be processed.</param>
        /// <param name="blockNumber">Block number or <see cref="BlockParameter.Latest" /> or <see cref="BlockParameter.Earliest" />.</param>
        /// <returns>Ddecoded result. If only one return value is expected the Object will be returned, if not an array of objects will be the result.</returns>
        public async Task<object> Call(TransactionRequest request, BigInteger blockNumber)
        {
            string jsonResponse = await _in3.SendRpc(EthCall, new object[] { await MapTransactionToRpc(request), BlockParameter.AsString(blockNumber) });
            if (request.IsFunctionInvocation())
            {
                return await AbiDecode(request.Function, RpcHandler.From<string>(jsonResponse));
            }

            return RpcHandler.From<object>(jsonResponse);
        }

        /// <summary>
        /// Gas estimation for transaction. Used to fill transaction.gas field. Check RawTransaction docs for more on gas.
        /// </summary>
        /// <param name="request">The transaction request whose cost will be estimated.</param>
        /// <param name="blockNumber">Block number or <see cref="BlockParameter.Latest" /> or <see cref="BlockParameter.Earliest" />.</param>
        /// <returns>Estimated gas in Wei.</returns>
        public async Task<long> EstimateGas(TransactionRequest request, BigInteger blockNumber)
        {
            string jsonResponse = await _in3.SendRpc(EthEstimateGas, new object[] { await MapTransactionToRpc(request), BlockParameter.AsString(blockNumber) });
            return (long)DataTypeConverter.HexStringToBigint(RpcHandler.From<string>(jsonResponse));
        }

        /// <summary>
        /// Creates a filter object, based on filter options, to notify when the state changes (logs). To check if the state has changed, call <see cref="Eth1.Api.GetFilterChangesFromLogs" />.
        /// Filters are event catchers running on the Ethereum Client. Incubed has a client-side implementation.
        /// An event will be stored in case it is within to and from blocks, or in the block of blockhash, contains a
        /// transaction to the designed address, and has a word listed on topics.
        /// </summary>
        /// <param name="filter">Model that holds the data for the filter creation.</param>
        /// <returns>The filter id.</returns>
        /// <remarks>
        /// <para>Use the returned filter id to perform other filter operations.</para>
        /// </remarks>
        public async Task<long> NewLogFilter(LogFilter filter)
        {
            string jsonResponse = await _in3.SendRpc(EthNewFilter, new object[] { filter.ToRPc() });
            return Convert.ToInt64(RpcHandler.From<string>(jsonResponse), 16);
        }

        /// <summary>
        /// Retrieve the logs for a certain filter. Logs marks changes of state on the chan for events. Equivalent to <see cref="Eth1.Api.GetFilterLogs" />.
        /// </summary>
        /// <param name="filterId">Id returned during the filter creation.</param>
        /// <returns>Array of logs which occurred since last poll.</returns>
        /// <remarks>
        /// <para>Since the return is the <see lang="Log[]" /> since last poll, executing this multiple times changes the state making this a "non-idempotent" getter.</para>
        /// </remarks>
        public async Task<Log[]> GetFilterChangesFromLogs(long filterId)
        {
            string jsonResponse = await _in3.SendRpc(EthGetFilterChanges, new object[] { DataTypeConverter.BigIntToPrefixedHex((BigInteger)filterId) });
            return RpcHandler.From<Log[]>(jsonResponse);
        }

        /// <summary>
        /// Retrieve the logs for a certain filter. Logs marks changes of state on the blockchain for events. Equivalent to <see cref="Eth1.Api.GetFilterChangesFromLogs" />.
        /// </summary>
        /// <param name="filterId">Id returned during the filter creation.</param>
        /// <returns>Array of logs which occurred since last poll.</returns>
        /// <remarks>
        /// <para>Since the return is the <see langword="Log[]" /> since last poll, executing this multiple times changes the state making this a "non-idempotent" getter.</para>
        /// </remarks>
        public async Task<Log[]> GetFilterLogs(long filterId)
        {
            string jsonResponse = await _in3.SendRpc(EthGetFilterLogs, new object[] { DataTypeConverter.BigIntToPrefixedHex((BigInteger)filterId) });
            return RpcHandler.From<Log[]>(jsonResponse);
        }

        /// <summary>
        /// Retrieve the logs for a certain filter. Logs marks changes of state on the blockchain for events. Unlike <see cref="Eth1.Api.GetFilterChangesFromLogs" /> or <see cref="Eth1.Api.GetFilterLogs" /> this is made to be used in a non-incremental manner (aka no poll) and will return the Logs that satisfy the filter condition.
        /// </summary>
        /// <param name="filter">Filter conditions.</param>
        /// <returns>Logs that satisfy the <paramref name="filter" />.</returns>
        public async Task<Log[]> GetLogs(LogFilter filter)
        {
            string jsonResponse = await _in3.SendRpc(EthGetLogs, new object[] { filter.ToRPc() });
            return RpcHandler.From<Log[]>(jsonResponse);
        }

        /// <summary>
        /// After a transaction is received the by the client, it returns the transaction hash. With it, it is possible to gather the receipt, once a miner has mined and it is part of an acknowledged block. Because how it is possible, in distributed systems, that data is asymmetric in different parts of the system, the transaction is only "final" once a certain number of blocks was mined after it, and still it can be possible that the transaction is discarded after some time. But, in general terms, it is accepted that after 6 to 8 blocks from latest, that it is very likely that the transaction will stay in the chain.
        /// </summary>
        /// <param name="transactionHash">Desired transaction hash.</param>
        /// <returns>The mined transaction data including event logs.</returns>
        public async Task<TransactionReceipt> GetTransactionReceipt(string transactionHash)
        {
            string jsonResponse = await _in3.SendRpc(EthGetTransactionReceipt, new object[] { transactionHash });
            return RpcHandler.From<TransactionReceipt>(jsonResponse);
        }

        /// <summary>
        /// Signs and sends the assigned transaction. The <see cref="Signer" /> used to sign the transaction is the one set by <see cref="IN3.Signer" />.
        /// Transactions change the state of an account, just the balance, or additionally, the storage and the code.
        /// Every transaction has a cost, gas, paid in Wei. The transaction gas is calculated over estimated gas times the gas cost, plus an additional miner fee, if the sender wants to be sure that the transaction will be mined in the latest block.
        /// </summary>
        /// <param name="tx">All information needed to perform a transaction.</param>
        /// <returns>Transaction hash, used to get the receipt and check if the transaction was mined.</returns>
        /// <example>
        ///   <code>
        ///    SimpleWallet wallet = (SimpleWallet) client.Signer;
        ///    TransactionRequest tx = new TransactionRequest();
        ///    tx.From = wallet.AddRawKey(pk);;
        ///    tx.To = "0x3940256B93c4BE0B1d5931A6A036608c25706B0c";
        ///    tx.Gas = 21000;
        ///    tx.Value = 100000000;
        ///    client.Eth1.SendTransaction(tx);
        ///   </code>
        /// </example>
        public async Task<string> SendTransaction(TransactionRequest tx)
        {
            if (_in3.Signer == null)
                throw new InvalidSignerException("No Signer set. This is needed in order to sign transaction.");
            if (tx.From == null)
                throw new InvalidTransactionRequestException("No from address set");
            if (!_in3.Signer.CanSign(tx.From))
                throw new InvalidSignerException("The from address is not supported by the signer");
            tx = _in3.Signer.PrepareTransaction(tx);

            string jsonResponse = await _in3.SendRpc(EthSendTransaction, new object[] { await MapTransactionToRpc(tx) });
            return RpcHandler.From<string>(jsonResponse);
        }

        /// <summary>
        /// Retrieve the of uncle of a block for the given <paramref name="blockNumber" /> and a position. Uncle blocks are valid blocks and are mined in a genuine manner, but get rejected from the main blockchain.
        /// </summary>
        /// <param name="blockNumber">Block number or <see cref="BlockParameter.Latest" /> or <see cref="BlockParameter.Earliest" />.</param>
        /// <param name="position">Position of the block.</param>
        /// <returns>The uncle block.</returns>
        public async Task<Block> GetUncleByBlockNumberAndIndex(BigInteger blockNumber, int position)
        {
            string jsonResponse = await _in3.SendRpc(EthGetUncleByBlockNumberAndIndex,
                new object[] { BlockParameter.AsString(blockNumber), DataTypeConverter.BigIntToPrefixedHex(position) });
            return RpcHandler.From<TransactionBlock>(jsonResponse);
        }

        /// <summary>
        /// Resolves ENS domain name.
        /// </summary>
        /// <param name="name">ENS domain name.</param>
        /// <param name="type"> One of <see cref="ENSParameter" />.</param>
        /// <returns>The resolved entity for the domain.</returns>
        /// <remarks>
        /// The actual semantics of the returning value changes according to <paramref name="type" />.
        /// </remarks>
        public async Task<string> Ens(string name, ENSParameter type = null)
        {
            string jsonResponse = await _in3.SendRpc(EthENS, new object[] { name, type?.Value });
            return RpcHandler.From<string>(jsonResponse);
        }

        // For sure this should not be here at all. As soon as there is a decoupled way to abiEncode, got to extract this to a wrapper (e.g.: adapter or a factory method on TransactionRpc itself.
        private async Task<Rpc.Transaction> MapTransactionToRpc(TransactionRequest tx)
        {
            Rpc.Transaction result = new Rpc.Transaction();

            result.Data = tx.Data == null || tx.Data.Length < 2 ? "0x" : tx.Data;
            if (!String.IsNullOrEmpty(tx.Function))
            {
                string fnData = await AbiEncode(tx.Function, tx.Params);
                if (fnData != null && fnData.Length > 2 && fnData.StartsWith("0x"))
                    result.Data += fnData.Substring(2 + (result.Data.Length > 2 ? 8 : 0));
            }

            result.To = tx.To;
            result.From = tx.From;
            if (tx.Value.HasValue)
            {
                result.Value = DataTypeConverter.BigIntToPrefixedHex(tx.Value.Value);
            }

            if (tx.Nonce.HasValue)
            {
                result.Nonce = DataTypeConverter.BigIntToPrefixedHex(tx.Nonce.Value);
            }

            if (tx.Gas.HasValue)
            {
                result.Gas = DataTypeConverter.BigIntToPrefixedHex(tx.Gas.Value);
            }

            if (tx.GasPrice.HasValue)
            {
                result.GasPrice = DataTypeConverter.BigIntToPrefixedHex(tx.GasPrice.Value);
            }

            if (result.Data == null || result.Data.Length < 2)
            {
                result.Data = "0x";
            }
            else
            {
                result.Data = DataTypeConverter.AddHexPrefixer(result.Data);
            }

            return result;
        }
    }
}