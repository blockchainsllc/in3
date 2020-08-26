import json


class DataTransferObject:
    """
    Maps marshalling objects transferred to, and from a remote facade, in this case, libin3 rpc api.
    For more on design-patterns see [Martin Fowler's](https://martinfowler.com/eaaCatalog/) Catalog of Patterns of Enterprise Application Architecture.
    """

    def to_dict(self, int_to_hex: bool = False) -> dict:
        dictionary = {k: v for k, v in self.__dict__.items() if v is not None}
        if int_to_hex:
            integers = {k: hex(v) for k, v in dictionary.items() if isinstance(v, int)}
            dictionary.update(integers)
        return dictionary

    def serialize(self) -> str:
        return json.dumps(self.to_dict())


class Transaction(DataTransferObject):
    """
    Args:
        From (hex str): Address of the sender account.
        to (hex str): Address of the receiver account. Left undefined for a contract creation transaction.
        gas (int): Gas for the transaction miners and execution in wei. Will get multiplied by `gasPrice`. Use in3.eth.account.estimate_gas to get a calculated value. Set too low and the transaction will run out of gas.
        value (int): Value transferred in wei. The endowment for a contract creation transaction.
        data (hex str): Either a ABI byte string containing the data of the function call on a contract, or in the case of a contract-creation transaction the initialisation code.
        gasPrice (int): Price of gas in wei, defaults to in3.eth.gasPrice. Also know as `tx fee price`. Set your gas price too low and your transaction may get stuck. Set too high on your own loss.
        gasLimit (int); Maximum gas paid for this transaction. Set by the client using this rationale if left empty: gasLimit = G(transaction) + G(txdatanonzero) × dataByteLength. Minimum is 21000.
        nonce (int): Number of transactions mined from this address. Nonce is a value that will make a transaction fail in case it is different from (transaction count + 1). It exists to mitigate replay attacks. This allows to overwrite your own pending transactions by sending a new one with the same nonce. Use in3.eth.account.get_transaction_count to get the latest value.
        hash (hex str): Keccak of the transaction bytes, not part of the transaction. Also known as receipt, because this field is filled after the transaction is sent, by eth_sendTransaction
        blockHash (hex str): Block hash that this transaction was mined in. null when its pending.
        blockHash (int): Block number that this transaction was mined in. null when its pending.
        transactionIndex (int): Integer of the transactions index position in the block. null when its pending.
        signature (hex str): ECDSA of transaction.data, calculated r, s and v concatenated. V is parity set by v = 27 + (r % 2).
    """

    def __init__(self, From: str, to: str, gas: int, gasPrice: int, hash: str, nonce: int, transactionIndex: int,
                 blockHash: str, value: int, input: str, publicKey: str, standardV: int, raw: str, creates: str,
                 chainId: int, r: int, s: int, v: int):
        self.blockHash = blockHash
        self.From = From
        self.gas = gas
        self.gasPrice = gasPrice
        self.hash = hash
        self.nonce = nonce
        self.to = to
        self.transactionIndex = transactionIndex
        self.value = value
        self.input = input
        self.publicKey = publicKey
        self.raw = raw
        self.standardV = standardV
        self.creates = creates
        self.chainId = chainId
        self.r = r
        self.s = s
        self.v = v


class NewTransaction(DataTransferObject):
    """
    Unsent transaction. Use to send a new transaction.
    Args:
        From (hex str): Address of the sender account.
        to (hex str): Address of the receiver account. Left undefined for a contract creation transaction.
        value (int): (optional) Value transferred in wei. The endowment for a contract creation transaction.
        data (hex str): (optional) Either a ABI byte string containing the data of the function call on a contract, or in the case of a contract-creation transaction the initialisation code.
        gasPrice (int): (optional) Price of gas in wei, defaults to in3.eth.gasPrice. Also know as `tx fee price`. Set your gas price too low and your transaction may get stuck. Set too high on your own loss.
        gasLimit (int); (optional) Maximum gas paid for this transaction. Set by the client using this rationale if left empty: gasLimit = G(transaction) + G(txdatanonzero) × dataByteLength. Minimum is 21000.
        nonce (int): (optional) Number of transactions mined from this address. Nonce is a value that will make a transaction fail in case it is different from (transaction count + 1). It exists to mitigate replay attacks. This allows to overwrite your own pending transactions by sending a new one with the same nonce. Use in3.eth.account.get_transaction_count to get the latest value.
        hash (hex str): (optional) Keccak of the transaction bytes, not part of the transaction. Also known as receipt, because this field is filled after the transaction is sent.
        signature (hex str): (optional) ECDSA of transaction, r, s and v concatenated. V is parity set by v = 27 + (r % 2).
    """

    def __init__(self, From: str = None, to: str = None, nonce: int = None, value: int = None,
                 data: str = None, gasPrice: int = None, gasLimit: int = None, hash: str = None, signature: str = None):
        self.From = From
        self.gasPrice = gasPrice
        self.gasLimit = gasLimit
        self.hash = hash
        self.data = data
        self.nonce = nonce
        self.to = to
        self.value = value
        self.signature = signature
        super().__init__()

    def serialize(self) -> dict:
        dictionary = self.to_dict(int_to_hex=True)
        if 'From' in dictionary:
            del dictionary['From']
            dictionary['from'] = self.From
        return dictionary


