#ifdef WIN32
#include <windows.h>
#endif

#include "../../../core/util/log.h"
#include "device_apdu_commands.h"
#include <memory.h>
#include <stdbool.h>
#include <stdlib.h>

static uint8_t CHANNEL[] = {0x01, 0x01};

uint8_t CLA                = 0x80;
uint8_t INS_GET_PUBLIC_KEY = 0x04;
uint8_t INS_SIGN           = 0x02;
uint8_t P1_MORE            = 0x00;
uint8_t P1_FINAL           = 0X80;
uint8_t P2_FINAL           = 0X00;
uint8_t TAG                = 0x05;

void wrap_apdu(bytes_t i_apdu, uint16_t seq, bytes_t* o_wrapped_hid_cmd) {

  uint16_t apdu_len = (uint16_t) i_apdu.len;
  uint8_t  data[2];
  int      index = 0;
  uint8_t  cmd[HID_CMD_MAX_LEN];

  memset(cmd + index, 0x00, sizeof(cmd));

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

  memcpy(cmd + index, i_apdu.data, i_apdu.len);

  o_wrapped_hid_cmd->len  = HID_CMD_MAX_LEN;
  o_wrapped_hid_cmd->data = malloc(HID_CMD_MAX_LEN);

  memcpy(o_wrapped_hid_cmd->data, cmd, HID_CMD_MAX_LEN);
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