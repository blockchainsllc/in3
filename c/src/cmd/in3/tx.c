#include "tx.h"
#include "helper.h"
static tx_t _tx = {0};

tx_t* get_txdata() {
  return &_tx;
}

// prepare a eth_call or eth_sendTransaction
static abi_sig_t* prepare_tx(char* fn_sig, char* to, sb_t* args, char* block_number, uint64_t gas, uint64_t gas_price, char* value, bytes_t* data, char* from, char* token) {
  char*      error = NULL;
  bytes_t    rdata = {0};
  abi_sig_t* req   = fn_sig ? abi_sig_create(fn_sig, &error) : NULL; // only if we have a function signature, we will parse it and create a call_request.
  if (error) die(error);                                             // parse-error we stop here.
  if (req) {                                                         // if type is a tuple, it means we have areuments we need to parse.
    json_ctx_t* in_data = parse_json(args->data);                    // the args are passed as a "[]"- json-array string.
    rdata               = abi_encode(req, in_data->result, &error);  // encode data
    if (error) die(error);                                           // we then set the data, which appends the arguments to the functionhash.
    json_free(in_data);                                              // of course we clean up ;-)
  }                                                                  //
  sb_t* params = sb_new("[{");                                       // now we create the transactionobject as json-argument.
  if (to) {                                                          // if this is a deployment we must not include the to-property
    sb_add_chars(params, "\"to\":\"");
    sb_add_chars(params, to);
    sb_add_chars(params, "\" ");
  }
  if (req || data) {                                      // if we have a request context or explicitly data we create the data-property
    if (params->len > 2) sb_add_char(params, ',');        // add comma if this is not the first argument
    sb_add_chars(params, "\"data\":");                    // we will have a data-property
    if (req && data) {                                    // if we have a both, we need to concat thewm (this is the case when depkloying a contract with constructorarguments)
      uint8_t* full = _malloc(rdata.len - 4 + data->len); // in this case we skip the functionsignature.
      memcpy(full, data->data, data->len);
      memcpy(full + data->len, rdata.data + 4, rdata.len - 4);
      bytes_t bb = bytes(full, rdata.len - 4 + data->len);
      sb_add_bytes(params, "", &bb, 1, false);
      _free(full);
    }
    else if (req)
      sb_add_bytes(params, "", &rdata, 1, false);
    else if (data)
      sb_add_bytes(params, "", data, 1, false);
  }

  if (block_number) {
    sb_add_chars(params, "},\"");
    sb_add_chars(params, block_number);
    sb_add_chars(params, "\"]");
  }
  else {
    uint8_t gasdata[8];
    bytes_t g_bytes = bytes(gasdata, 8);

    if (value) {
      sb_add_chars(params, ", \"value\":\"");
      sb_add_chars(params, value);
      sb_add_chars(params, "\"");
    }
    if (from) {
      sb_add_chars(params, ", \"from\":\"");
      sb_add_chars(params, from);
      sb_add_chars(params, "\"");
    }
    if (token) {
      sb_add_chars(params, ", \"token\":\"");
      sb_add_chars(params, token);
      sb_add_chars(params, "\"");
    }

    if (gas_price) {
      long_to_bytes(gas_price, gasdata);
      b_optimize_len(&g_bytes);
      sb_add_bytes(params, ", \"gasPrice\":", &g_bytes, 1, false);
    }
    if (gas) {
      long_to_bytes(gas, gasdata);
      g_bytes = bytes(gasdata, 8);
      b_optimize_len(&g_bytes);
      sb_add_bytes(params, ", \"gasLimit\":", &g_bytes, 1, false);
    }
    sb_add_chars(params, "}]");
  }
  args->len = 0;
  sb_add_chars(args, params->data);
  sb_free(params);
  return req;
}

void encode_abi(in3_t* c, sb_t* args, bool with_block) {
  _tx.abi_sig = prepare_tx(_tx.sig, resolve(c, _tx.to), args, _tx.block == NULL && with_block ? "latest" : _tx.block, _tx.gas, _tx.gas_price, _tx.value, _tx.data, _tx.from, _tx.token);
}