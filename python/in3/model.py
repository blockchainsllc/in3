from in3.eth.model import DataTransferObject, Account
from in3.libin3.enum import In3ProofLevel, Chain


class In3Node(DataTransferObject):
    """
    Registered remote verifier that attest, by signing the block hash, that the requested block and transaction were
    indeed mined are in the correct chain fork.
    Args:
        url (str): Endpoint to post to example: https://in3.slock.it
        index (int): Index within the contract example: 13
        address (in3.Account): Address of the node, which is the public address it iis signing with. example: 0x6C1a01C2aB554930A937B0a2E8105fB47946c679
        deposit (int): Deposit of the node in wei example: 12350000
        props (int): Properties of the node. example: 3
        timeout (int): Time (in seconds) until an owner is able to receive his deposit back after he unregisters himself example: 3600
        registerTime (int): When the node was registered in (unixtime?)
        weight (int): Score based on qualitative metadata to base which nodes to ask signatures from.
    """

    def __init__(self, url: str, address: Account, index: int, deposit: int, props: int, timeout: int,
                 registerTime: int, weight: int):
        self.url = url
        self.index = index
        self.address = address
        self.deposit = deposit
        self.props = props
        self.timeout = timeout
        self.registerTime = registerTime
        self.weight = weight


class NodeList(DataTransferObject):
    """
    List of incubed nodes and its metadata, in3 registry contract from which the list was taken,
    network/registry id, and last block number in the selected chain.
    Args:
        nodes ([In3Node]): list of incubed nodes
        contract (Account): incubed registry contract from which the list was taken
        registryId (str): uuid of this incubed network. one chain could contain more than one incubed networks.
        lastBlockNumber (int): last block signed by the network
        totalServers (int): Total servers number (for integrity?)
    """

    def __init__(self, nodes: [In3Node], contract: Account, registryId: str, lastBlockNumber: int, totalServers: int):
        self.nodes = nodes
        self.contract = contract
        self.registryId = registryId
        self.lastBlockNumber = lastBlockNumber
        self.totalServers = totalServers


