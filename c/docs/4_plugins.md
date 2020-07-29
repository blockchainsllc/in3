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

All plugins are stored in a linked list and when we want to trigger a specific actions we will loop through all, but only execute the function if the required action is set in the bitmask. 
Except for `PLGN_ACT_TERM` we will loop until the first plugin handles it. The handle-function must return a return code indicating this:

- `IN3_OK` - the plugin handled it and it was succesful
- `IN3_WAITING` - the plugin handled the action, but is waiting for more data, which happens in a sub context added. As soon as this was resolved, the plugin will be called again.
- `IN3_EIGNORE` - the plugin did **NOT** handle the action and we should continue with the other plugins.
- `IN3_E...` - the plugin did handle it, but raised a error and returned the error-code. In addition you should always use the current `in3_ctx_t`to report a detailed error-message (using `ctx_set_error()`)



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
    uint32_t       time      /**<  the time (in ms) this request took in ms or 0 if not possible (it will be used to calculate the weights)*/    
);
```

In case of a succesful response:

```c
in3_req_add_response(request, index, false, response_data, -1, current_ms() - start);
```

in case of an error, the data is the error message itself:

```c
in3_req_add_response(request, index, true, "Timeout waiting for a response", -1, 0);
```

#### PLGN_ACT_TRANSPORT_CLEAN

If a previous `PLGN_ACT_TRANSPORT_SEND` has set a `cptr` this will be triggered in order to clean up memory.

`arguments` : `in3_request_t*` - a request-object holding the data. ( the payload and urls may not be set!)

#### PLGN_ACT_SIGN_ACCOUNT

if we are about to sign data and need to know the address of the account abnout to sign, this action will be triggered in order to find out. This is needed if you want to send a transaction without specifying the `from` address, we will still need to get the nonce for this account before signing.

`arguments` : `in3_sign_account_ctx_t*` - the account context will hold those data:

```c
typedef struct sign_account_ctx {
  in3_ctx_t* ctx;     /**< the context of the request in order report errors */
  address_t  account; /**< the account to use for the signature */
} in3_sign_account_ctx_t;
```

The implementation should return a status code ´IN3_OK` if it successfully wrote the address of the account into the content:

Example:

```c
in3_ret_t eth_sign_pk(void* data, in3_plugin_act_t action, void* args) {
  // the data are our pk
  uint8_t* pk = data; 

  switch (action) {

    case PLGN_ACT_SIGN_ACCOUNT: {
      // cast the context
      in3_sign_account_ctx_t* ctx = args;

      // generate the address from the key
      // and write it into account
      get_address(pk, ctx->account);
      return IN3_OK;
    }

    // handle other actions ...

    default:
      return IN3_ENOTSUP;
  }
}


```


#### PLGN_ACT_SIGN

This action is triggered as a request to sign data.

`arguments` : `in3_sign_ctx_t*` - the sign context will hold those data:

```c

typedef struct sign_ctx {
  uint8_t            signature[65]; /**< the resulting signature needs to be writte into these bytes */
  d_signature_type_t type;          /**< the type of signature*/
  in3_ctx_t*         ctx;           /**< the context of the request in order report errors */
  bytes_t            message;       /**< the message to sign*/
  bytes_t            account;       /**< the account to use for the signature  (if set)*/
} in3_sign_ctx_t;
```

The signature must be 65 bytes and in the format 
```c
r[32]|s[32]|v[1]
```
where v must be the recovery byte and should only be 1 or 0.

Currently there are 2 types of sign-request:

- SIGN_EC_RAW : the data is already 256bits and may be used directly
- SIGN_EC_HASH : the data may be any kind of message, and need to be hashed first. As hash we will use Keccak.


Example:


