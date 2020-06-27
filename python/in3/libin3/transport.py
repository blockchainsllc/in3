import ctypes as c

from in3.libin3.lib_loader import libin3


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


def factory(custom_transport):
    """
    Factory-like higher-order function that decorates a transport function augmenting its capabilities for native interoperability
    """

    def new(native_request: NativeRequest):
        request = In3Request(native_request)
        response = In3Response(native_request)
        return custom_transport(request, response)

    # the transport function to be implemented by the transport provider.
    # typedef in3_ret_t (*in3_transport_send)(in3_request_t* request);
    custom_transport_signature = c.CFUNCTYPE(c.c_int, c.POINTER(NativeRequest))
    return custom_transport_signature(new)
