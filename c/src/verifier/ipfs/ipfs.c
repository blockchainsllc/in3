
#include "ipfs.h"
#include "../../core/client/keys.h"
#include "../../core/util/mem.h"
#include "../../third-party/crypto/base58.h"
#include "../../third-party/crypto/sha2.h"
#include "../../third-party/libb64/cdecode.h"
#include "../../third-party/multihash/hashes.h"
#include "../../third-party/multihash/multihash.h"
#include "../../third-party/nanopb/pb_decode.h"
#include "../../third-party/nanopb/pb_encode.h"
#include "ipfs.pb.h"
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
  if (hash == MH_H_SHA2_256) {
    uint8_t d_[32] = {0};
    digest_len     = 32;
    SHA256_CTX ctx;
    sha256_Init(&ctx);
    sha256_Update(&ctx, buf2, stream.bytes_written);
    sha256_Final(&ctx, d_);
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
  if (!b58enc(*b58, &b58sz, out, mhlen))
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
    buf = b_new(content, strlen(content));
  else if (!strcmp(encoding, "base64")) {
    size_t   l    = 0;
    uint8_t* data = base64_decode(content, &l);
    buf           = b_new((char*) data, l);
    free(data);
  } else
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

in3_ret_t in3_verify_ipfs(in3_vctx_t* vc) {
  char*      method = NULL;
  d_token_t* params = d_get(vc->request, K_PARAMS);

  if (vc->config->verification == VERIFICATION_NEVER)
    return IN3_OK;

  // do we have a result? if not it is a vaslid error-response
  if (!vc->result)
    return IN3_OK;

  // do we support this request?
  if (!(method = d_get_stringk(vc->request, K_METHOD)))
    return vc_err(vc, "No Method in request defined!");

  if (strcmp(method, "in3_nodeList") && d_type(vc->result) != T_STRING)
    return vc_err(vc, "Invalid response!");

  if (strcmp(method, "in3_nodeList") == 0)
    return true;
  else if (strcmp(method, "ipfs_get") == 0)
    return ipfs_verify_hash(d_string(vc->result),
                            d_get_string_at(params, 1) ? d_get_string_at(params, 1) : "base64",
                            d_get_string_at(params, 0));
  else if (strcmp(method, "ipfs_put") == 0)
    return ipfs_verify_hash(d_get_string_at(params, 0),
                            d_get_string_at(params, 1) ? d_get_string_at(params, 1) : "base64",
                            d_string(vc->result));
  else
    return vc_err(vc, "method cannot be verified with ipfs verifier!");
}

void in3_register_ipfs() {
  in3_verifier_t* v = _calloc(1, sizeof(in3_verifier_t));
  v->type           = CHAIN_IPFS;
  v->verify         = in3_verify_ipfs;
  in3_register_verifier(v);
}