```c
in3_ret_t eth_sign_pk(void* data, in3_plugin_act_t action, void* args) {
  // the data are our pk
  uint8_t* pk = data; 

  switch (action) {

    case PLGN_ACT_SIGN: {
      // cast the context
      in3_sign_ctx_t* ctx = args;

      // if there is a account set, we only sign if this matches our account
      // this way we allow multiple accounts to added as plugin
      if (ctx->account.len == 20) {
        address_t adr;
        get_address(pk, adr);
        if (memcmp(adr, ctx->account.data, 20)) 
           return IN3_EIGNORE; // does not match, let someone else handle it
      }

      // sign based on sign type
      switch (ctx->type) {
        case SIGN_EC_RAW:
          return ec_sign_pk_raw(ctx->message.data, pk, ctx->signature);
        case SIGN_EC_HASH:
          return ec_sign_pk_hash(ctx->message.data, ctx->message.len, pk, hasher_sha3k, ctx->signature);
        default:
          return IN3_ENOTSUP;
      }
    }

    case PLGN_ACT_SIGN_ACCOUNT: {
      // cast the context
      in3_sign_account_ctx_t* ctx = args;

      // generate the address from the key
      get_address(pk, ctx->account);
      return IN3_OK;
    }

    default:
      return IN3_ENOTSUP;
  }
}

/** sets the signer and a pk to the client*/
in3_ret_t eth_set_pk_signer(in3_t* in3, bytes32_t pk) {
  // we register for both ACCOUNT and SIGN
  return in3_plugin_register(in3, PLGN_ACT_SIGN_ACCOUNT | PLGN_ACT_SIGN, eth_sign_pk, pk, false);
}

```

#### PLGN_ACT_SIGN_PREPARE

The Prepare-action is triggered before signing and gives a plugin the chance to change the data. This is needed if you want to send a transaction through a multisig. Here we have to change the `data` and `to` address.

`arguments` : `in3_sign_prepare_ctx_t*` - the prepare context will hold those data:

```c

typedef struct sign_prepare_ctx {
  struct in3_ctx* ctx;     /**< the context of the request in order report errors */
  address_t       account; /**< the account to use for the signature */
  bytes_t         old_tx;  /**< the data to sign */
  bytes_t         new_tx;  /**< the new data to be set */

} in3_sign_prepare_ctx_t;
```

the tx-data will be in a form ready to sign, which means those are rlp-encoded data of a transaction without a signature, but the chain-id as v-value.

In order to decode the data you must use rlp.h:


```c
#define decode(data,index,dst,msg) if (rlp_decode_in_list(data, index, dst) != 1) return ctx_set_error(ctx, "invalid" msg "in txdata", IN3_EINVAL);
static in3_ret_t decode_tx(in3_ctx_t* ctx, bytes_t raw, tx_data_t* result) {
  decode(&raw, 0, &result->nonce    , "nonce");
  decode(&raw, 1, &result->gas_price, "gas_price");
  decode(&raw, 2, &result->gas      , "gas");
  decode(&raw, 3, &result->to       , "to");
  decode(&raw, 4, &result->value    , "value");
  decode(&raw, 5, &result->data     , "data");
  decode(&raw, 6, &result->v        , "v");
  return IN3_OK;
}

```

and of course once the data has changes you need to encode it again and set it as `nex_tx``


#### PLGN_ACT_RPC_HANDLE

Triggered for each rpc-request in order to give plugins a chance to directly handle it. If no onoe handles it it will be send to the nodes.

`arguments` : `in3_rpc_handle_ctx_t*` - the rpc_handle context will hold those data:

```c
typedef struct {
  in3_ctx_t*       ctx;      /**< Request context. */
  d_token_t*       request;  /**< request */
  in3_response_t** response; /**< the response which a prehandle-method should set*/
} in3_rpc_handle_ctx_t;
```

the steps to add a new custom rpc-method will be the following.

1. get the method and params:

    ```c
    char* method      = d_get_stringk(rpc->request, K_METHOD);
    d_token_t* params = d_get(rpc->request, K_PARAMS);
    ```
2. check if you can handle it
3. handle it and set the result
    ```c
    in3_rpc_handle_with_int(rpc,result);
    ```

    for setting the result you should use one of the `in3_rpc_handle_...` methods. Those will create the response and build the JSON-string with the result.
    WHile most of those expect the result as a sngle value you can also return a complex JSON-Object. In this case you have to create a string builder:

    ```c
    sb_t* writer = in3_rpc_handle_start(rpc);
    sb_add_chars(writer, "{\"raw\":\"");
    sb_add_escaped_chars(writer, raw_string);
    // ... more data
    sb_add_chars(writer, "}");
    return in3_rpc_handle_finish(rpc);
    ```

4. In case of an error, simply set the error in the context, with the right message and error-code:

    ```c
    if (d_len(params)<1) return ctx_set_error(rpc->ctx, "Not enough parameters", IN3_EINVAL);
    ```

If the reequest needs additional subrequests, you need to follow the pattern of sending a request asynchron in a state machine:

```c

  // we want to get the nonce.....
  uint64_t  nonce =0;

  // check if a request is already existing
  in3_ctx_t* ctx = ctx_find_required(rpc->ctx, "eth_getTransactionCount");
  if (ctx) {
    // found one - so we check if it is ready.
    switch (in3_ctx_state(ctx)) {
      // in case of an error, we report it back to the parent context
      case CTX_ERROR:
        return ctx_set_error(rpc->ctx, ctx->error, IN3_EUNKNOWN);
      // if we are still waiting, we stop here and report it.
      case CTX_WAITING_FOR_RESPONSE:
      case CTX_WAITING_TO_SEND:
        return IN3_WAITING;

      // if it is useable, we can now handle the result.
      case CTX_SUCCESS: {
        // check if the response contains a error.
        TRY(ctx_check_response_error(ctx, 0))

        // read the nonce
        nonce = d_get_longk(ctx->responses[0], K_RESULT);
      }
    }
  }
  else {
    // no required context found yet, so we create one:

    // since this is a subrequest it will be freed when the parent is freed.
    // allocate memory for the request-string
    char* req = _malloc(strlen(params) + 200);
    // create it
    sprintf(req, "{\"method\":\"eth_getTransactionCount\",\"jsonrpc\":\"2.0\",\"id\":1,\"params\":[\"%s\",\"latest\"]}", account_hex_string);
    // and add the request context to the parent.
    return ctx_add_required(parent, ctx_new(parent->client, req));
  }

  // continue here and use the nonce....

