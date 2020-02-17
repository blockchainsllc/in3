from __future__ import annotations
import enum
import json

from typing import List, Tuple

from in3.model.enum import BlockStatus, Chain, In3ProofLevel
from in3.model.exception import In3HashFormatException, In3AddressFormatException


class In3Model:

    def __str__(self):
        return json.dumps(to_dict(self))

    def value(self):
        return to_dict(self)


class In3NodeWeight(In3Model):

    def value(self) -> int:
        return self.weight

    def __int__(self):
        return self.weight

    def __init__(self, weight: int = None, responseCount: int = None, avgResponseTime: int = None,
                 pricePerRequest: int = None, lastRequest: int = None, blacklistedUntil: int = None):
        """

        :param avgResponseTime :number (optional) - average time of a response in ms example: 240
        :param blacklistedUntil :number (optional) - blacklisted because of failed requests until the timestamp example: 1529074639623
        :param lastRequest :number (optional) - timestamp of the last request in ms example: 1529074632623
        :param pricePerRequest :number (optional) - last price
        :param responseCount :number (optional) - number of uses. example: 147
        :param weight :number (optional) - factor the weight this noe (default 1.0) example: 0.5
        """
        self.weight: int = weight
        self.responseCount: int = responseCount
        self.avgResponseTime: int = avgResponseTime
        self.pricePerRequest: int = pricePerRequest
        self.lastRequest: int = lastRequest
        self.blacklistedUntil: int = blacklistedUntil


class In3NodeConfig(In3Model):

    def __init__(self, index: int = None, address: str = None, timeout: int = None, url: str = None,
                 chainIds: List[str] = None, deposit: int = None, capacity: int = None, props: int = None):
        """
        :param address :string - the address of the node, which is the public address it iis signing with. example: 0x6C1a01C2aB554930A937B0a2E8105fB47946c679
        :param capacity :number (optional) - the capacity of the node. example: 100
        :param chainIds :string[] - the list of supported chains example: 0x1
        :param deposit :number - the deposit of the node in wei example: 12350000
        :param index :number (optional) - the index within the contract example: 13
        :param props :number (optional) - the properties of the node. example: 3
        :param registerTime :number (optional) - the UNIX-timestamp when the node was registered example: 1563279168
        :param timeout :number (optional) - the time (in seconds) until an owner is able to receive his deposit back after he unregisters himself example: 3600
        :param unregisterTime :number (optional) - the UNIX-timestamp when the node is allowed to be deregister example: 1563279168
        :param url :string - the endpoint to post to example: https://in3.slock.it
        """
        self.index: int = index
        self.address: str = address
        self.timeout: int = timeout
        self.url: str = url
        self.chainIds: List[str] = chainIds
        self.deposit: int = deposit
        self.capacity: int = capacity
        self.props: int = props


class ChainSpec(In3Model):

    def __init__(self, engine: str = None, validatorContract: str = None, validatorList: List = None):
        """
        :param block :number (optional) - the blocknumnber when this configuration should apply
        :param bypassFinality :number (optional) - Bypass finality check for transition to contract based Aura Engines example: bypassFinality = 10960502 -> will skip the finality check and add the list at block 10960502
        :param contract :string (optional) - The validator contract at the block
        :param engine :'ethHash'|'authorityRound'|'clique' (optional) - the engine type (like Ethhash, authorityRound, … )
        :param list :string[] (optional) - The list of validators at the particular block
        :param requiresFinality :boolean (optional) - indicates whether the transition requires a finality check example: true
        """
        self.engine: str = engine
        self.validatorContract: str = validatorContract
        self.validatorList: List = validatorList


class In3ServerConfig(In3Model):

    def __init__(self, verifier: str = None, name: str = None, chainSpec: ChainSpec = None):
        self.verifier: str = verifier
        self.name: str = name
        self.chainSpec: ChainSpec = chainSpec


