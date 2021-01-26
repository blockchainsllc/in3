#include "../../core/client/context_internal.h"
#include "../../core/client/plugin.h"
#include "../../core/util/log.h"
#include "../../third-party/crypto/bignum.h"
#include "../../third-party/zkcrypto/lib.h"
#include "zk_helper.h"
#include "zksync.h"
#include <assert.h>
#include <limits.h> /* strtoull */
#include <stdlib.h> /* strtoull */

#define expect_params_eq(n) \
  if (d_len(params) != n) return ctx_set_error(ctx->ctx, "Wrong number of arguments", IN3_EINVAL);
#define TRY_SIGNER(x) TRY_FINAL(x, zkcrypto_signer_free(signer))
#define TRY_SIG(exp)                                                 \
  {                                                                  \
    int _r = (exp);                                                  \
    if (_r < 0) {                                                    \
      if (_r != IN3_WAITING) {                                       \
        in3_log_debug(":::Cleanup Session in line %d \n", __LINE__); \
        cleanup_session(s, conf);                                    \
      }                                                              \
      return _r;                                                     \
    }                                                                \
  }

static int get_pubkey_pos(zksync_config_t* conf, bytes_t pub_keys, in3_ctx_t* ctx) {
  if (!pub_keys.data) return ctx_set_error(ctx, "missing public keys in config", IN3_EINVAL);
  if (memiszero(conf->sync_key, 32)) return ctx_set_error(ctx, "missing signing keys in config", IN3_EINVAL);
  if (memiszero(conf->pub_key, 32)) {
    TRY(zkcrypto_pk_to_pubkey(conf->sync_key, conf->pub_key));
  }
  for (unsigned int i = 0; i < pub_keys.len / 32; i++) {
    if (memcmp(conf->pub_key, pub_keys.data + i * 32, 32) == 0) return i;
  }
  return -1;
}

static in3_ret_t send_sign_request(in3_ctx_t* parent, int pos, zksync_config_t* conf, char* method, char* params, d_token_t** result) {
  if (params == NULL) params = "";
  char* url = conf->musig_urls ? conf->musig_urls[pos] : NULL;
  if (!url) return ctx_set_error(parent, "missing url to fetch a signature", IN3_EINVAL);
  char* in3 = alloca(strlen(url) + 26);
  sprintf(in3, "{\"rpc\":\"%s\"}", url);
  return ctx_send_sub_request(parent, method, params, in3, result);
}

static in3_ret_t update_session(zk_musig_session_t* s, in3_ctx_t* ctx, d_token_t* data) {
  if (!data || d_type(data) != T_OBJECT) return ctx_set_error(ctx, "invalid response from signer handler", IN3_EINVAL);
  bytes_t d = d_to_bytes(d_get(data, key("pre_commitment")));
  if (!d.data || d.len != s->len * 32) return ctx_set_error(ctx, "invalid precommitment from signer handler", IN3_EINVAL);
  for (unsigned int i = 0; i < s->len; i++) {
    if (i != s->pos && memiszero(s->precommitments.data + i * 32, 32) && !memiszero(d.data + i * 32, 32)) memcpy(s->precommitments.data + i * 32, d.data + i * 32, 32);
  }
  d = d_to_bytes(d_get(data, key("commitment")));
  if (d.data) {
    if (d.len != s->len * 32) return ctx_set_error(ctx, "invalid commitment from signer handler", IN3_EINVAL);
    for (unsigned int i = 0; i < s->len; i++) {
      if (i != s->pos && memiszero(s->commitments.data + i * 32, 32) && !memiszero(d.data + i * 32, 32)) memcpy(s->commitments.data + i * 32, d.data + i * 32, 32);
    }
  }
  d = d_to_bytes(d_get(data, key("sig")));
  if (d.data) {
    if (d.len != s->len * 32) return ctx_set_error(ctx, "invalid sigshares from signer handler", IN3_EINVAL);
    for (unsigned int i = 0; i < s->len; i++) {
      if (i != s->pos && memiszero(s->signature_shares.data + i * 32, 32) && !memiszero(d.data + i * 32, 32)) memcpy(s->signature_shares.data + i * 32, d.data + i * 32, 32);
    }
  }
  return IN3_OK;
}

