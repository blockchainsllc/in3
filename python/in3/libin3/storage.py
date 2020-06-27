import ctypes as c


class NativeRequest(c.Structure):
    """
    Request sent by the libin3 to the In3 Network, transported over the _http_transport function
    Based on in3/client/.h in3_request_t struct
    """
    """
    _fields_ = [("payload", c.POINTER(c.c_char)),
                ("urls", c.POINTER(c.POINTER(c.c_char))),
                ("urls_len", c.c_int),
                ("results", c.c_void_p),
                ("timeout", c.c_uint32),
                ("times", c.c_uint32)]
    """
#
#     @c.CFUNCTYPE(c.c_void_p, c.POINTER(c.c_int), c.POINTER(c.c_char_p), c.POINTER(c.c_char))
#
#     @c.CFUNCTYPE(c.c_ubyte, c.POINTER(c.c_int), c.POINTER(c.c_char))
#
#     @c.CFUNCTYPE(c.c_void_p, c.POINTER(c.c_int))
#
# def factory(transport_fn):
#     """
#     C level abstraction of a transport handler.
#     Decorates a transport function augmenting its capabilities for native interoperability
#     """
#
#     def new(native_request: NativeRequest):
#         request = In3Request(native_request)
#         response = In3Response(native_request)
#         return transport_fn(request, response)
#
#     # the transport function to be implemented by the transport provider.
#     # typedef in3_ret_t (*in3_transport_send)(in3_request_t* request);
#     c_transport_fn = c.CFUNCTYPE(c.c_int, c.POINTER(NativeRequest))
#     return c_transport_fn(new)
