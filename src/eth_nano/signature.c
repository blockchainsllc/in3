#include "eth_nano.h"
#include <util/utils.h>
#include <client/context.h>
#include <string.h>
#include <crypto/secp256k1.h>
#include <crypto/ecdsa.h>
#include <util/mem.h>


int eth_verify_signature(in3_vctx_t *vc, bytes_t *msg_hash, jsmntok_t *sig)
{
    jsmntok_t *t;
    int res = 0, i;

    // check messagehash
    if (!(t = res_get(vc, sig, "msgHash")))
        return 0;
    bytes_t *sig_msg_hash = res_to_bytes(vc, t);
    if (!b_cmp(sig_msg_hash, msg_hash))
    {
        b_free(sig_msg_hash);
        return 0;
    }
    b_free(sig_msg_hash);

    uint8_t pubkey[65];
    bytes_t pubkey_bytes = {.len = 64, .data = ((uint8_t *)&pubkey) + 1};

    uint8_t sdata[64];

    bytes_t *r = res_to_bytes(vc, res_get(vc, sig, "r"));
    bytes_t *s = res_to_bytes(vc, res_get(vc, sig, "s"));
    int v = res_to_long(vc, res_get(vc, sig, "v"), 0);
    if (v >= 27)
        v -= 27;
    if (r == NULL || s == NULL || r->len + s->len != 64)
    {
        b_free(r);
        b_free(s);
        vc_err(vc, "wrong signatuire length");
        return 0;
    }
    memcpy(sdata, r->data, r->len);
    memcpy(sdata + r->len, s->data, s->len);
    b_free(r);
    b_free(s);

    // verify signature
    ecdsa_recover_pub_from_sig(&secp256k1, pubkey, sdata, msg_hash->data, v);

    // now create the address
    bytes_t *hash = sha3(&pubkey_bytes);
    bytes_t *addr = b_new((char *)hash->data + 12, 20);

    for (i = 0; i < vc->config->signaturesCount; i++)
    {
        if (b_cmp(vc->config->signatures + i, addr))
        {
            res = 1 << i;
            break;
        }
    }

    b_free(hash);
    b_free(addr);

    return res;
}
