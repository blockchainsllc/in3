#include "../../core/util/data.h"
#include "../../core/util/utils.h"
#include "../core/client/client.h"
#include <crypto/aes/aes.h>
#include <crypto/pbkdf2.h>

in3_ret_t decrypt_key(d_token_t* key_data, char* password, bytes32_t dst) {
  if (d_get_int(key_data, "version") != 3) return IN3_EVERS;
  d_token_t* crypto = d_get(key_data, key("crypto"));
  char*      kdf    = d_get_string(crypto, "kdf");
  if (!crypto || !kdf) return IN3_EINVALDT;
  if (strcmp(kdf, "scrypt") == 0)
    return IN3_ENOTSUP;
  else if (strcmp(kdf, "pbkdf2") == 0) {
    d_token_t* kdf_params = d_get(crypto, key("kdfparams"));
    if (!kdf_params || strcmp(d_get_string(kdf_params, "prf"), "hmac-sha256")) return IN3_ENOTSUP;
    if (strcmp(d_get_string(crypto, "cipher"), "aes-128-ctr")) return IN3_ENOTSUP;
    char*   salt_hex = d_get_string(kdf_params, "salt");
    uint8_t salt_data[strlen(salt_hex) >> 1];
    bytes_t salt = bytes(salt_data, hex2byte_arr(salt_hex, -1, salt_data, 0xFF));
    int     klen = d_get_int(kdf_params, "dklen");
    uint8_t aeskey[klen], cipher_data[64];
    pbkdf2_hmac_sha256((const uint8_t*) password, strlen(password), salt.data, salt.len, d_get_int(kdf_params, "c"), aeskey, klen);
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
    uint8_t         iv_data[strlen(iv_hex) >> 1];
    aes_decrypt_key128(aeskey, (aes_decrypt_ctx*) cx);
    aes_ctr_crypt(cipher.data, dst, 16, iv_data, aes_ctr_cbuf_inc, cx);

    /*
    aes_decrypt_key128

    aes_ctr_decrypt(key,dst,16,cipher.data,aes_ctr_cbuf_inc,)

    const decipher = crypto.createDecipheriv(
        json.crypto.cipher,
        derivedKey.slice(0, 16),
        Buffer.from(json.crypto.cipherparams.iv, 'hex'));
    const seed = `0x $ { Buffer.concat([ decipher.update(ciphertext), decipher.final() ]).toString('hex') }`;

    return Account.fromPrivateKey(seed, accounts);
*/
    return IN3_OK;
  } else
    return IN3_ENOTSUP;
}