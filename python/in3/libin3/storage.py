import ctypes as c

in3_storage_get_item = c.CFUNCTYPE(c.POINTER(c.c_char), c.c_void_p, c.POINTER(c.c_char))
in3_storage_set_item = c.CFUNCTYPE(None, c.c_void_p, c.POINTER(c.c_char_p), c.POINTER(c.c_char))
in3_storage_clear = c.CFUNCTYPE(None, c.c_void_p)


class NativeRequest(c.Structure):
    """
    Request sent by the libin3 to the In3 Network, transported over the _http_transport function
    Based on in3/client/.h in3_request_t struct
    """
    _fields_ = [("get_item", c.POINTER(in3_storage_get_item)),
                ("set_item", c.POINTER(in3_storage_set_item)),
                ("clear", c.POINTER(in3_storage_clear)),
                ("cptr", c.c_void_p)]


def factory(get_item_fn, set_item_fn, clear_fn):
    """
    C level abstraction of a transport handler.
    Decorates a transport function augmenting its capabilities for native interoperability

    void in3_set_default_storage(in3_storage_handler_t* cacheStorage) {
      default_storage = cacheStorage;
    }

    typedef struct in3_storage_handler {
      in3_storage_get_item get_item; /**< function pointer returning a stored value for the given key.*/
      in3_storage_set_item set_item; /**< function pointer setting a stored value for the given key.*/
      in3_storage_clear    clear;    /**< function pointer clearing all contents of cache.*/
      void*                cptr;     /**< custom pointer which will be passed to functions */
    } in3_storage_handler_t;

    typedef bytes_t* (*in3_storage_get_item)(
        void*       cptr, /**< a custom pointer as set in the storage handler*/
        const char* key   /**< the key to search in the cache */
    );

    typedef void (*in3_storage_set_item)(
        void*       cptr, /**< a custom pointer as set in the storage handler*/
        const char* key,  /**< the key to store the value.*/
        bytes_t*    value /**< the value to store.*/
    );

    typedef void (*in3_storage_clear)(
        void* cptr /**< a custom pointer as set in the storage handler*/
    );

    """

    c_transport_fn = c.CFUNCTYPE(c.c_int, c.POINTER(NativeRequest))
    return c_transport_fn(in3_storage_get_item, in3_storage_set_item, in3_storage_clear, None)