static void add_sessiondata(sb_t* sb, zk_musig_session_t* s) {
  sb_add_bytes(sb, "\"pre_commitment\":", &s->precommitments, 1, false);
  if (!memiszero(s->commitments.data, s->commitments.len))
    sb_add_bytes(sb, ",\"commitment\":", &s->commitments, 1, false);
  if (!memiszero(s->signature_shares.data, s->signature_shares.len))
    sb_add_bytes(sb, ",\"sig\":", &s->signature_shares, 1, false);
}

static in3_ret_t request_message(zksync_config_t* conf, zk_musig_session_t* s, int pos, bytes_t* message, in3_ctx_t* ctx, d_token_t** result) {
  sb_t sb = {0};
  sb_add_bytes(&sb, "{\"message\":", message, 1, false);
  sb_add_bytes(&sb, ",\"pub_keys\":", &s->pub_keys, 1, false);
  sb_add_char(&sb, ',');
  add_sessiondata(&sb, s);
  sb_add_char(&sb, '}');
  in3_ret_t r = send_sign_request(ctx, pos, conf, "zk_musig_sign", sb.data, result);
  if (r == IN3_OK) r = update_session(s, ctx, *result);
  _free(sb.data);
  return r;
}

static zk_musig_session_t* get_session(zksync_config_t* conf, uint64_t id) {
  for (zk_musig_session_t* s = conf->musig_sessions; s; s = s->next) {
    if (s->id == id) return s;
  }
  return NULL;
}

zk_musig_session_t* zk_musig_session_free(zk_musig_session_t* s) {
  in3_log_debug("Freeing session %p\n", s);
  if (!s) return NULL;
  zk_musig_session_t* next = s->next;
  if (s->commitments.data) _free(s->commitments.data);
  if (s->precommitments.data) _free(s->precommitments.data);
  if (s->signature_shares.data) _free(s->signature_shares.data);
  if (s->pub_keys.data) _free(s->pub_keys.data);
  if (s->signer) zkcrypto_signer_free(s->signer);
  _free(s);
  return next;
}

void cleanup_session(zk_musig_session_t* s, zksync_config_t* conf) {
  for (zk_musig_session_t** p = &conf->musig_sessions; *p; p = &((*p)->next)) {
    if (*p == s) {
      *p = zk_musig_session_free(s);
      return;
    }
  }
}

static bool is_complete(bytes_t data) {
  for (unsigned int i = 0; i < data.len; i += 32) {
    if (memiszero(data.data + i, 32)) return false;
  }
  return true;
}

