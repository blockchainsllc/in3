#include "ethereum_apdu_client.h"
#include "../../../core/client/context.h"
#include "../../../core/util/log.h"
#include "device_apdu_commands.h"
#include "ethereum_apdu_client_priv.h"
#include "types.h"
#include "utility.h"

static uint8_t public_key[65];
static int     is_public_key_assigned = false;

in3_ret_t eth_ledger_sign_txn(in3_sign_ctx_t* sc) {
  in3_log_debug("eth_ledger_sign_txn:enter\n");
  uint8_t* bip_path_bytes = sc->wallet;
  uint8_t  bip_data[5];
  bool     is_msg = false;

  in3_ret_t ret;
  uint8_t   apdu[256];
  int       index_counter = 0;

  uint8_t  hash[32];
  uint8_t  bip32_len = 5;
  uint32_t bip32[5];

  bool        is_hashed = false;
  bytes_t     response;
  hid_device* handle;
  char        prefix[] = "msg";

  handle = open_device();
  memcpy(bip_data, bip_path_bytes, sizeof(bip_data));
  set_command_params_eth();

  //parse and convert bip into hardened form
  read_bip32_path(5, bip_data, bip32);

  int recid = 0;

  if (NULL != handle) {

    switch (sc->type) {
      case SIGN_EC_RAW:
        memcpy(hash, sc->message.data, sc->message.len);
        is_hashed = true;
      case SIGN_EC_HASH:
        if (memcmp(prefix, sc->message.data, strlen(prefix)) == 0) {
          is_msg = true;
        }

        if (!is_hashed && is_msg == true)
          hasher_Raw(HASHER_SHA3K, sc->message.data + strlen(prefix), sc->message.len - strlen(prefix), hash);
        else {
          hasher_Raw(HASHER_SHA3K, sc->message.data, sc->message.len, hash);
        }

        apdu[index_counter++] = 0xe0;

        if (is_msg == true) // different instruction bytes for message and transaction
          apdu[index_counter++] = 0x08;
        else
          apdu[index_counter++] = 0x04;

        apdu[index_counter++] = 0x00;
        apdu[index_counter++] = 0x00;

        if (is_msg == true) { // final apdu lenghts will be adjusted differenly for message and transaction`

          apdu[index_counter++] = bip32_len * sizeof(uint32_t) + 5 + (sc->message.len - strlen(prefix));
        } else {
          apdu[index_counter++] = bip32_len * sizeof(uint32_t) + 1 + sc->message.len;
        }

        apdu[index_counter++] = bip32_len;
        memcpy(apdu + index_counter, bip32, bip32_len * sizeof(uint32_t));
        index_counter += bip32_len * sizeof(uint32_t);

        if (is_msg == true) {
          apdu[index_counter++] = 0x00;
          apdu[index_counter++] = 0x00;
          apdu[index_counter++] = 0x00;
          apdu[index_counter++] = sc->message.len - strlen(prefix);
          memcpy(apdu + index_counter, sc->message.data + strlen(prefix), sc->message.len - strlen(prefix));
          index_counter += sc->message.len - strlen(prefix);
        } else {
          memcpy(apdu + index_counter, sc->message.data, sc->message.len);
          index_counter += sc->message.len;
        }

        write_hid(handle, apdu, index_counter);

        read_hid_response(handle, &response);

#ifdef DEBUG
        in3_log_debug("response received from device\n");
        ba_print(response.data, response.len);
#endif

        if (response.len > 1) {
          if (response.data[response.len - 2] == 0x90 && response.data[response.len - 1] == 0x00) {
            ret = IN3_OK;

            memcpy(sc->signature, response.data + 1, 64);


            recid             = get_recid_from_pub_key(&secp256k1, public_key, sc->signature, hash);

            sc->signature[64] = recid;
            in3_log_debug("recid %d\n", recid);
#ifdef DEBUG
            in3_log_debug("hash value\n");
            ba_print(hash, 32);
            in3_log_debug("recid %d\n", recid);
            in3_log_debug("printing signature returned by device with recid value\n");

            ba_print(sc->signature, 65);
#endif

          } else {
            in3_log_fatal("error in apdu execution \n");
            close_device(handle);
            free(response.data);
            return IN3_EAPDU;
          }
        } else {
          in3_log_fatal("error in apdu execution \n");
          close_device(handle);
          free(response.data);
          return IN3_EAPDU;
        }

        close_device(handle);
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
  return IN3_OK;
}

in3_ret_t eth_ledger_get_public_addr(uint8_t* i_bip_path, uint8_t* o_public_key) {
  in3_log_debug("eth_ledger_get_public_addr:enter\n");
  in3_ret_t     ret;
  uint8_t       apdu[64];
  int           index_counter = 0;
  const uint8_t bip32_len     = 5;
  uint32_t      bip32[5];

  bytes_t     response;
  hid_device* handle;

  handle = open_device();
  if (NULL != handle) {
    if (is_public_key_assigned) {
      memcpy(o_public_key, public_key, 65);
      close_device(handle);
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

    write_hid(handle, apdu, index_counter);

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
        free(response.data);
        close_device(handle);
        return IN3_EAPDU;
      }
    } else {
      in3_log_fatal("error in apdu execution \n");
      free(response.data);
      close_device(handle);
      return IN3_EAPDU;
    }

    free(response.data);
    close_device(handle);
  } else {

    return IN3_ENODEVICE;
  }

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

  // generate the address from the key
  uint8_t   public_key[65], sdata[32];
  bytes_t   pubkey_bytes = {.data = public_key + 1, .len = 64};
  bytes32_t bip32;
  memcpy(bip32, bip_path, 5);
  eth_ledger_get_public_addr(bip32, public_key);
  sha3_to(&pubkey_bytes, sdata);
  memcpy(in3->signer->default_address, sdata + 12, 20);

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
