from in3.eth.account import EthAccountApi
from in3.eth.contract import EthContractApi
from in3.eth.factory import EthObjectFactory
from in3.eth.model import Transaction, Block
from in3.libin3.enum import EthMethods, BlockAt
from in3.libin3.runtime import In3Runtime


class EthereumApi:
    """
    Module based on Ethereum's api and web3.js
    """

    def __init__(self, runtime: In3Runtime):
        self._runtime = runtime
        self._factory = EthObjectFactory(runtime)
        self.account = EthAccountApi(runtime, self._factory)
        self.contract = EthContractApi(runtime, self._factory)

    def keccak256(self, message: str) -> str:
        """
        Keccak-256 digest of the given data. Compatible with Ethereum but not with SHA3-256.
        Args:
            message (str): Message to be hashed.
        Returns:
            digest (str): The message digest.
        """
        return self._runtime.call(EthMethods.KECCAK, message)

    def gas_price(self) -> int:
        """
        The current gas price in Wei (1 ETH equals 1000000000000000000 Wei ).
        Returns:
            price (int): minimum gas value for the transaction to be mined
        """
        return self._factory.get_integer(self._runtime.call(EthMethods.GAS_PRICE))

    def block_number(self) -> int:
        """
        Returns the number of the most recent block the in3 network can collect signatures to verify.
        Can be changed by Client.Config.replaceLatestBlock.
        If you need the very latest block, change Client.Config.signatureCount to zero.
        Returns:
            block_number (int) : Number of the most recent block
        """
        return self._factory.get_integer(self._runtime.call(EthMethods.BLOCK_NUMBER))

    def get_block_by_hash(self, block_hash: str, get_full_block: bool = False) -> BlockAt:
        """
        Blocks can be identified by root hash of the block merkle tree (this), or sequential number in which it was mined (get_block_by_number).
        Args:
            block_hash (str):  Desired block hash
            get_full_block (bool): If true, returns the full transaction objects, otherwise only its hashes.
        Returns:
            block (Block): Desired block, if exists.
        """
        serialized: dict = self._runtime.call(EthMethods.BLOCK_BY_HASH, self._factory.get_hash(block_hash),
                                              get_full_block)
        return self._factory.get_block(serialized)

    def get_block_by_number(self, block_number: [int or str], get_full_block: bool = False) -> Block:
        """
        Blocks can be identified by sequential number in which it was mined, or root hash of the block merkle tree (this) (get_block_by_hash).
        Args:
            block_number (int or str):  Desired block number integer or 'latest', 'earliest', 'pending'.
            get_full_block (bool): If true, returns the full transaction objects, otherwise only its hashes.
        Returns:
            block (Block): Desired block, if exists.
        """
        if isinstance(block_number, str) and not block_number.upper() in [str(e) for e in BlockAt]:
            raise AssertionError('Block number must be an integer.')
        serialized: dict = self._runtime.call(EthMethods.BLOCK_BY_NUMBER, hex(block_number), get_full_block)
        return self._factory.get_block(serialized)

    def get_transaction_by_hash(self, tx_hash: str) -> Transaction:
        """
        Transactions can be identified by root hash of the transaction merkle tree (this) or by its position in the block transactions merkle tree.
        Every transaction hash is unique for the whole chain. Collision could in theory happen, chances are 67148E-63%.
        Args:
            tx_hash: Transaction hash.
        Returns:
            transaction: Desired transaction, if exists.
        """
        serialized: dict = self._runtime.call(EthMethods.TRANSACTION_BY_HASH, self._factory.get_hash(tx_hash))
        return self._factory.get_transaction(serialized)
