#include <stdio.h>
#include <stdlib.h>

#define WASM_RT_MODULE_PREFIX zkcrypto_
#include "lib.h"
#include "zk-crypto.h"

static void hex(bytes_t b) {
  printf("0x");
  for (int i = 0; i < b.len; i++) printf("%02x", b.data[i]);
  printf("\n");
}

/* import: './web.js' '__wbg_new_59cb74e423758ede' */
u32 (*Z_Z2EZ2FwebZ2EjsZ___wbg_new_59cb74e423758edeZ_iv)(void);
/* import: './web.js' '__wbg_stack_558ba5917b466edd' */
void (*Z_Z2EZ2FwebZ2EjsZ___wbg_stack_558ba5917b466eddZ_vii)(u32, u32);
/* import: './web.js' '__wbg_error_4bb6c2a97407129a' */
void (*Z_Z2EZ2FwebZ2EjsZ___wbg_error_4bb6c2a97407129aZ_vii)(u32, u32);
/* import: './web.js' '__wbindgen_object_drop_ref' */
void (*Z_Z2EZ2FwebZ2EjsZ___wbindgen_object_drop_refZ_vi)(u32);

#define wmalloc(l)  zkcrypto_Z___wbindgen_mallocZ_ii(l)
#define wfree(p, l) zkcrypto_Z___wbindgen_freeZ_vii(p, l)
#define mem_ptr(p)  (zkcrypto_Z_memory->data + (p))
#define mem_u32(p)  *((u32*) mem_ptr((p) *4))
#define mem_u8(p)   zkcrypto_Z_memory->data[(p)]

/* import: './web.js' '__wbg_new_59cb74e423758ede' */
u32 zke_add() {
  printf("# zke_add\n");
  // pushes a stack and returns the a idx.
  return 1;
}
/* import: './web.js' '__wbg_stack_558ba5917b466edd' */
void zke_stack(u32 p, u32 idx) {
  printf("# zke_stack\n");
  // returns the stack added previously
  *mem_ptr(p)     = 0;
  *mem_ptr(p + 4) = 0;
}

/* import: './web.js' '__wbg_error_4bb6c2a97407129a' */
void zke_error(u32 a, u32 b) {
  printf("# zke_error\n");
  // report error
  // do nothing for now
}
/* import: './web.js' '__wbindgen_object_drop_ref' */
void zke_drop(u32 a) {
  printf("# zke_drop\n");
}
void zkcrypto_initialize() {
  zkcrypto_init();

  /* import: './web.js' '__wbg_new_59cb74e423758ede' */
  Z_Z2EZ2FwebZ2EjsZ___wbg_new_59cb74e423758edeZ_iv = zke_add;
  /* import: './web.js' '__wbg_stack_558ba5917b466edd' */
  Z_Z2EZ2FwebZ2EjsZ___wbg_stack_558ba5917b466eddZ_vii = zke_stack;
  /* import: './web.js' '__wbg_error_4bb6c2a97407129a' */
  Z_Z2EZ2FwebZ2EjsZ___wbg_error_4bb6c2a97407129aZ_vii = zke_error;
  /* import: './web.js' '__wbindgen_object_drop_ref' */
  Z_Z2EZ2FwebZ2EjsZ___wbindgen_object_drop_refZ_vi = zke_drop;
}

in3_ret_t zkcrypto_pk_from_seed(bytes_t seed, bytes32_t dst) {
  u32 sp = wmalloc(seed.len);
  memcpy(mem_ptr(sp), seed.data, seed.len);
  zkcrypto_Z_privateKeyFromSeedZ_viii(8, sp, seed.len);
  u32 r0 = mem_u32(2);
  u32 r1 = mem_u32(3);
  if (r1 == 32) memcpy(dst, mem_ptr(r0), 32);
  wfree(r0, r1);
  return r1 == 32 ? IN3_OK : IN3_EINVAL;
}
in3_ret_t zkcrypto_pk_to_pubkey(bytes32_t pk, uint8_t* dst) {
  u32 sp = wmalloc(32);
  memcpy(mem_ptr(sp), pk, 32);
  zkcrypto_Z_private_key_to_pubkey_hashZ_viii(8, sp, 32);
  u32 r0 = mem_u32(2);
  u32 r1 = mem_u32(3);
  if (r1 == 32) memcpy(dst, mem_ptr(r0), 32);
  wfree(r0, r1);
  return r1 == 32 ? IN3_OK : IN3_EINVAL;
}

in3_ret_t zkcrypto_sign_musig(bytes32_t pk, bytes_t msg, uint8_t* dst) {
  u32 pkp = wmalloc(32);
  u32 mp  = wmalloc(msg.len);
  memcpy(mem_ptr(pkp), pk, 32);
  memcpy(mem_ptr(mp), msg.data, msg.len);
  zkcrypto_Z_sign_musigZ_viiiii(8, pkp, 32, mp, msg.len);
  u32 r0 = mem_u32(2);
  u32 r1 = mem_u32(3);
  if (r1 == 96) memcpy(dst, mem_ptr(r0), 96);
  wfree(r0, r1);
  return r1 == 96 ? IN3_OK : IN3_EINVAL;
}
