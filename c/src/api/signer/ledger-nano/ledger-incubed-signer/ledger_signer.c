#ifdef WIN32
#include <windows.h>
#endif

#include "device_apdu_commands.h"
#include "ledger_signer.h"

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
    res = hid_get_manufacturer_string(handle, wstr, MAX_STR);
    printf("Manufacturer String: %ls\n", wstr);

    hid_get_product_string(handle, wstr, MAX_STR);
    printf("Product String: %ls\n", wstr);

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
  int       res = 0;
  in3_ret_t ret;
  res = hid_init();
  hid_device* handle;
  uint8_t     apdu[64];
  uint8_t     buf[2];
  int         index_counter = 0;
  uint8_t     msg_len       = 32;
  handle                    = hid_open(LEDGER_NANOS_VID, LEDGER_NANOS_PID, NULL);
  bytes_t apdu_bytes;
  bytes_t final_apdu_command;
  bytes_t public_key;
  bytes_t bip_path;

  unsigned char cmd_file[] = {0x01, 0x01, 0x05, 0x00, 0x00, 0x00, 0x07, 0x80, 0x02, 0x00, 0x00, 0x02, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  int           cmd_size   = 64;

  if (NULL != handle) {
    ret = eth_ledger_get_public_key(bip_path, &public_key, &handle);
    switch (type) {
      case SIGN_EC_RAW:
        res = hid_write(handle, cmd_file, cmd_size);
        ret = IN3_OK;
        break;
      case SIGN_EC_HASH:
        // unsigned char cmd_file[] = {0x01,0x01,0x05,0x00,0x00,0x00,0x07,0x80,0x02,0x00,0x00,0x02
        // 						,0x01,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
        // 						,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
        // 						,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
        // 						,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
        // 						,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
        // 						,0x00,0x00,0x00,0x00};
        memcpy(apdu + index_counter, &CLA, 1);
        index_counter += 1;
        memcpy(apdu + index_counter, &INS_SIGN, 1);
        index_counter += 1;
        memcpy(apdu + index_counter, &P1_FINAL, 1);
        index_counter += 1;
        memcpy(apdu + index_counter, &P2_FINAL, 1);
        index_counter += 1;

        memcpy(apdu + index_counter, &msg_len, sizeof(uint8_t));
        index_counter += sizeof(uint8_t);

        memcpy(apdu + index_counter, message.data, msg_len);
        index_counter += msg_len;
        apdu_bytes.data = malloc(index_counter);
        apdu_bytes.len  = index_counter;
        memcpy(apdu_bytes.data, apdu, index_counter);

        wrap_apdu(apdu_bytes, 0, &final_apdu_command);

        print_bytes(final_apdu_command.data, final_apdu_command.len, "eth_ledger_sign");

        res = hid_write(handle, final_apdu_command.data, final_apdu_command.len);
        ret = IN3_OK;

        free(final_apdu_command.data);
        free(apdu_bytes.data);
        break;

      default:
        return IN3_ENOTSUP;
    }

  } else {
    ret = IN3_ENODEVICE;
  }
  hid_close(handle);
  res = hid_exit();
  return ret;
}

in3_ret_t eth_ledger_get_public_key(bytes_t i_bip_path, bytes_t* o_public_key, hid_device* handle) {
  int       res = 0;
  in3_ret_t ret;
  uint8_t  apdu[64];
  uint8_t  buf[2];
  int      index_counter = 0;
  uint16_t msg_len       = 0;

  bytes_t apdu_bytes;
  bytes_t final_apdu_command;

  apdu[index_counter++] = CLA;
  apdu[index_counter++] = INS_GET_PUBLIC_KEY ;
  apdu[index_counter++] = P1_FINAL;
  apdu[index_counter++] = P2_FINAL;
  apdu[index_counter++] = msg_len;
  

  apdu_bytes.data = malloc(index_counter);
  apdu_bytes.len  = index_counter;
  memcpy(apdu_bytes.data, apdu, index_counter);

  wrap_apdu(apdu_bytes, 0, &final_apdu_command);

  print_bytes(final_apdu_command.data, final_apdu_command.len, "eth_ledger_get_public_key");

  res = hid_write(handle, final_apdu_command.data, final_apdu_command.len);
  ret = IN3_OK;

  free(final_apdu_command.data);
  free(apdu_bytes.data);

  //hid_close(handle);
  // res = hid_exit();
  return ret;
}

in3_ret_t eth_ledger_set_signer(in3_t* in3) {
  if (in3->signer) free(in3->signer);
  in3->signer             = malloc(sizeof(in3_signer_t));
  in3->signer->sign       = eth_ledger_sign;
  in3->signer->prepare_tx = NULL;
  in3->signer->wallet     = NULL;
  return IN3_OK;
}
