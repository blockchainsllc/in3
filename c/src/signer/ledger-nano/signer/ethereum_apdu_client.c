#include "ethereum_apdu_client.h"
#include "../../../core/client/context.h"
#include "../../../core/util/log.h"
#include "device_apdu_commands.h"
#include "ethereum_apdu_client_priv.h"
#include "types.h"
#include "utility.h"

static uint8_t public_key[65];
static int     is_public_key_assigned = false;

in3_ret_t eth_ledger_sign_txn(void* ctx, d_signature_type_t type, bytes_t message, bytes_t account, uint8_t* dst) {
  in3_log_debug("eth_ledger_sign_txn:enter\n");
  //UNUSED_VAR(account); // at least for now
  uint8_t* bip_path_bytes = ((in3_ctx_t*) ctx)->client->signer->wallet;

  uint8_t bip_data[5];

  int       res = 0;
  in3_ret_t ret;

  uint8_t apdu[256];
  uint8_t buf[2];
  int     index_counter = 0;
  uint8_t bytes_read    = 0;
  int     i             = 0;

  uint8_t  msg_len = 32;
  uint8_t  hash[32];
  uint8_t  read_buf[255];
  uint8_t  bip32_len = 5;
  uint32_t bip32[5];

  bool    is_hashed = false;
  bytes_t apdu_bytes;
  // uint8_t public_key[65];
  bytes_t     response;
  hid_device* handle;

  res    = hid_init();
  handle = hid_open(LEDGER_NANOS_VID, LEDGER_NANOS_PID, NULL);
  memcpy(bip_data, bip_path_bytes, sizeof(bip_data));
  set_command_params_eth();

  //parse and convert bip into hardened form
  read_bip32_path(5, bip_data, bip32);

  int cmd_size = 64;
  int recid    = 0;

  if (NULL != handle) {

    switch (type) {
      case SIGN_EC_RAW:
        memcpy(hash, message.data, message.len);
        is_hashed = true;
      case SIGN_EC_HASH:
        if (!is_hashed)
          hasher_Raw(HASHER_SHA3K, message.data, message.len, hash);

        apdu[index_counter++] = 0xe0;
        apdu[index_counter++] = 0x04;
        apdu[index_counter++] = 0x00;
        apdu[index_counter++] = 0x00;

        apdu[index_counter++] = bip32_len * sizeof(uint32_t) + 1 + message.len;
        apdu[index_counter++] = bip32_len;
        memcpy(apdu + index_counter, bip32, bip32_len * sizeof(uint32_t));
        index_counter += bip32_len * sizeof(uint32_t);

        // apdu[index_counter++] = message.len; //2nd arg tag
        memcpy(apdu + index_counter, message.data, message.len);
        index_counter += message.len;

        apdu_bytes.data = malloc(index_counter);
        apdu_bytes.len  = index_counter;
        memcpy(apdu_bytes.data, apdu, index_counter);

#ifdef DEBUG
        in3_log_debug("apdu commnd sent to device\n");
        ba_print(final_apdu_command.data, final_apdu_command.len);
#endif

        write_hid(handle, apdu_bytes.data, apdu_bytes.len);

        read_hid_response(handle, &response);

#ifdef DEBUG
        in3_log_debug("response received from device\n");
        ba_print(response.data, response.len);
#endif

        if (response.len > 1) {
          if (response.data[response.len - 2] == 0x90 && response.data[response.len - 1] == 0x00) {
            ret = IN3_OK;

            memcpy(dst, response.data + 1, 64);
            recid   = get_recid_from_pub_key(&secp256k1, public_key, dst, hash);
            dst[64] = recid;
            in3_log_debug("recid %d\n", recid);
#ifdef DEBUG
            in3_log_debug("printing signature returned by device with recid value\n");
            ba_print(dst, 65);
#endif

          } else {
            in3_log_fatal("error in apdu execution \n");
            return IN3_EAPDU;
          }
        } else {
          in3_log_fatal("error in apdu execution \n");
          return IN3_EAPDU;
        }
        free(apdu_bytes.data);
        free(response.data);
        ret = IN3_OK;
        break;

      default:
        return IN3_ENOTSUP;
    }

  } else {
    in3_log_fatal("no ledger device connected \n");
    return IN3_ENODEVICE;
  }

  in3_log_debug("eth_ledger_sign_txn:exit\n");
  return 65;
}

in3_ret_t eth_ledger_get_public_addr(uint8_t* i_bip_path, uint8_t* o_public_key) {
  in3_log_debug("eth_ledger_get_public_addr:enter\n");

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

    if (is_public_key_assigned) {
      memcpy(o_public_key, public_key, 65);
      hid_close(handle);
      res = hid_exit();
      return IN3_OK;
    }

    //parse and convert bip into hardened form
    read_bip32_path(5, i_bip_path, bip32);

    apdu[index_counter++] = 0xe0;
    apdu[index_counter++] = 0x02;
    apdu[index_counter++] = 0x01;
    apdu[index_counter++] = 0x00;

    apdu[index_counter++] = bip32_len * sizeof(uint32_t) + 1;
    apdu[index_counter++] = bip32_len;
    memcpy(apdu + index_counter, bip32, bip32_len * sizeof(uint32_t));
    index_counter += bip32_len * sizeof(uint32_t);

    apdu_bytes.data = malloc(index_counter);
    apdu_bytes.len  = index_counter;
    memcpy(apdu_bytes.data, apdu, index_counter);

    write_hid(handle, apdu_bytes.data, apdu_bytes.len);

    read_hid_response(handle, &response);

#ifdef DEBUG
    in3_log_debug("response received from device\n");
    ba_print(response.data, response.len);
#endif

    if (response.len > 1) {
      if (response.data[response.len - 2] == 0x90 && response.data[response.len - 1] == 0x00) {
        ret = IN3_OK;
        memcpy(public_key, response.data + 1, 65);
        memcpy(o_public_key, public_key, 65);
        is_public_key_assigned = true;
      } else {
        in3_log_fatal("error in apdu execution \n");
        return IN3_EAPDU;
      }
    } else {
      in3_log_fatal("error in apdu execution \n");
      return IN3_EAPDU;
    }
    // free(final_apdu_command.data);
    free(apdu_bytes.data);
    free(response.data);
  } else {

    ret = IN3_ENODEVICE;
  }
  hid_close(handle);
  res = hid_exit();
  in3_log_debug("eth_ledger_get_public_addr:exit\n");

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
