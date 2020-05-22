#include "ethereum_apdu_client.h"
#include "../../../core/client/context.h"
#include "../../../core/util/log.h"
#include "device_apdu_commands.h"
#include "ethereum_apdu_client_priv.h"
#include "types.h"
#include "utility.h"

in3_ret_t eth_ledger_sign_txn(void* ctx, d_signature_type_t type, bytes_t message, bytes_t account, uint8_t* dst) {
  //UNUSED_VAR(account); // at least for now
  printf("eth_ledger_sign_txn:enter\n");
  uint8_t* bip_path_bytes = ((in3_ctx_t*) ctx)->client->signer->wallet;

  uint8_t bip_data[5];

  int       res = 0;
  in3_ret_t ret;

  hid_device* handle;
  uint8_t     apdu[64];
  uint8_t     buf[2];
  int         index_counter = 0;
  uint8_t     bytes_read    = 0;
  int         i             = 0;

  uint8_t msg_len = 32;
  uint8_t hash[32];
  uint8_t read_buf[255];

  bool    is_hashed = false;
  bytes_t apdu_bytes;
  bytes_t final_apdu_command;
  uint8_t public_key[65];
  bytes_t response;

  memcpy(bip_data, bip_path_bytes, sizeof(bip_data));
  set_command_params_eth();

  ret          = eth_ledger_get_public_addr(bip_data, public_key);
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

        for (i = 0; i < message.len; i++) {
          printf("%02x ", message.data[i]);
        }
        printf("\n");

        apdu[index_counter++] = CLA;
        apdu[index_counter++] = INS_SIGN_TX;
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

        apdu_bytes.data = malloc(index_counter);
        apdu_bytes.len  = index_counter;
        memcpy(apdu_bytes.data, apdu, index_counter);

        wrap_apdu(apdu_bytes, 0, &final_apdu_command);

#ifdef DEBUG
        in3_log_debug("apdu commnd sent to device\n");
        ba_print(final_apdu_command.data, final_apdu_command.len);
#endif

        res = hid_write(handle, final_apdu_command.data, final_apdu_command.len);

        in3_log_debug("written to hid %d\n", res);

        read_hid_response(handle, &response);

#ifdef DEBUG
        in3_log_debug("response received from device\n");
        ba_print(response.data, response.len);
#endif

        if (response.data[response.len - 2] == 0x90 && response.data[response.len - 1] == 0x00) {
          ret = IN3_OK;

          in3_log_debug("apdu executed succesfully \n");
          extract_signture(response, dst);
          recid   = get_recid_from_pub_key(&secp256k1, public_key, dst, hash);
          dst[64] = recid;

#ifdef DEBUG
          in3_log_debug("printing signature returned by device with recid value\n");
          ba_print(dst, 65);
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
  printf("eth_ledger_sign_txn:exit\n");
  return 65;
}

in3_ret_t eth_ledger_get_public_addr(uint8_t* i_bip_path, uint8_t* o_public_key) {
  printf("eth_ledger_get_public_addr:enter\n");
  int           res = 0;
  in3_ret_t     ret;
  uint8_t       apdu[64];
  uint8_t       buf[2];
  int           index_counter = 0;
  uint16_t      msg_len       = 0;
  uint8_t       bytes_read    = 0;
  const uint8_t bip32_len     = 5;
  uint32_t      bip32[5];
  int           i = 0;
  bytes_t       apdu_bytes;
  bytes_t       final_apdu_command;
  bytes_t       response;
  hid_device*   handle;

  res    = hid_init();
  handle = hid_open(LEDGER_NANOS_VID, LEDGER_NANOS_PID, NULL);
  if (NULL != handle) {
    //parse and convert bip into hardened form
    printf("device handle fechted\n");
    read_bip32_path(5, i_bip_path, bip32);

    apdu[index_counter++] = CLA;
    apdu[index_counter++] = INS_GET_PUB_ADDR;
    apdu[index_counter++] = P1_FINAL;
    apdu[index_counter++] = 0x00;

    apdu[index_counter++] = bip32_len * sizeof(uint32_t) + 1;
    apdu[index_counter++] = bip32_len;
    memcpy(apdu + index_counter, bip32, bip32_len * sizeof(uint32_t));
    index_counter += bip32_len * sizeof(uint32_t);

    apdu_bytes.data = malloc(index_counter);
    apdu_bytes.len  = index_counter;
    memcpy(apdu_bytes.data, apdu, index_counter);

    printf("get pub key apdu cmd\n");
    for (i = 0; i < apdu_bytes.len; i++) {
      printf("%02x ", apdu_bytes.data[i]);
    }
    printf("\n");

    wrap_apdu(apdu_bytes, 0, &final_apdu_command);

    printf("get pub key final apdu cmd\n");
    for (i = 0; i < final_apdu_command.len; i++) {
      printf("%02x ", final_apdu_command.data[i]);
    }
    printf("\n");

    res = hid_write(handle, final_apdu_command.data, final_apdu_command.len);

    read_hid_response(handle, &response);

    printf("get pub key final apdu cmd\n");
    for (i = 0; i < response.len; i++) {
      printf("%02x ", response.data[i]);
    }
    printf("\n");
#ifdef DEBUG
    in3_log_debug("response received from device\n");
    ba_print(response.data, response.len);
#endif

    if (response.data[response.len - 2] == 0x90 && response.data[response.len - 1] == 0x00) {
      ret = IN3_OK;
      memcpy(o_public_key, response.data + 1, 65);
      printf("pub key final\n");
      for (i = 0; i < 65; i++) {
        printf("%02x ", o_public_key[i]);
      }
      printf("\n");
    } else {
      ret = IN3_ENOTSUP;
    }
    free(final_apdu_command.data);
    free(apdu_bytes.data);
    free(response.data);

  } else {
    printf("device handle not fechted\n");
    ret = IN3_ENODEVICE;
  }
  hid_close(handle);
  res = hid_exit();
  printf("eth_ledger_get_public_addr:exit\n");
  return ret;
}

void read_bip32_path(uint8_t path_length, const uint8_t* path, uint32_t* bip32_path) {
  for (size_t i = 0; i < path_length; i++) {

    if (i < 3)
      bip32_path[i] = reverse_bytes(path[i] | (0x80000000));
    else
      bip32_path[i] = reverse_bytes((0x00000000) | path[i]);
  }
}

in3_ret_t eth_ledger_set_signer_txn(in3_t* in3, uint8_t* bip_path) {
  if (in3->signer) free(in3->signer);
  in3->signer             = malloc(sizeof(in3_signer_t));
  in3->signer->sign       = eth_ledger_sign_txn;
  in3->signer->prepare_tx = NULL;
  in3->signer->wallet     = bip_path;
  return IN3_OK;
}

void set_command_params_eth() {
  CLA              = 0xE0;
  INS_SIGN_TX      = 0x04;
  INS_SIGN_MSG     = 0x08;
  INS_GET_PUB_ADDR = 0x02;
  P1_MORE          = 0x00;
  P1_FINAL         = 0X01;
  P2_FINAL         = 0X00;
  TAG              = 0x05;
}
