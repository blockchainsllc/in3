"""
Load libin3 shared library for the current system, map function signatures, map and set transport functions.

Example of RPC to In3-Core library, In3 Network and back.
```
+----------------+                               +----------+                       +------------+                        +------------------+
|                | in3.client.eth.block_number() |          |  in3_client_exec_req  |            |  In3 Network Request   |                  |e
|     python     +------------------------------>+  python  +----------------------->   libin3   +------------------------>     python       |
|   application  |                               |   in3    |                       |  in3-core  |                        |  http_transport  |
|                <-------------------------------+          <-----------------------+            <------------------------+                  |
+----------------+     primitive or Object       +----------+     ctype object      +------------+  in3_req_add_response  +------------------+
```
"""
from ctypes import POINTER, CFUNCTYPE
from ctypes import c_char, c_int, c_void_p, c_uint32, c_char_p, c_bool
from ctypes import Structure, string_at, cast, cdll
from pathlib import Path

import math
import platform
import requests

DEBUG: bool = False
TIMEOUT = 5000


class In3Request(Structure):
    """
    Request sent by the libin3 to the In3 Network, transported over the _http_transport function
    Based on in3/client/.h in3_request_t struct
    Attributes:
        payload (str): the payload to send
        urls ([str]): array of urls
        urls_len (int): number of urls
        results (str): the responses
        timeout (int): the timeout 0= no timeout
        times (int): measured times (in ms) which will be used for ajusting the weights
    """
    _fields_ = [("payload", POINTER(c_char)),
                ("urls", POINTER(POINTER(c_char))),
                ("urls_len", c_int),
                ("results", c_void_p),
                ("timeout", c_uint32),
                ("times", POINTER(c_uint32))]


def libin3_new(timeout: int):
    """
    RPC to free libin3 objects in memory.
    Args:
        timeout (int): Time in milliseconds for http requests to fail due to timeout
    Returns:
         instance (int): Memory address of the shared library instance, return value from libin3_new
    """
    global TIMEOUT
    TIMEOUT = float(timeout * math.pow(10, -3))
    return libin3.in3_new()


def libin3_free(instance: int):
    """
    RPC to free libin3 objects in memory.
    Args:
        instance (int): Memory address of the shared library instance, return value from libin3_new
    """
    libin3.in3_free(instance)


def libin3_call(instance: int, rpc: bytes):
    """
    Make Remote Procedure Call to an arbitrary method of a libin3 instance
    Args:
        instance (int): Memory address of the shared library instance, return value from libin3_new
        rpc (bytes): Serialized function call, a ethreum api json string.
    Returns:
        returned_value (object): The returned function value(s)
    """
    ptr_res = libin3.in3_client_exec_req(instance, rpc)
    result = cast(ptr_res, c_char_p).value
    libin3._free_(ptr_res)
    return result


def _transport_report_success(in3_request: In3Request, i: int, response: requests.Response):
    libin3.in3_req_add_response(
        in3_request.results, i, False, response.content, -1)


def _transport_report_failure(in3_request: In3Request, i: int, err: Exception):
    libin3.in3_req_add_response(in3_request.results, i, True, str(err), -1)


def _http_transport(in3_request: In3Request):
    """
    Transports each request coming from libin3 to the in3 network and and reports the answer back
    Args:
        in3_request (In3Request): request sent by the In3 Client Core to the In3 Network
    Returns:
        exit_status (int): Always zero for signaling libin3 the function executed OK.
    """
    in3_request = in3_request.contents

    for i in range(0, in3_request.urls_len):
        try:
            rpc_request = {
                'url': string_at(in3_request.urls[i]),
                'data': string_at(in3_request.payload),
                'headers': {'Content-type': 'application/json'},
                'timeout': TIMEOUT
            }
            response = requests.post(**rpc_request)
            response.raise_for_status()
            _transport_report_success(in3_request, i, response)
        except requests.exceptions.RequestException as err:
            _transport_report_failure(in3_request, i, err)
        except requests.exceptions.SSLError as err:
            _transport_report_failure(in3_request, i, err)
        except Exception as err:
            _transport_report_failure(in3_request, i, err)
    return 0


def _multi_platform_selector() -> str:
    """
    Helper to define the path of installed shared libraries. In this case libin3.
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
    extension = None
    if processor == 'i386' or 'Intel' in processor:
        # AMD64 x86_64 64bit ...
        if '64' in machine:
            if system == 'Windows':
                extension = "x64.dll"
            elif system == "Linux":
                extension = "x64.so"
            elif system == 'Darwin':
                extension = "x64.dylib"
        elif '32' in machine or '86' in machine:
            if system == 'Windows':
                fail()
            elif system == "Linux":
                extension = "x32.so"
            elif system == 'Darwin':
                fail()
        elif 'armv' in machine:
            extension = 'arm7.so'
    elif 'ARM' in processor:
        if machine == 'ARM7':
            extension = 'arm7.so'

    if not extension:
        fail()

    if system == 'Windows':
        return Path(path.parent, "libin3", "shared", "in3.{}".format(extension))
    elif DEBUG:
        # Debug only available on mac.
        # If you need to it run on your system, run in3-core/scripts/build_debug.sh to get a build.
        # Then add it to libin3/shared folder
        return str(Path(path.parent, "libin3", "shared",  "libin3d.{}".format(extension)))
    else:
        return str(Path(path.parent, "libin3", "shared", "libin3.{}".format(extension)))


# =================== LIBIN3 SHARED LIBRARY MAPPING ===================
libin3 = cdll.LoadLibrary(_multi_platform_selector())
# map free
libin3._free_.argtypes = c_void_p,
libin3._free_.restype = None
# map new in3
libin3.in3_new.argtypes = []
libin3.in3_new.restype = c_void_p
# map free in3
libin3.in3_free.argtypes = c_void_p,
libin3.in3_free.restype = None
# map transport request function
libin3.in3_client_exec_req.argtypes = [c_void_p, c_char_p]
libin3.in3_client_exec_req.restype = c_void_p
# map transport function for response
libin3.in3_req_add_response.argtypes = [
    c_void_p, c_int, c_bool, c_void_p, c_int]
libin3.in3_req_add_response.restype = None
# create a transport function pointer
transport_fn = CFUNCTYPE(c_int, POINTER(In3Request))(_http_transport)
# define this function as default transport for in3 requests from client to server and back
libin3.in3_set_default_transport(transport_fn)
# register transport and verifiers (needed only once)
libin3.in3_register_eth_full()
libin3.in3_register_eth_api()
# TODO: in3_set_storage_handler(c, storage_get_item, storage_set_item, storage_clear, NULL);
# enable logging
if DEBUG:
    # map logging functions
    libin3.in3_log_set_quiet_.argtypes = c_bool,
    libin3.in3_log_set_quiet_.restype = None
    libin3.in3_log_set_level_.argtypes = c_int,
    libin3.in3_log_set_level_.restype = None
    # set logger level to TRACE
    libin3.in3_log_set_quiet_(False)
    libin3.in3_log_set_level_(0)
