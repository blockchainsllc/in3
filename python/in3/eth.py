from __future__ import annotations
from in3.bind.runtime import In3Runtime
from in3.model.client import RPCRequest, Address, Hash, Transaction, Filter, Block
from in3.model.enum import EthCall, BlockStatus



"""
    Ethereum module of In3 Client
    Based on we3 eth
"""


def web3_sha3(data: str):
    """ Keccak-256 (not the standardized SHA3-256) of the given data.

        Args:
            data (str): the data to convert into a SHA3 hash.

        Returns:
            data (str): The SHA3 result of the given string.
    """

    rpc = RPCRequest(method=EthCall.WEB3_SHA3, params=(data,))
    return In3Runtime.call_in3_rpc(rpc)


def gas_price() -> int:
    """ The current price per gas in wei.

        Args:
            -

        Returns:
            quantity (int): integer of the current gas price in wei.
    """

    rpc = RPCRequest(method=EthCall.GAS_PRICE)
    return In3Runtime.call_in3_rpc(request=rpc)


def block_number() -> int:
    """ Returns the number of most recent block.

        Args:
            -
        Returns:
            block number (int) : number of most recent block

    """
    rpc = RPCRequest(method=EthCall.BLOCK_NUMBER)
    return In3Runtime.call_in3_rpc(request=rpc)


def get_balance(address: Address,
                number: [BlockStatus, int] = BlockStatus.LATEST) -> int:
    """ Returns the balance of the account of given address.

        Args:
            address (Address): address to check for balance
            number (BlockStatus orint):  block number IN3BlockNumber  or EnumBlockStatus

        Returns:
            balance (int): integer of the current balance in wei.

    """
    rpc = RPCRequest(method=EthCall.BALANCE, params=(address, number,))
    return In3Runtime.call_in3_rpc(request=rpc)


def get_storage_at(address: Address, position: int, number: [BlockStatus, int]):
    """ Returns the value from a storage position at a given address.

        Args:
            address (Address): address to check for balance
            position (int):  integer of the position in the storage.
            number (BlockStatus or int):  block number IN3BlockNumber  or EnumBlockStatus

        Returns:
            the value at this storage position.

    """
    rpc = RPCRequest(method=EthCall.STORAGE_AT, params=(address, position, number,))
    return In3Runtime.call_in3_rpc(request=rpc)


def get_transaction_count(address: Address, number: [int, BlockStatus]) -> int:
    """
    Returns the number of transactions sent from an address.

    Args:
        address (Address): address to check
        number (BlockStatus orint):  block number IN3BlockNumber  or EnumBlockStatus

    Returns:
        quantity (int): integer of the number of transactions send from this address.

    """
    rpc = RPCRequest(method=EthCall.TRANSACTION_COUNT, params=(address, number,))
    return In3Runtime.call_in3_rpc(request=rpc)


def get_block_transaction_count_by_hash(block_hash: Hash):
    rpc = RPCRequest(method=EthCall.BLOCK_TRANSACTION_COUNT_BY_HASH, params=(block_hash,))
    return In3Runtime.call_in3_rpc(request=rpc)


def get_block_transaction_count_by_number(number: [int, BlockStatus]):
    rpc = RPCRequest(method=EthCall.BLOCK_TRANSACTION_COUNT_BY_NUMBER, params=(number,))
    return In3Runtime.call_in3_rpc(request=rpc)


# def get_uncle_count_by_block_hash(block_hash: Hash):
#     rpc = RPCRequest(method=EthCall.UNCLE_COUNT_BY_BLOCK_HASH, params=(block_hash,))
#     return In3Runtime.call_in3_rpc(request=rpc)


# def get_uncle_count_by_block_number(number: [int, BlockStatus]):
#     rpc = RPCRequest(method=EthCall.UNCLE_COUNT_BY_BLOCK_NUMBER, params=(number,))
#     return In3Runtime.call_in3_rpc(request=rpc)


def get_code(address: Address, number: [int, BlockStatus]):
    rpc = RPCRequest(method=EthCall.CODE, params=(address, number,))
    return In3Runtime.call_in3_rpc(request=rpc)


def sign(address: Address, message: str):
    rpc = RPCRequest(method=EthCall.SIGN, params=(address, message,))
    return In3Runtime.call_in3_rpc(request=rpc)


def send_transaction(transaction: Transaction):
    rpc = RPCRequest(method=EthCall.SEND_TRANSACTION, params=(transaction,))
    return In3Runtime.call_in3_rpc(request=rpc)


