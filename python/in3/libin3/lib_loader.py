"""
Load libin3 shared library for the current system, map function ABI, sets in3 network transport functions.

Example of RPC to In3-Core library, In3 Network and back.
```
+----------------+                               +----------+                       +------------+                        +------------------+
|                | in3.client.eth.block_number() |          |     in3_client_rpc    |            |  In3 Network Request   |                  |e
|     python     +------------------------------>+  python  +----------------------->   libin3   +------------------------>     python       |
|   application  |                               |   in3    |                       |  in3-core  |                        |  http_transport  |
|                <-------------------------------+          <-----------------------+            <------------------------+                  |
+----------------+     primitive or Object       +----------+     ctype object      +------------+  in3_req_add_response  +------------------+
```
"""
import ctypes as c
import platform

from pathlib import Path


def libin3_new(chain_id: int, transport: c.CFUNCTYPE, debug=False) -> int:
    """
    Instantiate new In3 Client instance.
    Args:
        chain_id (int): Chain id as integer
        transport: Transport function for the in3 network requests
        debug: Turn on debugger logging
    Returns:
         instance (int): Memory address of the client instance, return value from libin3_new
    """
    assert isinstance(chain_id, int)
    global libin3
    _map_function_signatures()
    # define this function as the transport for in3 requests from client to server and back
    libin3.in3_set_default_transport(transport)
    # register transport and verifiers (needed only once)
    libin3.in3_register_eth_full()
    # TODO: IPFS libin3.in3_register_ipfs();
    libin3.in3_register_eth_api()
    # TODO: in3_set_storage_handler(c, storage_get_item, storage_set_item, storage_clear, NULL);
    # enable logging
    if debug:
        # set logger level to TRACE
        libin3.in3_log_set_quiet_(False)
        libin3.in3_log_set_level_(0)
    return libin3.in3_for_chain_auto_init(chain_id)


def libin3_free(instance: int):
    """
    Free In3 Client objects from memory.
    Args:
        instance (int): Memory address of the client instance, return value from libin3_new
    """
    libin3.in3_free(instance)


def libin3_exec(instance: int, rpc: bytes):
    """
    Make Remote Procedure Call mapped methods in the client.
    Args:
        instance (int): Memory address of the client instance, return value from libin3_new
        rpc (bytes): Serialized function call, a json string.
    Returns:
        returned_value (object): The returned function value(s)
    """
    ptr_res = libin3.in3_client_exec_req(instance, rpc)
    result = c.cast(ptr_res, c.c_char_p).value
    libin3._free_(ptr_res)
    return result


def libin3_call(instance: int, fn_name: bytes, fn_args: bytes) -> (str, str):
    """
    Make Remote Procedure Call to an arbitrary method of a libin3 instance
    Args:
        instance (int): Memory address of the client instance, return value from libin3_new
        fn_name (bytes): Name of function that will be called in the client rpc.
        fn_args: (bytes) Serialized list of arguments, matching the parameters order of this function. i.e. ['0x123']
    Returns:
        result (int): Function execution status.
    """
    response = c.c_char_p()
    error = c.c_char_p()
    result = libin3.in3_client_rpc(instance, fn_name, fn_args, c.byref(response), c.byref(error))
    return result, response.value, error.value


def libin3_set_pk(instance: int, private_key: bytes):
    """
    Register the signer module in the In3 Client instance, with selected private key loaded in memory.
    Args:
        instance (int): Memory address of the client instance, return value from libin3_new
        private_key: 256 bit number.
    """
    libin3.eth_set_pk_signer_hex(instance, private_key)


def _multi_platform_selector(prefix: str) -> str:
    """
    Helper to define the path of installed shared libraries.
    Returns:
        libin3_file_path (pathlib.Path): Path to the correct library, compiled to the current platform.
    """
    path = Path(__file__).parent
    system, node, release, version, machine, processor = platform.uname()

    def fail():
        raise OSError('Not available on this platform ({}, {}, {}).'.format(
            system, processor, machine))

    # Fail over
    if not processor:
        processor = 'i386'
    # Similar behavior could be achieved with regex expressions if we known them better.
    suffix = None
    if processor == 'i386' or 'Intel' in processor:
        # AMD64 x86_64 64bit ...
        if '64' in machine:
            if system == 'Windows':
                suffix = "x64.dll"
            elif system == "Linux":
                suffix = "x64.so"
            elif system == 'Darwin':
                suffix = "x64.dylib"
        elif '32' in machine or '86' in machine:
            if system == 'Windows':
                fail()
            elif system == "Linux":
                suffix = "x86.so"
            elif system == 'Darwin':
                fail()
        elif 'armv' in machine:
            suffix = 'arm7.so'
    elif 'ARM' in processor:
        if machine == 'ARM7':
            suffix = 'arm7.so'
    if not suffix:
        fail()
    return str(Path(path.parent, "libin3", "shared", "{}.{}".format(prefix, suffix)))


def _map_function_signatures():
    # =================== LIBIN3 SHARED LIBRARY MAPPING ===================
    global libin3
    # map new in3
    libin3.in3_for_chain_auto_init.argtypes = [c.c_int]
    libin3.in3_for_chain_auto_init.restype = c.c_void_p
    libin3.in3_for_chain_default.argtypes = [c.c_int]
    libin3.in3_for_chain_default.restype = c.c_void_p
    # map free in3
    libin3.in3_free.argtypes = c.c_void_p,
    libin3.in3_free.restype = None
    libin3._free_.argtypes = c.c_void_p,
    libin3._free_.restype = None
    # map set pk signer
    libin3.eth_set_pk_signer_hex.argtypes = [c.c_void_p, c.c_char_p]
    libin3.eth_set_pk_signer_hex.restype = None
    # map transport request function
    libin3.in3_client_exec_req.argtypes = [c.c_void_p, c.c_char_p]
    libin3.in3_client_exec_req.restype = c.c_void_p
    libin3.in3_client_rpc.argtypes = [c.c_void_p, c.c_char_p, c.c_char_p,
                                      c.POINTER(c.c_char_p),
                                      c.POINTER(c.c_char_p)]
    libin3.in3_client_rpc.restype = c.c_int
    # map transport function for response
    libin3.in3_req_add_response.argtypes = [c.c_void_p, c.c_int, c.c_bool, c.c_char_p, c.c_int]
    libin3.in3_req_add_response.restype = None
    # map logging functions
    libin3.in3_log_set_quiet_.argtypes = c.c_bool,
    libin3.in3_log_set_quiet_.restype = None
    libin3.in3_log_set_level_.argtypes = c.c_int,
    libin3.in3_log_set_level_.restype = None


def init():
    """
    Loads library depending on host system.
    """
    return c.cdll.LoadLibrary(_multi_platform_selector('libin3'))


libin3 = init()
