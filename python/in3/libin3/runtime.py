import json

from enum import Enum
from in3.exception import ClientException
from in3.libin3.lib_loader import libin3_new, libin3_free, libin3_call


class RPCRequest:
    """
    RPC request to libin3
    Args:
        fn_name: Name of function that will be called in libin3
        args: Arguments matching the parameters order of this function
        _id: (optional) Message id
        rpc_version: (optional) Version of the json-rpc api
    """

    def __init__(self, fn_name: str, args: tuple = (), verification: str = "proof", _id: int = 1,
                 rpc_version: str = "2.0"):
        self.id = _id
        self.rpc_version = rpc_version
        self.libin3_function = fn_name
        self.args = args
        self.in3_headers = {
            "verification": verification,
            "version": "2.1.0"
        }

    def __bytes__(self):
        """
        [{"id":1,"jsonrpc":"2.0","method":"in3_nodeList","params":[],"in3":{"verification":"proof","version": "2.1.0"}}]
        """
        return json.dumps({
            "id": self.id,
            "jsonrpc": self.rpc_version,
            "method": self.libin3_function,
            "params": [arg for arg in self.args],
            "in3": self.in3_headers
        }).encode('utf8')


class RPCResponse:
    """
    RPC response from libin3.
    Args:
        id: Message id
        jsonrpc: Version of the json-rpc api
        result: Function returned value(s)
        error: (Optional) Filled in case of error
    """

    def __init__(self, id: str, jsonrpc: str, result: str = None, error: str = None):
        self.id = id
        self.jsonrpc = jsonrpc
        self.result = result
        self.error = error


class In3Runtime:
    """
    Instantiate libin3 and frees it when garbage collected.
    Args:
        timeout (int): Time for http request connection and content timeout in milliseconds
    """

    def __init__(self, timeout: int):
        self.in3 = libin3_new(timeout)

    def __del__(self):
        libin3_free(self.in3)

    def call(self, fn_name: str or Enum, *args) -> str or dict:
        """
        Make a remote procedure call to a function in libin3
        Args:
            fn_name (str or Enum): Name of the function to be called
            *args: Arguments matching the parameters order of this function
        Returns:
            fn_return (str): String of values returned by the function, if any.
        """
        request = RPCRequest(str(fn_name), args=args)
        request_bytes = bytes(request)
        response_bytes = libin3_call(self.in3, rpc=request_bytes)
        response_dict = json.loads(response_bytes.decode('utf8').replace('\n', ' '))
        response = RPCResponse(**response_dict)
        if response.error is not None:
            raise ClientException(response.error)
        return response.result
