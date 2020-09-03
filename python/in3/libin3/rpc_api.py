"""
Load libin3 shared library for the current system, map function ABI, sets in3 network transport functions.
"""
import ctypes as c
import platform
from pathlib import Path

from in3.libin3.storage import get_item, set_item, clear

DEBUG = False


def _load_shared_library() -> c.CDLL:
    """
    Loads library depending on host system.
    """

    def platform_selector(prefix: str, lib_path: str) -> c.CDLL:
        system, node, release, version, machine, processor = platform.uname()

        global DEBUG
        suffix = None
        if processor in ('i386', 'x86_64') or 'Intel' in processor or 'AMD' in processor:
            if '64' in machine:
                suffix = 'x64'
            elif '32' in machine or '86' in machine:
                suffix = 'x86'
            elif 'arm' in machine.lower():
                suffix = 'arm7'
            if DEBUG:
                suffix += 'd'
            if system == 'Windows':
                suffix += ".dll"
            elif system == "Linux":
                suffix += ".so"
            elif system == 'Darwin':
                suffix += ".dylib"
        if not suffix:
            raise OSError()
        return c.cdll.LoadLibrary(str(Path(lib_path, "{}.{}".format(prefix, suffix))))

    def fallback_platform_selector(search_string: str) -> c.CDLL:
        import glob

        system, node, release, version, machine, processor = platform.uname()
        lib_list = glob.glob(search_string)
        for lib in lib_list:
            try:
                lib_instance = c.cdll.LoadLibrary(lib)
                return lib_instance
            except Exception:
                pass
        raise OSError('Not available on this platform ({}, {}, {}).'.format(
            system, processor, machine))

    path = Path(Path(__file__).parent, "shared")
    try:
        return platform_selector('libin3', path)
    except OSError:
        return fallback_platform_selector(str(path) + '/*')


_libin3 = _load_shared_library()


def libin3_new(chain_id: int, cache_enabled: bool, transport_fn: c.CFUNCTYPE) -> int:
    """
    Instantiate new In3 Client instance.
    Args:
        chain_id (int): Chain id as integer
        cache_enabled (bool): False will disable local storage cache.
        transport_fn: Transport function for the in3 network requests
        storage_fn: Cache Storage function for node list and requests caching
    Returns:
         instance (int): Memory address of the client instance, return value from libin3_new
    """
    assert isinstance(chain_id, int)
    # ctypes mappings
    _libin3.in3_req_add_response.argtypes = c.c_void_p, c.c_int, c.c_bool, c.c_char_p, c.c_int, c.c_uint32
    _libin3.in3_init()
    # In3 init and module loading
    _libin3.in3_set_default_legacy_transport(transport_fn)
    global DEBUG
    if DEBUG:
        # set logger level to TRACE
        _libin3.in3_log_set_quiet_(False)
        _libin3.in3_log_set_level_(0)
    _libin3.in3_for_chain_auto_init.argtypes = c.c_int,
    _libin3.in3_for_chain_auto_init.restype = c.c_void_p
    _libin3.in3_register_eth_full.argtypes = c.c_void_p,
    _libin3.in3_register_eth_api.argtypes = c.c_void_p,
    instance = _libin3.in3_for_chain_auto_init(chain_id)
    # TODO: in3_set_default_signer
    _libin3.in3_register_eth_full(instance)
    # TODO: IPFS libin3.in3_register_ipfs();
    _libin3.in3_register_eth_api(instance)
    if cache_enabled:
        _libin3.in3_set_storage_handler.argtypes = c.c_void_p, c.c_void_p, c.c_void_p, c.c_void_p, c.c_void_p
        _libin3.in3_set_storage_handler(
            instance, get_item, set_item, clear, None)
    return instance


def libin3_free(instance: int):
    """
    Free In3 Client objects from memory.
    Args:
        instance (int): Memory address of the client instance, return value from libin3_new
    """
    _libin3.in3_free.argtypes = c.c_void_p,
    _libin3.in3_free(instance)


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
    _libin3.in3_client_rpc.argtypes = c.c_void_p, c.c_char_p, c.c_char_p, c.POINTER(
        c.c_char_p), c.POINTER(c.c_char_p)
    _libin3.in3_client_rpc.restype = c.c_int
    result = _libin3.in3_client_rpc(
        instance, fn_name, fn_args, c.byref(response), c.byref(error))
    return result, response.value, error.value


def libin3_set_pk(instance: int, private_key: bytes):
    """
    Register the signer module in the In3 Client instance, with selected private key loaded in memory.
    Args:
        instance (int): Memory address of the client instance, return value from libin3_new
        private_key: 256 bit number.
    """
    _libin3.eth_set_pk_signer_hex.argtypes = c.c_void_p, c.c_char_p
    _libin3.eth_set_pk_signer_hex(instance, private_key)


def libin3_in3_req_add_response(*args):
    """
    Transport function that registers a response to a request.
    Args:
        *args:
    """
    _libin3.in3_req_add_response(*args)


def libin3_new_bytes_t(value: bytes, length: int) -> int:
    """
    C Bytes struct
    Args:
        length: byte array length
        value: byte array
    Returns:
        ptr_addr: address of the instance of this struct
    """
    _libin3.b_new.argtypes = c.c_char_p, c.c_uint
    _libin3.b_new.restype = c.c_void_p
    return _libin3.b_new(value, length)
