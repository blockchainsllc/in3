#ifndef ZK_CRYPTO_H_GENERATED_
#define ZK_CRYPTO_H_GENERATED_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "wasm-rt.h"

#ifndef WASM_RT_MODULE_PREFIX
#define WASM_RT_MODULE_PREFIX zkcrypto_
#endif

#define WASM_RT_PASTE_(x, y)  x##y
#define WASM_RT_PASTE(x, y)   WASM_RT_PASTE_(x, y)
#define WASM_RT_ADD_PREFIX(x) WASM_RT_PASTE(WASM_RT_MODULE_PREFIX, x)

/* TODO(binji): only use stdint.h types in header */
typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;
typedef float f32;
typedef double f64;

extern void WASM_RT_ADD_PREFIX(init)(void);

/* import: '__wbindgen_placeholder__' '__wbindgen_object_drop_ref' */
extern void (*Z___wbindgen_placeholder__Z___wbindgen_object_drop_refZ_vi)(u32);
/* import: '__wbindgen_placeholder__' '__wbindgen_string_new' */
extern u32 (*Z___wbindgen_placeholder__Z___wbindgen_string_newZ_iii)(u32, u32);
/* import: '__wbindgen_placeholder__' '__wbg_new_59cb74e423758ede' */
extern u32 (*Z___wbindgen_placeholder__Z___wbg_new_59cb74e423758edeZ_iv)(void);
/* import: '__wbindgen_placeholder__' '__wbg_stack_558ba5917b466edd' */
extern void (*Z___wbindgen_placeholder__Z___wbg_stack_558ba5917b466eddZ_vii)(u32, u32);
/* import: '__wbindgen_placeholder__' '__wbg_error_4bb6c2a97407129a' */
extern void (*Z___wbindgen_placeholder__Z___wbg_error_4bb6c2a97407129aZ_vii)(u32, u32);
/* import: '__wbindgen_placeholder__' '__wbindgen_debug_string' */
extern void (*Z___wbindgen_placeholder__Z___wbindgen_debug_stringZ_vii)(u32, u32);
/* import: '__wbindgen_placeholder__' '__wbindgen_throw' */
extern void (*Z___wbindgen_placeholder__Z___wbindgen_throwZ_vii)(u32, u32);
/* import: '__wbindgen_placeholder__' '__wbindgen_rethrow' */
extern void (*Z___wbindgen_placeholder__Z___wbindgen_rethrowZ_vi)(u32);

