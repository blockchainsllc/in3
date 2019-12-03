from ctypes import *
import requests

lib = cdll.LoadLibrary('../../../build/lib/libin3.dylib')

# free
lib._free_.argtypes = c_void_p,
lib._free_.restype = None

# new in3
lib.in3_new.argtypes = []
lib.in3_new.restype = c_void_p

#free in3
lib.in3_free.argtypes = c_void_p,
lib.in3_free.restype = None


# exec
lib.in3_client_exec_req.argtypes = [c_void_p,POINTER(c_char)]
lib.in3_client_exec_req.restype = c_void_p

# turn off logging
lib.in3_log_set_quiet_.argtypes = c_bool,
lib.in3_log_set_quiet_.restype = None



## adding a transport function..

# the request struct
class Request(Structure):
    _fields_ = [("payload", POINTER(c_char)),
               ("urls", POINTER(POINTER(c_char))),
               ("urls_len", c_int),
               ("results", c_void_p)]

# declaring the function to report
lib.in3_req_add_response.argtypes = [c_void_p,c_int,c_bool,c_void_p,c_int]
lib.in3_req_add_response.restype = None

# the function called for each request
def transport(req):
    for i in range(0,req[0].urls_len):
        try:
            res = requests.post(string_at(req[0].urls[i]), data=string_at(req[0].payload), headers={'Content-type': 'application/json'})
            res.raise_for_status()
            lib.in3_req_add_response(req[0].results, i, False, c_char_p(res.content.decode("utf-8")) ,-1)
        except requests.exceptions.RequestException as e:
            lib.in3_req_add_response(req[0].results, i, True, e.strerror,-1)
    return 0

# define a transport function as callback
transport_fn = CFUNCTYPE(c_int, POINTER(Request))(transport)
# and set it as default
lib.in3_set_default_transport(transport_fn)

# register transport and verifiers (needed only once)
lib.in3_register_eth_full()
lib.in3_register_eth_api()
lib.in3_log_set_quiet_(True)

# test

# create the in3-instance
def in3_create():
    return lib.in3_new()

# free the in3-instance
def in3_free(in3_ptr):
    lib.in3_free(in3_ptr)

# execute a rpc-request
def in3_exec(in3_ptr, rpc):
    ptr_res = lib.in3_client_exec_req(in3_ptr, rpc)
    result = cast(ptr_res, c_char_p).value
    lib._free_(ptr_res)
    return result


#example....

# create a client
in3 = in3_create()

# run some rpc-requests
print("current block:", in3_exec(in3,"{\"method\":\"eth_blockNumber\",\"params\":[]}"))

# clean up when done
in3_free(in3)
