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
typedef uint8_t  u8;
typedef int8_t   s8;
typedef uint16_t u16;
typedef int16_t  s16;
typedef uint32_t u32;
typedef int32_t  s32;
typedef uint64_t u64;
typedef int64_t  s64;
typedef float    f32;
typedef double   f64;

extern void WASM_RT_ADD_PREFIX(init)(void);

/* import: './web.js' '__wbg_new_59cb74e423758ede' */
extern u32 (*Z_Z2EZ2FwebZ2EjsZ___wbg_new_59cb74e423758edeZ_iv)(void);
/* import: './web.js' '__wbg_stack_558ba5917b466edd' */
extern void (*Z_Z2EZ2FwebZ2EjsZ___wbg_stack_558ba5917b466eddZ_vii)(u32, u32);
/* import: './web.js' '__wbg_error_4bb6c2a97407129a' */
extern void (*Z_Z2EZ2FwebZ2EjsZ___wbg_error_4bb6c2a97407129aZ_vii)(u32, u32);
/* import: './web.js' '__wbindgen_object_drop_ref' */
extern void (*Z_Z2EZ2FwebZ2EjsZ___wbindgen_object_drop_refZ_vi)(u32);

/* export: 'memory' */
extern wasm_rt_memory_t(*WASM_RT_ADD_PREFIX(Z_memory));
/* export: 'init' */
extern void (*WASM_RT_ADD_PREFIX(Z_initZ_vv))(void);
/* export: 'privateKeyFromSeed' */
extern void (*WASM_RT_ADD_PREFIX(Z_privateKeyFromSeedZ_viii))(u32, u32, u32);
/* export: 'private_key_to_pubkey_hash' */
extern void (*WASM_RT_ADD_PREFIX(Z_private_key_to_pubkey_hashZ_viii))(u32, u32, u32);
/* export: 'sign_musig' */
extern void (*WASM_RT_ADD_PREFIX(Z_sign_musigZ_viiiii))(u32, u32, u32, u32, u32);
/* export: '__wbindgen_malloc' */
extern u32 (*WASM_RT_ADD_PREFIX(Z___wbindgen_mallocZ_ii))(u32);
/* export: '__wbindgen_free' */
extern void (*WASM_RT_ADD_PREFIX(Z___wbindgen_freeZ_vii))(u32, u32);
/* export: '__wbindgen_realloc' */
extern u32 (*WASM_RT_ADD_PREFIX(Z___wbindgen_reallocZ_iiii))(u32, u32, u32);
#ifdef __cplusplus
}
#endif

#endif /* ZK_CRYPTO_H_GENERATED_ */
