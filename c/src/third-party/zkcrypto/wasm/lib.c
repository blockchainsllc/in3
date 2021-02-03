#include <stdio.h>
#include <stdlib.h>

#define WASM_RT_MODULE_PREFIX zkcrypto_
#include "../lib.h"
#include "zk-crypto.h"

static void hex(bytes_t b) {
  printf("0x");
  for (int i = 0; i < b.len; i++) printf("%02x", b.data[i]);
  printf("\n");
}

/* import: './web.js' '__wbg_new_59cb74e423758ede' */
u32 (*Z___wbindgen_placeholder__Z___wbg_new_59cb74e423758edeZ_iv)(void);
/* import: './web.js' '__wbg_stack_558ba5917b466edd' */
void (*Z___wbindgen_placeholder__Z___wbg_stack_558ba5917b466eddZ_vii)(u32, u32);
/* import: './web.js' '__wbg_error_4bb6c2a97407129a' */
void (*Z___wbindgen_placeholder__Z___wbg_error_4bb6c2a97407129aZ_vii)(u32, u32);
/* import: './web.js' '__wbindgen_object_drop_ref' */
void (*Z___wbindgen_placeholder__Z___wbindgen_object_drop_refZ_vi)(u32);



/* import: '__wbindgen_placeholder__' '__wbindgen_string_new' */
u32 (*Z___wbindgen_placeholder__Z___wbindgen_string_newZ_iii)(u32, u32);
/* import: '__wbindgen_placeholder__' '__wbindgen_throw' */
void (*Z___wbindgen_placeholder__Z___wbindgen_throwZ_vii)(u32, u32);
/* import: '__wbindgen_placeholder__' '__wbindgen_rethrow' */
void (*Z___wbindgen_placeholder__Z___wbindgen_rethrowZ_vi)(u32);