in3_ret_t zksync_musig_sign(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx, d_token_t* params) {
  if (d_get(d_get(ctx->request, key("in3")), key("rpc"))) return IN3_EIGNORE;
  expect_params_eq(1);
  d_token_t* result = NULL;
  bytes_t    message;

  if (d_type(params + 1) == T_OBJECT) {
    result  = params + 1;
    message = d_to_bytes(d_get(result, key("message")));
    if (!message.data) return ctx_set_error(ctx->ctx, "missing message in request", IN3_EINVAL);
  }
  else {
    message = d_to_bytes(params + 1);
    if (!conf->musig_pub_keys.data) {
      bytes32_t pk;
      uint8_t   sig[96];
      TRY(zksync_get_sync_key(conf, ctx->ctx, pk))
      TRY(zkcrypto_sign_musig(pk, message, sig))
      return in3_rpc_handle_with_bytes(ctx, bytes(sig, 96));
    }
  }
  bytes32_t hash;
  keccak(message, hash);
  uint64_t            session_id = *((uint64_t*) (void*) hash);
  zk_musig_session_t* s          = get_session(conf, session_id);
  if (s == NULL) {
    bytes_t pub_keys = result ? d_to_bytes(d_get(result, key("pub_keys"))) : bytes(NULL, 0);
    if (!pub_keys.data && conf->musig_pub_keys.data) pub_keys = conf->musig_pub_keys;
    if (!pub_keys.data) return ctx_set_error(ctx->ctx, "no public keys found for musig signature", IN3_EINVAL);
    int pos = get_pubkey_pos(conf, pub_keys, ctx->ctx);
    in3_log_debug("create new session with pub_key pos %d\n", pos);
    TRY(pos)

    // create a new session
    s                    = _calloc(1, sizeof(zk_musig_session_t));
    s->id                = session_id;
    s->pos               = (unsigned int) pos;
    s->next              = conf->musig_sessions;
    conf->musig_sessions = s;
    s->pub_keys          = bytes_dup(pub_keys);
    s->len               = s->pub_keys.len / 32;
    s->signer            = zkcrypto_signer_new(s->pub_keys, (uint32_t) s->pos);
    s->precommitments    = bytes(_calloc(s->len, 32), s->len * 32);
    s->commitments       = bytes(_calloc(s->len, 32), s->len * 32);
    s->signature_shares  = bytes(_calloc(s->len, 32), s->len * 32);

    uint8_t seed[96];
    memcpy(seed, hash, 32);
    keccak(pub_keys, seed + 32);
    keccak(bytes(conf->sync_key, 32), seed + 64);
    keccak(bytes(seed, 96), s->seed);

    // now we generate our precommit
    TRY_SIG(zkcrypto_signer_compute_precommitment(s->signer, bytes(s->seed, 32), s->precommitments.data + s->pos * 32))
  }

  bool pre_commit_complete = is_complete(s->precommitments);
  bool commit_complete     = is_complete(s->commitments);

  // update data from incoming request
  if (result) TRY_SIG(update_session(s, ctx->ctx, result))

  // do we have all precommits?
  for (unsigned int i = 0; i < s->len; i++) {
    if (s->pos != i && memiszero(s->precommitments.data + i * 32, 32) && conf->musig_urls && conf->musig_urls[i]) {
      TRY_SIG(request_message(conf, s, i, &message, ctx->ctx, &result))
      if (memiszero(s->precommitments.data + i * 32, 32)) TRY_SIG(ctx_set_error(ctx->ctx, "no precommit from signer set", IN3_EINVAL))
    }
  }

  // set the precommits
  if (!pre_commit_complete && is_complete(s->precommitments)) TRY_SIG(zkcrypto_signer_receive_precommitment(s->signer, s->precommitments, s->commitments.data + s->pos * 32))

  // do we have all commits?
  for (unsigned int i = 0; i < s->len; i++) {
    if (s->pos != i && memiszero(s->commitments.data + i * 32, 32) && conf->musig_urls && conf->musig_urls[i]) {
      TRY_SIG(request_message(conf, s, i, &message, ctx->ctx, &result))
      if (memiszero(s->commitments.data + i * 32, 32)) TRY_SIG(ctx_set_error(ctx->ctx, "no commit from signer set", IN3_EINVAL))
    }
  }

  // set the precommits
  if (!commit_complete && is_complete(s->commitments)) {
    TRY_SIG(zkcrypto_signer_receive_commitment(s->signer, s->commitments, hash))
    TRY_SIG(zkcrypto_signer_sign(s->signer, conf->sync_key, message, s->signature_shares.data + s->pos * 32))
  }

  // do we have all signatures?
  for (unsigned int i = 0; i < s->len; i++) {
    if (s->pos != i && memiszero(s->signature_shares.data + i * 32, 32) && conf->musig_urls && conf->musig_urls[i]) {
      TRY_SIG(request_message(conf, s, i, &message, ctx->ctx, &result))
      if (memiszero(s->signature_shares.data + i * 32, 32)) TRY_SIG(ctx_set_error(ctx->ctx, "no signature from signer set", IN3_EINVAL))
    }
  }

  if (is_complete(s->signature_shares) && d_type(params + 1) != T_OBJECT) {
    uint8_t res[96];
    TRY_SIG(zkcrypto_compute_aggregated_pubkey(s->pub_keys, res))
    TRY_SIG(zkcrypto_signer_receive_signature_shares(s->signer, s->signature_shares, res + 32))
    cleanup_session(s, conf);
    in3_log_debug("message:\n");
    b_print(&message);
    in3_log_debug("sig:\n");
    ba_print(res, 96);
    ;
    in3_log_debug("check signature:\n");
    if (!zkcrypto_verify_signatures(message, conf->musig_pub_keys, bytes(res, 96)))
      return ctx_set_error(ctx->ctx, "invalid signature", IN3_EINVAL);
    return in3_rpc_handle_with_bytes(ctx, bytes(res, 96));
  }

  sb_t* rr = in3_rpc_handle_start(ctx);
  sb_add_char(rr, '{');
  add_sessiondata(rr, s);
  sb_add_char(rr, '}');
  return in3_rpc_handle_finish(ctx);
}
