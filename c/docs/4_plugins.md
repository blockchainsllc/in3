## Plugins

While the core is kept as small as possible, we defined actions, which can be implemented by plugins. The core alone would not be able to do any good. While the in3-c repository already provides default implementations for all actions, as a developer you can always extend or replace those. There are good reasons to do so:
- optimizing by using a smaller plugin (like replacing the nodelist handling)
- allowing custom rpc-commands
- changing behavior ...

### What is a plugin?

Each plugin needs to define those 3 things:

1. Actions
    Which actions do I want handle. This is a bitmask with the actions set. You can use any combination.
2. Custom data
    This optional data object may contain configurations or other data. If you don't need to hold any data, you may pass `NULL`
3. Exec-function
    This is a function pointer to a function with the following signature:

    ```c
    in3_ret_t handle(void* custom_data, in3_plugin, in3_plugin_act_t action, void* arguments);
    ```

    while the `custom_data` is just the pointer to your data-object, the `arguments` contain a pointer to a context object. This object depends on the action you are reacting.

### Actions

The following actions are available:

#### PLGN_ACT_TERM

This action will be triggered during `in3_free` and must be used to free up resources which were allocated.

`arguments` : `in3_t*` - the in3-instance will be passed as argument.

#### PLGN_ACT_TRANSPORT_SEND

Send will be triggered only if the request is executed synchron, whenever a new request needs to be send out. This request may contain multiple urls, but the same payload.

`arguments` : `in3_request_t*` - a request-object holding the following data:

```c
typedef struct in3_request {
  char*           payload;  /**< the payload to send */
  char**          urls;     /**< array of urls */
  uint_fast16_t   urls_len; /**< number of urls */
  struct in3_ctx* ctx;      /**< the current context */
  void*           cptr;     /**< a custom ptr to hold information during */
} in3_request_t;
```

It is expected that a plugin will send out http-requests to each (iterating until `urls_len`) url from `urls` with the `payload`. 
if the payload is NULL or empty the request is a `GET`-request. Otherwise, the plugin must use send it with HTTP-Header `Content-Type: application/json` and attach the `payload`.

After the request is send out the `cptr` may be set in order to fetch the responses later. This allows us the fetch responses as they come in instead of waiting for the last response before continuing.

Example:
```c
in3_ret_t transport_handle(void* custom_data, in3_plugin, in3_plugin_act_t action, void* arguments) {
  switch (action) {

    case PLGN_ACT_TRANSPORT_SEND: {
      in3_request_t* req = arguments; // cast it to in3_request_t* 

      // init the cptr
      in3_curl_t* c = _malloc(sizeof(in3_curl_t));
      c->cm         = curl_multi_init(); // init curl
      c->start      = current_ms();      // keep the staring time
      req->cptr     = c;                 // set the cptr

      // define headers
      curl_multi_setopt(c->cm, CURLMOPT_MAXCONNECTS, (long) CURL_MAX_PARALLEL);
      struct curl_slist* headers = curl_slist_append(NULL, "Accept: application/json");
      if (req->payload && *req->payload)
        headers = curl_slist_append(headers, "Content-Type: application/json");
      headers    = curl_slist_append(headers, "charsets: utf-8");
      c->headers = curl_slist_append(headers, "User-Agent: in3 curl " IN3_VERSION);

      // send out requests in parallel
      for (unsigned int i = 0; i < req->urls_len; i++)
        readDataNonBlocking(c->cm, req->urls[i], req->payload, c->headers, req->ctx->raw_response + i, req->ctx->client->timeout);

      return IN3_OK;
    }

    // handle other actions ...
  }
}
    
```

#### PLGN_ACT_TRANSPORT_RECEIVE

This will only triggered if the previously triggered `PLGN_ACT_TRANSPORT_SEND` 
- was successfull (IN3_OK) 
- if the responses were not all set yet.
- if a `cptr` was set

`arguments` : `in3_request_t*` - a request-object holding the data. ( the payload and urls may not be set!)

The plugin needs to wait until the first response was received ( or runs into a timeout). To report, please use `in3_req_add_response()``

```c
void in3_req_add_response(
    in3_request_t* req,      /**< [in]the the request */
    int            index,    /**< [in] the index of the url, since this request could go out to many urls */
    bool           is_error, /**< [in] if true this will be reported as error. the message should then be the error-message */
    const char*    data,     /**<  the data or the the string of the response*/
    int            data_len, /**<  the length of the data or the the string (use -1 if data is a null terminated string)*/
    uint32_t       time      /**<  the time this request took in ms or 0 if not possible (it will be used to calculate the weights)*/    
);
```

In case of a succesful response:

```c
in3_req_add_response(request, index, false, response_data, -1, current() - start);
```

in case of an error:

```c
in3_req_add_response(request, index, true, "Timeout waiting for a response", -1, 0);
```

#### PLGN_ACT_TRANSPORT_CLEAN

If a previous `PLGN_ACT_TRANSPORT_SEND` has set a `cptr` this will bne triggered in order to clean up memory.

  PLGN_ACT_TRANSPORT_CLEAN   = 0x10,     /**< freeup transport resources - the transport plugin will receive a request_t as plgn_ctx if the cptr was set.*/
  PLGN_ACT_SIGN_ACCOUNT      = 0x20,     /**<  returns the default account of the signer */
  PLGN_ACT_SIGN_PREPARE      = 0x40,     /**< allowes a wallet to manipulate the payload before signing - the plgn_ctx will be in3_sign_ctx_t. This way a tx can be send through a multisig */
  PLGN_ACT_SIGN              = 0x80,     /**<  signs the payload - the plgn_ctx will be in3_sign_ctx_t.  */
  PLGN_ACT_RPC_HANDLE        = 0x100,    /**< a plugin may respond to a rpc-request directly (without sending it to the node). */
  PLGN_ACT_RPC_VERIFY        = 0x200,    /**< verifies the response. the plgn_ctx will be a in3_vctx_t holding all data */
  PLGN_ACT_CACHE_SET         = 0x400,    /**< stores data to be reused later - the plgn_ctx will be a in3_cache_ctx_t containing the data */
  PLGN_ACT_CACHE_GET         = 0x800,    /**< reads data to be previously stored - the plgn_ctx will be a in3_cache_ctx_t containing the key. if the data was found the data-property needs to be set. */
  PLGN_ACT_CACHE_CLEAR       = 0x1000,   /**< clears alls stored data - plgn_ctx will be NULL  */
  PLGN_ACT_CONFIG_SET        = 0x2000,   /**< gets a config-token and reads data from it */
  PLGN_ACT_CONFIG_GET        = 0x4000,   /**< gets a stringbuilder and adds all config to it. */
  PLGN_ACT_PAY_PREPARE       = 0x8000,   /**< prerpares a payment */
  PLGN_ACT_PAY_FOLLOWUP      = 0x10000,  /**< called after a requeest to update stats. */
  PLGN_ACT_PAY_HANDLE        = 0x20000,  /**< handles the payment */
  PLGN_ACT_NL_PICK_DATA      = 0x40000,  /**< picks the data nodes */
  PLGN_ACT_NL_PICK_SIGNER    = 0x80000,  /**< picks the signer nodes */
  PLGN_ACT_NL_PICK_FOLLOWUP  = 0x100000, /**< called after receiving a response in order to decide whether a update is needed. */