class Address(In3Model):

    def __init__(self, address: str, skip_validation: bool = False):
        self.__skip_validation = skip_validation
        self.__address = self.__validate(address=address)

    def __str__(self):
        return self.__address

    def value(self) -> str:
        return self.__address

    @staticmethod
    def checksum(_address: str, _prefix: bool = True) -> str:
        checked = ""
        _address = _address.lower()
        if _address.startswith("0x"):
            _address = _address[2:]
        from in3.eth import web3_sha3
        sha3_aux = web3_sha3(_address)[2:]
        for i_addr, i_sha in zip(list(_address), list(sha3_aux)):
            if int(i_sha, 16) >= 8:
                i_addr = str(i_addr).upper()
            checked += i_addr
        if _prefix:
            return "0x" + checked
        return checked

    def __validate(self, address: str) -> str:
        if address.startswith("0x"):
            address = address[2:]

        if self.__skip_validation:
            return "0x" + address

        if len(address.encode("utf-8")) is not 40:
            raise In3AddressFormatException("The address don't have enough size as needed.")

        if Address.checksum(address, _prefix=False) != address:
            raise In3AddressFormatException("The Address it's not in a correct checksum.")

        return "0x" + address


class Hash(In3Model):

    def __init__(self, hash_str: str, skip_validation: bool = False):
        self.__skip_validation = skip_validation
        self.__hash = self.__validate(hash_str=hash_str)

    def __str__(self):
        return self.__hash

    def value(self) -> str:
        return self.__hash

    def __validate(self, hash_str: str):
        if hash_str.startswith("0x"):
            hash_str = hash_str[2:]

        if self.__skip_validation:
            return "0x" + hash_str

        if len(hash_str.encode("utf-8")) is not 64:
            raise In3HashFormatException("Hash size is not a proper size of 32 bytes")

        return "0x" + hash_str


class Config(In3Model):

    def __init__(self, autoUpdateList: bool = None, chainId: str = str(Chain.MAINNET), finality: int = None,
                 includeCode: bool = False, keepIn3: bool = False, key: str = None, maxAttempts: int = None,
                 maxBlockCache: int = None, maxCodeCache: int = None, minDeposit: int = None, nodeLimit: int = None,
                 proof: In3ProofLevel = In3ProofLevel.STANDARD, replaceLatestBlock: int = None, requestCount: int = 2,
                 rpc: str = None, servers: List[In3ServerConfig] = None, signatureCount: int = 1,
                 verifiedHashes: List = None):
        """
        :param autoUpdateList :boolean (optional) - if true the nodelist will be automaticly updated if the lastBlock is newer
        :param chainId :string - servers to filter for the given chain. The chain-id based on EIP-155. example: 0x1
        :param finality :number (optional) - the number in percent needed in order reach finality (% of signature of the validators) example: 50
        :param includeCode :boolean (optional) - if true, the request should include the codes of all accounts. otherwise only the the codeHash is returned. In this case the client may ask by calling eth_getCode() afterwards
        :param keepIn3 :boolean (optional) - if true, the in3-section of thr response will be kept. Otherwise it will be removed after validating the data. This is useful for debugging or if the proof should be used afterwards.
        :param key :str (optional) - the client key to sign requests example: 0x387a8233c96e1fc0ad5e284353276177af2186e7afa85296f106336e376669f7
        :param maxAttempts :number (optional) - max number of attempts in case a response is rejected example: 10
        :param maxBlockCache :number (optional) - number of number of blocks cached in memory example: 100
        :param maxCodeCache :number (optional) - number of max bytes used to cache the code in memory example: 100000
        :param minDeposit :number - min stake of the server. Only nodes owning at least this amount will be chosen.
        :param nodeLimit :number (optional) - the limit of nodes to store in the client. example: 150
        :param proof :'none'|'standard'|'full' (optional) - if true the nodes should send a proof of the response
        :param replaceLatestBlock :number (optional) - if specified, the blocknumber latest will be replaced by blockNumber- specified value example: 6
        :param requestCount :number - the number of request send when getting a first answer example: 3
        :param rpc :string (optional) - url of one or more rpc-endpoints to use. (list can be comma seperated)
        :param servers (optional) - the nodelist per chain
        :param signatureCount :number (optional) - number of signatures requested example: 2
        :param verifiedHashes :string[] (optional) - if the client sends a array of blockhashes the server will not deliver any signatures or blockheaders for these blocks, but only return a string with a number. This is automaticly updated by the cache, but can be overriden per request.
        """
        self.autoUpdateList: bool = autoUpdateList
        self.chainId: str = chainId
        self.finality: int = finality
        self.includeCode: bool = includeCode
        self.keepIn3: bool = keepIn3
        self.key = key
        self.maxAttempts: int = maxAttempts
        self.maxBlockCache: int = maxBlockCache
        self.maxCodeCache: int = maxCodeCache
        self.minDeposit: int = minDeposit
        self.nodeLimit: int = nodeLimit
        self.proof: In3ProofLevel = proof
        self.replaceLatestBlock: int = replaceLatestBlock
        self.requestCount: int = requestCount
        self.rpc: str = rpc
        self.servers: List[In3ServerConfig] = servers
        self.signatureCount: int = signatureCount
        self.verifiedHashes: List = verifiedHashes