/* export: 'memory' */
extern wasm_rt_memory_t (*WASM_RT_ADD_PREFIX(Z_memory));
/* export: 'zc_verify_signatures' */
extern u32 (*WASM_RT_ADD_PREFIX(Z_zc_verify_signaturesZ_iiiiii))(u32, u32, u32, u32, u32);
/* export: '__wbg_musigbn256wasmverifier_free' */
extern void (*WASM_RT_ADD_PREFIX(Z___wbg_musigbn256wasmverifier_freeZ_vi))(u32);
/* export: 'musigbn256wasmverifier_verify' */
extern u32 (*WASM_RT_ADD_PREFIX(Z_musigbn256wasmverifier_verifyZ_iiiiiii))(u32, u32, u32, u32, u32, u32);
/* export: 'zc_compute_aggregated_pubkey' */
extern void (*WASM_RT_ADD_PREFIX(Z_zc_compute_aggregated_pubkeyZ_viii))(u32, u32, u32);
/* export: '__wbg_musigbn256wasmaggregatedpubkey_free' */
extern void (*WASM_RT_ADD_PREFIX(Z___wbg_musigbn256wasmaggregatedpubkey_freeZ_vi))(u32);
/* export: 'musigbn256wasmaggregatedpubkey_compute' */
extern void (*WASM_RT_ADD_PREFIX(Z_musigbn256wasmaggregatedpubkey_computeZ_viii))(u32, u32, u32);
/* export: 'zc_signer_new' */
extern u32 (*WASM_RT_ADD_PREFIX(Z_zc_signer_newZ_iiii))(u32, u32, u32);
/* export: 'zc_signer_free' */
extern void (*WASM_RT_ADD_PREFIX(Z_zc_signer_freeZ_vi))(u32);
/* export: 'zc_signer_compute_precommitment' */
extern void (*WASM_RT_ADD_PREFIX(Z_zc_signer_compute_precommitmentZ_viiii))(u32, u32, u32, u32);
/* export: 'zc_signer_receive_precommitments' */
extern void (*WASM_RT_ADD_PREFIX(Z_zc_signer_receive_precommitmentsZ_viiii))(u32, u32, u32, u32);
/* export: 'zc_signer_receive_commitments' */
extern void (*WASM_RT_ADD_PREFIX(Z_zc_signer_receive_commitmentsZ_viiii))(u32, u32, u32, u32);
/* export: 'zc_signer_sign' */
extern void (*WASM_RT_ADD_PREFIX(Z_zc_signer_signZ_viiiii))(u32, u32, u32, u32, u32);
/* export: 'zc_signer_receive_signature_shares' */
extern void (*WASM_RT_ADD_PREFIX(Z_zc_signer_receive_signature_sharesZ_viiii))(u32, u32, u32, u32);
/* export: '__wbg_musigbn256wasmsigner_free' */
extern void (*WASM_RT_ADD_PREFIX(Z___wbg_musigbn256wasmsigner_freeZ_vi))(u32);
/* export: 'musigbn256wasmsigner_new' */
extern u32 (*WASM_RT_ADD_PREFIX(Z_musigbn256wasmsigner_newZ_iiii))(u32, u32, u32);
/* export: 'musigbn256wasmsigner_compute_precommitment' */
extern void (*WASM_RT_ADD_PREFIX(Z_musigbn256wasmsigner_compute_precommitmentZ_viiii))(u32, u32, u32, u32);
/* export: 'musigbn256wasmsigner_receive_precommitments' */
extern void (*WASM_RT_ADD_PREFIX(Z_musigbn256wasmsigner_receive_precommitmentsZ_viiii))(u32, u32, u32, u32);
/* export: 'musigbn256wasmsigner_receive_commitments' */
extern void (*WASM_RT_ADD_PREFIX(Z_musigbn256wasmsigner_receive_commitmentsZ_viiii))(u32, u32, u32, u32);
/* export: 'musigbn256wasmsigner_sign' */
extern void (*WASM_RT_ADD_PREFIX(Z_musigbn256wasmsigner_signZ_viiiiii))(u32, u32, u32, u32, u32, u32);
/* export: 'musigbn256wasmsigner_receive_signature_shares' */
extern void (*WASM_RT_ADD_PREFIX(Z_musigbn256wasmsigner_receive_signature_sharesZ_viiii))(u32, u32, u32, u32);
/* export: 'zc_private_key_from_seed' */
extern void (*WASM_RT_ADD_PREFIX(Z_zc_private_key_from_seedZ_viii))(u32, u32, u32);
/* export: 'zc_private_key_to_pubkey_hash' */
extern void (*WASM_RT_ADD_PREFIX(Z_zc_private_key_to_pubkey_hashZ_vii))(u32, u32);
/* export: 'zc_private_key_to_pubkey' */
extern void (*WASM_RT_ADD_PREFIX(Z_zc_private_key_to_pubkeyZ_vii))(u32, u32);
/* export: 'zc_pubkey_to_pubkey_hash' */
extern void (*WASM_RT_ADD_PREFIX(Z_zc_pubkey_to_pubkey_hashZ_vii))(u32, u32);
/* export: 'zc_sign_musig' */
extern void (*WASM_RT_ADD_PREFIX(Z_zc_sign_musigZ_viiii))(u32, u32, u32, u32);
/* export: 'zc_verify_musig' */
extern u32 (*WASM_RT_ADD_PREFIX(Z_zc_verify_musigZ_iiii))(u32, u32, u32);
/* export: 'zc_init' */
extern void (*WASM_RT_ADD_PREFIX(Z_zc_initZ_vv))(void);
/* export: 'zksync_crypto_init' */
extern void (*WASM_RT_ADD_PREFIX(Z_zksync_crypto_initZ_vv))(void);
/* export: 'privateKeyFromSeed' */
extern void (*WASM_RT_ADD_PREFIX(Z_privateKeyFromSeedZ_viii))(u32, u32, u32);
/* export: 'private_key_to_pubkey_hash' */
extern void (*WASM_RT_ADD_PREFIX(Z_private_key_to_pubkey_hashZ_viii))(u32, u32, u32);
/* export: 'pubKeyHash' */
extern void (*WASM_RT_ADD_PREFIX(Z_pubKeyHashZ_viii))(u32, u32, u32);
/* export: 'private_key_to_pubkey' */
extern void (*WASM_RT_ADD_PREFIX(Z_private_key_to_pubkeyZ_viii))(u32, u32, u32);
/* export: 'verify_musig' */
extern u32 (*WASM_RT_ADD_PREFIX(Z_verify_musigZ_iiiii))(u32, u32, u32, u32);
/* export: 'sign_musig' */
extern void (*WASM_RT_ADD_PREFIX(Z_sign_musigZ_viiiii))(u32, u32, u32, u32, u32);
/* export: '__wbindgen_malloc' */
extern u32 (*WASM_RT_ADD_PREFIX(Z___wbindgen_mallocZ_ii))(u32);
/* export: '__wbindgen_realloc' */
extern u32 (*WASM_RT_ADD_PREFIX(Z___wbindgen_reallocZ_iiii))(u32, u32, u32);
/* export: '__wbindgen_free' */
extern void (*WASM_RT_ADD_PREFIX(Z___wbindgen_freeZ_vii))(u32, u32);
#ifdef __cplusplus
}
#endif

#endif  /* ZK_CRYPTO_H_GENERATED_ */
