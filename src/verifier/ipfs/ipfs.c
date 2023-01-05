
#include "ipfs.h"
#include "../../core/client/keys.h"
#include "../../core/client/plugin.h"
#include "../../core/util/crypto.h"
#include "../../core/util/debug.h"
#include "../../core/util/mem.h"
#include "../../nodeselect/full/rpcs.h"
#include "../../third-party/multihash/hashes.h"
#include "../../third-party/multihash/multihash.h"
#include "../../third-party/nanopb/pb_decode.h"
#include "../../third-party/nanopb/pb_encode.h"
#include "../../verifier/eth1/nano/eth_nano.h"
#include "ipfs.pb.h"
#include "rpcs.h"
#include <stdio.h>

#define GOTO_RET(label, val) \
  do {                       \
    ret = val;               \
    goto label;              \
  } while (0)

typedef struct data_ {
  size_t         len;
  const uint8_t* buf;
} cb_arg_bytes_t;

static bool cb_encode_bytes(pb_ostream_t* stream, const pb_field_t* field, void* const* arg) {
  cb_arg_bytes_t* data = *arg;
  return pb_encode_tag_for_field(stream, field) && pb_encode_string(stream, data->buf, data->len);
}

static size_t pb_encode_size(const pb_msgdesc_t* fields, const void* src_struct) {
  pb_ostream_t s_ = PB_OSTREAM_SIZING;
  if (pb_encode(&s_, fields, src_struct))
    return s_.bytes_written;
  return 0;
}

static in3_ret_t ipfs_create_hash(const uint8_t* content, size_t len, int hash, char** b58) {
  in3_ret_t      ret = IN3_OK;
  cb_arg_bytes_t tmp = {.buf = NULL, .len = 0};
  pb_ostream_t   stream;
  size_t         wlen = 0;
  uint8_t *      buf1 = NULL, *buf2 = NULL, *out = NULL;

  Data data              = Data_init_zero;
  data.Type              = Data_DataType_File;
  data.has_filesize      = true;
  data.filesize          = len;
  data.Data.funcs.encode = &cb_encode_bytes;
  tmp.buf                = content;
  tmp.len                = len;
  data.Data.arg          = &tmp;

  wlen = pb_encode_size(Data_fields, &data);
  if ((buf1 = _malloc(wlen)) == NULL)
    GOTO_RET(EXIT, IN3_ENOMEM);

  stream = pb_ostream_from_buffer(buf1, wlen);
  if (!pb_encode(&stream, Data_fields, &data))
    GOTO_RET(EXIT, IN3_EUNKNOWN);

  PBNode node            = PBNode_init_zero;
  node.Data.funcs.encode = &cb_encode_bytes;
  tmp.buf                = buf1;
  tmp.len                = stream.bytes_written;
  node.Data.arg          = &tmp;

  wlen = pb_encode_size(PBNode_fields, &node);
  if ((buf2 = _malloc(wlen)) == NULL)
    GOTO_RET(EXIT, IN3_ENOMEM);

  stream = pb_ostream_from_buffer(buf2, wlen);
  if (!pb_encode(&stream, PBNode_fields, &node))
    GOTO_RET(EXIT, IN3_EUNKNOWN);

  uint8_t* digest     = NULL;
  size_t   digest_len = 0;
  uint8_t  d_[32]     = {0};
  if (hash == MH_H_SHA2_256) {
    digest_len     = 32;
    in3_digest_t d = crypto_create_hash(DIGEST_SHA256);
    crypto_update_hash(d, bytes(buf2, stream.bytes_written));
    crypto_finalize_hash(d, d_);
    digest = d_;
  }

  if (digest == NULL)
    GOTO_RET(EXIT, IN3_ENOTSUP);

  size_t mhlen = mh_new_length(hash, digest_len);
  if ((out = _malloc(mhlen)) == NULL)
    GOTO_RET(EXIT, IN3_ENOMEM);

  if (mh_new(out, hash, digest, digest_len) < 0)
    GOTO_RET(EXIT, IN3_EUNKNOWN);

  size_t b58sz = 64;
  *b58         = _malloc(b58sz);
  if (encode(ENC_BASE58, bytes(out, mhlen), *b58) < 0)
    ret = IN3_EUNKNOWN;

EXIT:
  _free(out);
  _free(buf2);
  _free(buf1);
  return ret;
}

in3_ret_t ipfs_verify_hash(const char* content, const char* encoding, const char* requested_hash) {
  bytes_t* buf = NULL;
  if (!strcmp(encoding, "hex"))
    buf = hex_to_new_bytes(content, strlen(content));
  else if (!strcmp(encoding, "utf8"))
    buf = b_new((uint8_t*) content, strlen(content));
  else if (!strcmp(encoding, "base64")) {
    int      l    = strlen(content);
    uint8_t* data = _malloc(decode_size(ENC_BASE64, l));
    l             = decode(ENC_BASE64, content, l, data);
    if (l < 0) {
      _free(data);
      return IN3_ENOTSUP;
    }

    buf  = _malloc(sizeof(bytes_t));
    *buf = bytes(data, l);
  }
  else
    return IN3_ENOTSUP;

  if (buf == NULL)
    return IN3_ENOMEM;

  char*     out = NULL;
  in3_ret_t ret = ipfs_create_hash(buf->data, buf->len, MH_H_SHA2_256, &out);
  if (ret == IN3_OK)
    ret = !strcmp(requested_hash, out) ? IN3_OK : IN3_EINVALDT;

  _free(out);
  b_free(buf);
  return ret;
}

in3_ret_t in3_verify_ipfs(void* pdata, in3_plugin_act_t action, void* pctx) {
  UNUSED_VAR(pdata);
  UNUSED_VAR(action);
  in3_vctx_t* vc = pctx;
  if (vc->chain->type != CHAIN_IPFS) return IN3_EIGNORE;
  if (in3_req_get_proof(vc->req, vc->index) == PROOF_NONE) return IN3_OK;

  // do we have a result? if not it is a vaslid error-response
  if (!vc->result) return IN3_OK;
  d_token_t* params = d_get(vc->request, K_PARAMS);

  // do we support this request?
  if (strcmp(vc->method, FN_IN3_NODELIST) && d_type(vc->result) != T_STRING)
    return vc_err(vc, "Invalid response!");

  if (strcmp(vc->method, FN_IN3_NODELIST) == 0)
    return true;
#if !defined(RPC_ONLY) || defined(RPC_IPFS_GET)
  if (VERIFY_RPC(FN_IPFS_GET))
    return ipfs_verify_hash(d_string(vc->result),
                            d_get_string_at(params, 1) ? d_get_string_at(params, 1) : "base64",
                            d_get_string_at(params, 0));
#endif
#if !defined(RPC_ONLY) || defined(RPC_IPFS_PUT)
  if (VERIFY_RPC(FN_IPFS_PUT))
    return ipfs_verify_hash(d_get_string_at(params, 0),
                            d_get_string_at(params, 1) ? d_get_string_at(params, 1) : "base64",
                            d_string(vc->result));
#endif
  return IN3_EIGNORE;
}
in3_ret_t in3_register_ipfs(in3_t* c) {
  in3_register_eth_nano(c);
  return in3_plugin_register(c, PLGN_ACT_RPC_VERIFY, in3_verify_ipfs, NULL, false);
}
