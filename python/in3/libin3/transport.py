import ctypes as c

from in3.libin3.enum import PluginAction
from in3.libin3.rpc_api import libin3_in3_req_add_response


class NativeRequest(c.Structure):
    """
    Based on in3/client/.h in3_request_t struct
    """
    _fields_ = [("payload", c.POINTER(c.c_char)),
                ("urls", c.POINTER(c.POINTER(c.c_char))),
                ("urls_len", c.c_int),
                ("results", c.c_void_p),
                ("timeout", c.c_uint32),
                ("times", c.c_uint32)]
    """
    /** request-object. 
     * 
     * represents a RPC-request
     */
    typedef struct in3_request {
      char*           payload;  /**< the payload to send */
      char**          urls;     /**< array of urls */
      uint_fast16_t   urls_len; /**< number of urls */
      struct in3_ctx* ctx;      /**< the current context */
      void*           cptr;     /**< a custom ptr to hold information during */
      uint32_t        wait;     /**< time in ms to wait before sending out the request */
    } in3_request_t;
    """


class NativeResponse(c.Structure):
    """
    Based on in3/client/.h in3_response_t struct
    """
    """
    typedef struct in3_response {
      sb_t error;  /**< a stringbuilder to add any errors! */
      sb_t result; /**< a stringbuilder to add the result */
    """
    """
        /**
         * adds a response for a request-object.
         * This function should be used in the transport-function to set the response.
         */
        NONULL void in3_req_add_response(
            in3_request_t* req,      /**< [in]the the request */
            int            index,    /**< [in] the index of the url, since this request could go out to many urls */
            bool           is_error, /**< [in] if true this will be reported as error. the message should then be the error-message */
            const char*    data,     /**<  the data or the the string*/
            int            data_len, /**<  the length of the data or the the string (use -1 if data is a null terminated string)*/
            uint32_t       time      /**<  the time this request took in ms or 0 if not possible (it will be used to calculate the weights)*/
        );
    """


class In3Request:
    """
    C level abstraction of an Incubed request.
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
        return c.string_at(self.in3_request.contents.urls[index])

    def urls_len(self):
        """
        Gets the the size of the request url node list
        """
        return self.in3_request.contents.urls_len

    def payload(self):
        """
        Gets the payload to be sent
        """
        return c.string_at(self.in3_request.contents.payload)

    def timeout(self):
        """
        Get timeout of the request, `0` being no set timeout
        """
        return self.in3_request.contents.timeout


class In3Response:
    """
    C level abstraction of an Incubed response.
    """

    def __init__(self, in3_response: NativeResponse):
        self.in3_response = in3_response

    def success(self, index: int, msg: bytes):
        """
        Function to be invoked in order to write the result for the request in case of success
        Args:
            index (int): Positional argument related to which url on the `In3Request` list this response is associated with. Use `In3Request#url_at` to get the url. The value of both parameters are shared
            msg (str): The actual response to be returned to in3 client
        """
        libin3_in3_req_add_response(self.in3_response, index, False, msg, len(msg), 0)

    def failure(self, index: int, msg: bytes):
        """
        Function to be invoked in order to write the result for the request in case of failure
        Args:
            index (int): Positional argument related to which url on the `In3Request` list this response is associated with. Use `In3Request#url_at` to get the url. The value of both parameters are shared.
            msg (str): The actual response to be returned to in3 client.
        """
        libin3_in3_req_add_response(self.in3_response, index, True, msg, len(msg), 0)


# TODO: Move to a OO perspective
def factory(transport_fn):
    """
    C level abstraction of a transport handler.
    Decorates a transport function augmenting its capabilities for native interoperability
    """

    @c.CFUNCTYPE(c.c_int32, c.c_void_p, c.c_int32, c.POINTER(NativeRequest))
    def new(plugin_data, action: PluginAction, plugin_ctx: NativeRequest or NativeResponse):
        request = In3Request(plugin_ctx)
        response = In3Response(plugin_ctx)
        if PluginAction(action) == PluginAction.PLGN_ACT_TRANSPORT_SEND:
            return transport_fn(request, response)
        elif PluginAction(action) == PluginAction.PLGN_ACT_TRANSPORT_RECEIVE:
            return transport_fn(request, response)
        elif PluginAction(action) == PluginAction.PLGN_ACT_TRANSPORT_CLEAN:
            return transport_fn(request, response)
        else:
            raise Exception("In3 Transport Plugin: Unknown action: ", action)

    return new
