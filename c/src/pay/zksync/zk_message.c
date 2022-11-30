#include "../../core/client/request_internal.h"
#include "../../core/util/crypto.h"
#include "../../core/util/log.h"
#include "../../third-party/zkcrypto/lib.h"
#include "zk_helper.h"
#include <limits.h> /* strtoull */
#include <stdlib.h> /* strtoull */

static int to_dec(char* dst, zk_fee_t val) {
#ifdef ZKSYNC_256
  int l = encode(ENC_DECIMAL, bytes(val, 32), dst);
  if (l < 0) sprintf(dst, "<NOT SUPPORTED>");
  return l;
#else
  return sprintf(dst, "%" PRId64, val);
#endif
}

static void add_amount(sb_t* sb, zksync_token_t* token, zk_fee_t val) {
  int   dec = token ? token->decimals : 0;
  char  tmp[80]; // UINT64_MAX => 18446744073709551615 => 0xFFFFFFFFFFFFFFFF
  char* sep = NULL;
  int   l   = to_dec(tmp, val);

  if (dec) {
    if (l > dec) {
      memmove(tmp + l - dec + 1, tmp + l - dec, dec + 1);
      tmp[l - dec] = '.';
      sep          = tmp + l - dec + 2;
    }
    else {
      memmove(tmp + dec - l + 2, tmp, l + 1);
      memset(tmp, '0', dec - l + 2);
      tmp[1] = '.';
      sep    = tmp + 3;
    }
    l = strlen(sep);
    while (l && sep[l - 1] == '0') {
      l--;
      sep[l] = 0;
    }
  }

  sb_add_chars(sb, tmp);
}

static void shift_left(uint8_t* a, int bits) {
  wlen_t        r;
  uint_fast16_t carry = 0;
  int           i;
  if ((r = bits % 8)) {
    for (i = 8 - 1; i >= 0; i--) {
      a[i] = (carry |= a[i] << r) & 0xFF;
      carry >>= 8;
    }
  }
  if ((r = (bits - r) >> 3)) {
    for (i = 0; i < (int) 8; i++)
      a[i] = i + r < 8 ? a[i + r] : 0;
  }
}

static int bitlen(uint64_t val) {
  for (int i = 63; i >= 0; --i) {
    if ((val >> i) & 1)
      return i;
  }
  return 0;
}
const char* MAX_MANTISSA_35 = "34359738368";
const char* MAX_MANTISSA_11 = "2048";

static in3_ret_t pack(char* dec, int mantissa_len, int exp_len, uint8_t* dst, in3_req_t* ctx) {
  while (*dec == '0') dec++;                // remove leading zeros (if any)
  int l     = strlen(dec);                  // trimmed size
  int total = (exp_len + mantissa_len) / 8; // the target size in bytes
  int cl    = -1;                           // the content length
  memset(dst, 0, total);                    // clear the target first
  uint8_t     tmp[8];
  const char* max_matissa = mantissa_len == 35 ? MAX_MANTISSA_35 : MAX_MANTISSA_11;
  int         max_m_len   = strlen(max_matissa);

  if (!l) return IN3_OK;             // this means we had a "0" which was trimmed away
  for (int i = l - 1; i >= 0; i--) { // now we loop backwards
    if (i + 1 < max_m_len || (i + 1 == max_m_len && memcmp(dec, max_matissa, max_m_len) < 0)) {
      cl = i + 1;                    // now we know how many bytes actually have value
      break;
    }

    if (dec[i] != '0')
      return req_set_error(ctx, "The value (mantissa) can not be packed", IN3_EINVAL); // its an error
  }
  if (cl == -1) return req_set_error(ctx, "The value (mantissa) can not be packed", IN3_EINVAL);
  dec[cl]    = 0;                                                                    // terminate the string after the value cutting off all zeros
  uint64_t c = strtoull(dec, NULL, 10);                                              // and convert this value
  if (c == ULLONG_MAX || bitlen(c) > mantissa_len)                                   // if the value can be represented with the max bits
    return req_set_error(ctx, "The value (mantissa) can not be packed", IN3_EINVAL); // its an error
  long_to_bytes(c, tmp);                                                             // we copy the value to bytes
  shift_left(tmp, 64 - mantissa_len);                                                // and shift it so the bits are on the start and
  memcpy(dst, tmp, total);                                                           // copy the bytes to the dest
  long_to_bytes(l - cl, tmp);                                                        // now we do the same wit the exp which are the number of zeros counted (l-cl)
  if (bitlen(l - cl) > exp_len)                                                      // if the exp does not fit in the expected bytes
    return req_set_error(ctx, "The value (exp) can not be packed", IN3_EINVAL);      // its an error
  shift_left(tmp, 64 - exp_len - mantissa_len);                                      // now we shift it to the  position after the mantissa
  for (int i = 0; i < total; i++) dst[i] |= tmp[i];                                  // and copy them to the destination using or since we already have bytes there
  return IN3_OK;
}

