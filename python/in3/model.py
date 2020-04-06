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
        chain_id (str): (optional) - 'main'|'goerli'|'kovan' Chain-id based on EIP-155. If None provided, will connect to the Ethereum network. example: 0x1 for mainNet
        chain_finality_threshold (int): (optional) - Behavior depends on the chain consensus algorithm: POA - percent of signers needed in order reach finality (% of the validators) i.e.: 60 %. POW - mined blocks on top of the requested, i.e. 8 blocks. Defaults are defined in enum.Chain.
        latest_block_stall (int): (optional) - Distance considered safe, consensus wise, from the very latest block. Higher values exponentially increases state finality, and therefore data security, as well guaranteeded responses from in3 nodes. example: 10 - will ask for the state from (latestBlock-10).
        account_priavate_key (str): (optional) - Account SK to sign requests. example: 0x387a8233c96e1fc0ad5e284353276177af2186e7afa85296f106336e376669f7
        node_signatures (int): (optional) - Node signatures attesting the response to your request. Will send a separate request for each. example: 3 nodes will have to sign the response.
        node_signature_consensus (int): - Useful when signatureCount <= 1. The client will check for consensus in responses. example: 10 - will ask for 10 different nodes and compare results looking for a consensus in the responses.
        node_min_deposit (int): - Only nodes owning at least this amount will be chosen to sign responses to your requests. i.e. 1000000000000000000 Wei
        node_list_auto_update (bool): (optional) - If true the nodelist will be automatically updated. False may compromise data security.
        node_limit (int): (optional) - Limit nodes stored in the client. example: 150 nodes
        request_timeout (int): Milliseconds before a request times out. example: 100000 ms
        request_retries (int): (optional) - Maximum times the client will retry to contact a certain node. example: 10 retries
        response_proof_level (str): (optional) - 'none'|'standard'|'full' Full gets the whole block Patricia-Merkle-Tree, Standard only verifies the specific tree branch concerning the request, None only verifies the root hashes, like a light-client does.
        response_includes_code (bool): (optional) - If true, every request with the address field will include the data, if existent, that is stored in that wallet/smart-contract. If false, only the code digest is included.
        response_keep_proof (bool): (optional) - If true, proof data will be kept in every rpc response. False will remove this data after using it to verify the responses. Useful for debugging and manually verifying the proofs.
        cached_blocks (int): (optional) - Maximum blocks kept in memory. example: 100 last requested blocks
        cached_code_bytes (int): (optional) - Maximum number of bytes used to cache EVM code in memory. example: 100000 bytes
    """

    def __init__(self,
                 chain_id: str = str(Chain.MAINNET),
                 chain_finality_threshold: int = int(Chain.MAINNET),
                 account_priavate_key: str = None,
                 latest_block_stall: int = 8,
                 node_signatures: int = 3,
                 node_signature_consensus: int = 1,
                 node_min_deposit: int = 10000000000000000,
                 node_list_auto_update: bool = True,
                 node_limit: int = None,
                 request_timeout: int = 5000,
                 request_retries: int = 0,
                 response_proof_level: In3ProofLevel = In3ProofLevel.STANDARD,
                 response_includes_code: bool = False,
                 response_keep_proof: bool = False,
                 cached_blocks: int = 1,
                 cached_code_bytes: int = 1000000):
        self.chainId: str = chain_id
        self.finality: int = chain_finality_threshold
        self.key: str = account_priavate_key
        self.replaceLatestBlock: int = latest_block_stall
        self.signatureCount: int = node_signatures
        self.requestCount: int = node_signature_consensus
        self.minDeposit: int = node_min_deposit
        self.autoUpdateList: bool = node_list_auto_update
        self.nodeLimit: int = node_limit
        self.timeout: int = request_timeout
        self.maxAttempts: int = request_retries
        self.proof: In3ProofLevel = response_proof_level
        self.includeCode: bool = response_includes_code
        self.keepIn3: bool = response_keep_proof
        self.maxBlockCache: int = cached_blocks
        self.maxCodeCache: int = cached_code_bytes

    def to_dict(self) -> dict:
        return {

        }


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
