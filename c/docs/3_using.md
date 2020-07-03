## How it works

The core of incubed is the processing of json-rpc requests by fetching data from the network and verifying them. This is why in the `core`-module it is all about rpc-requests and their responses.

### the statemachine

Each request is represented internally by the `in3_ctx_t` -struct. This context is responsible for trying to find a verifyable answer to the request and acts as a statemachine.

```

digraph G {
    node[fontname="Helvetica",   shape=Box, color=lightblue, style=filled ]
    edge[fontname="Helvetica",   style=solid,  fontsize=8 , color=grey]
    rankdir = TB;
    
    RPC[label="RPC-Request"]
    CTX[label="in3_ctx_t"]
    
    sign[label="sign",color=lightgrey, style=""]
    request[label="fetch http",color=lightgrey, style=""]
    
    exec[ label="in3_ctx_exec_state()",color=lightgrey, style="", shape=ellipse ]
    free[label="ctx_free()",color=lightgrey, style=""]

    waiting[label="need input"]


    RPC -> CTX [label="ctx_new()"]
    CTX -> exec
    
    
    exec -> error [label="CTX_ERROR"]
    exec -> response[label="CTX_SUCCESS"]
    exec -> waiting[label="CTX_WAITING_TO_SEND"]
    exec -> request[label="CTX_WAITING_FOR_RESPONSE"]

    
    waiting -> sign[label=CT_SIGN]
    waiting -> request[label=CT_RPC] 
    
    sign -> exec [label="in3_ctx_add_response()"]
    request -> exec[label="in3_ctx_add_response()"]
    
    response -> free
    error->free
    

  { rank = same; error, response }

  { rank = same; exec,waiting }
  { rank = same; CTX,request }


    }
```

In order to process a request we follow these steps.
1. `ctx_new` which creates a new context by parsing a JSON-RPC request.
2. `in3_ctx_exec_state` this will try to process the state and returns the new state, which will be one of he following:

    - `CTX_SUCCESS` - we have a response
    - `CTX_ERROR` - we stop because of an unrecoverable error
    - `CTX_WAITING_TO_SEND` - we need input and need to send out a request. By calling `in3_create_request()` the ctx will switch to the state to `CTX_WAITING_FOR_RESPONSE` until all the needed responses are repoorted. While it is possible to fetch all responses and add them before calling `in3_ctx_exec_state()`, but it would be more efficient if can send all requests out, but then create a response-queue and set one response add a time so we can return as soon as we have the first verifiable  response.
    - `CTX_WAITING_FOR_RESPONSE` - the request has been send, but no verifieable response is available. Once the next (or more) responses have been added, we call `in3_ctx_exec_state()` again, which will verify all available responses. If we could verify it, we have a respoonse, if not we may either wait for more responses ( in case we send out multiple requests -> `CTX_WAITING_FOR_RESPONSE` ) or we send out new requests (`CTX_WAITING_TO_SEND`)

the `in3_send_ctx`-function will executly this:

```c
in3_ret_t in3_send_ctx(in3_ctx_t* ctx) {
  ctx_req_transports_t transports = {0};
  while (true) {
    switch (in3_ctx_exec_state(ctx)) {
      case CTX_ERROR:
      case CTX_SUCCESS:
        transport_cleanup(ctx, &transports, true);
        return ctx->verification_state;

      case CTX_WAITING_FOR_RESPONSE:
        in3_handle_rpc_next(ctx, &transports);
        break;

      case CTX_WAITING_TO_SEND: {
        in3_ctx_t* last = in3_ctx_last_waiting(ctx);
        switch (last->type) {
          case CT_SIGN:
            in3_handle_sign(last);
            break;
          case CT_RPC:
            in3_handle_rpc(last, &transports);
        }
      }
    }
  }
}
```

### sync calls with in3_send_ctx

This statemachine can be used to process requests synchronously or asynchronously. The  `in3_send_ctx` function, which is used in most convinience-functions will do this synchronously. In order to get user input it relies on 2 callback-functions:

