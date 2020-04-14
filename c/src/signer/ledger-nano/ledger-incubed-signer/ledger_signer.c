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

  hid_device* handle;
  uint8_t     apdu[64];
  uint8_t     buf[2];
  int         index_counter = 0;
  uint8_t     msg_len       = 32;
  uint8_t     hash[32];
  uint8_t     bytes_read    = 0;
  uint8_t     read_buf[255];
  bool        is_hashed = false;
  bytes_t apdu_bytes;
  bytes_t final_apdu_command;
  bytes_t public_key;
  bytes_t bip_path;
  bytes_t response;

  ret                      = eth_ledger_get_public_key(bip_path, &public_key);
  res                      = hid_init();
  handle                   = hid_open(LEDGER_NANOS_VID, LEDGER_NANOS_PID, NULL);
  unsigned char cmd_file[] = {0x01, 0x01, 0x05, 0x00, 0x00, 0x00, 0x07, 0x80, 0x02, 0x80, 0x00, 0x02, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  int           cmd_size   = 64;
  int recid = 0;
  if (NULL != handle) {

    hid_set_nonblocking(handle, 0);

    switch (type) {
      case SIGN_EC_RAW:
        memcpy(hash,message.data, message.len);
        is_hashed = true;
      case SIGN_EC_HASH:
        if(!is_hashed)
          hasher_Raw(HASHER_SHA3K, message.data, message.len, hash);
            
        
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

        memcpy(apdu + index_counter, hash, msg_len);
        index_counter += msg_len;
        apdu_bytes.data = malloc(index_counter);
        apdu_bytes.len  = index_counter;
        memcpy(apdu_bytes.data, apdu, index_counter);

        wrap_apdu(apdu_bytes, 0, &final_apdu_command);

        // print_bytes(final_apdu_command.data, final_apdu_command.len, "eth_ledger_sign");

        res = hid_write(handle, final_apdu_command.data, final_apdu_command.len);
        // res = hid_write(handle, cmd_file, cmd_size);

        printf("written to hid %d\n", res);

        read_hid_response(handle, &response);
        print_bytes(response.data, response.len, "eth_ledger_sign :raw signature");

        if (response.data[response.len - 2] == 0x90 && response.data[response.len - 1] == 0x00) {
          ret              = IN3_OK;
          printf("success respons\n");
          
          extract_signture(response, dst);
          recid = get_recid_from_pub_key(&secp256k1, public_key.data, dst, hash);
          dst[64] = recid;
          print_bytes(dst, 65, "eth_ledger_sign : signature");
        } else {
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
    apdu[index_counter++] = P2_FINAL;
    apdu[index_counter++] = msg_len;

    apdu_bytes.data = malloc(index_counter);
    apdu_bytes.len  = index_counter;
    memcpy(apdu_bytes.data, apdu, index_counter);

    wrap_apdu(apdu_bytes, 0, &final_apdu_command);

    res = hid_write(handle, final_apdu_command.data, final_apdu_command.len);

    read_hid_response(handle, &response);
    // print_bytes(response.data, response.len, "eth_ledger_get_public_key : hid_read");

    if (response.data[response.len - 2] == 0x90 && response.data[response.len - 1] == 0x00) {
      ret              = IN3_OK;
      o_public_key->len = response.len - 2;
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

in3_ret_t eth_ledger_set_signer(in3_t* in3) {
  if (in3->signer) free(in3->signer);
  in3->signer             = malloc(sizeof(in3_signer_t));
  in3->signer->sign       = eth_ledger_sign;
  in3->signer->prepare_tx = NULL;
  in3->signer->wallet     = NULL;
  return IN3_OK;
}

void extract_signture(bytes_t i_raw_sig, uint8_t* o_sig) {

  //ECDSA signature encoded as TLV:  30 L 02 Lr r 02 Ls s
  int lr = i_raw_sig.data[3];
  int ls = i_raw_sig.data[lr+5];
  printf("lr %d, ls %d \n", lr,ls);
  memcpy(o_sig, i_raw_sig.data + 4, lr);
  memcpy(o_sig + lr, i_raw_sig.data + lr + 6, ls);
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
      // print_bytes(read_chunk, bytes_read, "hid_read");
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
  // printf("bytes to read %d total_bytes_available %d\n", bytes_to_read, total_bytes_available);
}


int get_recid_from_pub_key(const ecdsa_curve *curve, uint8_t *pub_key, const uint8_t *sig, const uint8_t *digest)
{

  int i = 0;
  uint8_t p_key[65]; 
  int ret = 0;
  int recid = -1;
  for (i=0; i<4; i++)
  {
    ret = ecdsa_recover_pub_from_sig(curve,p_key,sig,digest,i);
    if(ret ==0 )
    { 
       printf("ret 0 i %d\n",i);
      if(memcmp(pub_key,p_key,65) == 0)
      {
        recid = i;
        printf("recid is %d\n",i);
        break;
      }
    }

  }
  return recid; 
}