#include "btc_script.h"
#include "btc_types.h"

static bool is_p2ms_format(const bytes_t* script) {
  if (!script) return false;

  uint8_t* p   = script->data;
  uint32_t len = script->len;

  uint8_t m = p[0];
  uint8_t n = p[len - 2];

  // check if the script looks like a p2ms
  return (p[len - 1] == OP_CHECKMULTISIG) && (m <= n) && (m >= OP_1) && (n <= OP_15) && (len > 36) && (len < 993);
}

uint8_t btc_get_multisig_pub_key_count(const bytes_t* ms_script) {
  uint8_t pk_count = 0;
  if (is_p2ms_format(ms_script)) {
    uint8_t* p   = ms_script->data;
    uint32_t len = ms_script->len;

    pk_count = p[len - 2] - 0x50; // OP_1 = 0x51, OP_2 = 0x52, OP_3 = 0x53... we want the n of OP_n
  }
  return pk_count;
}

uint8_t btc_get_multisig_req_sig_count(const bytes_t* ms_script) {
  uint8_t pk_count = 0;
  if (is_p2ms_format(ms_script)) {
    uint8_t* p = ms_script->data;

    pk_count = p[0] - 0x50; // OP_1 = 0x51, OP_2 = 0x52, OP_3 = 0x53... we want the n of OP_n
  }
  return pk_count;
}

bool is_p2pk(const bytes_t* script) {
  // locking script has format: PUB_KEY_LEN(1) PUB_KEY(33 or 65 bytes) OP_CHECKSIG(1)
  uint32_t len = script->len;
  uint8_t* p   = script->data;
  return (len == (uint32_t) p[0] + 2) && (p[0] == 33 || p[0] == 65) && (p[len - 1] == OP_CHECKSIG);
}

bool is_p2pkh(const bytes_t* script) {
  // locking script has format: OP_DUP(1) OP_HASH160(1) PUB_KEY_HASH_LEN(1) PUB_KEY_HASH(20) OP_EQUALVERIFY(1) OP_CHECKSIG(1)
  uint32_t len = script->len;
  uint8_t* p   = script->data;
  return (len == 25) && (p[0] == OP_DUP) && (p[1] == OP_HASH160) && (p[2] == 20) && (p[len - 2] == OP_EQUALVERIFY) && (p[len - 1] == OP_CHECKSIG);
}

bool is_p2sh(const bytes_t* script) {
  // locking script has format: OP_HASH160(1) SCRIPT_HASH_LEN(1) SCRIPT_HASH(20) OP_EQUAL(1)
  uint32_t len = script->len;
  uint8_t* p   = script->data;
  return (len == 23) && (p[0] == OP_HASH160) && (p[1] == 20) && (p[len - 1] == OP_EQUAL);
}

bool is_p2ms(const bytes_t* script) {
  // locking script has format: M(1) LEN_PK_1(1) PK_1(33 or 65 bytes) ... LEN_PK_N(1) PK_N(33 or 65 bytes) N(1) OP_CHECKMULTISIG
  uint8_t* p = script->data;

  // check if the script looks like a p2ms
  bool result = is_p2ms_format(script);

  if (result) {
    uint32_t n = btc_get_multisig_pub_key_count(script);

    // Check if all public keys are valid
    p++;
    uint32_t pk_len;
    uint8_t* pk_data;
    for (uint32_t i = 0; i < n; i++) {
      pk_len          = *p;
      pk_data         = p + 1;
      bytes_t pub_key = bytes(pk_data, pk_len);
      if (!pub_key_is_valid(&pub_key)) {
        return false;
      }
      p += pk_len + 1;
    }
  }

  return result;
}

bool is_p2wpkh(const bytes_t* script) {
  // locking script has format: OP_0(1) PUB_KEY_HASH_LEN(1) PUB_KEY_HASH(20)
  uint8_t* p = script->data;
  return (script->len == 22) && (p[0] == OP_0) && (p[1] == 0x14);
}

bool is_p2wsh(const bytes_t* script) {
  // locking script has format: VERSION_BYTE(1) WITNESS_SCRIPT_HASH_LEN(1) WITNESS_SCRIPT_HASH(32)
  uint8_t* p = script->data;
  return (script->len == 34) && (p[0] < OP_PUSHDATA1) && (p[1] == 32);
}

bool is_witness_program(const bytes_t* script) {
  return ((script->len > 4) && (script->len < 42)) &&
         ((script->data[0] == OP_0) || (script->data[0] >= OP_1 && script->data[0] <= OP_16)) &&
         (((uint32_t) script->data[1] + 2) == script->len);
}

btc_stype_t btc_get_script_type(const bytes_t* script) {
  if ((!script->data) || (script->len > MAX_SCRIPT_SIZE_BYTES)) {
    return BTC_UNSUPPORTED;
  }

  btc_stype_t script_type = BTC_NON_STANDARD;

  if (is_p2pk(script)) {
    script_type = BTC_P2PK;
  }
  else if (is_p2pkh(script)) {
    script_type = BTC_P2PKH;
  }
  else if (is_p2sh(script)) {
    script_type = BTC_P2SH;
  }
  else if (is_witness_program(script)) {
    if (is_p2wpkh(script)) {
      script_type = BTC_V0_P2WPKH;
    }
    else if (is_p2wsh(script)) {
      script_type = BTC_P2WSH;
    }
  }
  else if (is_p2ms(script)) {
    script_type = BTC_P2MS;
  }
  return script_type;
}

bool script_is_standard(btc_stype_t script_type) {
  return script_type != BTC_NON_STANDARD && script_type != BTC_UNSUPPORTED && script_type != BTC_UNKNOWN;
}

btc_stype_t btc_string_to_script_type(const char* type) {
  btc_stype_t result = BTC_UNKNOWN;
  if (!strcmp(type, "p2pkh"))
    result = BTC_P2PKH;
  else if (!strcmp(type, "p2sh"))
    result = BTC_P2SH;
  else if (!strcmp(type, "p2ms"))
    result = BTC_P2MS;
  else if (!strcmp(type, "p2wpkh"))
    result = BTC_V0_P2WPKH;
  else if (!strcmp(type, "p2wsh"))
    result = BTC_P2WSH;
  else if (!strcmp(type, "p2pk"))
    result = BTC_P2PK;
  else if (!strcmp(type, "unsupported"))
    result = BTC_UNSUPPORTED;
  else if (!strcmp(type, "non_standard"))
    result = BTC_NON_STANDARD;
  return result;
}

const char* btc_script_type_to_string(btc_stype_t type) {
  switch (type) {
    // Default case is purposefuly missing, so the compiler
    // will complain in case we miss any script type
    case BTC_UNKNOWN:
      return "unknown";
    case BTC_UNSUPPORTED:
      return "unsupported";
    case BTC_NON_STANDARD:
      return "non_standard";
    case BTC_P2PK:
      return "p2pk";
    case BTC_P2PKH:
      return "p2pkh";
    case BTC_P2SH:
      return "p2sh";
    case BTC_V0_P2WPKH:
      return "p2wpkh";
    case BTC_P2WSH:
      return "p2wsh";
    case BTC_P2MS:
      return "p2ms";
  }
  return "unknown";
}