- to sign : [`in3_signer_t`](#in3-signer-t) struct including its callback function is set in the `in3_t` configuration.
- to fetch data : a [in3_transport_send](#in3-transport-send) function-pointer will be set in the `in3_t` configuration.

#### signing

For signing the client expects a  [`in3_signer_t`](#in3-signer-t) struct to be set. Setting should be done by using the [`in3_set_signer()`](#in3-set-signer) function.
This function expects 3 arguments  (after the client config itself):

- `sign` - this is a function pointer to actual signing-function. Whenever the incubed client needs a signature it will prepare a signing context [`in3_sign_ctx_t`](#in3-sign-ctx-t), which holds all relevant data, like message and the address for signing. The result will always be a signature which you need to copy into the `signature`-field of this context. The return value must signal the success of the execution. While `IN3_OK` represents success, `IN3_WAITING`can be used to indicate that we need to execute again since there may be a sub-request that needs to finished up before being able to sign. In case of an error [`ctx_set_error`](#ctx-set-error) should be used to report the details of the error including returning the `IN3_E...` as error-code.

- `prepare_tx`- this function is optional and gives you a chance to change the data before signing. For example signing with a mutisig would need to do manipulate the data and also the target in order to redirect it to the multisig contract.

- `wallet` - this is a optional `void*` which will be set in the signing context. It can be used to point to any data structure you may need in order to sign.

As a example this is the implemantation of the signer-function for a simple raw private key:

```c
/** signs the given data */
in3_ret_t eth_sign_pk_ctx(in3_sign_ctx_t* ctx) {
  uint8_t* pk = ctx->wallet;
  switch (ctx->type) {
    case SIGN_EC_RAW:
      return ec_sign_pk_raw(ctx->message.data, pk, ctx->signature);
    case SIGN_EC_HASH:
      return ec_sign_pk_hash(ctx->message.data, ctx->message.len, pk, hasher_sha3k, ctx->signature);
    default:
      return IN3_ENOTSUP;
  }
  return IN3_OK;
}
```

The pk-signer uses the wallet-pointer to point to the raw 32 bytes private key and will use this to sign.

#### transport

The transport function is a function-pointer set in the client configuration (`in3_t`) which will be used in the `in3_send_ctx()` function whenever data are required to get from the network. the function will get a [`request_t`](#request-t) object as argument.

The main responsibility of this function is to fetch the requested data and the call [`in3_ctx_add_response`](#in3-ctx-add-response) to report this to the context.
if the request only sends one request to one url, this is all you have to do.
But if the user uses a configuration of `request_count` >1, the `request` object will contain a list of multiples urls. In this case transport function still has 3 options to accomplish this:
1. send the payload to each url sequentially. This is **NOT** recommented, since this increases the time the user has to wait for a response. Especially if some of the request may run into a timeout.
2. send the all in parallel and wait for all the finish. This is better, but it still means, we may have to wait until the last one responses even though we may have a verifiable response already reported.
3. send them all in parallel and return as soon as we have the first response. This increases the performance since we don't have to wait if we have one. But since we don't know yet whether this response is also correct, we must be prepared to also read the other responses if needed, which means the transport would be called multiple times for the same request. 
In order to process multiple calls to the same resouces the request-object contains two fields:
    - `cptr` - a custom `void*` which can be set in the first call pointing to recources you may need to continue in the subsequent calls. 
    - `action` - This value is enum ( [`#in3_req_action_t`](#in3-req-action-t) ), which indicates these current state

So only if you need to continue your call later, because you don't want to and can't set all the responses yet, you need set the `cptr` to a non NULL value. And only in this case `in3_send_ctx()` will follow this process with these states:


```

digraph G {
    node[fontname="Helvetica",   shape=Box, color=lightblue, style=filled ]
    rankdir = TB;

    REQ_ACTION_SEND -> REQ_ACTION_RECEIVE -> REQ_ACTION_CLEANUP
    REQ_ACTION_RECEIVE -> REQ_ACTION_RECEIVE

```

- `REQ_ACTION_SEND` - this will always be set in the first call.
- `REQ_ACTION_RECEIVE` - a call with this state indicates that there was a send call prior but since we do not have all responses yet, the transport should now set the next reponse. So this call may be called multiple times until either we have found a verifieable response or the number of urls is reached. Important during this call the `urls` field of the request will be NULL since this should not send a new request.
- `REQ_ACTION_CLEANUP` - this will only be used if the `cptr` was set before. Here the transport should only clean up any allocated resources. This will also be called if not all responses were used.


While there are of course existing implementations for the transport-function ( as default we use `in3_curl_c`), especially for embedded devices you may even implement your own.

### async calls

While for sync calls you can just implement a transport function, you can also take full control of the process which allows to execute it completly async. The basic process is the same layed out in the [state machine](#the-statemachine).

For the js for example the main-loop is part of a async function.


