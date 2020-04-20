#ifdef WIN32
#include <windows.h>
#endif

#include "../../../core/client/context.h"
#include "../../../core/util/log.h"
#include "device_apdu_commands.h"
#include "ledger_signer.h"
#include "ledger_signer_priv.h"

#include <memory.h>
#include <stdbool.h>
#include <stdlib.h>

#define MAX_STR 255

in3_ret_t is_ledger_device_connected() {
  int       res = 0;
  in3_ret_t ret;
  wchar_t   wstr[MAX_STR];

  res = hid_init();
  hid_device* handle;

  handle = hid_open(LEDGER_NANOS_VID, LEDGER_NANOS_PID, NULL);

  if (NULL != handle) {
    in3_log_info("device connected \n");

    res = hid_get_manufacturer_string(handle, wstr, MAX_STR);
    in3_log_debug("device manufacturer: %ls\n", wstr);

    hid_get_product_string(handle, wstr, MAX_STR);
    in3_log_debug("product: %ls\n", wstr);

    ret = IN3_OK;
  } else {
    ret = IN3_ENODEVICE;
  }

  hid_close(handle);
  res = hid_exit();

  return ret;
}

in3_ret_t eth_get_address_from_path(bytes_t i_bip_path, bytes_t o_address) {
  //not implemented currently
  return IN3_EUNKNOWN;
}

in3_ret_t eth_ledger_sign(void* ctx, d_signature_type_t type, bytes_t message, bytes_t account, uint8_t* dst) {
  //UNUSED_VAR(account); // at least for now
  uint8_t* bip_path_bytes = ((in3_ctx_t*) ctx)->client->signer->wallet;
  uint8_t  bip_path_len   = 5;
  uint8_t  bip_data[5];

  int       res = 0;
  in3_ret_t ret;

  hid_device* handle;
  uint8_t     apdu[64];
  uint8_t     buf[2];
  int         index_counter = 0;
  uint8_t     bytes_read    = 0;

  uint8_t msg_len = 32;
  uint8_t hash[32];
  uint8_t read_buf[255];

  bool    is_hashed = false;
  bytes_t apdu_bytes;
  bytes_t final_apdu_command;
  bytes_t public_key;
  bytes_t response;
  bytes_t bip_path;

  memcpy(bip_data, bip_path_bytes, bip_path_len);

  bip_path.data = bip_path_bytes;
  bip_path.len  = bip_path_len;

  ret          = eth_ledger_get_public_key(bip_path, &public_key);
  res          = hid_init();
  handle       = hid_open(LEDGER_NANOS_VID, LEDGER_NANOS_PID, NULL);
  int cmd_size = 64;
  int recid    = 0;

  if (NULL != handle) {

    hid_set_nonblocking(handle, 0);

    switch (type) {
      case SIGN_EC_RAW:
        memcpy(hash, message.data, message.len);
        is_hashed = true;
      case SIGN_EC_HASH:
        if (!is_hashed)
          hasher_Raw(HASHER_SHA3K, message.data, message.len, hash);

        apdu[index_counter++] = CLA;
        apdu[index_counter++] = INS_SIGN;
        apdu[index_counter++] = P1_FINAL;
        apdu[index_counter++] = IDM_SECP256K1;

        apdu[index_counter++] = 0x01; //1st arg tag
        apdu[index_counter++] = bip_path_len;
        memcpy(apdu + index_counter, &bip_data, bip_path_len);
        index_counter += bip_path_len;

        apdu[index_counter++] = 0x02; //2nd arg tag
        apdu[index_counter++] = msg_len;
        memcpy(apdu + index_counter, hash, msg_len);
        index_counter += msg_len;

        apdu_bytes.data = malloc(index_counter);
        apdu_bytes.len  = index_counter;
        memcpy(apdu_bytes.data, apdu, index_counter);

        wrap_apdu(apdu_bytes, 0, &final_apdu_command);

#ifdef DEBUG
        print_bytes(final_apdu_command.data, final_apdu_command.len, "eth_ledger_sign :wraping");
#endif

        res = hid_write(handle, final_apdu_command.data, final_apdu_command.len);

        in3_log_debug("written to hid %d\n", res);

        read_hid_response(handle, &response);

#ifdef DEBUG
        print_bytes(response.data, response.len, "eth_ledger_sign :raw signature");
#endif

        if (response.data[response.len - 2] == 0x90 && response.data[response.len - 1] == 0x00) {
          ret = IN3_OK;

          in3_log_debug("apdu executed succesfully \n");
          extract_signture(response, dst);
          recid   = get_recid_from_pub_key(&secp256k1, public_key.data, dst, hash);
          dst[64] = recid;

#ifdef DEBUG
          print_bytes(dst, 65, "eth_ledger_sign : signature");
#endif

        } else {
          in3_log_fatal("error in apdu execution \n");
          ret = IN3_ENOTSUP;
        }

        free(final_apdu_command.data);
        free(apdu_bytes.data);
        ret = IN3_OK;
        break;

      default:
        return IN3_ENOTSUP;
    }

  } else {
    in3_log_fatal("no ledger device connected \n");
    ret = IN3_ENODEVICE;
  }
  hid_close(handle);
  res = hid_exit();
  free(public_key.data);
  return 65;
}

