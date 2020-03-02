from __future__ import annotations

import json

from in3.model.exception import In3RequestException
from in3.model.enum import EthCall
from in3.bind.lib_loader import in3_create, in3_free, in3_exec
from in3.model.client import RPCResponse, RPCRequest


class _In3Shared:

    def __init__(self):
        self.in3 = in3_create()

    def call(self, rpc):
        return in3_exec(self.in3, rpc)

    def release(self):
        in3_free(self.in3)


class In3Runtime:
    """
    TODO: error to fix in in3 core c > methods names
    """

    __INSTANCE = None

    @staticmethod
    def get_instance() -> _In3Shared:
        if In3Runtime.__INSTANCE is None:
            In3Runtime.__INSTANCE = _In3Shared()
        return In3Runtime.__INSTANCE

    @staticmethod
    def call_in3_rpc(request: RPCRequest):

        response = In3Runtime.get_instance().call(rpc=request.to_utf8())
        response_dict = json.loads(response.decode('utf8').replace('\n', ' '))

        rpc_response = RPCResponse()
        rpc_response.__dict__.update(response_dict)

        if rpc_response.error is not None:
            raise In3RequestException(rpc_response.error)

        def get_as_block():
            from in3.model.client import Block
            return Block.from_json(json_response=rpc_response.result)

        def get_as_number():
            return int(rpc_response.result, 16)

        def get_as_transaction():
            from in3.model.client import Transaction
            return Transaction.from_json(json_response=rpc_response.result)

        if request.method in [EthCall.BALANCE, EthCall.GAS_PRICE, EthCall.BLOCK_NUMBER,EthCall.ESTIMATE_TRANSACTION,
                              EthCall.TRANSACTION_COUNT]:
            return get_as_number()

        if request.method in [EthCall.BLOCK_BY_NUMBER, EthCall.BLOCK_BY_HASH]:
            return get_as_block()

        if request.method in [EthCall.TRANSACTION_BY_HASH, EthCall.TRANSACTION_BY_BLOCKNUMBER_AND_INDEX, EthCall.TRANSACTION_BY_BLOCKHASH_AND_INDEX, EthCall.TRANSACTION_RECEIPT]:
            return get_as_transaction()

        return rpc_response.result
