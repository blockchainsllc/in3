#include "apdu.h"

#include "menu.h"
#include "operations.h"
#include "ui.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

ux_state_t ux;

void main_loop(void) {
  volatile unsigned int rx    = 0;
  volatile unsigned int tx    = 0;
  volatile unsigned int flags = 0;
  int                   isequal;
  char*                 ans;

  // next timer callback in 500 ms
  UX_CALLBACK_SET_INTERVAL(500);

  // DESIGN NOTE: the bootloader ignores the way APDU are fetched. The only
  // goal is to retrieve APDU.
  // When APDU are to be fetched from multiple IOs, like NFC+USB+BLE, make
  // sure the io_event is called with a
  // switch event, before the apdu is replied to the bootloader. This avoid
  // APDU injection faults.
  for (;;) {
    volatile unsigned short sw = 0;

    BEGIN_TRY {
      TRY {
        rx = tx;
        tx = 0; // ensure no race in catch_other if io_exchange throws an error

        rx    = io_exchange(CHANNEL_APDU | flags, rx);
        flags = 0;

        // no apdu received, well, reset the session, and reset the
        // bootloader configuration
        if (rx == 0) {
          THROW(EXC_SECURITY);
        }

        if (G_io_apdu_buffer[0] != CLA) {
          THROW(EXC_CLASS);
        }

        switch (G_io_apdu_buffer[1]) {

          case INS_SIGN: {

            if ((G_io_apdu_buffer[2] != P1_MORE) &&
                (G_io_apdu_buffer[2] != P1_LAST)) {
              THROW(EXC_WRONG_PARAM_P1P2);
            }

            current_text_pos = 0;
            text_y           = 60;
            cx_ecfp_public_key_t  publicKey;
            cx_ecfp_private_key_t privateKey;
            cx_curve_t            curve_id = curve_code_to_curve(G_io_apdu_buffer[3]);

            if (G_io_apdu_buffer[4] != TAG_ARG1) {
              THROW(EXC_WRONG_PARAM);
            }

            int           path_length = G_io_apdu_buffer[5];
            unsigned char bip32_path[path_length];
            memcpy(bip32_path, G_io_apdu_buffer + 6, path_length);

            publicKey = public_key_at_given_path(curve_id, bip32_path, path_length);

            if (G_io_apdu_buffer[6 + path_length] != TAG_ARG2) {
              THROW(EXC_WRONG_PARAM);
            }

            int hash_len = G_io_apdu_buffer[7 + path_length];
            PRINTF(" hash length %d \n", hash_len);
            if (hash_len != HASH_LEN) {
              THROW(EXC_WRONG_PARAM);
            }

            memcpy(msg_hash, G_io_apdu_buffer + 8 + path_length, HASH_LEN);
            PRINTF("msg hash copied %02x %02x %02x %02x\n", msg_hash[0], msg_hash[1], msg_hash[30], msg_hash[31]);

            private_key_at_given_path(curve_id, bip32_path, path_length);

            PRINTF("private key fetched\n");

            display_text_part();
            ui_text();
            flags |= IO_ASYNCH_REPLY;
          } break;

          case INS_GET_PUBLIC_KEY: {

            cx_ecfp_public_key_t publicKey;
            cx_curve_t           curve_id = curve_code_to_curve(G_io_apdu_buffer[3]);

            int           path_length = G_io_apdu_buffer[4];
            unsigned char bip32_path[path_length];

            memcpy(bip32_path, G_io_apdu_buffer + 5, path_length);

            publicKey = public_key_at_given_path(curve_id, bip32_path, path_length);

            os_memmove(G_io_apdu_buffer, publicKey.W, 65);
            tx = 65;
            THROW(EXC_NO_ERROR);
          } break;

          case 0xFF: // return to dashboard
            goto return_to_dashboard;

          default:
            THROW(EXC_INVALID_INS);
            break;
        }
      }
      CATCH_OTHER(e) {
        switch (e & 0xF000) {
          case 0x6000:
          case 0x9000:
            sw = e;
            break;
          default:
            sw = 0x6800 | (e & 0x7FF);
            break;
        }
        // Unexpected exception => report
        G_io_apdu_buffer[tx]     = sw >> 8;
        G_io_apdu_buffer[tx + 1] = sw;
        tx += 2;
      }
      FINALLY {
      }
    }
    END_TRY;
  }

return_to_dashboard:
  return;
}