static void create_human_readable_tx_info(sb_t* sb, zksync_tx_data_t* data, char* type) {
  if (!data->token) return;
  sb_add_chars(sb, type);
  add_amount(sb, data->token, data->amount);
  sb_add_chars(sb, " ");
  sb_add_chars(sb, data->token->symbol);
  sb_add_rawbytes(sb, " to: 0x", bytes(data->to, 20), 0);
  sb_add_chars(sb, "\nFee: ");
  add_amount(sb, data->token, data->fee);
  sb_add_chars(sb, " ");
#ifdef __clang_analyzer__
  if (data->token->symbol)
#endif
    sb_add_chars(sb, data->token->symbol);
  sb_add_chars(sb, "\nNonce: ");
  sb_add_int(sb, data->nonce);

  in3_log_debug("Human readable message : \n%s\n", sb->data);
}

static in3_ret_t sign_sync_transfer(zksync_tx_data_t* data, in3_req_t* ctx, zksync_config_t* conf, uint8_t* raw, uint8_t* sig, uint32_t* total) {
  char     dec[80];
  uint16_t tid = data->token ? data->token->id : 0;
  raw[0]       = data->type;               // 0: type(1)
  int_to_bytes(data->account_id, raw + 1); // 1 account_id(4)
  memcpy(raw + 5, data->from, 20);         // 5: from(20)
  memcpy(raw + 25, data->to, 20);          // 25: to(20)
  raw[45] = (tid >> 8) & 0xff;             // 45: token_id (2)
  raw[46] = tid & 0xff;                    //
  if (data->type == ZK_WITHDRAW) {
    *total = 85;
#ifdef ZKSYNC_256
    memcpy(raw + 47, data->amount + 16, 16);
#else
    memset(raw + 47, 0, 8);
    long_to_bytes(data->amount, raw + 55);
#endif
    to_dec(dec, data->fee);              //    create a decimal represntation and pack it
    TRY(pack(dec, 11, 5, raw + 63, ctx)) // 63: amount packed (2)
    int_to_bytes(data->nonce, raw + 65); // 65: nonce(4)
  }
  else {
    *total = 74;
    to_dec(dec, data->amount);           //    create a decimal represntation and pack it
    TRY(pack(dec, 35, 5, raw + 47, ctx)) // 47: amount packed (5)
    to_dec(dec, data->fee);              //    create a decimal represntation and pack it
    TRY(pack(dec, 11, 5, raw + 52, ctx)) // 52: amount packed (2)
    int_to_bytes(data->nonce, raw + 54); // 54: nonce(4)
  }

  long_to_bytes(data->valid.from, raw + (*total) - 16);
  long_to_bytes(data->valid.to, raw + (*total) - 8);

  if (!data->prepare) // sign data
    return zksync_sign(conf, bytes(raw, (*total)), ctx, sig);
  else

    data->prepare->zk_message = bytes_dup(bytes(raw, (*total)));
  return IN3_OK;
}

