#include "../../core/client/context_internal.h"
#include "../../core/client/plugin.h"
#include "../../core/util/log.h"
#include "../../third-party/crypto/bignum.h"
#include "../../third-party/zkcrypto/lib.h"
#include "zksync.h"
#include <assert.h>
#include <limits.h> /* strtoull */
#include <stdlib.h> /* strtoull */

#define expect_params_eq(n) if (d_len(params)!=n) return  ctx_set_error(ctx->ctx, "Wrong number of arguments", IN3_EINVAL);
#define TRY_SIGNER(x) TRY_FINAL(x,zkcrypto_signer_free(signer))

static int get_pubkey_pos(zksync_config_t* conf) {
    bytes32_t pubkey;
    if (!conf->musig_pub_keys.data) return -1;
    if (memiszero(conf->sync_key,32)) return -1; 
    TRY(zkcrypto_pk_to_pubkey(conf->sync_key,pubkey));
    for (unsigned int i=0;i<conf->musig_pub_keys.len/32;i++) {
        if (memcmp(pubkey,conf->musig_pub_keys.data+i*32,32)==0) return i;
    }
    return -1;
} 


in3_ret_t zksync_musig_create_pre_commit(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx, d_token_t* params) {
    expect_params_eq(1)
    int pos = get_pubkey_pos(conf);
    TRY(pos)
    bytes_t seed = d_to_bytes(params+1);
    zkcrypto_signer_t signer = zkcrypto_signer_new(conf->musig_pub_keys,pos);
    bytes32_t pre_commit;
    in3_ret_t r = zkcrypto_signer_compute_precommitment(signer, seed, pre_commit);
    zkcrypto_signer_free(signer);
    TRY(r)
    return in3_rpc_handle_with_bytes(ctx, bytes(pre_commit, 32));
}
