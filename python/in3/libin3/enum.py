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
    IN3_ENODEVICE = -19  # harware wallet device not connected
    IN3_EAPDU = -20  # error in hardware wallet communication
    IN3_EPLGN_NONE = -21  # no plugin could handle specified action


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


class PluginAction(enum.Enum):
    PLGN_ACT_INIT = 0x1  # initialize plugin - use for allocating/setting-up internal resources
    PLGN_ACT_TERM = 0x2  # terminate plugin - use for releasing internal resources and cleanup.* /
    PLGN_ACT_TRANSPORT_SEND = 0x4  # sends out a request - the transport plugin will receive a request_t as plgn_ctx, it may set a cptr which will be passed back when fetching more responses.* /
    PLGN_ACT_TRANSPORT_RECEIVE = 0x8  # fetch next response - the transport plugin will receive a request_t as plgn_ctx, which contains a cptr if set previously
    PLGN_ACT_TRANSPORT_CLEAN = 0x10  # free-up transport resources - the transport plugin will receive a request_t as plgn_ctx if the cptr was set.* /
    PLGN_ACT_SIGN_ACCOUNT = 0x20  # returns the default account of the signer
    PLGN_ACT_SIGN_PREPARE = 0x40  # allows a wallet to manipulate the payload before signing - the plgn_ctx will be in3_sign_ctx_t.This way a tx can be send through a multisig
    PLGN_ACT_SIGN = 0x80  # signs the payload - the plgn_ctx will be in3_sign_ctx_t.* /
    PLGN_ACT_RPC_HANDLE = 0x100  # a plugin may respond to a rpc-request directly (without sending it to the node).* /
    PLGN_ACT_RPC_VERIFY = 0x200  # verifies the response.the plgn_ctx will be a in3_vctx_t holding all data
    PLGN_ACT_CACHE_SET = 0x400  # stores data to be reused later - the plgn_ctx will be a in3_cache_ctx_t containing the data
    PLGN_ACT_CACHE_GET = 0x800  # reads data to be previously stored - the plgn_ctx will be a in3_cache_ctx_t containing the key.if the data was found the data-property needs to be set.* /
    PLGN_ACT_CACHE_CLEAR = 0x1000  # clears all stored data - plgn_ctx will be NULL
    PLGN_ACT_CONFIG_SET = 0x2000  # gets a config-token and reads data from it
    PLGN_ACT_CONFIG_GET = 0x4000  # gets a string-builder and adds all config to it.* /
    PLGN_ACT_PAY_PREPARE = 0x8000  # prepares a payment
    PLGN_ACT_PAY_FOLLOWUP = 0x10000  # called after a request to update stats.* /
    PLGN_ACT_PAY_HANDLE = 0x20000  # handles the payment
    PLGN_ACT_PAY_SIGN_REQ = 0x40000  # signs a request
    PLGN_ACT_LOG_ERROR = 0x80000  # report an error
    PLGN_ACT_NL_PICK = 0x100000  # picks the data nodes, plgn_ctx will be a pointer to in3_ctx_t
    PLGN_ACT_NL_PICK_FOLLOWUP = 0x200000  # called after receiving a response in order to decide whether a update is needed, plgn_ctx will be a pointer to in3_ctx_t
    PLGN_ACT_NL_BLACKLIST = 0x400000  # blacklist a particular node in the nodelist, plgn_ctx will be a pointer to the node's address.
    PLGN_ACT_NL_FAILABLE = 0x800000  # handle fail-able request, plgn_ctx will be a pointer to in3_ctx_t
    PLGN_ACT_NL_OFFLINE = 0x1000000  # mark a particular node in the nodelist as offline, plgn_ctx will be a pointer to in3_nl_offline_ctx_t.* /
    PLGN_ACT_CHAIN_CHANGE = 0x2000000  # chain id change event, called after setting new chain id
    PLGN_ACT_GET_DATA = 0x4000000  # get access to plugin data as a void ptr
    PLGN_ACT_ADD_PAYLOAD = 0x8000000  # add plugin specific metadata to payload, plgn_ctx will be a sb_t pointer, make sure to begin with a comma
    # Handy Symbols
    PLGN_ACT_LIFECYCLE = (PLGN_ACT_INIT | PLGN_ACT_TERM)
    PLGN_ACT_TRANSPORT = (PLGN_ACT_TRANSPORT_SEND | PLGN_ACT_TRANSPORT_RECEIVE | PLGN_ACT_TRANSPORT_CLEAN)
    PLGN_ACT_NODELIST = (PLGN_ACT_NL_PICK | PLGN_ACT_NL_PICK_FOLLOWUP | PLGN_ACT_NL_BLACKLIST | PLGN_ACT_NL_FAILABLE |
                         PLGN_ACT_NL_OFFLINE)
    PLGN_ACT_CACHE = (PLGN_ACT_CACHE_SET | PLGN_ACT_CACHE_GET | PLGN_ACT_CACHE_CLEAR)
    PLGN_ACT_CONFIG = (PLGN_ACT_CONFIG_SET | PLGN_ACT_CONFIG_GET)
