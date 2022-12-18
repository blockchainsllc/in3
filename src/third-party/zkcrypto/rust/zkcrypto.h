#ifndef zkcrypto_h
#define zkcrypto_h

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>


#define STANDARD_ENCODING_LENGTH 32

typedef struct MusigBN256WasmSigner MusigBN256WasmSigner;

void zc_compute_aggregated_pubkey(uint8_t *pks, size_t pks_len, uint8_t *dst);

/**
 * This method initializes params for current thread, otherwise they will be initialized when signing
 * first message.
 */
void zc_init(void);

void zc_private_key_from_seed(uint8_t *seed, size_t seed_len, uint8_t *dst);

void zc_private_key_to_pubkey(uint8_t *pk, uint8_t *dst);

void zc_private_key_to_pubkey_hash(uint8_t *pk, uint8_t *dst);

void zc_pubkey_to_pubkey_hash(uint8_t *pk, uint8_t *dst);

void zc_sign_musig(uint8_t *pk, uint8_t *msg, size_t msg_len, uint8_t *dst);

void zc_signer_compute_precommitment(struct MusigBN256WasmSigner *signer,
                                     uint32_t *seed,
                                     size_t seed_len,
                                     uint8_t *dst);

void zc_signer_free(struct MusigBN256WasmSigner *ptr);

struct MusigBN256WasmSigner *zc_signer_new(uint8_t *pks, size_t pks_len, size_t pos);

void zc_signer_receive_commitments(struct MusigBN256WasmSigner *signer,
                                   uint8_t *input,
                                   size_t len,
                                   uint8_t *dst);

void zc_signer_receive_precommitments(struct MusigBN256WasmSigner *signer,
                                      uint8_t *input,
                                      size_t len,
                                      uint8_t *dst);

void zc_signer_receive_signature_shares(struct MusigBN256WasmSigner *signer,
                                        uint8_t *input,
                                        size_t len,
                                        uint8_t *dst);

void zc_signer_sign(struct MusigBN256WasmSigner *signer,
                    uint8_t *pk,
                    uint8_t *input,
                    size_t len,
                    uint8_t *dst);

bool zc_verify_musig(uint8_t *msg, size_t msg_len, uint8_t *signature);

bool zc_verify_signatures(uint8_t *msg,
                          size_t msg_len,
                          uint8_t *pubkeys,
                          size_t pubkeys_len,
                          uint8_t *signature);

#endif /* zkcrypto_h */