in3_ret_t eth_ledger_get_public_key(bytes_t i_bip_path, bytes_t* o_public_key) {
  int       res = 0;
  in3_ret_t ret;
  uint8_t   apdu[64];
  uint8_t   buf[2];
  int       index_counter = 0;
  uint16_t  msg_len       = 0;
  uint8_t   bytes_read    = 0;

  bytes_t     apdu_bytes;
  bytes_t     final_apdu_command;
  bytes_t     response;
  hid_device* handle;

  res    = hid_init();
  handle = hid_open(LEDGER_NANOS_VID, LEDGER_NANOS_PID, NULL);
  if (NULL != handle) {
    apdu[index_counter++] = CLA;
    apdu[index_counter++] = INS_GET_PUBLIC_KEY;
    apdu[index_counter++] = P1_FINAL;
    apdu[index_counter++] = IDM_SECP256K1;

    apdu[index_counter++] = i_bip_path.len;
    memcpy(apdu + index_counter, i_bip_path.data, i_bip_path.len);
    index_counter += i_bip_path.len;

    apdu_bytes.data = malloc(index_counter);
    apdu_bytes.len  = index_counter;
    memcpy(apdu_bytes.data, apdu, index_counter);

    wrap_apdu(apdu_bytes, 0, &final_apdu_command);

#ifdef DEBUG
    print_bytes(final_apdu_command.data, final_apdu_command.len, "eth_ledger_get_public_key :wraping");
#endif

    res = hid_write(handle, final_apdu_command.data, final_apdu_command.len);

    read_hid_response(handle, &response);

#ifdef DEBUG
    print_bytes(response.data, response.len, "eth_ledger_get_public_key :response hid");
#endif

    if (response.data[response.len - 2] == 0x90 && response.data[response.len - 1] == 0x00) {
      ret                = IN3_OK;
      o_public_key->len  = response.len - 2;
      o_public_key->data = malloc(response.len - 2);
      memcpy(o_public_key->data, response.data, response.len - 2);
    } else {
      ret = IN3_ENOTSUP;
    }
    free(final_apdu_command.data);
    free(apdu_bytes.data);
    free(response.data);

  } else {
    ret = IN3_ENODEVICE;
  }
  hid_close(handle);
  res = hid_exit();
  return ret;
}

in3_ret_t eth_ledger_set_signer(in3_t* in3, uint8_t* bip_path) {
  if (in3->signer) free(in3->signer);
  in3->signer             = malloc(sizeof(in3_signer_t));
  in3->signer->sign       = eth_ledger_sign;
  in3->signer->prepare_tx = NULL;
  in3->signer->wallet     = bip_path;
  return IN3_OK;
}

void extract_signture(bytes_t i_raw_sig, uint8_t* o_sig) {

  //ECDSA signature encoded as TLV:  30 L 02 Lr r 02 Ls s
  int lr     = i_raw_sig.data[3];
  int ls     = i_raw_sig.data[lr + 5];
  int offset = 0;
  in3_log_debug("lr %d, ls %d \n", lr, ls);
  if (lr > 0x20) {
    memcpy(o_sig + offset, i_raw_sig.data + 5, lr - 1);
    offset = lr - 1;
  } else {
    memcpy(o_sig, i_raw_sig.data + 4, lr);
    offset = lr;
  }

  if (ls > 0x20) {
    memcpy(o_sig + offset, i_raw_sig.data + lr + 7, ls - 1);
  } else {
    memcpy(o_sig + offset, i_raw_sig.data + lr + 6, ls);
  }
}

void read_hid_response(hid_device* handle, bytes_t* response) {
  uint8_t read_chunk[64];
  uint8_t read_buf[255];
  int     index_counter         = 0;
  int     bytes_to_read         = 0;
  int     total_bytes_available = 0;
  int     bytes_read            = 0;
  do {
    bytes_read = hid_read(handle, read_chunk, sizeof(read_chunk));

    if (bytes_read > 0) {

      if (index_counter == 0) //first chunk read
      {
        total_bytes_available = read_chunk[6];
        index_counter += (bytes_read - 7);

        memcpy(read_buf, read_chunk + 7, bytes_read - 7);
      } else {
        memcpy(read_buf + index_counter, read_chunk + 5, total_bytes_available - index_counter);
        index_counter += (bytes_read - 5);
      }
      bytes_to_read = total_bytes_available - index_counter;
    }
    if (bytes_to_read <= 0) {
      break;
    }

  } while (bytes_read > 0);

  response->len  = total_bytes_available;
  response->data = malloc(total_bytes_available);
  memcpy(response->data, read_buf, total_bytes_available);
}

int get_recid_from_pub_key(const ecdsa_curve* curve, uint8_t* pub_key, const uint8_t* sig, const uint8_t* digest) {

  int     i = 0;
  uint8_t p_key[65];
  int     ret   = 0;
  int     recid = -1;
  for (i = 0; i < 255; i++) {
    ret = ecdsa_recover_pub_from_sig(curve, p_key, sig, digest, i);
    if (ret == 0) {
      if (memcmp(pub_key, p_key, 65) == 0) {
        recid = i;
#ifdef DEBUG
        print_bytes(p_key, 65, "get_recid_from_pub_key :keys matched");
#endif
        break;
      }
    }
  }
  return recid;
}