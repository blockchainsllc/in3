from in3.exception import HashFormatException, EthAddressFormatException
from in3.eth.model import Block, Transaction, Account, Log, TransactionReceipt
from in3.libin3.enum import In3Methods


class EthObjectFactory:
    """
    Deserialize and instantiate marshalled objects from rpc responses
    For more on design-patterns see [Martin Fowler's](https://martinfowler.com/eaaCatalog/) Catalog of Patterns of Enterprise Application Architecture.
    """

    def __init__(self, runtime, chain_id: str):
        self._runtime = runtime
        self._chain_id = chain_id

    def _deserialize(self, obj_dict: dict, mapping: dict) -> dict:
        result = {}
        for m in mapping:
            if obj_dict.get(m) is None:
                result[m] = None
                continue
            result[m] = mapping[m](obj_dict[m])
        return result

    def get_integer(self, hex_str: str) -> int:
        if hex_str == "0x":
            return 0
        return int(hex_str, 16)

    def get_block(self, serialized: dict):
        mapping = {
            "number": int,
            "hash": self.get_hash,
            "parentHash": self.get_hash,
            "nonce": int,
            "sha3Uncles": str,
            "logsBloom": str,
            "transactionsRoot": str,
            "stateRoot": str,
            "miner": self.get_account,
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
        obj = self._deserialize(serialized, mapping)
        transactions = []
        if serialized["transactions"] is not None and len(serialized["transactions"]) > 0:
            for t in serialized["transactions"]:
                if type(t) is dict:
                    aux = self.get_transaction(t)
                    transactions.append(aux)
                else:
                    # In case the user wants tx hashes only not full blocks
                    transactions.append(self.get_hash(t))
        obj["transactions"] = transactions
        return Block(**obj)

    def get_transaction(self, serialized: dict) -> Transaction:
        mapping = {
            "blockHash": self.get_hash,
            "from": self.get_account,
            "gas": int,
            "gasPrice": int,
            "hash": self.get_hash,
            "input": str,
            "nonce": int,
            "to": self.get_account,
            "transactionIndex": int,
            "value": int,
            "v": str,
            "r": str,
            "s": str
        }
        aux = self._deserialize(serialized, mapping)
        aux["From"] = self.get_account(serialized["from"])
        return Transaction(**aux)

    def get_tx_receipt(self, serialized: dict) -> TransactionReceipt:
        mapping = {
            'blockHash': self.get_hash,
            'blockNumber': self.get_integer,
            'contractAddress': self.get_account if serialized['contractAddress'] else None,
            'cumulativeGasUsed': self.get_integer,
            'gasUsed': self.get_integer,
            'logsBloom': str,
            'status': self.get_integer,
            'to': self.get_account if serialized['to'] else None,
            'transactionHash': self.get_hash,
            'transactionIndex': self.get_integer
        }
        obj = self._deserialize(serialized, mapping)
        obj["From"] = self.get_account(serialized["from"])
        if serialized["logs"] is not None and len(serialized["logs"]) > 0:
            obj["logs"] = [self.get_log(log) for log in serialized["logs"]]
        return TransactionReceipt(**obj)

    def get_log(self, serialized: dict) -> Log:
        mapping = {
            'address': self.get_account,
            'blockHash': self.get_integer,
            'blockNumber': self.get_integer,
            'data': str,
            'logIndex': self.get_integer,
            'removed': bool,
            'transactionHash': self.get_hash,
            'transactionIndex': self.get_integer,
            'transactionLogIndex': self.get_integer,
        }
        aux = self._deserialize(serialized, mapping)
        aux["topics"] = [self.get_hash(topic) for topic in serialized["topics"]]
        aux["Type"] = serialized["type"]
        return Log(**aux)

    def checksum_address(self, address: str, add_chain_id: bool = True) -> hex:
        return self._runtime.call(In3Methods.CHECKSUM_ADDRESS, address, add_chain_id)

    def get_hash(self, hash_str: str) -> hex:
        if not hash_str.startswith('0x'):
            raise HashFormatException("Ethereum hashes start with 0x")
        if len(hash_str[2:].encode('utf-8')) != 64:
            raise HashFormatException("Hash size is not of an Ethereum hash.")
        return hash_str

    def get_account(self, address: str) -> Account:
        if not address.startswith("0x"):
            raise EthAddressFormatException("Ethereum addresses start with 0x")
        if len(address.encode("utf-8")) != 42:
            raise EthAddressFormatException("The string don't have the size of an Ethereum address.")
        # if self.checksum_address(address, True) != address:
        #     raise EthAddressFormatException("The string checksum is invalid for an Ethereum address.")
        return Account(address, self._chain_id)
