from in3.exception import HashFormatException, EthAddressFormatException
from in3.eth.model import Block, Transaction, Account
from in3.libin3.enum import In3Methods


class EthObjectFactory:
    """
    Deserialize and instantiate objects from rpc responses
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
            "miner": self.get_address,
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

    def get_transaction(self, serialized: dict):
        mapping = {
            "blockHash": self.get_hash,
            "from": self.get_address,
            "gas": int,
            "gasPrice": int,
            "hash": self.get_hash,
            "input": str,
            "nonce": int,
            "to": self.get_address,
            "transactionIndex": int,
            "value": int,
            "v": str,
            "r": str,
            "s": str
        }
        aux = self._deserialize(serialized, mapping)
        aux["From"] = self.get_address(serialized["from"])
        return Transaction(**aux)

    def checksum_address(self, address: str, add_chain_id: bool = True) -> str:
        return self._runtime.call(In3Methods.CHECKSUM_ADDRESS, address, add_chain_id)

    @staticmethod
    def get_hash(hash_str: str):
        if not hash_str.startswith('0x'):
            raise HashFormatException("Ethereum hashes start with 0x")
        if len(hash_str.encode('utf-8')) != 64:
            raise HashFormatException("Hash size is not of an Ethereum hash.")
        return hash_str

    # TODO: Check if checksum address is working properly
    def get_address(self, address: str):
        if not address.startswith("0x"):
            raise EthAddressFormatException("Ethereum addresses start with 0x")
        if len(address.encode("utf-8")) != 42:
            raise EthAddressFormatException("The string don't have the size of an Ethereum address.")
        # if self.checksum_address(address, True) != address:
        #     raise EthAddressFormatException("The string checksum is invalid for an Ethereum address.")
        return Account(address, self._chain_id)