in3_ret_t zksync_sign_transfer(sb_t* sb, zksync_tx_data_t* data, in3_req_t* ctx, zksync_config_t* conf) {
  // fix valid.to first
  if (!data->valid.to) data->valid.to = 0xffffffffl;

  bytes_t signature = NULL_BYTES;

  if (data->conf->sign_type != ZK_SIGN_CREATE2) {
    char msg_data[256];
    sb_t msg = sb_stack(msg_data);
    create_human_readable_tx_info(&msg, data, data->type == ZK_WITHDRAW ? "Withdraw " : "Transfer ");

    if (data->prepare)
      data->prepare->human_message = _strdupn(msg_data, -1);
    else {
      TRY(req_require_signature(ctx, SIGN_EC_PREFIX, ECDSA_SECP256K1, PL_SIGN_ANY, &signature, bytes((uint8_t*) msg_data, msg.len), bytes(data->from, 20), req_get_request(ctx, 0), NULL))
      in3_log_debug("zksync_sign_transfer human readable :\n%s\n", msg_data);

      if (signature.len == 65 && signature.data[64] < 27)
        signature.data[64] += 27; // because EIP155 chainID = 0
    }
  }

  // now create the packed sync transfer
  uint8_t  raw[85], sig[96];
  uint32_t total = 0;
  TRY(sign_sync_transfer(data, ctx, conf, raw, sig, &total));

  if (in3_log_level_is(LOG_DEBUG) || in3_log_level_is(LOG_TRACE)) {
    char* hex = alloca(142);
    bytes_to_hex(raw, total, hex);
    in3_log_debug("zksync_sign_transfer  bin :\n%s\n", hex);
  }

  sb_add_chars(sb, "{\"type\":\"");
  sb_add_chars(sb, data->type == ZK_WITHDRAW ? "Withdraw" : "Transfer");
  sb_add_chars(sb, "\",\"accountId\":");
  sb_add_int(sb, data->account_id);
  sb_add_rawbytes(sb, ",\"from\":\"0x", bytes(data->from, 20), 0);
  sb_add_rawbytes(sb, "\",\"to\":\"0x", bytes(data->to, 20), 0);
  sb_add_chars(sb, "\",\"token\":");
  sb_add_int(sb, data->token->id);
  sb_add_chars(sb, ",\"tokenId\":");
  sb_add_int(sb, data->token->id);
  sb_add_chars(sb, data->prepare ? ",\"value\":" : ",\"amount\":");
#ifdef ZKSYNC_256
  char dec[80];
  to_dec(dec, data->amount);
  sb_add_chars(sb, dec);
  sb_add_chars(sb, data->prepare ? ",\"gas\":" : ",\"fee\":");
  to_dec(dec, data->fee);
  sb_add_chars(sb, dec);
#else
  sb_add_int(sb, data->amount);
  sb_add_chars(sb, ",\"fee\":");
  sb_add_int(sb, data->fee);
#endif
  sb_add_chars(sb, ",\"validFrom\":");
  sb_add_int(sb, (int64_t) data->valid.from);
  sb_add_chars(sb, ",\"validUntil\":");
  sb_add_int(sb, (int64_t) data->valid.to);
  sb_add_chars(sb, ",\"nonce\":");
  sb_add_int(sb, data->nonce);
  if (!data->prepare) {
    sb_add_rawbytes(sb, ",\"signature\":{\"pubKey\":\"", bytes(sig, 32), 0);
    sb_add_rawbytes(sb, "\",\"signature\":\"", bytes(sig + 32, 64), 0);
    sb_add_chars(sb, "\"}},");
    if (data->conf->sign_type == ZK_SIGN_CREATE2)
      sb_add_chars(sb, "null");
    else {
      sb_add_chars(sb, "{\"type\":\"");
      sb_add_chars(sb, data->conf->sign_type == ZK_SIGN_CONTRACT ? "EIP1271Signature" : "EthereumSignature");
      sb_add_rawbytes(sb, "\",\"signature\":\"0x", signature, 0);
      sb_add_chars(sb, "\"}");
    }
  }
  else
    sb_add_chars(sb, "}");

  return IN3_OK;
}

in3_ret_t zksync_sign(zksync_config_t* conf, bytes_t msg, in3_req_t* ctx, uint8_t* sig) {
  TRY(zksync_get_sync_key(conf, ctx, NULL))
  if (memiszero(conf->sync_key, 32)) return req_set_error(ctx, "no signing key set", IN3_ECONFIG);
  if (!conf->musig_pub_keys.data) return zkcrypto_sign_musig(conf->sync_key, msg, sig);
  char* p = alloca(msg.len * 2 + 5);
  p[0]    = '"';
  p[1]    = '0';
  p[2]    = 'x';
  bytes_to_hex(msg.data, msg.len, p + 3);
  p[msg.len * 2 + 3] = '"';
  p[msg.len * 2 + 4] = 0;
  d_token_t* result;
  TRY(req_send_sub_request(ctx, "zk_sign", p, NULL, &result, NULL))
  bytes_t bsig = d_bytes(result);
  if (!bsig.data || bsig.len != 96) return req_set_error(ctx, "invalid signature returned", IN3_ECONFIG);
  memcpy(sig, bsig.data, 96);
  return IN3_OK;
}

