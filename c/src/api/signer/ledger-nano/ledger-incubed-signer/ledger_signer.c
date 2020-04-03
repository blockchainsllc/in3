#ifdef WIN32
#include <windows.h>
#endif

#include "ledger_signer.h"
#include <hidapi.h>
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
  UNUSED_VAR(account); // at least for now
  int       res = 0;
  in3_ret_t ret;
  res = hid_init();
  hid_device* handle;

  handle = (LEDGER_NANOS_VID, LEDGER_NANOS_PID, NULL);

  if (NULL != handle) {
    switch (type) {
      case SIGN_EC_RAW:
        // unsigned char cmd_file[] = {0x01,0x01,0x05,0x00,0x00,0x00,0x07,0x80,0x02,0x00,0x00,0x02
        // 						,0x01,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
        // 						,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
        // 						,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
        // 						,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
        // 						,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
        // 						,0x00,0x00,0x00,0x00};
        int cmd_size = 64;
        res          = hid_write(handle, cmd_file, cmd_size);
        ret          = IN3_OK;
        break;
      case SIGN_EC_HASH:
        // unsigned char cmd_file[] = {0x01,0x01,0x05,0x00,0x00,0x00,0x07,0x80,0x02,0x00,0x00,0x02
        // 						,0x01,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
        // 						,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
        // 						,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
        // 						,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
        // 						,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
        // 						,0x00,0x00,0x00,0x00};

        int cmd_size = 64;
        res          = hid_write(handle, cmd_file, cmd_size);
        ret          = IN3_OK;
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

in3_ret_t eth_ledger_get_public_key(bytes_t i_bip_path, bytes_t o_public_key) {
}

in3_ret_t eth_ledger_set_signer(in3_t* in3) {
  if (in3->signer) free(in3->signer);
  in3->signer             = malloc(sizeof(in3_signer_t));
  in3->signer->sign       = eth_ledger_sign;
  in3->signer->prepare_tx = NULL;
  in3->signer->wallet     = NULL;
  return IN3_OK;
}