class Transaction(In3Model):


    def __init__(self,
                 blockHash: Hash = None,
                 From: Address = None,
                 gas: int = None,
                 gasPrice: int = None,
                 hash: Hash = None,
                 input: str = None,
                 nonce: int = None,
                 to: Address = None,
                 transactionIndex: int = None,
                 value: int = None,
                 v: str = None,
                 r: str = None,
                 s: str = None):

        self.blockHash = blockHash
        self.From = From
        self.gas = gas
        self.gasPrice = gasPrice
        self.hash = hash
        self.input = input
        self.nonce = nonce
        self.to = to
        self.transactionIndex = transactionIndex
        self.value = value
        self.v = v
        self.r = r
        self.s = s

    def __str__(self):
        return json.dumps(to_dict(self))

    @staticmethod
    def from_json(json_response) -> Transaction:
        aux = from_json(_data=json_response, _mapping=HelperMapping.transaction())
        aux["From"] = Address(Address.checksum(json_response["from"],True))
        return Transaction(**aux)


class HelperMapping:

    @staticmethod
    def transaction():
        mapping = {
            "blockHash": Hash,
            "from": Address,
            "gas": int,
            "gasPrice": int,
            "hash": Hash,
            "input": str,
            "nonce": int,
            "to": Address,
            "transactionIndex": int,
            "value": int,
            "v": str,
            "r": str,
            "s": str
        }
        return mapping

    @staticmethod
    def block():
        mapping = {
            "number": int,
            "hash": Hash,
            "parentHash": Hash,
            "nonce": int,
            "sha3Uncles": str,
            "logsBloom": str,
            "transactionsRoot": str,
            "stateRoot": str,
            "miner": Address,
            "difficulty": int,
            "totalDifficulty": int,
            "extraData": str,
            "size": int,
            "gasLimit": int,
            "gasUsed": int,
            "timestamp": int,
            # "transactions": [{...}, {...}],
            "uncles": list
        }
        return mapping


class Block(In3Model):

    def __init__(self,
            number: int = None,
            hash: Hash = None,
            parentHash: Hash = None,
            nonce: int = None,
            sha3Uncles: list = None,
            logsBloom: str = None,
            transactionsRoot: str = None,
            stateRoot: str = None,
            miner:str =  None,
            difficulty: int = None,
            totalDifficulty: int = None,
            extraData: str = None,
            size: int = None,
            gasLimit: int = None,
            gasUsed: int = None,
            timestamp: int = None,
            transactions: list= None,
            uncles: list=None):

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

    @staticmethod
    def from_json(json_response) -> Block:
        obj = from_json(_data=json_response, _mapping=HelperMapping.block())
        block = Block(**obj)

        transaction_aux = []

        if json_response["transactions"] is not None and len(json_response["transactions"]) > 0:
            for t in json_response["transactions"]:
                if type(t) is dict:
                    aux = Transaction.from_json(t)
                    transaction_aux.append(aux)
                else:
                    transaction_aux.append(Hash(t))

        block.transactions = transaction_aux
        return block


