from in3.eth.account import EthAccountApi
from in3.eth.factory import EthObjectFactory
from in3.eth.model import Transaction, Block
from in3.libin3.runtime import In3Runtime
from in3.libin3.enum import EthMethods, BlockStatus


class EthereumApi:
    """
    Module based on Ethereum's api and web3.js
    """

    def __init__(self, runtime: In3Runtime, chain_id: str):
        self._runtime = runtime
        self.factory = EthObjectFactory(runtime, chain_id)
        self.account = EthAccountApi(runtime, self.factory)

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
        return self.factory.get_integer(self._runtime.call(EthMethods.GAS_PRICE))

    def block_number(self) -> int:
        """
        Returns the number of the most recent block the in3 network can collect signatures to verify.
        Can be changed by Client.Config.replaceLatestBlock.
        If you need the very latest block, change Client.Config.signatureCount to zero.
        Returns:
            block_number (int) : Number of the most recent block
        """
        return self.factory.get_integer(self._runtime.call(EthMethods.BLOCK_NUMBER))

    def get_balance(self, address: str, at_block: int or str = str(BlockStatus.LATEST)) -> int:
        """
        Returns the balance of the account of given address.
        Args:
            address (str): address to check for balance
            at_block (int or str):  block number IN3BlockNumber  or EnumBlockStatus
        Returns:
            balance (int): integer of the current balance in wei.
        """
        account = self.factory.get_address(address)
        if isinstance(at_block, int):
            at_block = hex(at_block)
        return self.factory.get_integer(self._runtime.call(EthMethods.BALANCE, account.address, at_block))

    def get_storage_at(self, address: str, position: int = 0, at_block: int or str = BlockStatus.LATEST) -> str:
        """
        Stored value in designed position at a given address. Storage can be used to store a smart contract state, constructor or just any data.
        Each contract consists of a EVM bytecode handling the execution and a storage to save the state of the contract.
        The storage is essentially a key/value store. Use get_code to get the smart-contract code.
        Args:
            address (str): Ethereum account address
            position (int):  Position index, 0x0 up to 0x64
            at_block (int or str):  Block number
        Returns:
            storage_at (str): Stored value in designed position. Use decode('hex') to see ascii format of the hex data.
        """
        account = self.factory.get_address(address)
        return self._runtime.call(EthMethods.STORAGE_AT, account.address, position, at_block)

    def get_code(self, address: str, at_block: int or str = BlockStatus.LATEST) -> str:
        """
        Smart-Contract bytecode in hexadecimal. If the account is a simple wallet the function will return '0x'.
        Args:
            address (str): Ethereum account address
            at_block (int or str): Block number
        Returns:
            bytecode (str): Smart-Contract bytecode in hexadecimal.
        """
        account = self.factory.get_address(address)
        return self._runtime.call(EthMethods.CODE, account.address, at_block)

    def get_transaction_count(self, address: str, at_block: int or str = BlockStatus.LATEST) -> int:
        """
        Number of transactions mined from this address. Used to set transaction nonce.
        Nonce is a value that will make a transaction fail in case it is different from (transaction count + 1).
        It exists to mitigate replay attacks.
        Args:
            address (str): Ethereum account address
            at_block (int):  Block number
        Returns:
            tx_count (int): Number of transactions mined from this address.
        """
        account = self.factory.get_address(address)
        return self._runtime.call(EthMethods.TRANSACTION_COUNT, account.address, at_block)

    def get_block_by_hash(self, block_hash: str, get_full_block: bool = False) -> Block:
        """
        Blocks can be identified by root hash of the block merkle tree (this), or sequential number in which it was mined (get_block_by_number).
        Args:
            block_hash (str):  Desired block hash
            get_full_block (bool): If true, returns the full transaction objects, otherwise only its hashes.
        Returns:
            block (Block): Desired block, if exists.
        """
        serialized: dict = self._runtime.call(EthMethods.BLOCK_BY_HASH, self.factory.get_hash(block_hash), get_full_block)
        return self.factory.get_block(serialized)

    def get_block_by_number(self, block_number: [int or str], get_full_block: bool = False) -> Block:
        """
        Blocks can be identified by sequential number in which it was mined, or root hash of the block merkle tree (this) (get_block_by_hash).
        Args:
            block_number (int or str):  Desired block number integer or 'latest', 'earliest', 'pending'.
            get_full_block (bool): If true, returns the full transaction objects, otherwise only its hashes.
        Returns:
            block (Block): Desired block, if exists.
        """
        if isinstance(block_number, str) and not block_number.upper() in [e.value for e in BlockStatus]:
            raise AssertionError('Block number must be an integer or \'latest\', \'earliest\', or \'pending\'')
        serialized: dict = self._runtime.call(EthMethods.BLOCK_BY_NUMBER, block_number, get_full_block)
        return self.factory.get_block(serialized)

    def get_transaction_by_hash(self, tx_hash: str) -> Transaction:
        """
        Transactions can be identified by root hash of the transaction merkle tree (this) or by its position in the block transactions merkle tree.
        Every transaction hash is unique for the whole chain. Collision could in theory happen, chances are 67148E-63%.
        Args:
            tx_hash: Transaction hash.
        Returns:
            transaction: Desired transaction, if exists.
        """
        serialized: dict = self._runtime.call(EthMethods.TRANSACTION_BY_HASH, self.factory.get_hash(tx_hash))
        return self.factory.get_transaction(serialized)