in3_ret_t zksync_sign_change_pub_key(sb_t* sb, in3_req_t* ctx, uint8_t* sync_pub_key, uint32_t nonce, zksync_config_t* conf, zk_fee_t fee, zksync_token_t* token, zksync_valid_t valid) {

  // create sign_msg for the rollup
  char    dec[80];
  uint8_t sign_msg_bytes[69], sig[96];
  sign_msg_bytes[0] = 7;                              // tx type 7 (1 byte)
  int_to_bytes(conf->account_id, sign_msg_bytes + 1); // acount_id (4 bytes)
  memcpy(sign_msg_bytes + 5, conf->account, 20);      // account address
  memcpy(sign_msg_bytes + 25, sync_pub_key, 20);      // new sync pub key
  sign_msg_bytes[45] = token->id >> 8 & 0xff;         // token_id (high)
  sign_msg_bytes[46] = token->id & 0xff;              // token_id (low)
  to_dec(dec, fee);                                   // create a decimal represntation and pack it
  TRY(pack(dec, 11, 5, sign_msg_bytes + 47, ctx))     // 47: fee packed (2)
  int_to_bytes(nonce, sign_msg_bytes + 49);           // nonce
  long_to_bytes(valid.from, sign_msg_bytes + 53);     // valid_from
  long_to_bytes(valid.to, sign_msg_bytes + 61);       // valid_to

  // now sign it with the new pk
  TRY(zksync_sign(conf, bytes(sign_msg_bytes, 69), ctx, sig))

  // create 2fa-message to be signed with the eth-signer
  uint8_t ethmsg[60];
  bytes_t signature = NULL_BYTES;
  memcpy(ethmsg, sync_pub_key, 20);            // pubkeyhash (20)
  int_to_bytes(nonce, ethmsg + 20);            // nonce (4)
  int_to_bytes(conf->account_id, ethmsg + 24); // acount_id (4 bytes)
  memset(ethmsg + 28, 0, 32);                  //  msgBatch hash  - currently not supported, so 32x0

  if (conf->sign_type != ZK_SIGN_CREATE2) {
    TRY(req_require_signature(ctx, SIGN_EC_PREFIX, ECDSA_SECP256K1, PL_SIGN_ANY, &signature, bytes((uint8_t*) ethmsg, 60), bytes(conf->account, 20), req_get_request(ctx, 0), NULL))
    if (signature.len == 65 && signature.data[64] < 27)
      signature.data[64] += 27; // because EIP155 chainID = 0
  }

  sb_add_chars(sb, "{\"type\":\"ChangePubKey\",\"accountId\":");
  sb_add_int(sb, conf->account_id);
  sb_add_rawbytes(sb, ",\"account\":\"0x", bytes(conf->account, 20), 0);
  sb_add_rawbytes(sb, "\",\"newPkHash\":\"sync:", bytes(sync_pub_key, 20), 0);
  sb_add_chars(sb, "\",\"feeTokenId\":");
  sb_add_int(sb, token->id);
  sb_add_chars(sb, ",\"feeToken\":");
  sb_add_int(sb, token->id);
  sb_add_chars(sb, ",\"validFrom\":");
  sb_add_int(sb, (int64_t) valid.from);
  sb_add_chars(sb, ",\"validUntil\":");
  sb_add_int(sb, (int64_t) valid.to);
  sb_add_chars(sb, ",\"fee\":\"");
#ifdef ZKSYNC_256
  to_dec(dec, fee);
  sb_add_chars(sb, dec);
#else
  sb_add_int(sb, fee);
#endif
  sb_add_chars(sb, "\",\"nonce\":");
  sb_add_int(sb, nonce);
  if (conf->version > 0) {
    sb_add_chars(sb, ",\"ethAuthData\":{");
    if (conf->sign_type == ZK_SIGN_PK)
      sb_add_rawbytes(sb, "\"type\":\"ECDSA\",\"ethSignature\":\"0x", signature, 0);
    else if (conf->sign_type == ZK_SIGN_CONTRACT)
      sb_add_chars(sb, "\"type\":\"Onchain");
    else if (conf->sign_type == ZK_SIGN_CREATE2) {
      sb_add_rawbytes(sb, "\"type\":\"CREATE2\",\"creatorAddress\":\"0x", bytes(conf->create2.creator, 20), 0);
      sb_add_rawbytes(sb, "\",\"saltArg\":\"0x", bytes(conf->create2.salt_arg, 32), 0);
      sb_add_rawbytes(sb, "\",\"codeHash\":\"0x", bytes(conf->create2.codehash, 32), 0);
    }
    sb_add_chars(sb, "\"}");
  }
  sb_add_rawbytes(sb, ",\"signature\":{\"pubKey\":\"", bytes(sig, 32), 0);
  sb_add_rawbytes(sb, "\",\"signature\":\"", bytes(sig + 32, 64), 0);
  sb_add_chars(sb, "\"}},null,false");

  return IN3_OK;
}
