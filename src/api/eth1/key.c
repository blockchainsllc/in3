#include "../../core/client/client.h"
#include "../../core/util/data.h"
#include "../../core/util/utils.h"
#include "../../third-party/crypto/aes/aes.h"
#include "../../third-party/crypto/pbkdf2.h"

#ifdef SCRYPT
// only if scrypt is installed we support it.
#include <libscrypt.h>
#endif

in3_ret_t decrypt_key(d_token_t* key_data, char* password, bytes32_t dst) {
  if (d_get_int(key_data, "version") != 3) return IN3_EVERS;
  d_token_t* crypto     = d_get(key_data, key("crypto"));
  char*      kdf        = d_get_string(crypto, "kdf");
  d_token_t* kdf_params = d_get(crypto, key("kdfparams"));
  if (!crypto || !kdf || !kdf_params) return IN3_EINVALDT;
  int     klen     = d_get_int(kdf_params, "dklen");
  char*   salt_hex = d_get_string(kdf_params, "salt");
  uint8_t salt_data[strlen(salt_hex) >> 1], aeskey[klen], cipher_data[64];
  bytes_t salt = bytes(salt_data, hex2byte_arr(salt_hex, -1, salt_data, 0xFF));

  if (strcmp(kdf, "scrypt") == 0) {
#ifdef SCRYPT
    if (libscrypt_scrypt((const uint8_t*) password, strlen(password), salt.data, salt.len,
                         d_get_long(kdf_params, "n"), d_get_int(kdf_params, "r"), d_get_long(kdf_params, "p"), aeskey, klen))
      return IN3_EPASS;
#else
    return IN3_ENOTSUP;
#endif

  } else if (strcmp(kdf, "pbkdf2") == 0) {
    if (!kdf_params || strcmp(d_get_string(kdf_params, "prf"), "hmac-sha256")) return IN3_ENOTSUP;
    if (strcmp(d_get_string(crypto, "cipher"), "aes-128-ctr")) return IN3_ENOTSUP;
    pbkdf2_hmac_sha256((const uint8_t*) password, strlen(password), salt.data, salt.len, d_get_int(kdf_params, "c"), aeskey, klen);
  } else
    return IN3_ENOTSUP;

  bytes_t cipher = bytes(cipher_data, hex2byte_arr(d_get_string(crypto, "ciphertext"), -1, cipher_data, 64));
  uint8_t msg[cipher.len + 16], mac[32];
  bytes_t msgb = bytes(msg, cipher.len + 16);
  memcpy(msg, aeskey + 16, 16);
  memcpy(msg + 16, cipher.data, cipher.len);
  sha3_to(&msgb, mac);
  bytes32_t mac_verify;
  hex2byte_arr(d_get_string(crypto, "mac"), -1, mac_verify, 32);
  if (memcmp(mac, mac_verify, 32)) return IN3_EPASS;

  // aes-128-ctr
  aes_init();
  aes_encrypt_ctx cx[1];
  char*           iv_hex = d_get_string(d_get(crypto, key("cipherparams")), "iv");
  int             iv_len = strlen(iv_hex) / 2;
  uint8_t         iv_data[iv_len];
  hex2byte_arr(iv_hex, -1, iv_data, iv_len);

  aes_encrypt_key128(aeskey, cx);
  return aes_ctr_decrypt(cipher.data, dst, cipher.len, iv_data, aes_ctr_cbuf_inc, cx) ? IN3_EPASS : IN3_OK;
}