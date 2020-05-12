import re

from in3.eth.factory import EthObjectFactory
from in3.eth.model import NewTransaction, DataTransferObject
from in3.libin3.enum import EthMethods, BlockAt, In3Methods
from in3.libin3.runtime import In3Runtime


class SmartContract(DataTransferObject):
    """
    Ethereum account containing smart-contract code.
    Args:
        address: Account address. Derived from public key.
        chain_id: ID of the chain the account is used in.
        abi: Contract methods ABI, used to call them via RPC. Parsed from json string.
        code: EVM Bytecode for contract execution. Important for STANDARD and FULL verification.
        secret: Account private key. A 256 bit number.
        domain: ENS Domain name. ie. niceguy.eth
    """

    def __init__(self, address: str, chain_id: int, abi: dict, code: str = None, secret: int = None,
                 domain: str = None):
        self.address = address
        self.chain_id = chain_id
        self.abi = abi
        self.code = code
        self.secret = secret
        self.domain = domain

    def __str__(self):
        return self.address


class EthContractApi:
    """
    Manages smart-contract data and transactions
    """

    def __init__(self, runtime: In3Runtime, factory: EthObjectFactory):
        self._runtime = runtime
        self._factory = factory

    def call(self, transaction: NewTransaction, block_number: int or str = 'latest') -> int or str:
        """
        Calls a smart-contract method. Will be executed locally by Incubed's EVM or signed and sent over to save the state changes.
        Check https://ethereum.stackexchange.com/questions/3514/how-to-call-a-contract-method-using-the-eth-call-json-rpc-api for more.
        Args:
            transaction (NewTransaction):
            block_number (int or str):  Desired block number integer or 'latest', 'earliest', 'pending'.
        Returns:
            method_returned_value: A hexadecimal. For decoding use in3.abi_decode.
        """
        # different than eth_call
        # eth_call_fn(c, contract, BLKNUM_LATEST(), "servers(uint256):(string,address,uint,uint,uint,address)", to_uint256(i));
        return self._runtime.call(EthMethods.CALL, transaction.serialize(), block_number)

    def storage_at(self, address: str, position: int = 0, at_block: int or str = str(BlockAt.LATEST)) -> str:
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
        account = self._factory.get_account(address)
        if isinstance(at_block, int):
            at_block = hex(at_block)
        return self._runtime.call(EthMethods.STORAGE_AT, account.address, hex(position), at_block)

    def code(self, address: str, at_block: int or str = str(BlockAt.LATEST)) -> str:
        """
        Smart-Contract bytecode in hexadecimal. If the account is a simple wallet the function will return '0x'.
        Args:
            address (str): Ethereum account address
            at_block (int or str): Block number
        Returns:
            bytecode (str): Smart-Contract bytecode in hexadecimal.
        """
        account = self._factory.get_account(address)
        if isinstance(at_block, int):
            at_block = hex(at_block)
        return self._runtime.call(EthMethods.CODE, account.address, at_block)

    def encode(self, fn_signature: str, *fn_args) -> str:
        """
        Smart-contract ABI encoder. Used to serialize a rpc to the EVM.
        Based on the [Solidity specification.](https://solidity.readthedocs.io/en/v0.5.3/abi-spec.html)
        Note: Parameters refers to the list of variables in a method declaration.
        Arguments are the actual values that are passed in when the method is invoked.
        When you invoke a method, the arguments used must match the declaration's parameters in type and order.
        Args:
            fn_signature (str): Function name, with parameters. i.e. `getBalance(uint256):uint256`, can contain the return types but will be ignored.
            fn_args (tuple): Function parameters, in the same order as in passed on to method_name.
        Returns:
            encoded_fn_call (str): i.e. "0xf8b2cb4f0000000000000000000000001234567890123456789012345678901234567890"
        """
        self._check_fn_signature(fn_signature)
        return self._runtime.call(In3Methods.ABI_ENCODE, fn_signature, fn_args)

    def decode(self, fn_signature: str, encoded_value: str) -> tuple:
        """
        Smart-contract ABI decoder. Used to parse rpc responses from the EVM.
        Based on the [Solidity specification.](https://solidity.readthedocs.io/en/v0.5.3/abi-spec.html)
        Args:
            fn_signature: Function signature. e.g. `(address,string,uint256)` or `getBalance(address):uint256`.
            In case of the latter, the function signature will be ignored and only the return types will be parsed.
            encoded_value: Abi encoded values. Usually the string returned from a rpc to the EVM.
        Returns:
            decoded_return_values (tuple):  "0x1234567890123456789012345678901234567890", "0x05"
        """
        if not encoded_value.startswith('0x'):
            raise AssertionError("Encoded values must start with 0x")
        if len(encoded_value[2:]) < 64:
            raise AssertionError("Encoded values must be longer than 64 characters.")
        self._check_fn_signature(fn_signature)
        return self._runtime.call(In3Methods.ABI_DECODE, fn_signature, encoded_value)

    @staticmethod
    def _check_fn_signature(fn_signature):
        is_signature = re.match(r'.*(\(.+\))', fn_signature)
        _types = ["address", "string", "uint", "string", "bool", "bytes", "int"]
        contains_type = [_type for _type in _types if _type in fn_signature]
        if not is_signature or not contains_type:
            raise AssertionError('Function signature is not valid. A valid example is balanceOf(address).')
