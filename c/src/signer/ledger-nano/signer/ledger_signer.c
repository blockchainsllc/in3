#ifdef WIN32
#include <windows.h>
#endif

#include "../../../core/client/context.h"
#include "../../../core/util/bytes.h"
#include "../../../core/util/log.h"
#include "device_apdu_commands.h"
#include "ledger_signer.h"
#include "ledger_signer_priv.h"
#include "utility.h"

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

in3_ret_t eth_ledger_sign(in3_sign_ctx_t* sc) {
  // void* ctx, d_signature_type_t type, bytes_t message, bytes_t account, uint8_t* dst
  uint8_t* bip_path_bytes = sc->wallet;

  uint8_t bip_data[5];

  int       res = 0;
  in3_ret_t ret;

  hid_device* handle;
  uint8_t     apdu[64];
  int         index_counter = 0;
  int         msg_len       = 0;

  uint8_t hash[32];

  bool    is_hashed = false;
  uint8_t public_key[65];
  bytes_t response;

  memcpy(bip_data, bip_path_bytes, sizeof(bip_data));
  set_command_params(); // setting apdu params for normal incubed signing app

  ret       = eth_ledger_get_public_key(bip_data, public_key);
  handle    = open_device();
  int recid = 0;

  if (NULL != handle) {

    hid_set_nonblocking(handle, 0);

    switch (sc->type) {
      case SIGN_EC_RAW:
        memcpy(hash, sc->message.data, sc->message.len);
        is_hashed = true;
      case SIGN_EC_HASH:
        if (!is_hashed)
          hasher_Raw(HASHER_SHA3K, sc->message.data, sc->message.len, hash);

        apdu[index_counter++] = CLA;
        apdu[index_counter++] = INS_SIGN;
        apdu[index_counter++] = P1_FINAL;
        apdu[index_counter++] = IDM_SECP256K1;

        apdu[index_counter++] = 0x01; //1st arg tag
        apdu[index_counter++] = sizeof(bip_data);
        memcpy(apdu + index_counter, &bip_data, sizeof(bip_data));
        index_counter += sizeof(bip_data);

        apdu[index_counter++] = 0x02; //2nd arg tag
        apdu[index_counter++] = msg_len;
        memcpy(apdu + index_counter, hash, msg_len);
        index_counter += msg_len;

#ifdef DEBUG
        in3_log_debug("apdu commnd sent to device\n");
        ba_print(apdu, index_counter);
#endif

        res = write_hid(handle, apdu, index_counter);

        in3_log_debug("written to hid %d\n", res);

        read_hid_response(handle, &response);

#ifdef DEBUG
        in3_log_debug("response received from device\n");
        ba_print(response.data, response.len);
#endif

        if (response.data[response.len - 2] == 0x90 && response.data[response.len - 1] == 0x00) {
          ret = IN3_OK;

          in3_log_debug("apdu executed succesfully \n");
          extract_signture(response, sc->signature);
          recid             = get_recid_from_pub_key(&secp256k1, public_key, sc->signature, hash);
          sc->signature[64] = recid;

#ifdef DEBUG
          in3_log_debug("printing signature returned by device with recid value\n");
          ba_print(sc->signature, 65);
#endif

        } else {
          in3_log_fatal("error in apdu execution \n");
          free(response.data);
          close_device(handle);
          return IN3_EAPDU;
        }

        ret = IN3_OK;
        free(response.data);
        close_device(handle);
        break;

      default:
        return IN3_ENOTSUP;
    }
  } else {
    in3_log_fatal("no ledger device connected \n");
    return IN3_ENODEVICE;
  }

  return 65;
}

in3_ret_t eth_ledger_get_public_key(uint8_t* i_bip_path, uint8_t* o_public_key) {
  int       res = 0;
  in3_ret_t ret;
  uint8_t   apdu[64];
  int       index_counter = 0;

  bytes_t     response;
  hid_device* handle;

  handle = open_device();
  if (NULL != handle) {
    apdu[index_counter++] = CLA;
    apdu[index_counter++] = INS_GET_PUBLIC_KEY;
    apdu[index_counter++] = P1_FINAL;
    apdu[index_counter++] = IDM_SECP256K1;

    apdu[index_counter++] = 5;
    memcpy(apdu + index_counter, i_bip_path, 5);
    index_counter += 5;

    res = write_hid(handle, apdu, index_counter);

    read_hid_response(handle, &response);

#ifdef DEBUG
    in3_log_debug("response received from device\n");
    ba_print(response.data, response.len);
#endif

    if (response.data[response.len - 2] == 0x90 && response.data[response.len - 1] == 0x00) {
      ret = IN3_OK;
      memcpy(o_public_key, response.data, response.len - 2);
    } else {
      free(response.data);
      close_device(handle);
      return IN3_EAPDU;
    }

    free(response.data);
    close_device(handle);

  } else {
    return IN3_ENODEVICE;
  }

  return ret;
}

in3_ret_t eth_ledger_set_signer(in3_t* in3, uint8_t* bip_path) {

  if (in3->signer) free(in3->signer);
  in3->signer             = malloc(sizeof(in3_signer_t));
  in3->signer->sign       = eth_ledger_sign;
  in3->signer->prepare_tx = NULL;
  in3->signer->wallet     = bip_path;
  // generate the address from the key
  uint8_t   public_key[65], sdata[32];
  bytes_t   pubkey_bytes = {.data = public_key + 1, .len = 64};
  bytes32_t bip32;
  memcpy(bip32, bip_path, 5);
  eth_ledger_get_public_key(bip32, public_key);
  sha3_to(&pubkey_bytes, sdata);
  memcpy(in3->signer->default_address, sdata + 12, 20);

  return IN3_OK;
}

void set_command_params() {
  CLA                = 0x80;
  INS_GET_PUBLIC_KEY = 0x04;
  INS_SIGN           = 0x02;
  P1_MORE            = 0x00;
  P1_FINAL           = 0X80;
  P2_FINAL           = 0X00;
  TAG                = 0x05;
}
