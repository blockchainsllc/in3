"""
Maps libin3 names, types and other definitions
"""
import enum


@enum.unique
class SimpleEnum(enum.Enum):
    """
    Abstract stringify enum class
    """

    def __str__(self):
        return self.value


class EthMethods(SimpleEnum):
    """
    Ethereum API methods
    """
    KECCAK = "web3_sha3"
    ACCOUNTS = "eth_accounts"
    BALANCE = "eth_getBalance"
    STORAGE_AT = "eth_getStorageAt"
    GAS_PRICE = "eth_gasPrice"
    BLOCK_NUMBER = "eth_blockNumber"
    BLOCK_BY_NUMBER = "eth_getBlockByNumber"
    BLOCK_BY_HASH = "eth_getBlockByHash"
    BLOCK_TRANSACTION_COUNT_BY_NUMBER = "eth_getBlockTransactionCountByNumber"
    BLOCK_TRANSACTION_COUNT_BY_HASH = "eth_getBlockTransactionCountByHash"
    TRANSACTION_BY_HASH = "eth_getTransactionByHash"
    TRANSACTION_BY_BLOCKHASH_AND_INDEX = "eth_getTransactionByBlockHashAndIndex"
    TRANSACTION_BY_BLOCKNUMBER_AND_INDEX = "eth_getTransactionByBlockNumberAndIndex"
    TRANSACTION_RECEIPT = "eth_getTransactionReceipt"
    PENDING_TRANSACTIONS = "eth_pendingTransactions"
    TRANSACTION_COUNT = "eth_getTransactionCount"
    SEND_TRANSACTION = "eth_sendTransaction"
    SEND_RAW_TRANSACTION = "eth_sendRawTransaction"
    CALL = "eth_call"
    ESTIMATE_TRANSACTION = "eth_estimateGas"
    UNCLE_COUNT_BY_BLOCK_HASH = "eth_getUncleCountByBlockHash"
    UNCLE_COUNT_BY_BLOCK_NUMBER = "eth_getUncleCountByBlockNumber"
    UNCLE_BY_BLOCKHASH_AND_INDEX = "eth_getUncleByBlockHashAndIndex"
    UNCLE_BY_BLOCKNUMBER_AND_INDEX = "eth_getUncleByBlockNumberAndIndex"
    CODE = "eth_getCode"
    # SIGN = "eth_sign"
    SIGN = "in3_signData"
    NEW_FILTER = "eth_newFilter"
    NEW_BLOCK_FILTER = "eth_newBlockFilter"
    NEW_PENDING_TRANSACTION_FILTER = "eth_newPendingTransactionFilter"
    UNINSTALL_FILTER = "eth_uninstallFilter"
    FILTER_CHANGES = "eth_getFilterChanges"
    FILTER_LOGS = "eth_getFilterLogs"
    LOGS = "eth_getLogs"


class In3Methods(SimpleEnum):
    """
    Incubed API methods
    """
    CONFIGURE = "in3_config"
    ECRECOVER = "in3_ecrecover"
    ENSRESOLVE = "in3_ens"
    SIGN = "in3_sign"
    NODE_STATS = "in3_stats"
    ABI_ENCODE = "in3_abiEncode"
    ABI_DECODE = "in3_abiDecode"
    GET_CONFIG = "in3_getConfig"
    PK_2_PUBLIC = "in3_pk2address"
    PK_2_ADDRESS = "in3_pk2address"
    NODE_LIST = "in3_nodeList"
    CHECKSUM_ADDRESS = "in3_checksumAddress"


class RPCCode(enum.Enum):
    """
    Codes returned from libin3 RPC.
    On success positive values (impl. defined) upto INT_MAX maybe returned
    """
    IN3_OK = 0  # Success
    IN3_EUNKNOWN = -1  # Unknown error - usually accompanied with specific error msg
    IN3_ENOMEM = -2  # No memory
    IN3_ENOTSUP = -3  # Not supported
    IN3_EINVAL = -4  # Invalid value
    IN3_EFIND = -5  # Not found
    IN3_ECONFIG = -6  # Invalid config
    IN3_ELIMIT = -7  # Limit reached
    IN3_EVERS = -8  # Version mismatch
    IN3_EINVALDT = -9  # Data invalid, eg. invalid/incomplete JSON
    IN3_EPASS = -10  # Wrong password
    IN3_ERPC = -11  # RPC error (i.e. in3_ctx_t::error set)
    IN3_ERPCNRES = -12  # RPC no response
    IN3_EUSNURL = -13  # USN URL parse error
    IN3_ETRANS = -14  # Transport error
    IN3_ERANGE = -15  # Not in range
    IN3_WAITING = -16  # the process can not be finished since we are waiting for responses
    IN3_EIGNORE = -17  # Ignorable error
    IN3_EPAYMENT_REQUIRED = -18  # payment required


class BlockAt(SimpleEnum):
    """
    Alias for Ethereum blocks
    """
    EARLIEST = "earliest"
    LATEST = "latest"
    PENDING = "pending"


class In3ProofLevel(SimpleEnum):
    """
    Alias for verification levels.
    Verification is done by calculating Ethereum Trie states requested by the Incubed network ans signed as proofs of a certain state.
    """
    NONE = "none"
    STANDARD = "standard"
    FULL = "full"
