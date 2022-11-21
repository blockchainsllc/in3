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
  sb_t params = SB_NULL;                                             //
  sb_add_chars(&params, "[{");                                       // now we create the transactionobject as json-argument.
  if (to) sb_printx(&params, "\"to\":\"%s\"", to);
  if (req || data) // if we have a request context or explicitly data we create the data-property
    sb_add_value(&params, "\"data\":\"0x%b%b\"", data ? *data : NULL_BYTES, rdata.len && data ? bytes(rdata.data + 4, rdata.len - 4) : rdata);

  if (block_number)
    sb_printx(&params, "},\"%s\"]", block_number);
  else {
    if (value) sb_printx(&params, ", \"value\":\"%s\"", value);
    if (from) sb_printx(&params, ", \"from\":\"%s\"", from);
    if (token) sb_printx(&params, ", \"token\":\"%s\"", token);
    if (gas_price) sb_printx(&params, ", \"gasPrice\":\"%x\"", gas_price);
    if (gas) sb_printx(&params, ", \"gas\":\"%x\"", gas);
    sb_add_chars(&params, "}]");
  }
  args->len = 0;
  sb_add_chars(args, params.data);
  _free(params.data);
  return req;
}

void encode_abi(in3_t* c, sb_t* args, bool with_block) {
  _tx.abi_sig = prepare_tx(_tx.sig, resolve(c, _tx.to), args, _tx.block == NULL && with_block ? "latest" : _tx.block, _tx.gas, _tx.gas_price, _tx.value, _tx.data, _tx.from, _tx.token);
}