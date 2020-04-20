import json

from enum import Enum
from in3.exception import ClientException
from in3.libin3.lib_loader import libin3_new, libin3_free, libin3_call, libin3_set_pk


class RPCRequest:
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
            self.fn_args = ''.encode('utf8')


class RPCResponse:
    # TODO: Verify need
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

    def call(self, fn_name: str or Enum, *fn_args, formatted: bool = False) -> str or dict:
        """
        Make a remote procedure call to a function in libin3
        Args:
            fn_name (str or Enum): Name of the function to be called
            *fn_args: Arguments matching the parameters order of this function
            formatted (bool): True if args must be sent as-is to RPC endpoint
        Returns:
            fn_return (str): String of values returned by the function, if any.
        """

        """
        typedef enum {
          /* On success positive values (impl. defined) upto INT_MAX maybe returned */
          IN3_OK                = 0,   /**< Success */
          IN3_EUNKNOWN          = -1,  /**< Unknown error - usually accompanied with specific error msg */
          IN3_ENOMEM            = -2,  /**< No memory */
          IN3_ENOTSUP           = -3,  /**< Not supported */
          IN3_EINVAL            = -4,  /**< Invalid value */
          IN3_EFIND             = -5,  /**< Not found */
          IN3_ECONFIG           = -6,  /**< Invalid config */
          IN3_ELIMIT            = -7,  /**< Limit reached */
          IN3_EVERS             = -8,  /**< Version mismatch */
          IN3_EINVALDT          = -9,  /**< Data invalid, eg. invalid/incomplete JSON */
          IN3_EPASS             = -10, /**< Wrong password */
          IN3_ERPC              = -11, /**< RPC error (i.e. in3_ctx_t::error set) */
          IN3_ERPCNRES          = -12, /**< RPC no response */
          IN3_EUSNURL           = -13, /**< USN URL parse error */
          IN3_ETRANS            = -14, /**< Transport error */
          IN3_ERANGE            = -15, /**< Not in range */
          IN3_WAITING           = -16, /**< the process can not be finished since we are waiting for responses */
          IN3_EIGNORE           = -17, /**< Ignorable error */
          IN3_EPAYMENT_REQUIRED = -18, /**< payment required */
        } in3_ret_t;
        """
        request = RPCRequest(fn_name, fn_args, formatted)
        result, response, error = libin3_call(self.in3, request.fn_name, request.fn_args)
        if result < 0 or error:
            with open('error.log', 'a+') as log_file:
                log_file.write(error)
            raise ClientException(error)
        # response_dict = json.loads(result)
        # response = RPCResponse(**response_dict)
        return json.loads(response)

    def set_signer(self, private_key: str):
        libin3_set_pk(self.in3, private_key)