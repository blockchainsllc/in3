
#define IN3_INTERNAL // important in order to
#include "../../core/client/request_internal.h"
#include "../../core/util/bitset.h"
#include "../../core/util/bytes.h"
#include "../../core/util/crypto.h"
#include "../../core/util/data.h"
#include "../../core/util/mem.h"
#include "../../core/util/stringbuilder.h"
#include "../../core/util/utils.h"
#include "../../verifier/eth1/nano/rpcs.h"

#include "abi.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
int abi_chars_len(char* s);

static char* add_token_arguments(json_ctx_t* jc, int arg_idx, abi_coder_t* c, d_token_t* t) {
  switch (c->type) {
    case ABI_ARRAY: {
      if (d_type(t) != T_ARRAY) return "argument must be an array";
      int array = json_create_array(jc);
      json_array_add_value(jc, arg_idx, jc->result + array);
      abi_coder_t* coder = c->data.array.component;
      for (int i = 0; i < d_len(t); i++) {
        d_token_t* val = d_get_at(t, i);
        if (!val) return "missing argument";
        char* err = add_token_arguments(jc, array, coder, val);
        if (err) return err;
      }
      break;
    }
    case ABI_TUPLE: {
      if (d_type(t) != T_OBJECT || d_len(t) < c->data.tuple.len) return "argument must be an object";
      int array = json_create_array(jc);
      json_array_add_value(jc, arg_idx, jc->result + array);
      for (int i = 0; i < c->data.tuple.len; i++) {
        abi_coder_t* coder = c->data.tuple.components[i];
        d_token_t*   val   = coder->name ? d_get(t, keyn(coder->name, abi_chars_len(coder->name))) : d_get_at(t, i);
        if (!val) return "missing argument";
        char* err = add_token_arguments(jc, array, coder, val);
        if (err) return err;
      }
      break;
    }
    default:
      json_array_add_value(jc, arg_idx, json_create_ref_item(jc, d_type(t), t->data, d_len(t)));
  }
  return NULL;
}

static char* add_arguments(json_ctx_t* jc, int arg_idx, abi_coder_t* c, va_list* args) {
  switch (c->type) {
    case ABI_ARRAY:
      return add_token_arguments(jc, arg_idx, c, va_arg(*args, d_token_t*));
    case ABI_TUPLE:
      if (c->type == ABI_TUPLE && c->data.tuple.len == 0) return NULL;
      if (jc->len == 1) {
        for (int i = 0; i < c->data.tuple.len; i++) {
          abi_coder_t* coder = c->data.tuple.components[i];
          char*        err   = add_arguments(jc, arg_idx, coder, args);
          if (err) return err;
        }
        return NULL;
      }
      else
        return add_token_arguments(jc, arg_idx, c, va_arg(*args, d_token_t*));
    case ABI_STRING:
      json_array_add_value(jc, arg_idx, json_create_string(jc, va_arg(*args, char*), -1));
      break;
    case ABI_NUMBER:
      if (c->data.number.size < 33)
        json_array_add_value(jc, arg_idx, json_create_int(jc, (uint64_t) va_arg(*args, uint32_t)));
      else
        json_array_add_value(jc, arg_idx, json_create_bytes(jc, va_arg(*args, bytes_t)));
      break;
    case ABI_BYTES:
      json_array_add_value(jc, arg_idx, json_create_bytes(jc, va_arg(*args, bytes_t)));
      break;
    case ABI_ADDRESS:
      json_array_add_value(jc, arg_idx, json_create_bytes(jc, bytes(va_arg(*args, uint8_t*), 20)));
      break;
    case ABI_FIXED_BYTES:
      json_array_add_value(jc, arg_idx, json_create_bytes(jc, va_arg(*args, bytes_t)));
      break;
    case ABI_BOOL:
      json_array_add_value(jc, arg_idx, json_create_bool(jc, va_arg(*args, int)));
      break;
  }
  return NULL;
}

in3_ret_t abi_call(in3_rpc_handle_ctx_t* ctx, json_ctx_t** ret, address_t to, char* sig, ...) {
  char*      error  = NULL;
  d_token_t* result = NULL;
  bytes_t    data   = NULL_BYTES;
  abi_sig_t* s      = abi_sig_create(sig, &error);
  if (error) return rpc_throw(ctx->req, "Invalid signature %s : %s", sig, error);
  json_ctx_t* jc      = json_create();
  int         arg_idx = json_create_array(jc);

  va_list args;
  va_start(args, sig);
  error = add_arguments(jc, arg_idx, s->input, &args);
  va_end(args);

  if (!error) data = abi_encode(s, jc->result, &error);
  json_free(jc);
  if (error) {
    abi_sig_free(s);
    return rpc_throw(ctx->req, "Could not encode the tx for %s : %s", sig, error);
  }

  char* tx_data = sprintx("{\"to\":\"%B\",\"data\":\"%B\"},\"latest\"", bytes(to, 20), data);
  _free(data.data);

  TRY_CATCH(req_send_sub_request(ctx->req, FN_ETH_CALL, tx_data, NULL, &result, NULL),
            abi_sig_free(s);
            _free(tx_data))

  *ret = abi_decode(s, d_bytes(result), &error);
  abi_sig_free(s);
  _free(tx_data);
  return error ? rpc_throw(ctx->req, "Could not decode the tx for %s : %s", sig, error) : IN3_OK;
}

bytes_t abi_encode_args(in3_rpc_handle_ctx_t* ctx, char* sig, ...) {
  char*      error = NULL;
  bytes_t    data  = NULL_BYTES;
  abi_sig_t* s     = abi_sig_create(sig, &error);
  if (error) {
    rpc_throw(ctx->req, "Invalid signature %s : %s", sig, error);
    return NULL_BYTES;
  }
  json_ctx_t* jc      = json_create();
  int         arg_idx = json_create_array(jc);

  va_list args;
  va_start(args, sig);
  error = add_arguments(jc, arg_idx, s->input, &args);
  va_end(args);

  if (!error) data = abi_encode(s, jc->result, &error);
  json_free(jc);
  abi_sig_free(s);
  if (error) {
    _free(data.data);
    rpc_throw(ctx->req, "Could not encode the tx for %s : %s", sig, error);
    return NULL_BYTES;
  }
  else
    return data;
}