```

Here is a simple Example how to register a plugin hashing data:


```c

static in3_ret_t handle_intern(void* pdata, in3_plugin_act_t action, void* args) {
  UNUSED_VAR(pdata);

  // cast args 
  in3_rpc_handle_ctx_t* rpc = args;

  swtch (action) {
    case PLGN_ACT_RPC_HANDLE: {
      // get method and params
      char*                 method  = d_get_stringk(rpc->request, K_METHOD);
      d_token_t*            params  = d_get(rpc->request, K_PARAMS);

      // do we support it?
      if (strcmp(method, "web3_sha3") == 0) {
        // check the params
        if (!params || d_len(params) != 1) return ctx_set_error(rpc->ctx, "invalid params", IN3_EINVAL);
        bytes32_t hash;
        // hash the first param
        keccak(d_to_bytes(d_get_at(params,0)), hash);
        // return the hash as resut.
        return in3_rpc_handle_with_bytes(ctx, bytes(hash, 32));
      }

      // we don't support this method, so we ignore it.
      return IN3_EIGNORE;
    }

    default:
      return IN3_ENOTSUP;
  }
}

in3_ret_t in3_register_rpc_handler(in3_t* c) {
  return in3_plugin_register(c, PLGN_ACT_RPC_HANDLE, handle_intern, NULL, false);
}
```

#### PLGN_ACT_RPC_VERIFY

This plugin reprresents a verifier. It will be triggered after we have received a response from a node.

`arguments` : `in3_vctx_t*` - the verification context will hold those data:

```c
typedef struct {
  in3_ctx_t*   ctx;                   /**< Request context. */
  in3_chain_t* chain;                 /**< the chain definition. */
  d_token_t*   result;                /**< the result to verify */
  d_token_t*   request;               /**< the request sent. */
  d_token_t*   proof;                 /**< the delivered proof. */
  in3_t*       client;                /**< the client. */
  uint64_t     last_validator_change; /**< Block number of last change of the validator list */
  uint64_t     currentBlock;          /**< Block number of latest block */
  int          index;                 /**< the index of the request within the bulk */
} in3_vctx_t;
```

Example:

```c
in3_ret_t in3_verify_ipfs(void* pdata, in3_plugin_act_t action, void* args) {
  if (action!=PLGN_ACT_RPC_VERIFY) return IN3_ENOTSUP;
  UNUSED_VAR(pdata);

  // we want this verifier to handle ipfs-chains
  if (vc->chain->type != CHAIN_IPFS) return IN3_EIGNORE;


  in3_vctx_t* vc     = args;
  char*       method = d_get_stringk(vc->request, K_METHOD);
  d_token_t*  params = d_get(vc->request, K_PARAMS);

  // did we ask for proof?
  if (in3_ctx_get_proof(vc->ctx, vc->index) == PROOF_NONE) return IN3_OK;

  // do we have a result? if not it is a vaslid error-response
  if (!vc->result)
    return IN3_OK;

  if (strcmp(method, "ipfs_get") == 0)
    return ipfs_verify_hash(d_string(vc->result),
                            d_get_string_at(params, 1) ? d_get_string_at(params, 1) : "base64",
                            d_get_string_at(params, 0));
  
  // could not verify, so we hope some other plugin will
  return IN3_EIGNORE;
}