/* import: '__wbindgen_placeholder__' '__wbindgen_debug_string' */
void (*Z___wbindgen_placeholder__Z___wbindgen_debug_stringZ_vii)(u32, u32);



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
/* import: './web.js' '__wbg_stack_558ba5917b466edd' */
void zke_debug_string(u32 p, u32 idx) {
  printf("# debug_string\n");
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

/* import: '__wbindgen_placeholder__' '__wbindgen_string_new' */
u32 zke_string_new(u32 a, u32 b) {
  printf("# zke_string_new\n");
  u32 sp = wmalloc(b+1);
  memcpy(mem_ptr(sp),mem_ptr(a),b+1);
  char* s = (void*)mem_ptr(sp);
  s[b]=0;
  return sp;
}
/* import: '__wbindgen_placeholder__' '__wbindgen_throw' */
void zke_throw(u32 a, u32 b) {
  if (a || b)
  printf("# zke_throw\n");

}
/* import: '__wbindgen_placeholder__' '__wbindgen_rethrow' */
void zke_rethrow(u32 x) {
  if (x)
  printf("# zke_rethrow\n");

}

void zkcrypto_initialize() {
  zkcrypto_init();

  /* import: './web.js' '__wbg_new_59cb74e423758ede' */
  Z___wbindgen_placeholder__Z___wbg_new_59cb74e423758edeZ_iv = zke_add;
  /* import: './web.js' '__wbg_stack_558ba5917b466edd' */
  Z___wbindgen_placeholder__Z___wbg_stack_558ba5917b466eddZ_vii = zke_stack;
  /* import: './web.js' '__wbg_error_4bb6c2a97407129a' */
  Z___wbindgen_placeholder__Z___wbg_error_4bb6c2a97407129aZ_vii = zke_error;
  /* import: './web.js' '__wbindgen_object_drop_ref' */
  Z___wbindgen_placeholder__Z___wbindgen_object_drop_refZ_vi = zke_drop;

  /* import: '__wbindgen_placeholder__' '__wbindgen_string_new' */
  Z___wbindgen_placeholder__Z___wbindgen_string_newZ_iii= zke_string_new;
  /* import: '__wbindgen_placeholder__' '__wbindgen_throw' */
  Z___wbindgen_placeholder__Z___wbindgen_throwZ_vii = zke_throw;
  /* import: '__wbindgen_placeholder__' '__wbindgen_rethrow' */
  Z___wbindgen_placeholder__Z___wbindgen_rethrowZ_vi = zke_rethrow;
  /* import: '__wbindgen_placeholder__' '__wbindgen_debug_string' */
  Z___wbindgen_placeholder__Z___wbindgen_debug_stringZ_vii = zke_debug_string;

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
  zkcrypto_Z_private_key_to_pubkeyZ_viii(8, sp, 32);
  u32 r0 = mem_u32(2);
  u32 r1 = mem_u32(3);
  if (r1 == 32) memcpy(dst, mem_ptr(r0), 32);
  wfree(r0, r1);
  return r1 == 32 ? IN3_OK : IN3_EINVAL;
}

in3_ret_t zkcrypto_pk_to_pubkey_hash(bytes32_t pk, uint8_t* dst) {
  u32 sp = wmalloc(32);
  memcpy(mem_ptr(sp), pk, 32);
  zkcrypto_Z_private_key_to_pubkey_hashZ_viii(8, sp, 32);
  u32 r0 = mem_u32(2);
  u32 r1 = mem_u32(3);
  if (r1 == 20) memcpy(dst, mem_ptr(r0), 20);
  wfree(r0, r1);
  return r1 == 20 ? IN3_OK : IN3_EINVAL;
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

in3_ret_t zkcrypto_compute_aggregated_pubkey(bytes_t keys, uint8_t* dst) {
  u32 pub_keys = wmalloc(keys.len);
  memcpy(mem_ptr(pub_keys), keys.data, keys.len);
  zkcrypto_Z_musigbn256wasmaggregatedpubkey_computeZ_viii(8, pub_keys, keys.len);
  u32 r0 = mem_u32(2);
  u32 r1 = mem_u32(3);
  if (r1 == 32) memcpy(dst, mem_ptr(r0), r1);
  wfree(r0, r1);
  return r1 == 32 ? IN3_OK : IN3_EINVAL;
}

zkcrypto_signer_t zkcrypto_signer_new(bytes_t pub_keys, uint32_t pos) {
  u32 keys = wmalloc(pub_keys.len);
  memcpy(mem_ptr(keys), pub_keys.data, pub_keys.len);
  return (void*) (uint64_t) zkcrypto_Z_musigbn256wasmsigner_newZ_iiii(keys, pub_keys.len, pos);
}

void zkcrypto_signer_free(zkcrypto_signer_t signer) {
  return zkcrypto_Z___wbg_musigbn256wasmsigner_freeZ_vi((u32) signer);
}

in3_ret_t zkcrypto_signer_compute_precommitment(zkcrypto_signer_t signer, bytes_t seed, uint8_t* dst) {
  u32 data = wmalloc(seed.len);
  memcpy(mem_ptr(data), seed.data, seed.len);
  zkcrypto_Z_musigbn256wasmsigner_compute_precommitmentZ_viiii(8, (u32) signer, data, seed.len/4);
  u32 r0 = mem_u32(2);
  u32 r1 = mem_u32(3);
  if (r1 == 32) memcpy(dst, mem_ptr(r0), r1);
  wfree(r0, r1);
  return r1 == 32 ? IN3_OK : IN3_EINVAL;
}


in3_ret_t zkcrypto_signer_receive_precommitment(zkcrypto_signer_t signer, bytes_t input, uint8_t* dst) {
  u32 data = wmalloc(input.len);
  memcpy(mem_ptr(data), input.data, input.len);
  zkcrypto_Z_musigbn256wasmsigner_receive_precommitmentsZ_viiii(8, (u32) signer, data, input.len);
  u32 r0 = mem_u32(2);
  u32 r1 = mem_u32(3);
  if (r1 == 32) memcpy(dst, mem_ptr(r0), r1);
  wfree(r0, r1);
  return r1 == 32 ? IN3_OK : IN3_EINVAL;
}


in3_ret_t zkcrypto_signer_receive_commitment(zkcrypto_signer_t signer, bytes_t input, uint8_t* dst) {
  u32 data = wmalloc(input.len);
  memcpy(mem_ptr(data), input.data, input.len);
  zkcrypto_Z_musigbn256wasmsigner_receive_commitmentsZ_viiii(8, (u32) signer, data, input.len);
  u32 r0 = mem_u32(2);
  u32 r1 = mem_u32(3);
  if (r1 == 32) memcpy(dst, mem_ptr(r0), r1);
  wfree(r0, r1);
  return r1 == 32 ? IN3_OK : IN3_EINVAL;
}



in3_ret_t zkcrypto_signer_sign(zkcrypto_signer_t signer, bytes32_t pk, bytes_t input, uint8_t* dst) {
  u32 data = wmalloc(input.len);
  memcpy(mem_ptr(data), input.data, input.len);
  u32 pkey = wmalloc(32);
  memcpy(mem_ptr(pkey), pk, 32);
  zkcrypto_Z_musigbn256wasmsigner_signZ_viiiiii(8, (u32) signer, pkey, 32,data, input.len);
  u32 r0 = mem_u32(2);
  u32 r1 = mem_u32(3);
  if (r1==32) memcpy(dst, mem_ptr(r0), r1);
  wfree(r0, r1);
  return r1 ==32 ? IN3_OK : IN3_EINVAL;
}

in3_ret_t zkcrypto_signer_receive_signature_shares(zkcrypto_signer_t signer,  bytes_t input, uint8_t* dst) {
  u32 data = wmalloc(input.len);
  memcpy(mem_ptr(data), input.data, input.len);
  zkcrypto_Z_musigbn256wasmsigner_receive_signature_sharesZ_viiii(8, (u32) signer, data, input.len);
  u32 r0 = mem_u32(2);
  u32 r1 = mem_u32(3);
  if (r1==64) memcpy(dst, mem_ptr(r0), r1);
  wfree(r0, r1);
  return r1==64  ? IN3_OK : IN3_EINVAL;
}

bool zkcrypto_verify_signatures(bytes_t message, bytes_t pubkeys, bytes_t signature) {
  u32 _message = wmalloc(message.len);
  memcpy(mem_ptr(_message), message.data, message.len);
  u32 _pubkeys = wmalloc(pubkeys.len);
  memcpy(mem_ptr(_pubkeys), pubkeys.data, pubkeys.len);
  u32 _signature = wmalloc(signature.len);
  memcpy(mem_ptr(_signature), signature.data, signature.len);
  return zkcrypto_Z_musigbn256wasmverifier_verifyZ_iiiiiii(_message, message.len, _pubkeys,pubkeys.len, _signature,signature.len)!=0;
}


in3_ret_t zkcrypto_pubkey_hash(bytes_t pubkey, uint8_t* dst) {
  u32 pk = wmalloc(pubkey.len);
  memcpy(mem_ptr(pk), pubkey.data, pubkey.len);
  zkcrypto_Z_pubKeyHashZ_viii(8, pk, pubkey.len);
  u32 r0 = mem_u32(2);
  u32 r1 = mem_u32(3);
  if (r1 == 20) memcpy(dst, mem_ptr(r0), r1);
  wfree(r0, r1);
  return r1 == 20 ? IN3_OK : IN3_EINVAL;
}



bool zkcrypto_verify_musig(bytes_t message,  bytes_t signature) {
  u32 _message = wmalloc(message.len);
  memcpy(mem_ptr(_message), message.data, message.len);
  u32 _signature = wmalloc(signature.len);
  memcpy(mem_ptr(_signature), signature.data, signature.len);
  return zkcrypto_Z_verify_musigZ_iiiii(_message, message.len, _signature,signature.len)!=0;
}

