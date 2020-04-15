using System;
using System.Globalization;
using System.Numerics;
using System.Text.Json;
using In3.Crypto;
using In3.Rpc;
using In3.Utils;

namespace In3.Eth1
{
    public class Api
    {
        private IN3 in3;

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

        public Api(IN3 in3)
        {
            this.in3 = in3;
        }

        public BigInteger BlockNumber()
        {
            string jsonResponse = in3.SendRpc(EthBlockNumber, new object[] { });
            return TypesMatcher.HexStringToBigint(RpcHandler.From<string>(jsonResponse));
        }

        public Block GetBlockByNumber(BigInteger blockNumber, bool shouldIncludeTransactions = true)
        {
            string jsonResponse = in3.SendRpc(EthGetBlockByNumber,
                new object[] { BlockParameter.AsString(blockNumber), true });
            if (shouldIncludeTransactions)
            {
                return RpcHandler.From<TransactionBlock>(jsonResponse);
            }
            else
            {
                return RpcHandler.From<TransactionHashBlock>(jsonResponse);
            }
        }

        public Block GetBlockByHash(string blockHash, bool shouldIncludeTransactions = true)
        {
            string jsonResponse = in3.SendRpc(EthGetBlockByHash,
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

        public string AbiEncode(string signature, object[] args)
        {
            string jsonResponse = in3.SendRpc(In3AbiEncode, new object[] {
                signature, args});
            return RpcHandler.From<string>(jsonResponse);
        }

        public string[] AbiDecode(string signature, string args)
        {
            string jsonResponse = in3.SendRpc(In3AbiDecode, new object[] {
                signature, args});
            return RpcHandler.From<string[]>(jsonResponse);
        }

        public BigInteger GetGasPrice()
        {
            string jsonResponse = in3.SendRpc(EthGasPrice, new object[] { });
            return TypesMatcher.HexStringToBigint(RpcHandler.From<string>(jsonResponse));
        }
        public Chain GetChainId()
        {
            string jsonResponse = in3.SendRpc(EthChainId, new object[] { });
            return (Chain)(long)TypesMatcher.HexStringToBigint(RpcHandler.From<string>(jsonResponse));
        }

        public BigInteger GetBalance(string address, BigInteger block)
        {
            string jsonResponse = in3.SendRpc(EthGetBalance, new object[] { address, BlockParameter.AsString(block) });
            return TypesMatcher.HexStringToBigint(RpcHandler.From<string>(jsonResponse));
        }

        public string GetCode(string address, BigInteger block)
        {
            string jsonResponse = in3.SendRpc(EthGetCode, new object[] { address, BlockParameter.AsString(block) });
            return RpcHandler.From<string>(jsonResponse);
        }

        public string GetStorageAt(string address, BigInteger position, BigInteger block)
        {
            string jsonResponse = in3.SendRpc(EthGetStorageAt, new object[] {
                address, TypesMatcher.BigIntToPrefixedHex(position), BlockParameter.AsString(block)
            });
            return RpcHandler.From<string>(jsonResponse);
        }

        public long GetBlockTransactionCountByHash(string blockHash)
        {
            string jsonResponse = in3.SendRpc(EthGetBlockTransactionCountByHash, new object[] { blockHash });
            return RpcHandler.From<long>(jsonResponse);
        }

        public long GetBlockTransactionCountByNumber(BigInteger block)
        {
            string jsonResponse = in3.SendRpc(EthGetBlockTransactionCountByNumber, new object[] { BlockParameter.AsString(block) });
            return RpcHandler.From<long>(jsonResponse);
        }

        public Transaction GetTransactionByBlockHashAndIndex(String blockHash, int index)
        {
            string jsonResponse = in3.SendRpc(EthGetTransactionByBlockHashAndIndex,
                new object[] { blockHash, BlockParameter.AsString(index) });
            return RpcHandler.From<Transaction>(jsonResponse);
        }

        public Transaction GetTransactionByBlockNumberAndIndex(BigInteger block, int index)
        {
            string jsonResponse = in3.SendRpc(EthGetTransactionByBlockNumberAndIndex,
                new object[] { BlockParameter.AsString(block), BlockParameter.AsString(index) });
            return RpcHandler.From<Transaction>(jsonResponse);
        }

        public Transaction GetTransactionByHash(string transactionHash)
        {
            string jsonResponse = in3.SendRpc(EthGetTransactionByHash, new object[] { transactionHash });
            return RpcHandler.From<Transaction>(jsonResponse);
        }

        public long GetTransactionCount(string address, BigInteger block)
        {
            string jsonResponse = in3.SendRpc(EthGetTransactionCount, new object[] { address, BlockParameter.AsString(block) });
            return (long)TypesMatcher.HexStringToBigint(RpcHandler.From<string>(jsonResponse));
        }

        public string SendRawTransaction(string data)
        {
            string jsonResponse = in3.SendRpc(EthSendRawTransaction, new object[] { data });
            return RpcHandler.From<string>(jsonResponse);
        }

        public string ChecksumAddress(string address, bool? shouldUseChainId = null)
        {
            string jsonResponse = in3.SendRpc(EthChecksumAddress, new object[] { address, shouldUseChainId });
            return RpcHandler.From<string>(jsonResponse);
        }

        public long GetUncleCountByBlockHash(string blockHash)
        {
            string jsonResponse = in3.SendRpc(EthGetUncleCountByBlockHash, new object[] { blockHash });
            return RpcHandler.From<long>(jsonResponse);
        }

        public long GetUncleCountByBlockNumber(BigInteger block)
        {
            string jsonResponse = in3.SendRpc(EthGetUncleCountByBlockNumber, new object[] { BlockParameter.AsString(block) });
            return RpcHandler.From<long>(jsonResponse);
        }

        public long NewBlockFilter()
        {
            string jsonResponse = in3.SendRpc(EthNewBlockFilter, new object[] { });
            return (long)TypesMatcher.HexStringToBigint(RpcHandler.From<string>(jsonResponse));
        }

        public bool UninstallFilter(long filter)
        {
            string jsonResponse = in3.SendRpc(EthUninstallFilter, new object[] { TypesMatcher.BigIntToPrefixedHex(filter) });
            return RpcHandler.From<bool>(jsonResponse);
        }

        public object Call(TransactionRequest request, BigInteger block)
        {
            string jsonResponse = in3.SendRpc(EthCall, new object[] { MapTransactionToRpc(request), BlockParameter.AsString(block) });
            return RpcHandler.From<object>(jsonResponse);
        }

        public long EstimateGas(TransactionRequest request, BigInteger block)
        {
            string jsonResponse = in3.SendRpc(EthEstimateGas, new object[] { MapTransactionToRpc(request), BlockParameter.AsString(block) });
            return (long)TypesMatcher.HexStringToBigint(RpcHandler.From<string>(jsonResponse));
        }

        public long NewLogFilter(LogFilter filter)
        {
            string jsonResponse = in3.SendRpc(EthNewFilter, new object[] { filter.ToRPc() });
            return (long)TypesMatcher.HexStringToBigint(RpcHandler.From<string>(jsonResponse));
        }

        public Log[] GetFilterChangesFromLogs(long id)
        {
            string jsonResponse = in3.SendRpc(EthGetFilterChanges, new object[] { TypesMatcher.BigIntToPrefixedHex((BigInteger)id) });
            return RpcHandler.From<Log[]>(jsonResponse);
        }

        public Log[] GetFilterLogs(long id)
        {
            string jsonResponse = in3.SendRpc(EthGetFilterLogs, new object[] { TypesMatcher.BigIntToPrefixedHex((BigInteger)id) });
            return RpcHandler.From<Log[]>(jsonResponse);
        }

        public Log[] GetLogs(LogFilter filter)
        {
            string jsonResponse = in3.SendRpc(EthGetLogs, new object[] { filter.ToRPc() });
            return RpcHandler.From<Log[]>(jsonResponse);
        }

        public TransactionReceipt GetTransactionReceipt(string transactionHash)
        {
            string jsonResponse = in3.SendRpc(EthGetTransactionReceipt, new object[] { transactionHash });
            return RpcHandler.From<TransactionReceipt>(jsonResponse);
        }

        public TransactionReceipt SendTransaction(TransactionRequest tx)
        {
            if (in3.Signer == null)
                throw new SystemException("No Signer set. This is needed in order to sign transaction.");
            if (tx.From == null)
                throw new SystemException("No from address set");
            if (!in3.Signer.CanSign(tx.From))
                throw new SystemException("The from address is not supported by the signer");
            tx = in3.Signer.PrepareTransaction(tx);

            JsonSerializerOptions options = new JsonSerializerOptions
            {
                IgnoreNullValues = true
            };

            string requestJson = JsonSerializer.Serialize(MapTransactionToRpc(tx), options);
            string jsonResponse = in3.SendRpc(EthSendTransaction, new object[] { requestJson });
            return RpcHandler.From<TransactionReceipt>(jsonResponse);
        }

        public Block GetUncleByBlockNumberAndIndex(BigInteger block, int pos)
        {
            string jsonResponse = in3.SendRpc(EthGetUncleByBlockNumberAndIndex,
                new object[] { BlockParameter.AsString(block), TypesMatcher.BigIntToPrefixedHex(pos) });
            return RpcHandler.From<Block>(jsonResponse);
        }

        public string ENS(string name, ENSParameter? type = null)
        {
            string jsonResponse = in3.SendRpc(EthENS, new object[] { name, type });
            return RpcHandler.From<string>(jsonResponse);
        }

        // For sure this should not be here at all. As soon as there is a decoupled way to abiEncode, got to extract this to a wrapper (e.g.: adapter or a factory method on TransactionRpc itself.
        private Rpc.Transaction MapTransactionToRpc(TransactionRequest tx)
        {
            Rpc.Transaction result = new Rpc.Transaction();

            result.Data = tx.Data == null || tx.Data.Length < 2 ? "0x" : tx.Data;
            if (!String.IsNullOrEmpty(tx.Function))
            {
                string fnData = AbiEncode(tx.Function, tx.Params);
                if (fnData != null && fnData.Length > 2 && fnData.StartsWith("0x"))
                    result.Data += fnData.Substring(2 + (result.Data.Length > 2 ? 8 : 0));
            }

            result.To = tx.To;
            result.From = tx.From;
            if (tx.Value.HasValue)
            {
                result.Value = TypesMatcher.BigIntToPrefixedHex(tx.Value.Value);
            }

            if (tx.Nonce.HasValue)
            {
                result.Nonce = TypesMatcher.BigIntToPrefixedHex(tx.Nonce.Value);
            }

            if (tx.Gas.HasValue)
            {
                result.Gas = TypesMatcher.BigIntToPrefixedHex(tx.Gas.Value);
            }

            if (tx.GasPrice.HasValue)
            {
                result.GasPrice = TypesMatcher.BigIntToPrefixedHex(tx.GasPrice.Value);
            }

            if (result.Data == null || result.Data.Length < 2)
            {
                result.Data = "0x";
            }
            else
            {
                result.Data = TypesMatcher.AddHexPrefixer(result.Data);
            }

            return result;
        }
    }
}