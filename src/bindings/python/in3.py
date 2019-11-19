from ctypes import *

lib = cdll.LoadLibrary('../../../build/lib/libin3.dylib')

# free
lib._free_.argtypes = c_void_p,
lib._free_.restype = None

# new in3
lib.in3_new.argtypes = []
lib.in3_new.restype = c_void_p

# exec
lib.in3_client_exec_req.argtypes = [c_void_p,POINTER(c_char)]
lib.in3_client_exec_req.restype = c_void_p

lib.in3_log_set_quiet_.argtypes = c_bool,
lib.in3_log_set_quiet_.restype = None
# test

# create the in3-instance

def in3_create():
    lib.in3_register_eth_full()
    lib.in3_register_eth_api()
    lib.in3_register_curl()
    lib.in3_log_set_quiet_(True)

    return lib.in3_new()

def in3_exec(in3_ptr, rpc):
    ptr_res = lib.in3_client_exec_req(in3_ptr, rpc)
    result = cast(ptr_res, c_char_p).value
    lib._free_(ptr_res)
    return result

in3 = in3_create()

# run a rpc
print("Result:", in3_exec(in3,"{\"method\":\"eth_getBlockByNumber\",\"params\":[\"latest\",false]}"))
