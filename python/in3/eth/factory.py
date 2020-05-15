"""
Ethereum Domain Object Factory
For more on design-patterns see [Martin Fowler's](https://martinfowler.com/eaaCatalog/) Catalog of Patterns of Enterprise Application Architecture.
"""
from in3.eth.model import Block, Transaction, Log, TransactionReceipt, Account
from in3.exception import HashFormatException, EthAddressFormatException
from in3.libin3.enum import In3Methods
from in3.libin3.runtime import In3Runtime


class EthObjectFactory:
    """
    Deserialize and instantiate marshalled objects from rpc responses
    """

    def __init__(self, runtime: In3Runtime):
        self._runtime = runtime

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
            "number": self.get_integer,
            "hash": self.get_hash,
            "parentHash": self.get_hash,
            "nonce": self.get_integer,
            "sha3Uncles": str,
            "logsBloom": str,
            "transactionsRoot": self.get_hash,
            "stateRoot": self.get_hash,
            "miner": str,
            "author": str,
            "difficulty": self.get_integer,
            "totalDifficulty": self.get_integer,
            "extraData": str,
            "size": self.get_integer,
            "gasLimit": self.get_integer,
            "gasUsed": self.get_integer,
            "timestamp": self.get_integer,
            # "transactions": [{...}, {...}],
            # "uncles": list
        }
        obj = self._deserialize(serialized, mapping)
        transactions = []
        if serialized["transactions"] is not None and len(serialized["transactions"]) > 0:
            for t in serialized["transactions"]:
                if type(t) is dict:
                    aux = self.get_transaction(t)
                    transactions.append(aux)
                else:
                    # In case the user wants tx hashes only not full txs
                    transactions.append(self.get_hash(t))
        obj["transactions"] = transactions
        uncles = []
        if serialized["uncles"] is not None and len(serialized["uncles"]) > 0:
            for t in serialized["uncles"]:
                if type(t) is dict:
                    aux = self.get_block(t)
                    uncles.append(aux)
                else:
                    # In case the user wants block hashes only not full blocks
                    uncles.append(self.get_hash(t))
        obj["uncles"] = uncles
        return Block(**obj)

    def get_transaction(self, serialized: dict) -> Transaction:
        mapping = {
            "blockHash": self.get_hash,
            # "from": self.get_address,
            "gas": self.get_integer,
            "gasPrice": self.get_integer,
            "hash": self.get_hash,
            "input": str,
            "nonce": self.get_integer,
            "to": self.get_address,
            "transactionIndex": self.get_integer,
            "value": self.get_integer,
            "raw": str,
            "standardV": self.get_integer,
            "publicKey": str,
            "creates": str,
            "chainId": self.get_integer,
            "v": self.get_integer,
            "r": self.get_integer,
            "s": self.get_integer
        }
        aux = self._deserialize(serialized, mapping)
        aux["From"] = self.get_address(serialized["from"])
        return Transaction(**aux)

    def get_tx_receipt(self, serialized: dict) -> TransactionReceipt:
        mapping = {
            'blockHash': self.get_hash,
            'blockNumber': self.get_integer,
            'contractAddress': self.get_address if serialized['contractAddress'] else None,
            'cumulativeGasUsed': self.get_integer,
            'gasUsed': self.get_integer,
            'logsBloom': str,
            'status': self.get_integer,
            'to': self.get_address if serialized['to'] else None,
            'transactionHash': self.get_hash,
            'transactionIndex': self.get_integer
        }
        obj = self._deserialize(serialized, mapping)
        obj["From"] = self.get_address(serialized["from"])
        if serialized["logs"] is not None and len(serialized["logs"]) > 0:
            obj["logs"] = [self.get_log(log) for log in serialized["logs"]]
        return TransactionReceipt(**obj)

    def get_log(self, serialized: dict) -> Log:
        mapping = {
            'address': self.get_address,
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

    def checksum_address(self, address: str, add_chain_id: bool = True) -> str:
        return self._runtime.call(In3Methods.CHECKSUM_ADDRESS, address, add_chain_id)

    def get_hash(self, hash_str: str) -> str:
        if not hash_str or not isinstance(hash_str, str):
            raise HashFormatException("Hash must be a string.")
        if not hash_str.startswith('0x'):
            raise HashFormatException("Hash must start with 0x.")
        if len(hash_str[2:].encode('utf-8')) != 64:
            raise HashFormatException("Hash size is not of an Ethereum hash.")
        return hash_str

    def get_hex(self, hash_str: str) -> str:
        if not hash_str or not isinstance(hash_str, str):
            raise AssertionError("Hexadecimal must be a string.")
        if not hash_str.startswith('0x'):
            raise AssertionError("Hexadecimal must start with 0x.")
        return hash_str

    def get_address(self, address: str) -> str:
        if not isinstance(address, str) or not address.startswith("0x"):
            raise EthAddressFormatException("Ethereum Addresses start with 0x.")
        if len(address.encode("utf-8")) != 42:
            raise EthAddressFormatException("Ethereum Address size string size incorrect.")
        if self.checksum_address(address, False) != address:
            address = self.checksum_address(address, False)
        return address.replace('\'', '"')

    def get_account(self, address: str, secret: int = None) -> Account:
        return Account(self.get_address(address), self._runtime.chain_id, secret)
