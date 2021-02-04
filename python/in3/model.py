"""
MVC Pattern Model domain classes for the Incubed client module
"""
import warnings

from in3.eth.model import DataTransferObject, Account


class In3Node(DataTransferObject):
    """
    Registered remote verifier that attest, by signing the block hash, that the requested block and transaction were
    indeed mined are in the correct chain fork.
    Args:
        url (str): Endpoint to post to example: https://in3.slock.it
        index (int): Index within the contract example: 13
        address (in3.Account): Address of the node, which is the public address it is signing with. example: 0x6C1a01C2aB554930A937B0a2E8105fB47946c679
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
    Determines the behavior of the in3 client, which chain to connect to and how to manage information security policies.

    Considering integrity is guaranteed and confidentiality is not available on public blockchains, these settings will provide a balance between availability, and financial stake in case of repudiation.

    The newer the block is, higher are the chances it gets repudiated by a fork in the chain. In3 nodes will decide individually to sign on repudiable information, reducing the availability. If the application needs the very latest block, consider using a calculated value in `node_signature_consensus` and set `node_signatures` to zero. This setting is as secure as a light-client.

    The older the block gets, the highest is its availability because of the close-to-zero repudiation risk, but blocks older than circa one year are stored in Archive Nodes, expensive computers, so, despite of the low risk, there are not many nodes available with such information, and they must search for the requested block in its database, lowering the availability as well. If the application needs access to _old_ blocks, consider setting `request_timeout` and `request_retries` to accomodate the time the archive nodes take to fetch the inforamtion.

    The verification policy enforces an extra step of security, adding a financial stake in case of repudiation or false/broken proof. For high security application, consider setting a calculated value in `node_min_deposit` and request as much signatures as necessary in `node_signatures`. Setting `chain_finality_threshold` high will guarantee non-repudiability.

    **All args are Optional. Defaults connect to Ethereum main network with regular security levels.**

    Args:
        chain_finality_threshold (int):  Behavior depends on the chain consensus algorithm: POA - percent of signers needed in order reach finality (% of the validators) i.e.: 60 %. POW - mined blocks on top of the requested, i.e. 8 blocks. Defaults are defined in enum.Chain.
        latest_block_stall (int): Distance considered safe, consensus wise, from the very latest block. Higher values exponentially increases state finality, and therefore data security, as well guaranteeded responses from in3 nodes. example: 10 - will ask for the state from (latestBlock-10).
        account_secret (str): Account SK to sign all in3 requests. (Experimental use `set_account_sk`) example: 0x387a8233c96e1fc0ad5e284353276177af2186e7afa85296f106336e376669f7
        node_signatures (int): Node signatures attesting the response to your request. Will send a separate request for each. example: 3 nodes will have to sign the response.
        node_signature_consensus (int): Useful when node_signatures <= 1. The client will check for consensus in responses. example: 10 - will ask for 10 different nodes and compare results looking for a consensus in the responses.
        node_min_deposit (int): Only nodes owning at least this amount will be chosen to sign responses to your requests. i.e. 1000000000000000000 Wei
        node_list_auto_update (bool): If true the nodelist will be automatically updated. False may compromise data security.
        node_limit (int): Limit nodes stored in the client. example: 150 nodes
        request_timeout (int): Milliseconds before a request times out. example: 100000 ms
        request_retries (int): Maximum times the client will retry to contact a certain node. example: 10 retries
        response_proof_level (str): 'none'|'standard'|'full' Full gets the whole block Patricia-Merkle-Tree, Standard only verifies the specific tree branch concerning the request, None only verifies the root hashes, like a light-client does.
        response_includes_code (bool): If true, every request with the address field will include the data, if existent, that is stored in that wallet/smart-contract. If false, only the code digest is included.
        response_keep_proof (bool): If true, proof data will be kept in every rpc response. False will remove this data after using it to verify the responses. Useful for debugging and manually verifying the proofs.
        transport_binary_format: If true, the client will communicate with the server using a binary payload instead of json.
        transport_ignore_tls: The client usually verify https tls certificates. To communicate over insecure http, turn this on.
        boot_weights (bool): if true, the first request (updating the nodelist) will also fetch the current health status and use it for blacklisting unhealthy nodes. This is used only if no nodelist is availabkle from cache.
        in3_registry (dict): In3 Registry Smart Contract configuration data
    """

    def __init__(self,
                 chain_finality_threshold: int = None,
                 account_secret: str = None,
                 latest_block_stall: int = None,
                 node_signatures: int = None,
                 node_signature_consensus: int = None,
                 node_min_deposit: int = None,
                 node_list_auto_update: bool = None,
                 node_limit: int = None,
                 request_timeout: int = None,
                 request_retries: int = None,
                 response_proof_level: str = None,
                 response_includes_code: bool = None,
                 response_keep_proof: bool = None,
                 transport_binary_format: bool = None,
                 transport_ignore_tls: bool = None,
                 boot_weights: bool = None,
                 in3_registry: dict = None):
        self.finality: int = chain_finality_threshold
        self.key: str = account_secret
        self.replaceLatestBlock: int = latest_block_stall
        self.signatureCount: int = node_signatures
        self.requestCount: int = node_signature_consensus
        self.minDeposit: int = node_min_deposit
        self.autoUpdateList: bool = node_list_auto_update
        self.nodeLimit: int = node_limit
        self.timeout: int = request_timeout
        self.maxAttempts: int = request_retries
        self.proof: str = response_proof_level
        self.bootWeights: bool = boot_weights
        self.includeCode: bool = response_includes_code
        self.keepIn3: bool = response_keep_proof
        self.useBinary: bool = transport_binary_format
        self.useHttp: bool = transport_ignore_tls
        self.nodeRegistry: dict = in3_registry
        if self.key:
            warnings.warn(
                'In3 Config: `account_secret` may cause instability.', DeprecationWarning)


class ChainConfig:
    """
    Default in3 client configuration for each chain see #clientConfig for details.
    """

    def __init__(self, chain_id: int, chain_id_alias: str, client_config: ClientConfig):
        self.chain_id: int = chain_id
        self.chain_id_alias: str = chain_id_alias
        self.client_config: ClientConfig = client_config


chain_configs = {
    "mainnet": ChainConfig(
        chain_id=int(0x1),
        chain_id_alias="mainnet",
        client_config=ClientConfig(
            chain_finality_threshold=10,
            latest_block_stall=10,
            node_signatures=2)
    ),
    "goerli": ChainConfig(
        chain_id=int(0x5),
        chain_id_alias="goerli",
        client_config=ClientConfig(
            chain_finality_threshold=1,
            latest_block_stall=6,
            node_signatures=2)
    ),
    "ipfs": ChainConfig(
        chain_id=int(0x7d0),
        chain_id_alias="ipfs",
        client_config=ClientConfig(
            chain_finality_threshold=1,
            latest_block_stall=5,
            node_signatures=1,
            node_signature_consensus=1
        )
    ),
    "ewc": ChainConfig(
        chain_id=int(0xf6),
        chain_id_alias="ewc",
        client_config=ClientConfig(
            chain_finality_threshold=1,
            latest_block_stall=6,
            node_signatures=2,
        )
    ),
}
