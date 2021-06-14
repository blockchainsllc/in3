#include "../../core/client/keys.h"
#include "../../core/client/plugin.h"
#include "../../core/client/request_internal.h"
#include "../../core/util/debug.h"
#include "../../core/util/log.h"
#include "../../third-party/crypto/bignum.h"
#include "../../third-party/zkcrypto/lib.h"
#include "zk_helper.h"
#include "zksync.h"
#include <assert.h>
#include <limits.h> /* strtoull */
#include <stdlib.h> /* strtoull */
#define MAX_SESSIONS 20

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
void       cleanup_session(zk_musig_session_t* s, zksync_config_t* conf);
static int get_pubkey_pos(zksync_config_t* conf, bytes_t pub_keys, in3_req_t* ctx) {
  if (!pub_keys.data) return req_set_error(ctx, "missing public keys in config", IN3_EINVAL);
  if (memiszero(conf->sync_key, 32)) return req_set_error(ctx, "missing signing keys in config", IN3_EINVAL);
  if (memiszero(conf->pub_key, 32)) {
    TRY(zkcrypto_pk_to_pubkey(conf->sync_key, conf->pub_key));
  }
  for (unsigned int i = 0; i < pub_keys.len / 32; i++) {
    if (memcmp(conf->pub_key, pub_keys.data + i * 32, 32) == 0) return i;
  }
  return -1;
}

static in3_ret_t send_sign_request(in3_req_t* parent, int pos, zksync_config_t* conf, char* method, char* params, d_token_t** result) {
  if (params == NULL) params = "";
  char* url = conf->musig_urls ? conf->musig_urls[pos] : NULL;
  if (!url) return req_set_error(parent, "missing url to fetch a signature", IN3_EINVAL);
  char* in3 = alloca(strlen(url) + 26);
  sprintf(in3, "{\"rpc\":\"%s\"}", url);
  return req_send_sub_request(parent, method, params, in3, result, NULL);
}

