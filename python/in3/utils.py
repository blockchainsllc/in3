import json

from in3.model.client import RPCRequest, to_dict
from in3.model.enum import Chain


def rpc_to_string(_rpc: RPCRequest) -> str:
    def params_to_value(params):
        ret = []
        for p in params:
            ret.append(to_dict(p))
        return ret
    aux = dict(_rpc)
    aux["method"] = _rpc.method.value
    aux["params"] = params_to_value(_rpc.params)

    return json.dumps(aux)


def get_chain_by_id(_chain_id: str) -> Chain:
    for c in Chain.__members__:
        if Chain[c].chain_id == _chain_id:
            return Chain[c]
