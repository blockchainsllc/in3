#include "evm.h"
#include "gas.h"
#include <crypto/ecdsa.h>
#include <crypto/ripemd160.h>
#include <crypto/secp256k1.h>

uint8_t evm_is_precompiled(evm_t* evm, address_t address) {
  UNUSED_VAR(evm);
  int l = 20;
  optimize_len(address, l);
  return (l == 1 && *address && *address < 9);
}
int pre_ecrecover(evm_t* evm) {
  subgas(G_PRE_EC_RECOVER);
  if (evm->call_data.len < 128) return 0;

  uint8_t pubkey[65], *vdata = evm->call_data.data + 32, vl = 32;
  optimize_len(vdata, vl);
  if (vl > 1) return 0;

  // verify signature
  if (ecdsa_recover_pub_from_sig(&secp256k1, pubkey, evm->call_data.data + 64, evm->call_data.data, *vdata >= 27 ? *vdata - 27 : *vdata) == 0) {
    evm->return_data.data = _malloc(20);
    evm->return_data.len  = 20;

    uint8_t hash[32];

    // hash it and return the last 20 bytes as address
    bytes_t public_key = {.data = pubkey + 1, .len = 64};
    if (sha3_to(&public_key, hash) == 0)
      memcpy(evm->return_data.data, hash + 12, 20);
  }
  return 0;
}

int pre_sha256(evm_t* evm) {
  subgas(G_PRE_SHA256 + (evm->call_data.len + 31) / 32 * G_PRE_SHA256_WORD);
  evm->return_data.data = _malloc(32);
  evm->return_data.len  = 32;
  sha3_to(&evm->call_data, evm->return_data.data);
  return 0;
}
int pre_ripemd160(evm_t* evm) {
  subgas(G_PRE_RIPEMD160 + (evm->call_data.len + 31) / 32 * G_PRE_RIPEMD160_WORD);
  evm->return_data.data = _malloc(20);
  evm->return_data.len  = 20;
  ripemd160(evm->call_data.data, evm->call_data.len, evm->return_data.data);
  return 0;
}
int pre_identity(evm_t* evm) {
  subgas(G_PRE_IDENTITY + (evm->call_data.len + 31) / 32 * G_PRE_IDENTITY_WORD);
  evm->return_data.data = _malloc(evm->call_data.len);
  evm->return_data.len  = evm->call_data.len;
  memcpy(evm->return_data.data, evm->call_data.data, evm->call_data.len);
  return 0;
}

int evm_run_precompiled(evm_t* evm, address_t address) {
  switch (address[19]) {
    case 1:
      return pre_ecrecover(evm);
    case 2:
      return pre_sha256(evm);
    case 3:
      return pre_ripemd160(evm);
    case 4:
      return pre_identity(evm);
    default:
      return -1;
  }
}
