#pragma once

#define CX_CURVE_256K1     CX_CURVE_SECP256K1
#define HASH_LEN           0x20
#define BIP32_PATH_LEN_MAX 0x14
#define MAX_CHARS_PER_LINE 49
#define DEFAULT_FONT       BAGL_FONT_OPEN_SANS_LIGHT_16px | BAGL_FONT_ALIGNMENT_LEFT
#define TEXT_HEIGHT        15
#define TEXT_SPACE         4

typedef unsigned char uchar_t;

typedef enum MODULES {
  SIGN           = 1,
  GET_PUBLIC_KEY = 2,
} MODULES;

typedef enum {
  IDM_ED        = 0,
  IDM_SECP256K1 = 1,
  IDM_SECP256R1 = 2,
  IDM_NO_CURVE  = 255,
} curve_code;

typedef struct {
  cx_curve_t curve;
  uint8_t    path_length;
  uint32_t   bip32_path[BIP32_PATH_LEN_MAX];
} nvram_data;
