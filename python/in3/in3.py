from in3.model.enum import In3Methods, EthCall
from in3.model.client import Config, RPCRequest, Address, Transaction
from in3.bind.runtime import In3Runtime
import math

def config(in3_config: Config):
    rpc = RPCRequest(In3Methods.CONFIG, params=(in3_config,))
    return In3Runtime.call_in3_rpc(rpc)


def checksum_address(address: str, chain: bool) -> Address:
    rpc = RPCRequest(In3Methods.CHECKSUM_ADDRESS, params=(str(address), chain,))
    return In3Runtime.call_in3_rpc(rpc)

def abi_encode(method: str, args: list) -> int:
    rpc = RPCRequest(In3Methods.ABI_ENCODE, params=(method, args,))
    return In3Runtime.call_in3_rpc(rpc)

def abi_decode(method: str, args: list) -> int:
    rpc = RPCRequest(In3Methods.ABI_DECODE, params=(method, args,))
    return In3Runtime.call_in3_rpc(rpc)

def node_list():
    rpc = RPCRequest(In3Methods.IN3_NODE_LIST)
    return In3Runtime.call_in3_rpc(rpc)

def node_stats():
    rpc = RPCRequest(In3Methods.IN3_STATS)
    return In3Runtime.call_in3_rpc(rpc)

def prepare_transaction(transaction:Transaction):
    dict = transaction.__dict__
    items = ["nonce", "gasPrice", "gas", "to", "value"]
    data = transaction.input




def send_transaction(signed_transaction:str):
    rpc = RPCRequest(method=EthCall.SEND_RAW_TRANSACTION, params=(signed_transaction,))
    return In3Runtime.call_in3_rpc(rpc)