class ClientConfig(DataTransferObject):
    """
    In3 Client Configuration class.
    Determines the behavior of client, which chain to connect to, verification policy, update cycle, minimum number of
    signatures collected on every request, and response timeout.
    Those are the settings that determine information security levels. Considering integrity is guaranteed by and
    confidentiality is not available on public blockchains, these settings will provide a balance between availability,
    and financial stake in case of repudiation. The "newer" the block is, or the closest to "latest", the higher are
    the chances it gets repudiated (a fork) by the chain, making lower the chances a node will sign on such information
    and thus reducing its availability. Up to a certain point, the older the block gets, the highest is its
    availability because of the close-to-zero repudiation risk. Blocks older than circa one year are stored in Archive
    Nodes, expensive computers, so, despite of the zero repudiation risk, there are not many nodes and they must search
    for the requested block in its database, lowering the availability as well. The verification policy enforces an
    extra step of security, that proves important in case you have only one response from an archive node
    and want to run a local integrity check, just to be on the safe side.

    Args:
        chainId (str): (optional) - servers to filter for the given chain. The chain-id based on EIP-155. example: 0x1
        replaceLatestBlock (int): (optional) - if specified, the blocknumber latest will be replaced by blockNumber- specified value example: 6
        signatureCount (int): (optional) - number of signatures requested example: 2
        finality (int): (optional) - the number in percent needed in order reach finality (% of signature of the validators) example: 50
        minDeposit (int): - min stake of the server. Only nodes owning at least this amount will be chosen.
        proof :'none'|'standard'|'full' (optional) - if true the nodes should send a proof of the response
        autoUpdateList (bool): (optional) - if true the nodelist will be automatically updated if the lastBlock is newer
        timeout (int): specifies the number of milliseconds before the request times out. increasing may be helpful if the device uses a slow connection. example: 100000
        key (str): (optional) - the client key to sign requests example: 0x387a8233c96e1fc0ad5e284353276177af2186e7afa85296f106336e376669f7
        includeCode (bool): (optional) - if true, the request should include the codes of all accounts. otherwise only the the codeHash is returned. In this case the client may ask by calling eth_getCode() afterwards
        maxAttempts (int): (optional) - max number of attempts in case a response is rejected example: 10
        keepIn3 (bool): (optional) - if true, the in3-section of thr response will be kept. Otherwise it will be removed after validating the data. This is useful for debugging or if the proof should be used afterwards.
        maxBlockCache (int): (optional) - number of number of blocks cached in memory example: 100
        maxCodeCache (int): (optional) - number of max bytes used to cache the code in memory example: 100000
        nodeLimit (int): (optional) - the limit of nodes to store in the client. example: 150
        requestCount (int): - Useful to be higher than 1 when using signatureCount <= 1. Then the client check for consensus in answers.
    """

    def __init__(self, chainId: str = str(Chain.MAINNET), key: str = None, replaceLatestBlock: int = 8,
                 signatureCount: int = 3, finality: int = 70, minDeposit: int = 10000000000000000,
                 proof: In3ProofLevel = In3ProofLevel.STANDARD, autoUpdateList: bool = True, timeout: int = 5000,
                 includeCode: bool = False, keepIn3: bool = False, maxAttempts: int = None, maxBlockCache: int = None,
                 maxCodeCache: int = None, nodeLimit: int = None, requestCount: int = 1):
        self.autoUpdateList: bool = autoUpdateList
        self.chainId: str = chainId
        self.finality: int = finality
        self.includeCode: bool = includeCode
        self.keepIn3: bool = keepIn3
        self.key: str = key
        self.timeout: int = timeout
        self.maxAttempts: int = maxAttempts
        self.maxBlockCache: int = maxBlockCache
        self.maxCodeCache: int = maxCodeCache
        self.minDeposit: int = minDeposit
        self.nodeLimit: int = nodeLimit
        self.proof: In3ProofLevel = proof
        self.replaceLatestBlock: int = replaceLatestBlock
        self.requestCount: int = requestCount
        self.signatureCount: int = signatureCount


class TransactionReceipt(DataTransferObject):

    def __init__(self, transaction_hash: str, transaction_index: int, block_hash: str, block_number: int,
                 _from: Account, to: Account, cumulative_gas_used: int, gas_used: int, contract_address: int,
                 logs: list, logs_bloom: str):
        """
          bytes32_t  transaction_hash;    /**< the transaction hash */
          int        transaction_index;   /**< the transaction index */
          bytes32_t  block_hash;          /**< hash of ther containnig block */
          uint64_t   block_number;        /**< number of the containing block */
          uint64_t   cumulative_gas_used; /**< total amount of gas used by block */
          uint64_t   gas_used;            /**< amount of gas used by this specific transaction */
          bytes_t*   contract_address;    /**< contract address created (if the transaction was a contract creation) or NULL */
          bool       status;              /**< 1 if transaction succeeded, 0 otherwise. */
          eth_log_t* logs;                /**< array of log objects, which this transaction generated */
        Args:
            transaction_hash:
            transaction_index:
            block_hash:
            block_number:
            _from:
            to:
            cumulative_gas_used:
            gas_used:
            contract_address:
            logs:
            logs_bloom:
        """
        self.transaction_hash = transaction_hash
        self.transaction_index = transaction_index
        self.block_hash = block_hash
        self.block_number = block_number
        self._from = _from
        self.to = to
        self.cumulative_gas_used = cumulative_gas_used
        self.gas_used = gas_used
        self.contract_address = contract_address
        self.logs = logs
        self.logs_bloom = logs_bloom


class Logs(DataTransferObject):

    def __init__(self, log_index: int, transaction_index: int, transaction_hash: str, block_hash: str,
                 block_number: int, address: Account, data: str, topics: list):
        self.log_index = log_index
        self.transaction_index = transaction_index
        self.transaction_hash = transaction_hash
        self.block_hash = block_hash
        self.block_number = block_number
        self.address = address
        self.data = data
        self.topics = topics