class RPCRequest:

    def __init__(self, method: enum.Enum, params: Tuple = (), id: int = 1, jsonrpc: str = "2.0"):
        """
        [{"id":1,"jsonrpc":"2.0","method":"in3_nodeList","params":[],"in3":{"verification":"proof","version": "2.1.0"}}]
        :param jsonrpc: Version of json-rpc
        :param method: In3 method name
        :param params: method params
        :param id: message id
        """
        self.id = id
        self.jsonrpc = jsonrpc
        self.method = method
        self.params = params
        self.in3 = {
            "verification": "proof",
            "version": "2.1.0"
        }

    def __str__(self):
        return json.dumps(to_dict(self))

    def to_utf8(self):
        return self.__str__().encode('utf8')


class RPCResponse:

    def __init__(self, id: str = None, jsonrpc: str = None, result: str = None, error=None):
        self.id = id
        self.jsonrpc = jsonrpc
        self.result = result
        self.error = error

    def __str__(self):
        return json.dumps(to_dict(self))


class TransactionReceipt(In3Model):

    def __init__(self, transaction_hash: Hash, transaction_index: int, block_hash: Hash, block_number: int,
                 _from: Address, to: Address, cumulative_gas_used: int, gas_used: int, contract_address: int,
                 logs: List, logs_bloom: str):
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


class Filter(In3Model):

    def __init__(self, fromBlock: [int, BlockStatus] = None, toBlock: [int, BlockStatus] = None,
                 address: Address = None, topics: List = None, blockhash: Hash = None):
        self.fromBlock = fromBlock
        self.toBlock = toBlock
        self.address = address
        self.topics = topics
        self.blockhash = blockhash


class Logs(In3Model):

    def __init__(self, log_index: int, transaction_index: int, transaction_hash: Hash, block_hash: Hash, 
                 block_number: int, address: Address, data: str, topics: List):
        self.log_index = log_index
        self.transaction_index = transaction_index
        self.transaction_hash = transaction_hash
        self.block_hash = block_hash
        self.block_number = block_number
        self.address = address
        self.data = data
        self.topics = topics


def from_json(_data: dict, _mapping: dict) -> dict:
    result = {}
    for m in _mapping:
        try:
            if _data.get(m) is None:
                result[m] = None
                continue

            if _mapping[m] == int:
                result[m] = int(_data[m],16)
                continue
            #otherwise
            result[m] = _mapping[m](_data[m])
        except:
            pass

    return result


def to_dict(_clazz: object) -> dict:
    aux = _clazz.__dict__
    result = {}
    for i in aux:
        if aux[i] is None:
            continue

        arr = i.split("_")
        r = ""
        index_aux = 0
        for arr_aux in arr:
            if arr_aux is '':
                continue
            r += arr_aux[index_aux].upper() + arr_aux[index_aux + 1:]
        key = r[0].lower() + r[1:]

        def get_val_from_object(_item) -> object:

            if isinstance(_item, enum.Enum):
                return _item.value
            # TODO: review or fix this policy
            elif type(_item) == int:
                return hex(int(_item))
            elif isinstance(_item, Address):
                return str(_item)
            elif isinstance(_item, Hash):
                return str(_item)
            elif issubclass(_item.__class__, In3Model):
                return to_dict(_item)
            return _item

        if isinstance(aux[i], Tuple):
            value = []
            for a in aux[i]:
                value.append(get_val_from_object(a))
        else:
            value = get_val_from_object(aux[i])

        result[key] = value

    return result