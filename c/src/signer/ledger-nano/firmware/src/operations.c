#include "operations.h"

#include "apdu.h"
#include "menu.h"

cx_sha256_t           hash;
unsigned int          path[BIP32_PATH_LEN_MAX];
int                   path_len_bip;
unsigned char         msg_hash[HASH_LEN];
cx_ecfp_private_key_t private_key;

//static const unsigned int path__[5]= {44|0x80000000, 60|0x80000000, 0|0x80000000, 0, 0};

//function to get public key at the given path
cx_ecfp_public_key_t public_key_at_given_path(cx_curve_t curve_id, unsigned char* loc, int path_len) {
  PRINTF("public_key_at_given_path:enter");
  cx_ecfp_public_key_t  public_key;
  cx_ecfp_private_key_t private_key;
  unsigned char         private_key_data[32];
  path_len_bip = read_bip32_path(path_len, loc, path);

  os_perso_derive_node_bip32(CX_CURVE_256K1, path, path_len_bip, private_key_data, NULL);

  cx_ecdsa_init_private_key(CX_CURVE_256K1, private_key_data, 32, &private_key);

  cx_ecfp_generate_pair(CX_CURVE_256K1, &public_key, &private_key, 1);

  PRINTF("public_key_at_given_path:exit");
  return public_key;
}

//function to get private key at the given path
cx_ecfp_private_key_t private_key_at_given_path(cx_curve_t curve_id, unsigned char* loc, int path_len) {
  cx_ecfp_public_key_t public_key;
  unsigned char        private_key_data[32];
  path_len_bip = read_bip32_path(path_len, loc, path);
  os_perso_derive_node_bip32(CX_CURVE_256K1, path, path_len_bip, private_key_data, NULL);
  cx_ecdsa_init_private_key(CX_CURVE_256K1, private_key_data, 32, &private_key);
  cx_ecfp_generate_pair(CX_CURVE_256K1, &public_key, &private_key, 1);
  return private_key;
}

const bagl_element_t* io_seproxyhal_touch_exit(const bagl_element_t* e) {
  // Go back to the dashboard
  os_sched_exit(0);
  return NULL; // do not redraw the widget
}

const bagl_element_t*
io_seproxyhal_touch_approve(const bagl_element_t* e) {
  unsigned int tx       = 0;
  cx_curve_t   curve_id = curve_code_to_curve(G_io_apdu_buffer[3]);

  if (G_io_apdu_buffer[2] == P1_LAST) {

    unsigned char result[32];
    os_memmove(result, msg_hash, HASH_LEN);
    PRINTF("msg hash signed %02x %02x %02x %02x\n", result[0], result[1], result[30], result[31]);
    tx = cx_ecdsa_sign((void*) &private_key, CX_RND_RFC6979 | CX_LAST, CX_KECCAK, result,
                       sizeof(result), G_io_apdu_buffer, NULL);

    G_io_apdu_buffer[0] &= 0xF0; // discard the parity information
  }
  G_io_apdu_buffer[tx++] = 0x90;
  G_io_apdu_buffer[tx++] = 0x00;
  // Send back the response, do not restart the event loop
  io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
  // Display back the original UX
  ui_idle();
  return 0; // do not redraw the widget
}

const bagl_element_t* io_seproxyhal_touch_deny(const bagl_element_t* e) {

  G_io_apdu_buffer[0] = 0x69;
  G_io_apdu_buffer[1] = 0x85;
  // Send back the response, do not restart the event loop
  io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
  // Display back the original UX
  ui_idle();
  return 0; // do not redraw the widget
}

unsigned short io_exchange_al(unsigned char channel, unsigned short tx_len) {
  switch (channel & ~(IO_FLAGS)) {
    case CHANNEL_KEYBOARD:
      break;

    // multiplexed io exchange over a SPI channel and TLV encapsulated protocol
    case CHANNEL_SPI:
      if (tx_len) {
        io_seproxyhal_spi_send(G_io_apdu_buffer, tx_len);

        if (channel & IO_RESET_AFTER_REPLIED) {
          reset();
        }
        return 0; // nothing received from the master so far (it's a tx
                  // transaction)
      }
      else {
        return io_seproxyhal_spi_recv(G_io_apdu_buffer, sizeof(G_io_apdu_buffer), 0);
      }

    default:
      THROW(EXC_WRONG_PARAM);
  }
  return 0;
}

void io_seproxyhal_display(const bagl_element_t* element) {
  io_seproxyhal_display_default((bagl_element_t*) element);
}

uint8_t curve_to_curve_code(cx_curve_t curve) {
  switch (curve) {
    case CX_CURVE_Ed25519:
      return IDM_ED;
    case CX_CURVE_SECP256K1:
      return IDM_SECP256K1;
    case CX_CURVE_SECP256R1:
      return IDM_SECP256R1;
    default:
      THROW(EXC_MEMORY_ERROR);
  }
}

cx_curve_t curve_code_to_curve(uint8_t curve_code) {
  static const cx_curve_t curves[] = {CX_CURVE_Ed25519, CX_CURVE_SECP256K1, CX_CURVE_SECP256R1};
  if (curve_code > sizeof(curves) / sizeof(*curves)) {
    THROW(EXC_WRONG_PARAM);
  }
  return curves[curve_code];
}

int generate_hash(unsigned char* data, int data_len, unsigned char* out_hash) {
  PRINTF("generate_hash:enter");
  PRINTF("generate_hash: data len %d %d %d\n", data_len, data[0], data[data_len - 1]);
  PRINTF("generate_hash: hash  %u  hash.header %u out_hash %u\n", hash, hash.header, out_hash);
  cx_sha256_init(&hash);
  cx_hash(&hash.header, 0, data, data_len, NULL);
  int size = cx_hash(&hash.header, CX_LAST, data, 0, out_hash);
  PRINTF("\ngenerate_hash: out_hash %d %d \n", out_hash[0], out_hash[HASH_LEN - 1]);
  PRINTF("generate_hash:exit");
  return size;
}

uint32_t read_bip32_path(uint32_t bytes, const uint8_t* buf, uint32_t* bip32_path) {
  uint32_t path_length = bytes;
  PRINTF(" bytes %d  path_length %d ", bytes, path_length);

  for (size_t i = 0; i < path_length; i++) {
    //PRINTF("\n buf %d  bip32_path %d path__ %d i %d",buf[i],bip32_path[i],path__[i], i);
    if (i < 3)
      bip32_path[i] = buf[i] | (0x80000000);
    else
      bip32_path[i] = buf[i] | (0x00000000);

    //PRINTF("\n buf %d  bip32_path %d path__ %d i %d \n",buf[i],bip32_path[i],path__[i], i);
  }

  return path_length;
}

void tohex(unsigned char* in, size_t insz, char* out, size_t outsz) {
  unsigned char* pin  = in;
  const char*    hex  = "0123456789ABCDEF";
  char*          pout = out;
  for (; pin < in + insz; pout += 3, pin++) {
    pout[0] = hex[(*pin >> 4) & 0xF];
    pout[1] = hex[*pin & 0xF];

    if (pout + 2 - out > outsz) {
      /* Better to truncate output string than overflow buffer */
      /* it would be still better to either return a status */
      /* or ensure the target buffer is large enough and it never happen */
      break;
    }
  }
}