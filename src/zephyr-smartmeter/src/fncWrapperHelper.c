#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <client/cache.h>
#include <client/client.h>
#include <eth_api.h>
#include <eth_full.h>
#include <evm.h>
#include <in3_curl.h>
#include <in3_storage.h>
#include <signer.h>
#include <inttypes.h>
#include <util/debug.h>
#include <util/utils.h>

#include <util/data.h>
#include "fncWrapperHelper.h"


// ##### hier weiter: welche includes? was zu exportieren? ####

call_request_t* prepare_tx(char* fn_sig, char* to, char* args, char* block_number, uint64_t gas, char* value) {
  call_request_t* req = parseSignature(fn_sig);
  if (req->in_data->type == A_TUPLE) {
    json_ctx_t* in_data = parse_json(args);
    if (set_data(req, in_data->result, req->in_data) < 0) { 
      dbg_log("Error: could not set the data"); 
    }
    free_json(in_data);
  }
  sb_t* params = sb_new("");
  sb_add_chars(params, "[{\"to\":\"");
  sb_add_chars(params, to);
  sb_add_chars(params, "\", \"data\":");
  sb_add_bytes(params, "", &req->call_data->b, 1, false);
  if (block_number) {
    sb_add_chars(params, "},\"");
    sb_add_chars(params, block_number);
    sb_add_chars(params, "\"]");
  } else {
    if (value) {
      sb_add_chars(params, ", \"value\":\"");
      sb_add_chars(params, value);
      sb_add_chars(params, "\"");
    }
    sb_add_chars(params, ", \"gasLimit\":");
    uint8_t gasdata[8];
    bytes_t g_bytes = bytes(gasdata, 8);
    long_to_bytes(gas ? gas : 100000, gasdata);
    b_optimize_len(&g_bytes);
    sb_add_bytes(params, "", &g_bytes, 1, false);
    sb_add_chars(params, "}]");
  }
  strcpy(args, params->data);
  sb_free(params);
  return req;
}

void extract_vals(d_token_t* t, CB_extractVal_t pFncCB, void* pUserData) {
  char buf[65];
  memset(buf, 0, sizeof(buf));
  switch (d_type(t)) {
    case T_ARRAY:
    case T_OBJECT:
      for (d_iterator_t it = d_iter(t); it.left; d_iter_next(&it))
        extract_vals(it.token, pFncCB, pUserData);
      break;
    case T_BOOLEAN:
      snprintX(buf, sizeof(buf),"%s", d_int(t) ? "true" : "false");
      pFncCB(buf, pUserData);
      break;
    case T_INTEGER:
      snprintX(buf, sizeof(buf)-1,"%i", d_int(t));
      pFncCB(buf, pUserData);
      break;
    case T_BYTES:
      if (t->len < 9) {
        // printk("### T_BYTES: %" PRId64 " - ",  d_long(t)); // does not work on nRF52
        char bufTmp[21];
        snprintX(buf, sizeof(buf)-1,"%s",u64tostr(d_long(t), bufTmp, sizeof(bufTmp)));
      } else {
        int pos = 0;
        snprintX(&buf[pos],3,"0x");
        pos += 2;
        for (int i = 0; i < (int)(t->len); i++) 
        {
          snprintX(&buf[pos],3,"%02x", t->data[i]);
          pos += 2;
        }
      }
      pFncCB(buf, pUserData);
      break;
    case T_NULL:
      snprintX(buf, sizeof(buf),"NULL");
      pFncCB(buf, pUserData);
      break;
    case T_STRING:
      snprintX(buf, sizeof(buf),"%s", d_string(t));
      pFncCB(buf, pUserData);
      break;
  }
}
