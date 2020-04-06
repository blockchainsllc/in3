import enum

"""
libin3 definitions
"""


@enum.unique
class SimpleEnum(enum.Enum):

    def __str__(self):
        return self.value


class EthMethods(SimpleEnum):
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
    SIGN = "eth_sign"
    NEW_FILTER = "eth_newFilter"
    NEW_BLOCK_FILTER = "eth_newBlockFilter"
    NEW_PENDING_TRANSACTION_FILTER = "eth_newPendingTransactionFilter"
    UNINSTALL_FILTER = "eth_uninstallFilter"
    FILTER_CHANGES = "eth_getFilterChanges"
    FILTER_LOGS = "eth_getFilterLogs"
    LOGS = "eth_getLogs"


class In3Methods(SimpleEnum):
    CHECKSUM_ADDRESS = "in3_checksumAddress"
    SEND = "send"
    SIGN = "sign"
    CALL = "call"
    IN3_NODE_LIST = "in3_nodeList"
    IN3_SIGN = "in3_sign"
    IN3_STATS = "in3_stats"
    ABI_ENCODE = "in3_abiEncode"
    ABI_DECODE = "in3_abiDecode"
    PK_2_ADDRESS = "pk2address"
    PK_2_PUBLIC = "pk2public"
    ECRECOVER = "ecrecover"
    CREATE_KEY = "createkey"
    KEY = "KEY"
    CONFIG = "in3_config"


class BlockStatus(SimpleEnum):
    EARLIEST = "earliest"
    LATEST = "latest"
    PENDING = "pending"


class In3ProofLevel(SimpleEnum):
    NONE = "none"
    STANDARD = "standard"
    FULL = "full"


class Chain(SimpleEnum):
    """
    Default chain values and consensus finality threshold
    """
    MAINNET = dict(
        chain_id="0x1",
        alias="mainnet",
        finality=10)

    KOVAN = dict(
        chain_id="0x2a",
        alias="kovan",
        finality=80)

    EVAN = dict(
        chain_id="0x4b1",
        alias="evan",
        finality=80)

    GOERLI = dict(
        chain_id="0x5",
        finality=80,
        alias="goerli")

    IPFS = dict(
        chain_id="0x7d0",
        alias="ipfs",
        finality=0)

    def __str__(self):
        return self.value['alias']

    def __hex__(self):
        return self.value['chain_id']

    def __int__(self):
        return self.value['finality']
