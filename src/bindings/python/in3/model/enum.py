import enum


@enum.unique
class EthCall(enum.Enum):
    WEB3_SHA3 = "web3_sha3"
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

    def __str__(self):
        return self.value


@enum.unique
class In3Methods(enum.Enum):
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

    def __str__(self):
        return self.value


@enum.unique
class BlockStatus(enum.Enum):
    EARLIEST = "earliest"
    LATEST = "latest"
    PENDING = "pending"

    def __str__(self):
        return self.value


@enum.unique
class In3ProofLevel(enum.Enum):
    NONE = "none"
    STANDARD = "standard"
    FULL = "full"

    def __str__(self):
        return self.value


@enum.unique
class Chain(enum.Enum):
    MAINNET = dict(
        registry="0x2736D225f85740f42D17987100dc8d58e9e16252",
        chain_id="0x1",
        alias="mainnet",
        status="https://libs.slock.it?n=mainnet",
        node_list="https://libs.slock.it/mainnet/nd-3")

    KOVAN = dict(
        registry="0x27a37a1210df14f7e058393d026e2fb53b7cf8c1",
        chain_id="0x2a",
        alias="kovan",
        status="https://libs.slock.it?n=kovan",
        node_list="https://libs.slock.it/kovan/nd-3")

    EVAN = dict(
        registry="0x85613723dB1Bc29f332A37EeF10b61F8a4225c7e",
        chain_id="0x4b1",
        alias="evan",
        status="https://libs.slock.it?n=evan",
        node_list="https://libs.slock.it/evan/nd-3")

    GOERLI = dict(
        registry="0x85613723dB1Bc29f332A37EeF10b61F8a4225c7e",
        chain_id="0x5",
        alias="goerli",
        status="https://libs.slock.it?n=goerli",
        node_list="https://libs.slock.it/goerli/nd-3")

    IPFS = dict(
        registry="0xf0fb87f4757c77ea3416afe87f36acaa0496c7e9",
        chain_id="0x7d0",
        alias="ipfs",
        status="https://libs.slock.it?n=ipfs",
        node_list="https://libs.slock.it/ipfs/nd-3")

    def __str__(self):
        return self.value['chain_id']
