#pragma once

#include "cx.h"
#include "error_codes.h"
#include "exception.h"
#include "os.h"
#include "os_io_seproxyhal.h"
#include <stdbool.h>
#include <string.h>
#include <types.h>

// data stored in non volatile flash memory,prefix N_ is mandatory to store in flash
extern WIDE nvram_data N_data_real;

extern nvram_data new_data;

#define N_data (*(WIDE nvram_data*) PIC(&N_data_real))

extern unsigned int          current_text_pos; // parsing cursor in the text to display
extern unsigned int          text_y;           // current location of the displayed text
extern cx_sha256_t           hash;
extern unsigned char         msg_hash[HASH_LEN];
extern cx_ecfp_private_key_t private_key;

//bip32 path stored after apdu request parsing
extern unsigned int path[BIP32_PATH_LEN_MAX];
extern int          path_len_bip;

// ui currently displayed
enum UI_STATE { UI_IDLE,
                UI_TEXT,
                UI_APPROVAL };

extern enum UI_STATE uiState;

extern char lineBuffer[100];
