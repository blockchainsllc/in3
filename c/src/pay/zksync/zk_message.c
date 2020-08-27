#include "../../core/client/context_internal.h"
#include "../../core/client/plugin.h"
#include "../../third-party/zkcrypto/lib.h"
#include "zksync.h"
#include <limits.h> /* strtoull */
#include <stdlib.h> /* strtoull */

#ifdef ZKSYNC_256
static int to_dec(char* dst, bytes32_t val) {
  uint64_t l =
      return sprintf(dst, "%" PRId64, val);
}
#else
static int to_dec(char* dst, uint64_t val) {
  return sprintf(dst, "%" PRId64, val);
}
#endif

static void add_amount(sb_t* sb, zksync_token_t* token,
#ifdef ZKSYNC_256
                       uint8_t* val
#else
                       uint64_t val
#endif
) {
  int  dec = token ? token->decimals : 0;
  char tmp[60]; // UINT64_MAX => 18446744073709551615 => 0xFFFFFFFFFFFFFFFF
  int  l = to_dec(tmp, val);

  if (dec) {
    if (l > dec) {
      memmove(tmp + l - dec + 1, tmp + l - dec, dec + 1);
      tmp[l - dec] = '.';
    }
    else {
      memmove(tmp + dec - l + 2, tmp, l + 1);
      memset(tmp, '0', dec - l + 2);
      tmp[1] = '.';
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
    if ((val >> i) & 1) return 64 - i;
  }
  return 0;
}

static in3_ret_t pack(char* dec, int mantissa_len, int exp_len, uint8_t* dst, in3_ctx_t* ctx) {
  while (*dec == '0') dec++;                // remove leading zeros (if any)
  int l     = strlen(dec);                  // trimmed size
  int total = (exp_len + mantissa_len) / 8; // the target size in bytes
  int cl    = -1;                           // the content length
  memset(dst, 0, total);                    // clear the target first
  uint8_t tmp[8];

  if (!l) return IN3_OK;             // this means we had a "0" which was trimmed away
  for (int i = l - 1; i >= 0; i--) { // now we loop backwards
    if (dec[i] != '0') {             // for the first character, which is not a zero
      cl = i + 1;                    // now we know how many bytes actually have value
      break;
    }
  }
  dec[cl]    = 0;                                                                    // terminate the string after the value cutting off all zeros
  uint64_t c = strtoull(dec, NULL, 10);                                              // and convert this value
  if (c == ULLONG_MAX || bitlen(c) > mantissa_len)                                   // if the value can be represented with the max bits
    return ctx_set_error(ctx, "The value (mantissa) can not be packed", IN3_EINVAL); // its an error
  long_to_bytes(c, tmp);                                                             // we copy the value to bytes
  shift_left(tmp, 64 - mantissa_len);                                                // and shift it so the bits are on the start and
  memcpy(dst, tmp, total);                                                           // copy the bytes to the dest
  long_to_bytes(l - cl, tmp);                                                        // now we do the same wit the exp which are the number of zeros counted (l-cl)
  if (bitlen(l - cl) > exp_len)                                                      // if the exp does not fit in the expected bytes
    return ctx_set_error(ctx, "The value (exp) can not be packed", IN3_EINVAL);      // its an error
  shift_left(tmp, 64 - exp_len - mantissa_len);                                      // now we shift it to the  position after the mantissa
  for (int i = 0; i < total; i++) dst[i] |= tmp[i];                                  // and copy them to the destination using or since we already have bytes there
  return IN3_OK;
}
/*
        const humanReadableTxInfo =
            `Transfer ${stringAmount} ${stringToken}\n` +
            `To: ${transfer.to.toLowerCase()}\n` +
            `Nonce: ${nonce}\n` +
            `Fee: ${stringFee} ${stringToken}\n` +
            `Account Id: ${this.accountId}`;*/
void create_human_readable_tx_info(sb_t* sb, zksync_tx_data_t* data) {
  sb_add_chars(sb, "Transfer ");
  add_amount(sb, data->token, data->amount);
  sb_add_chars(sb, " ");
  sb_add_chars(sb, data->token->symbol);
  sb_add_rawbytes(sb, "\nTo: 0x", bytes(data->to, 20), 0);
  sb_add_chars(sb, "\nNonce: ");
  sb_add_int(sb, data->nonce);
  sb_add_chars(sb, "\nFee: ");
  add_amount(sb, data->token, data->fee);
  sb_add_chars(sb, " ");
  sb_add_chars(sb, data->token->symbol);
  sb_add_chars(sb, "\nAccount Id: ");
  sb_add_int(sb, data->account_id);
}

void create_signed_bytes(sb_t* sb) {
  char* PREFIX = "\x19"
                 "Ethereum Signed Message:\n";
  char len_num[7];
  int  l   = strlen(PREFIX) + sprintf(len_num, "%d", (int) sb->len);
  int  len = sb->len;
  sb_add_chars(sb, PREFIX);
  sb_add_chars(sb, len_num);
  memmove(sb->data + l, sb->data, len);
  memcpy(sb->data, PREFIX, strlen(PREFIX));
  memcpy(sb->data + l - strlen(len_num), len_num, strlen(len_num));
}

in3_ret_t sign_sync_transfer(zksync_tx_data_t* data, in3_ctx_t* ctx, uint8_t* sync_key, uint8_t* raw, uint8_t* sig) {
  char     dec[70];
  uint16_t tid = data->token ? data->token->id : 0;
  raw[0]       = 5;                        // 0: type(1)
  int_to_bytes(data->account_id, raw + 1); // 1 account_id(4)
  memcpy(raw + 5, data->from, 20);         // 5: from(20)
  memcpy(raw + 25, data->to, 20);          // 25: to(20)
  raw[45] = (tid >> 8) & 0xff;             // 45: token_id (2)
  raw[46] = tid & 0xff;                    //
  to_dec(dec, data->amount);               //    create a decimal represntation and pack it
  TRY(pack(dec, 35, 5, raw + 47, ctx))     // 47: amount packed (5)
  to_dec(dec, data->fee);                  //    create a decimal represntation and pack it
  TRY(pack(dec, 11, 5, raw + 52, ctx))     // 52: amount packed (2)
  int_to_bytes(data->nonce, raw + 54);     // 54: nonce(4)

  // sign data
  TRY(zkcrypto_sign_musig(sync_key, bytes(raw, 58), sig));
  /**
   {
      type: "Transfer",
      accountId: transfer.accountId,
      from: transfer.from,
      to: transfer.to,
      token: transfer.tokenId,
      amount: utils.bigNumberify(transfer.amount).toString(),
      fee: utils.bigNumberify(transfer.fee).toString(),
      nonce: transfer.nonce,
     signature: {
        pubKey,
        signature
     }
   }
   * 
   */

  return IN3_OK;
}

in3_ret_t zksync_sign_transfer(sb_t* sb, zksync_tx_data_t* data, in3_ctx_t* ctx, uint8_t* sync_key) {
  char    msg_data[200];
  uint8_t signature[65];
  sb_t    msg = sb_stack(msg_data);
  create_human_readable_tx_info(&msg, data);
  create_signed_bytes(&msg);
  TRY(ctx_require_signature(ctx, "sign_ec_hash", signature, bytes((uint8_t*) msg_data, msg.len), bytes(data->from, 20)))

  // now create the packed sync transfer
  uint8_t raw[58], sig[96];
  TRY(sign_sync_transfer(data, ctx, sync_key, raw, sig));

  sb_add_chars(sb, "[{\"type\":\"Transfer\",\"accountId\":");
  sb_add_int(sb, data->account_id);
  sb_add_rawbytes(sb, ",\"from\":", bytes(data->from, 20), 0);
  sb_add_rawbytes(sb, ",\"to\":", bytes(data->to, 20), 0);
  sb_add_chars(sb, ",\"token\":");
  sb_add_int(sb, data->token->id);
  sb_add_chars(sb, ",\"amount\":");
  sb_add_int(sb, data->amount);
  sb_add_chars(sb, ",\"fee\":");
  sb_add_int(sb, data->fee);
  sb_add_chars(sb, ",\"nonce\":");
  sb_add_int(sb, data->nonce);
  sb_add_rawbytes(sb, ",\"signature\":{\"pubKey\":", bytes(sig, 32), 0);
  sb_add_rawbytes(sb, ",\"signature\":", bytes(sig + 32, 64), 0);
  sb_add_rawbytes(sb, "}},{\"type\":\"EthereumSignature\",\"signature\":",bytes(signature,65),0);
  sb_add_chars(sb, "}]");
  return IN3_OK;
}