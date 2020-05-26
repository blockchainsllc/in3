#ifdef WIN32
#include <windows.h>
#endif

#include "../../../core/util/log.h"
#include "device_apdu_commands.h"
#include "types.h"
#include <memory.h>
#include <stdbool.h>
#include <stdlib.h>

static uint8_t CHANNEL[] = {0x01, 0x01};

uint8_t CLA;
uint8_t INS_GET_PUBLIC_KEY;
uint8_t INS_GET_PUB_ADDR;
uint8_t INS_SIGN_MSG;
uint8_t INS_SIGN_TX;
uint8_t INS_SIGN;
uint8_t P1_MORE;
uint8_t P1_FINAL;
uint8_t P2_FINAL;
uint8_t TAG = 0x05;

void wrap_apdu(uint8_t* i_apdu, int len, uint16_t seq, bytes_t* o_wrapped_hid_cmd) {

  uint16_t apdu_len = (uint16_t) len;
  uint8_t  data[2];
  int      index   = 0;
  int      cmd_len = 0;

  if (len < 55) {
    cmd_len = 64;
  } else if (len > 64 && len < 128) {
    cmd_len = 128;
  } else if (len > 128 && len < 255) {
    cmd_len = 255;
  }
  uint8_t* cmd = malloc(cmd_len);

  memset(cmd + index, 0x00, cmd_len);

  memcpy(cmd + index, CHANNEL, sizeof(CHANNEL));
  index += sizeof(CHANNEL);

  memcpy(cmd + index, &TAG, 1);
  index += 1;

  len_to_bytes(seq, data);
  in3_log_debug("data[0] %d data[1] %d\n", data[0], data[1]);
  memcpy(cmd + index, data, sizeof(data));
  index += sizeof(data);

  len_to_bytes(apdu_len, data);
  in3_log_debug("data[0] %d data[1] %d\n", data[0], data[1]);
  memcpy(cmd + index, data, sizeof(data));
  index += sizeof(data);

  memcpy(cmd + index, i_apdu, len);

  o_wrapped_hid_cmd->len  = cmd_len;
  o_wrapped_hid_cmd->data = malloc(cmd_len);

  memcpy(o_wrapped_hid_cmd->data, cmd, cmd_len);
  free(cmd);
}

void unwrap_apdu(bytes_t i_wrapped_hid_cmd, bytes_t* o_apdu_res) {
  uint8_t buf[2];
  buf[0] = i_wrapped_hid_cmd.data[5];
  buf[1] = i_wrapped_hid_cmd.data[6];

  int len = bytes_to_len(buf);

  o_apdu_res->len  = len;
  o_apdu_res->data = malloc(len);
  memcpy(o_apdu_res->data, len, i_wrapped_hid_cmd.data + 7);
}

int len_to_bytes(uint16_t x, uint8_t* buf) {

  buf[1] = (uint8_t)(x & 0xFF);
  buf[0] = (uint8_t)((x >> 8) & 0xFF);
  return 2;
}

uint16_t bytes_to_len(uint8_t* buf) {
  uint16_t number = (buf[1] << 8) + buf[0];
  return number;
}

void read_hid_response(hid_device* handle, bytes_t* response) {
  uint8_t read_chunk[64];
  uint8_t read_buf[512];
  int     index_counter         = 0;
  int     bytes_to_read         = 0;
  int     total_bytes_available = 0;
  int     bytes_read            = 0;
  // ret          = eth_ledger_get_public_addr(bip_data, public_key);

  // int res = hid_init();
  // handle  = hid_open(LEDGER_NANOS_VID, LEDGER_NANOS_PID, NULL);
  hid_set_nonblocking(handle, 0);

  do {
    printf("reading bytes \n");
    bytes_read = hid_read(handle, read_chunk, sizeof(read_chunk));

    int i = 0;

    if (bytes_read > 0) {
      printf("bytes read %d\n", bytes_read);
      if (index_counter == 0) //first chunk read
      {
        total_bytes_available = read_chunk[6];
        index_counter += (bytes_read - 7);
        // if (index_counter > total_bytes_available) {
        //   index_counter         = 0;
        //   total_bytes_available = 0;
        //   continue;
        // }
        memcpy(read_buf, read_chunk + 7, bytes_read - 7);
      } else {
        memcpy(read_buf + index_counter, read_chunk + 5, total_bytes_available - index_counter);
        index_counter += (bytes_read - 5);
      }

      printf("bytes read %d\n", bytes_read);
      for (i = 0; i < bytes_read; i++) {
        printf("%02x ", read_chunk[i]);
      }
      printf("\n");

      bytes_to_read = total_bytes_available - index_counter;
    }
    if (bytes_to_read <= 0 && total_bytes_available > 1) {
      printf("exiting loop %d %d %d \n", bytes_to_read, total_bytes_available, index_counter);
      break;
    }

  } while (bytes_read > 0);

  printf("total bytes read %d\n", total_bytes_available);
  response->len  = total_bytes_available;
  response->data = malloc(total_bytes_available);
  // hid_close(handle);
  // hid_exit();
  memcpy(response->data, read_buf, total_bytes_available);
}

int write_hid(hid_device* handle, uint8_t* data, int len) {
  // ret          = eth_ledger_get_public_addr(bip_data, public_key);
  printf("write_hid:enter\n");

  bytes_t final_apdu_command;
  uint8_t chunk[64];
  int     res        = 0;
  int     i          = 0;
  int     seq        = 0;
  int     totalBytes = 0;
  int     sent       = 0;
  int     tobesent   = 0;
  int     bufsize    = 0;
  uint8_t header[]   = {0x01, 0x01, 0x05};
  uint8_t seq_data[2];
  uint8_t final_buffer[1000];
  // printf("printing apud %d\n", len);
  // for (i = 0; i < len; i++) {
  //   printf("%02x ", data[i]);
  // }
  // printf("\n");

  wrap_apdu(data, len, 0, &final_apdu_command);

  totalBytes = final_apdu_command.len;

  if (totalBytes > 64) {
    while (totalBytes > 0) {
      if (seq == 0) { // first packet
        memcpy(final_buffer, final_apdu_command.data, 64);
        bufsize += 64;
        totalBytes -= 64;
        sent += 64;
        seq++;
      } else {
        len_to_bytes(seq, seq_data);
        memcpy(chunk, header, sizeof(header));
        memcpy(chunk + sizeof(header), seq_data, sizeof(seq_data));
        tobesent = (totalBytes > (64 - (sizeof(header) + sizeof(seq_data)))) ? (64 - (sizeof(header) + sizeof(seq_data))) : totalBytes;
        memcpy(chunk + sizeof(header) + sizeof(seq_data), final_apdu_command.data + sent, tobesent);
        totalBytes -= tobesent;
        sent += tobesent;

        memcpy(final_buffer + bufsize, chunk, 64);
        bufsize += 64;
      }
    }
  } else {
    memcpy(final_buffer, final_apdu_command.data, final_apdu_command.len);
    bufsize += final_apdu_command.len;
  }

  printf("final apdu\n");
  for (i = 0; i < bufsize; i++) {
    printf("%02x ", final_buffer[i]);
  }
  printf("\n");

  res = hid_write(handle, final_buffer, bufsize);

  // res = hid_write(handle, data, len);
  printf("written to hid\n");

  free(final_apdu_command.data);

  printf("write_hid:exit\n");
  return res;
}