in3_ret_t in3_register_ipfs(in3_t* c) {
  return in3_plugin_register(c, PLGN_ACT_RPC_VERIFY, in3_verify_ipfs, NULL, false);
}

```

#### PLGN_ACT_CACHE_SET

This action will be triggered whenever there is something worth putting in a cache. If no plugin picks it up, it is ok, since the cache is optional.

`arguments` : `in3_cache_ctx_t*` - the cache context will hold those data:

```c
typedef struct in3_cache_ctx {
  in3_ctx_t* ctx;     /**< the request context  */
  char*      key;     /**< the key to fetch */
  bytes_t*   content; /**< the content to set */
} in3_cache_ctx_t;
```

in the case of `CACHE_SET` the content will point to the bytes we need to store somewhere.
If for whatever reason the item can not be stored, a `IN3_EIGNORE` should be send, since to indicate that no action took place. 

```
Example:

```c
in3_ret_t handle_storage(void* data, in3_plugin_act_t action, void* arg) {
  in3_cache_ctx_t* ctx = arg;
  switch (action) {
    case PLGN_ACT_CACHE_GET: {
       ctx->content = storage_get_item(data, ctx->key);
       return ctx->content ? IN3_OK : IN3_EIGNORE;
    }
    case PLGN_ACT_CACHE_SET: {
      storage_set_item(data, ctx->key, ctx->content);
      return IN3_OK;
    }
    case PLGN_ACT_CACHE_CLEAR: {
      storage_clear(data);
      return IN3_OK;
    }
    default: return IN3_EINVAL;
  }
}

in3_ret_t in3_register_file_storage(in3_t* c) {
  return in3_plugin_register(c, PLGN_ACT_CACHE, handle_storage, NULL, true);
}
```


#### PLGN_ACT_CACHE_GET

This action will be triggered whenever we access the cache in order to get values.

`arguments` : `in3_cache_ctx_t*` - the cache context will hold those data:

```c
typedef struct in3_cache_ctx {
  in3_ctx_t* ctx;     /**< the request context  */
  char*      key;     /**< the key to fetch */
  bytes_t*   content; /**< the content to set */
} in3_cache_ctx_t;
```

in the case of `CACHE_GET` the content will be NULL and needs to be set to point to the found values.
If we did not find it in the cache, we must return  `IN3_EIGNORE`.

```
Example:

```c
ctx->content = storage_get_item(data, ctx->key);
return ctx->content ? IN3_OK : IN3_EIGNORE;
```

#### PLGN_ACT_CACHE_CLEAR

This action will clear all stored values in the cache.

`arguments` :NULL - so no argument will be passed.

#### PLGN_ACT_CONFIG_GET

This action will be triggered during the configuration-process. While going through all config-properties, it will ask the plugins in case a config was not handled. So this action may be triggered multiple times. And the plugin should only return `IN3_OK` if it was handled. If no plugin handles it, a error will be thrown.

`arguments` : `in3_configure_ctx_t*` - the cache context will hold those data:

```c
typedef struct in3_configure_ctx {
  in3_t*     client; /**< the client to configure */
  d_token_t* token;  /**< the token not handled yet*/
} in3_configure_ctx_t;
```

in order



  PLGN_ACT_CONFIG_SET        = 0x2000,   /**< gets a config-token and reads data from it */
  PLGN_ACT_CONFIG_GET        = 0x4000,   /**< gets a stringbuilder and adds all config to it. */
  PLGN_ACT_PAY_PREPARE       = 0x8000,   /**< prerpares a payment */
  PLGN_ACT_PAY_FOLLOWUP      = 0x10000,  /**< called after a requeest to update stats. */
  PLGN_ACT_PAY_HANDLE        = 0x20000,  /**< handles the payment */
  PLGN_ACT_NL_PICK_DATA      = 0x40000,  /**< picks the data nodes */
  PLGN_ACT_NL_PICK_SIGNER    = 0x80000,  /**< picks the signer nodes */
  PLGN_ACT_NL_PICK_FOLLOWUP  = 0x100000, /**< called after receiving a response in order to decide whether a update is needed. */