def send_raw_transaction(data: str):
    rpc = RPCRequest(method=EthCall.SEND_RAW_TRANSACTION, params=(data,))
    return In3Runtime.call_in3_rpc(request=rpc)


def call(transaction: Transaction, number: [int, BlockStatus]):
    rpc = RPCRequest(method=EthCall.CALL, params=(transaction,number,))
    return In3Runtime.call_in3_rpc(rpc)


def estimate_gas(transaction: Transaction):
    rpc = RPCRequest(method=EthCall.ESTIMATE_TRANSACTION, params=(transaction,))
    return In3Runtime.call_in3_rpc(rpc)


def get_block_by_hash(hash_obj: Hash, is_full: bool = True) -> Block:
    """
    Returns information about a block by hash.

    Args:
        hash_obj (Hash):  Block hash of the block to retrive information
        is_full (bool): If true it returns the full transaction objects, if false only the hashes of the transactions.
    Returns:
        block (Block): Block object related to the block hash

    """
    rpc = RPCRequest(method=EthCall.BLOCK_BY_HASH, params=(hash_obj, is_full,))
    return In3Runtime.call_in3_rpc(rpc)


def get_block_by_number(number: [int, BlockStatus, str], is_full: bool = True) -> Block:
    rpc = RPCRequest(method=EthCall.BLOCK_BY_NUMBER, params=(number, is_full,))
    return In3Runtime.call_in3_rpc(rpc)


def get_transaction_by_hash(tx_hash: Hash):
    rpc = RPCRequest(method=EthCall.TRANSACTION_BY_HASH, params=(tx_hash,))
    return In3Runtime.call_in3_rpc(rpc)


def get_transaction_by_block_hash_and_index(block_hash: Hash, index: int):
    rpc = RPCRequest(method=EthCall.TRANSACTION_BY_BLOCKHASH_AND_INDEX, params=(block_hash, index,))
    return In3Runtime.call_in3_rpc(rpc)


def get_transaction_by_block_number_and_index(number: [int, BlockStatus], index: int):
    rpc = RPCRequest(method=EthCall.TRANSACTION_BY_BLOCKNUMBER_AND_INDEX, params=(number, index,))
    return In3Runtime.call_in3_rpc(rpc)


def get_transaction_receipt(tx_hash: Hash):
    rpc = RPCRequest(method=EthCall.TRANSACTION_RECEIPT, params=(tx_hash,))
    return In3Runtime.call_in3_rpc(rpc)


# def pending_transactions():
#     rpc = RPCRequest(method=EthCall.PENDING_TRANSACTIONS)
#     return In3Runtime.call_in3_rpc(rpc)


# def get_uncle_by_block_hash_and_index(block_hash: Hash, index: int):
#     rpc = RPCRequest(method=EthCall.UNCLE_BY_BLOCKHASH_AND_INDEX, params=(block_hash, index,))
#     return In3Runtime.call_in3_rpc(rpc)
#
#
# def get_uncle_by_block_number_and_index(number: int, index: int):
#     rpc = RPCRequest(method=EthCall.UNCLE_BY_BLOCKNUMBER_AND_INDEX, params=(number, index,))
#     return In3Runtime.call_in3_rpc(rpc)


def new_filter(filter: Filter):
    rpc = RPCRequest(method=EthCall.NEW_FILTER, params=(filter,))
    return In3Runtime.call_in3_rpc(rpc)


def new_block_filter():
    rpc = RPCRequest(method=EthCall.NEW_BLOCK_FILTER)
    return In3Runtime.call_in3_rpc(rpc)


# def new_pending_transaction_filter():
#     rpc = RPCRequest(method=EthCall.NEW_PENDING_TRANSACTION_FILTER)
#     return In3Runtime.call_in3_rpc(rpc)


def uninstall_filter(filter_id: int):
    rpc = RPCRequest(method=EthCall.UNINSTALL_FILTER, params=(filter_id,))
    return In3Runtime.call_in3_rpc(rpc)


def get_filter_changes(filter_id: int):
    rpc = RPCRequest(method=EthCall.FILTER_CHANGES, params=(filter_id,))
    return In3Runtime.call_in3_rpc(rpc)


# def get_filter_logs(filter_id: int):
#     rpc = RPCRequest(method=EthCall.FILTER_LOGS, params=(filter_id,))
#     return In3Runtime.call_in3_rpc(rpc)


def get_logs(from_filter: Filter):
    rpc = RPCRequest(method=EthCall.LOGS, params=(from_filter,))
    return In3Runtime.call_in3_rpc(rpc)