static in3_ret_t update_session(zk_musig_session_t* s, in3_req_t* ctx, d_token_t* data) {
  if (!data || d_type(data) != T_OBJECT) return req_set_error(ctx, "invalid response from signer handler", IN3_EINVAL);
  bytes_t d = d_to_bytes(d_get(data, key("pre_commitment")));
  if (!d.data || d.len != s->len * 32) return req_set_error(ctx, "invalid precommitment from signer handler", IN3_EINVAL);
  for (unsigned int i = 0; i < s->len; i++) {
    if (i != s->pos && memiszero(s->precommitments.data + i * 32, 32) && !memiszero(d.data + i * 32, 32)) memcpy(s->precommitments.data + i * 32, d.data + i * 32, 32);
  }
  d = d_to_bytes(d_get(data, key("commitment")));
  if (d.data) {
    if (d.len != s->len * 32) return req_set_error(ctx, "invalid commitment from signer handler", IN3_EINVAL);
    for (unsigned int i = 0; i < s->len; i++) {
      if (i != s->pos && memiszero(s->commitments.data + i * 32, 32) && !memiszero(d.data + i * 32, 32)) memcpy(s->commitments.data + i * 32, d.data + i * 32, 32);
    }
  }
  d = d_to_bytes(d_get(data, key("sig")));
  if (d.data) {
    if (d.len != s->len * 32) return req_set_error(ctx, "invalid sigshares from signer handler", IN3_EINVAL);
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

static in3_ret_t request_message(zksync_config_t* conf, zk_musig_session_t* s, int pos, bytes_t* message, in3_req_t* ctx, d_token_t** result) {
  sb_t sb = {0};
  sb_add_bytes(&sb, "{\"message\":", message, 1, false);
  sb_add_bytes(&sb, ",\"pub_keys\":", &s->pub_keys, 1, false);
  if (s->proof_data) {
    sb_add_chars(&sb, ",\"proof\":");
    sb_add_chars(&sb, s->proof_data);
    if (conf->account) {
      sb_add_rawbytes(&sb, ",\"account\":\"0x", bytes(conf->account, 20), 0);
      sb_add_chars(&sb, "\"");
    }
  }
  sb_add_char(&sb, ',');
  add_sessiondata(&sb, s);
  sb_add_char(&sb, '}');
  in3_ret_t r = send_sign_request(ctx, pos, conf, "zk_sign", sb.data, result);
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

static void check_max_sessions(zksync_config_t* conf) {
  int c = 0;
  for (zk_musig_session_t* s = conf->musig_sessions; s; s = s->next, c++) {
    if (c == MAX_SESSIONS) {
      cleanup_session(s, conf);
      return;
    }
  }
}

zk_musig_session_t* zk_musig_session_free(zk_musig_session_t* s) {
  in3_log_debug("Freeing session %p\n", s);
  if (!s) return NULL;
  zk_musig_session_t* next = s->next;
  if (s->commitments.data) _free(s->commitments.data);
  if (s->precommitments.data) _free(s->precommitments.data);
  if (s->signature_shares.data) _free(s->signature_shares.data);
  if (s->pub_keys.data) _free(s->pub_keys.data);
  if (s->proof_data) _free(s->proof_data);
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

static in3_ret_t verify_proof(zksync_config_t* conf, in3_req_t* ctx, bytes_t* account, d_token_t* proof, bytes_t* msg, bytes_t* pub_keys) {
  if (!conf->proof_verify_method && !proof) return IN3_OK; // no method to verify configured -> so we accept all
  if (!conf->proof_verify_method) return req_set_error(ctx, "No proof_method configured to verify the proof", IN3_ECONFIG);
  if (!account || account->len != 20) return req_set_error(ctx, "The account is missing in the sign data", IN3_EINVAL);

  bytes32_t pubkey;
  uint8_t*  signer_key = NULL;

  if (memiszero(conf->pub_key, 32)) {
    bytes32_t k;
    TRY(zksync_get_sync_key(conf, ctx, k))
    TRY(zkcrypto_pk_to_pubkey(k, pubkey))
  }
  else
    memcpy(pubkey, conf->pub_key, 32);
  for (unsigned int i = 0; i < pub_keys->len; i += 32) {
    if (memcmp(pubkey, pub_keys->data + i, 32) == 0) continue;
    signer_key = pub_keys->data + i;
    break;
  }

  if (!signer_key) return req_set_error(ctx, "the signer key could not be found!", IN3_EINVAL);
  d_token_t* result     = NULL;
  in3_req_t* sub        = NULL;
  char*      proof_data = d_create_json(ctx->request_context, proof);
  sb_t       sb         = {0};
  sb_add_rawbytes(&sb, "\"0x", *msg, 0);
  sb_add_rawbytes(&sb, "\",\"0x", *account, 0);
  sb_add_rawbytes(&sb, "\",\"0x", bytes(signer_key, 32), 0);
  sb_add_chars(&sb, "\",");
  sb_add_chars(&sb, proof_data);
  _free(proof_data);

  TRY_FINAL(req_send_sub_request(ctx, conf->proof_verify_method, sb.data, NULL, &result, &sub), _free(sb.data))

  in3_ret_t ret = (d_type(result) == T_BOOLEAN && d_int(result)) ? IN3_OK : req_set_error(ctx, "Proof could not be verified!", IN3_EINVAL);
  req_remove_required(ctx, sub, false);
  return ret;
}

static in3_ret_t create_proof(zksync_config_t* conf, in3_req_t* ctx, bytes_t* msg, char** proof_data) {
  if (!conf->proof_create_method) return req_set_error(ctx, "No proof_method configured to verify the proof", IN3_ECONFIG);

  // prepare the arguments to create the proof
  d_token_t* result = NULL;
  uint8_t*   account;
  in3_req_t* sub = NULL;
  TRY(zksync_get_account(conf, ctx, &account))
  sb_t sb = {0};
  sb_add_rawbytes(&sb, "\"0x", *msg, 0);
  sb_add_rawbytes(&sb, "\",\"0x", bytes(account, 20), 0);
  sb_add_chars(&sb, "\"");

  // send the subrequest and wait for a response
  TRY_FINAL(req_send_sub_request(ctx, conf->proof_create_method, sb.data, NULL, &result, &sub), _free(sb.data))

  // handle error
  if (!result) req_set_error(ctx, "Proof could not be created!", IN3_EINVAL);

  // only copy the data as json, so we can store them without a json_ctx and can clean up.
  if (sub) {
    *proof_data = d_create_json(sub->response_context, result);
    req_remove_required(ctx, sub, false);
    return IN3_OK;
  }
  return IN3_ERPC;
}

static bool is_complete(bytes_t data) {
  for (unsigned int i = 0; i < data.len; i += 32) {
    if (memiszero(data.data + i, 32)) return false;
  }
  return true;
}

in3_ret_t zksync_musig_sign(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx) {
  if (d_get(d_get(ctx->request, key("in3")), key("rpc"))) return IN3_EIGNORE;
  CHECK_PARAMS_LEN(ctx->req, ctx->params, 1);
  d_token_t* result  = NULL;
  d_token_t* proof   = NULL;
  bytes_t*   account = NULL;
  bytes_t    message;

  if (d_type(ctx->params + 1) == T_OBJECT) {
    result  = ctx->params + 1;
    message = d_to_bytes(d_get(result, key("message")));
    account = d_get_bytes(result, key("account"));
    proof   = d_get(result, K_PROOF);
    if (!message.data) return req_set_error(ctx->req, "missing message in request", IN3_EINVAL);
  }
  else {
    message = d_to_bytes(ctx->params + 1);
    if (d_len(ctx->params) > 1) proof = d_get_at(ctx->params, 1);
    if (!conf->musig_pub_keys.data) {
      bytes32_t pk;
      uint8_t   sig[96];
      TRY(zksync_get_sync_key(conf, ctx->req, pk))
      TRY(zkcrypto_sign_musig(pk, message, sig))
      return in3_rpc_handle_with_bytes(ctx, bytes(sig, 96));
    }
  }
  bytes32_t hash;
  keccak(message, hash);
  uint64_t            session_id = *((uint64_t*) (void*) hash);
  zk_musig_session_t* s          = get_session(conf, session_id);
  if (s == NULL) {
    TRY(zksync_get_sync_key(conf, ctx->req, NULL))

    char*   proof_data = NULL;
    bytes_t pub_keys   = result ? d_to_bytes(d_get(result, key("pub_keys"))) : bytes(NULL, 0);
    if (!pub_keys.data && conf->musig_pub_keys.data) pub_keys = conf->musig_pub_keys;
    if (!pub_keys.data) return req_set_error(ctx->req, "no public keys found for musig signature", IN3_EINVAL);
    int pos = get_pubkey_pos(conf, pub_keys, ctx->req);
    in3_log_debug("create new session with pub_key pos %d\n", pos);
    TRY(pos)

    // if a method is specified we create the proof here
    if (conf->proof_create_method && proof == NULL)
      TRY(create_proof(conf, ctx->req, &message, &proof_data))
    else
      TRY(verify_proof(conf, ctx->req, account, proof, &message, &pub_keys))

    // make sure we don't have too many old sessions.
    check_max_sessions(conf);

    // create a new session
    s                    = _calloc(1, sizeof(zk_musig_session_t));
    s->id                = session_id;
    s->pos               = (unsigned int) pos;
    s->proof_data        = proof_data;
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
  if (result) TRY_SIG(update_session(s, ctx->req, result))

  // do we have all precommits?
  for (unsigned int i = 0; i < s->len; i++) {
    if (s->pos != i && memiszero(s->precommitments.data + i * 32, 32) && conf->musig_urls && conf->musig_urls[i]) {
      TRY_SIG(request_message(conf, s, i, &message, ctx->req, &result))
      if (memiszero(s->precommitments.data + i * 32, 32)) TRY_SIG(req_set_error(ctx->req, "no precommit from signer set", IN3_EINVAL))
    }
  }

  // set the precommits
  if (!pre_commit_complete && is_complete(s->precommitments)) TRY_SIG(zkcrypto_signer_receive_precommitment(s->signer, s->precommitments, s->commitments.data + s->pos * 32))

  // do we have all commits?
  for (unsigned int i = 0; i < s->len; i++) {
    if (s->pos != i && memiszero(s->commitments.data + i * 32, 32) && conf->musig_urls && conf->musig_urls[i]) {
      TRY_SIG(request_message(conf, s, i, &message, ctx->req, &result))
      if (memiszero(s->commitments.data + i * 32, 32)) TRY_SIG(req_set_error(ctx->req, "no commit from signer set", IN3_EINVAL))
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
      TRY_SIG(request_message(conf, s, i, &message, ctx->req, &result))
      if (memiszero(s->signature_shares.data + i * 32, 32)) TRY_SIG(req_set_error(ctx->req, "no signature from signer set", IN3_EINVAL))
    }
  }

  if (is_complete(s->signature_shares) && d_type(ctx->params + 1) != T_OBJECT) {
    uint8_t res[96];
    TRY_SIG(zkcrypto_compute_aggregated_pubkey(s->pub_keys, res))
    TRY_SIG(zkcrypto_signer_receive_signature_shares(s->signer, s->signature_shares, res + 32))
    cleanup_session(s, conf);
    //    if (!zkcrypto_verify_signatures(message, conf->musig_pub_keys, bytes(res, 96)))
    //      return req_set_error(ctx->req, "invalid signature", IN3_EINVAL);
    return in3_rpc_handle_with_bytes(ctx, bytes(res, 96));
  }

  sb_t* rr = in3_rpc_handle_start(ctx);
  sb_add_char(rr, '{');
  add_sessiondata(rr, s);
  if (s->proof_data) {
    sb_add_chars(rr, ",\"proof\":");
    sb_add_chars(rr, s->proof_data);
  }
  sb_add_char(rr, '}');
  return in3_rpc_handle_finish(ctx);
}
