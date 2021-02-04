"""
Encapsulates low-level rpc calls into a comprehensive runtime.
"""
import json
from enum import Enum

import in3.libin3.transport as transport
from in3.exception import ClientException
from in3.libin3.enum import RPCCode
from in3.libin3.rpc_api import libin3_new, libin3_free, libin3_call, libin3_set_pk


class RPCCallRequest:
    """
    RPC request to libin3
    Args:
        fn_name: Name of function that will be called in libin3
        fn_args: Arguments matching the parameters order of this function
    """

    def __init__(self, fn_name: str or Enum, fn_args: tuple = None, formatted: bool = False):
        self.fn_name = str(fn_name).encode('utf8')
        if fn_args and formatted:
            self.fn_args = fn_args[0].encode('utf8')
        elif fn_args and not formatted:
            self.fn_args = json.dumps(list(fn_args)).replace('\'', '').encode('utf8')
        else:
            self.fn_args = '[]'.encode('utf8')


class In3Runtime:
    """
    Instantiate libin3 and frees it when garbage collected.
    Args:
        chain_id (int): Chain-id based on EIP-155. Default is 0x1 for Ethereum mainNet.
        transport_fn: Transport function to handle the HTTP Incubed Network requests.
        cache_enabled (bool): False will disable local storage cache.
        deterministic_node_sel (bool): True will make node selection deterministic.
    """

    def __init__(self, chain_id: int, transport_fn, cache_enabled: bool = True, deterministic_node_sel: bool = False):
        self.chain_id = chain_id
        self.transport_handler = transport.factory(transport_fn)
        self.cache_enabled = cache_enabled
        if deterministic_node_sel:
            import warnings
            warnings.warn("IN3 HIGH SECURITY RISK - Use deterministic node selection for tests ONLY!", RuntimeWarning)
        self.in3 = libin3_new(chain_id, self.transport_handler, cache_enabled, deterministic_node_sel)

    def __del__(self):
        libin3_free(self.in3)

    def call(self, fn_name: str or Enum, *fn_args, formatted: bool = False) -> str or dict:
        """
        Make a remote procedure call to a function in libin3
        Args:
            fn_name (str or Enum): Name of the function to be called
            fn_args: Arguments matching the parameters order of this function
            formatted (bool): True if args must be sent as-is to RPC endpoint
        Returns:
            fn_return (str): String of values returned by the function, if any.
        """
        request = RPCCallRequest(fn_name, fn_args, formatted)
        result, response, error = libin3_call(self.in3, request.fn_name, request.fn_args)
        in3_code = RPCCode(result)
        if not in3_code == RPCCode.IN3_OK or error:
            raise ClientException(str(error))
        return json.loads(response)

    # TODO: Refactor for the new signer api
    def set_signer_account(self, secret: int) -> int:
        """
        Load an account secret to sign Ethereum transactions with `eth_sendTransaction` and `eth_call`.
        Args:
            secret: SK 256 bit number. example: int(0x387a8233c96e1fc0ad5e284353276177af2186e7afa85296f106336e376669f7, 16)
        """
        return libin3_set_pk(self.in3, hex(secret).encode('utf8'))
