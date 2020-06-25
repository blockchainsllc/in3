"""
Encapsulates low-level rpc calls into a comprehensive runtime.
"""
import ctypes as c
import json
from enum import Enum

from in3.exception import ClientException
from in3.libin3.enum import RPCCode
from in3.libin3.lib_loader import libin3_new, libin3_free, libin3_call, libin3_exec, libin3_set_pk, libin3


class RPCExecRequest:
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


class RPCExecResponse:
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
        chain_id (int): Chain-id based on EIP-155. If None provided, will connect to the Ethereum network. i.e: 0x1 for mainNet
    """

    def __init__(self, chain_id: int, transport):
        # This is needed to prevent the handler function to be freed. Without it, there would be a fault.
        self.in3_transport = create_in3_transport(transport)
        self.in3 = libin3_new(chain_id, self.in3_transport)
        self.chain_id = chain_id

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

    def set_signer_account(self, secret: int) -> int:
        """
        Load an account secret to sign Ethereum transactions with `eth_sendTransaction` and `eth_call`.
        Args:
            secret: SK 256 bit number. example: int(0x387a8233c96e1fc0ad5e284353276177af2186e7afa85296f106336e376669f7, 16)
        """
        return libin3_set_pk(self.in3, hex(secret).encode('utf8'))

    def execute(self, fn_name: str or Enum, *args) -> str or dict:
        """
        Make a remote procedure call to a function in libin3
        Args:
            fn_name (str or Enum): Name of the function to be called
            *args: Arguments matching the parameters order of this function
        Returns:
            fn_return (str): String of values returned by the function, if any.
        """
        request = RPCExecRequest(str(fn_name), args=args)
        request_bytes = bytes(request)
        response_bytes = libin3_exec(self.in3, rpc=request_bytes)
        response_str = response_bytes.decode('utf8').replace('\n', ' ')
        if 'error' in response_str:
            raise ClientException(response_str)
        response_dict = json.loads(response_str)
        response = RPCExecResponse(**response_dict)
        return response.result


class NativeRequest(c.Structure):
    """
    Request sent by the libin3 to the In3 Network, transported over the _http_transport function
    Based on in3/client/.h in3_request_t struct
    """


class In3Request:
    """
    Higher level abstraction for an Incubed request.
    """

    def __init__(self, in3_request: NativeRequest):
        self.in3_request = in3_request

    def url_at(self, index: int):
        """
        Gets the `index` url on the request url node list.
        Args:
            index (int): Positional argument to retrieve the url of a node from the list of urls. The total length should be retreived with `urls_len`
        Returns:
            fn_return (str): The url of a node to request a response from.
        """
        return c.string_at(libin3.in3_get_request_urls(self.in3_request)[index])

    def urls_len(self):
        """
        Gets the the size of the request url node list
        """
        return libin3.in3_get_request_urls_len(self.in3_request)

    def payload(self):
        """
        Gets the payload to be sent
        """
        return c.string_at(libin3.in3_get_request_payload(self.in3_request))

    def timeout(self):
        """
        Get timeout of the request, `0` being no set timeout
        """
        return libin3.in3_get_request_timeout(self.in3_request)


class In3Response:
    """
    Higher level abstraction for an Incubed response.
    """

    def __init__(self, in3_request: NativeRequest):
        self.in3_request = in3_request

    def success(self, index: int, msg: bytes):
        """
        Function to be invoked in order to write the result for the request in case of success
        Args:
            index (int): Positional argument related to which url on the `In3Request` list this response is associated with. Use `In3Request#url_at` to get the url. The value of both parameters are shared
            msg (str): The actual response to be returned to in3 client
        """
        libin3.in3_req_add_response(self.in3_request, index, False, msg, len(msg))

    def failure(self, index: int, msg: bytes):
        """
        Function to be invoked in order to write the result for the request in case of failure
        Args:
            index (int): Positional argument related to which url on the `In3Request` list this response is associated with. Use `In3Request#url_at` to get the url. The value of both parameters are shared.
            msg (str): The actual response to be returned to in3 client.
        """
        libin3.in3_req_add_response(self.in3_request, index, True, msg, len(msg))


def create_in3_transport(custom_transport):
    """
    Factory-like higher-order function that decorates a transport function augmenting its capabilities for native interoperability
    """

    def transport(native_request: NativeRequest):
        request = In3Request(native_request)
        response = In3Response(native_request)
        return custom_transport(request, response)

    TRANSPORT_HANDLER = c.CFUNCTYPE(c.c_int, c.POINTER(NativeRequest))
    return TRANSPORT_HANDLER(transport)