class Block(DataTransferObject):

    def __init__(self, number: int, hash: str, parentHash: str, nonce: int, sha3Uncles: list, logsBloom: str,
                 transactionsRoot: str, stateRoot: str, miner: str, difficulty: int, totalDifficulty: int,
                 extraData: str, size: int, gasLimit: int, gasUsed: int, timestamp: int, transactions: list,
                 uncles: list, author: str):
        self.number = number
        self.hash = hash
        self.parentHash = parentHash
        self.nonce = nonce
        self.sha3Uncles = sha3Uncles
        self.logsBloom = logsBloom
        self.transactionsRoot = transactionsRoot
        self.stateRoot = stateRoot
        self.miner = miner
        self.difficulty = difficulty
        self.totalDifficulty = totalDifficulty
        self.extraData = extraData
        self.size = size
        self.gasLimit = gasLimit
        self.gasUsed = gasUsed
        self.timestamp = timestamp
        self.transactions = transactions
        self.uncles = uncles
        self.author = author


class Filter(DataTransferObject):
    """
    Filters are event catchers running on the Ethereum Client. Incubed has a client-side implementation.
    An event will be stored in case it is within to and from blocks, or in the block of blockhash, contains a
    transaction to the designed address, and has a word listed on topics.
    """

    def __init__(self, fromBlock: int or str, toBlock: int or str, address: str, topics: list, blockhash: str):
        self.fromBlock = fromBlock
        self.toBlock = toBlock
        self.address = address
        self.topics = topics
        self.blockhash = blockhash


class Log(DataTransferObject):
    """
    Transaction Log for events and data returned from smart-contract method calls.
    """

    def __init__(self, address: hex, blockHash: hex, blockNumber: int, data: hex, logIndex: int, removed: bool,
                 topics: [hex], transactionHash: hex, transactionIndex: int, transactionLogIndex: int, Type: str):
        self.address = address
        self.blockHash = blockHash
        self.blockNumber = blockNumber
        self.data = data
        self.logIndex = logIndex
        self.removed = removed
        self.topics = topics
        self.transactionHash = transactionHash
        self.transactionIndex = transactionIndex
        self.transactionLogIndex = transactionLogIndex
        self.Type = Type


class TransactionReceipt(DataTransferObject):
    """
    Receipt from a mined transaction.
    Args:
        blockHash:
        blockNumber:
        cumulativeGasUsed: total amount of gas used by block
        From:
        gasUsed: amount of gas used by this specific transaction
        logs:
        logsBloom:
        status: 1 if transaction succeeded, 0 otherwise.
        transactionHash:
        transactionIndex:
        to: Account to which this transaction was sent. If the transaction was a contract creation this value is set to None.
        contractAddress: Contract Account address created, f the transaction was a contract creation, or None otherwise.
    """

    def __init__(self, blockHash: hex, blockNumber: int, cumulativeGasUsed: int, From: str, gasUsed: int,
                 logsBloom: hex, status: int, transactionHash: hex, transactionIndex: int,
                 logs: [Log] = None, to: str = None, contractAddress: str = None):
        self.blockHash = blockHash
        self.blockNumber = blockNumber
        self.cumulativeGasUsed = cumulativeGasUsed
        self.From = From
        self.gasUsed = gasUsed
        self.logsBloom = logsBloom
        self.status = status
        self.transactionHash = transactionHash
        self.transactionIndex = transactionIndex
        self.logs = logs
        self.to = to
        self.contractAddress = contractAddress

    def to_dict(self, int_to_hex: bool = False) -> dict:
        base_dict = super(TransactionReceipt, self).to_dict()
        logs = [log.to_dict() for log in base_dict['logs']]
        base_dict['logs'] = logs
        return base_dict

    def __str__(self):
        return str(self.to_dict())


class Account(DataTransferObject):
    """
    An Ethereum account.
    Args:
        address: Account address. Derived from public key.
        chain_id: ID of the chain the account is used in.
        secret: Account private key. A 256 bit number.
        domain: ENS Domain name. ie. niceguy.eth
    """

    def __init__(self, address: str, chain_id: int, secret: int = None, domain: str = None):
        self.address = address
        self.chain_id = chain_id
        self.secret = secret
        self.domain = domain

    def __str__(self):
        return self.address
