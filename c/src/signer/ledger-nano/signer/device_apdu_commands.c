#ifdef WIN32
#include <windows.h>
#endif

#include "../../../core/util/log.h"
#include "device_apdu_commands.h"
#include "types.h"
#include "utility.h"
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
  int      index      = 0;
  int      cmd_len    = 0;
  int      header_len = 7;

  if (len < 64 - header_len) {
    cmd_len = 64;
  } else if (len > 64 - header_len && len < 128 - header_len) {
    cmd_len = 128;
  } else if (len > 128 - header_len && len < 255 - header_len) {
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
  uint8_t bug_header[]          = {0x01, 0x01, 0x05, 0x00, 0xbf, 0x00, 0x01, 0x04}; // known and open issue in ledger repo for mac
                                                                                    // https://github.com/LedgerHQ/ledger-nano-s/issues/55

  hid_set_nonblocking(handle, 0);

  do {
    bytes_read = hid_read(handle, read_chunk, sizeof(read_chunk));

    if (memcmp(bug_header, read_chunk, sizeof(bug_header)) == 0) { //random bug header received, signing will have to be reattempted
      total_bytes_available = 0;
      index_counter         = 0;
      break;
    }
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

    if (bytes_to_read <= 0 && total_bytes_available > 1) {
      break;
    }

  } while (bytes_read > 0);

  response->len  = total_bytes_available;
  response->data = malloc(total_bytes_available);

  memcpy(response->data, read_buf, total_bytes_available);
}

int write_hid(hid_device* handle, uint8_t* data, int len) {
  bytes_t final_apdu_command;
  uint8_t chunk[64];
  int     res        = 0;
  int     seq        = 0;
  int     totalBytes = 0;
  int     sent       = 0;
  int     tobesent   = 0;
  int     bufsize    = 0;
  uint8_t header[]   = {0x01, 0x01, 0x05};
  uint8_t seq_data[2];
  int     total_padding = 0;

  wrap_apdu(data, len, 0, &final_apdu_command);
  total_padding = final_apdu_command.len - (len + 7);
  totalBytes    = final_apdu_command.len;

  if (totalBytes > 64) {
    while (totalBytes > total_padding) {
      if (seq == 0) { // first packet
        hid_write(handle, final_apdu_command.data, 64);
        bufsize += 64;
        totalBytes -= 64;
        sent += 64;
        seq++;
      } else {
        len_to_bytes(seq, seq_data);
        memset(chunk, 0, 64);
        memcpy(chunk, header, sizeof(header));
        memcpy(chunk + sizeof(header), seq_data, sizeof(seq_data));
        tobesent = (totalBytes > (int) (64 - (sizeof(header) + sizeof(seq_data)))) ? (64 - (sizeof(header) + sizeof(seq_data))) : totalBytes;
        memcpy(chunk + sizeof(header) + sizeof(seq_data), final_apdu_command.data + sent, tobesent);
        totalBytes -= tobesent;
        sent += tobesent;
        seq++;
        hid_write(handle, chunk, 64);
        bufsize += 64;
      }
    }
  } else {
    hid_write(handle, final_apdu_command.data, final_apdu_command.len);
    bufsize += final_apdu_command.len;
  }

  free(final_apdu_command.data);
  return res;
}

hid_device* open_device() {
  struct hid_device_info* device_info;
  hid_device*             handle;
  int                     res = hid_init();
  if (res == 0) {
    printf("open_device:opening device\n");
    device_info = hid_enumerate(LEDGER_NANOS_VID, LEDGER_NANOS_PID);
    printf("open_device: hid enumerated\n");
    if (device_info != NULL) {
      printf("open_device:device found\n");
      handle = hid_open_path(device_info->path);
    } else {
      handle = NULL;
    }
    printf("open_device: freeing resouces\n");
    hid_free_enumeration(device_info);
  } else {
    handle = NULL;
  }

  return handle;
}

void close_device(hid_device* handle) {
  hid_close(handle);
  hid_exit();
}