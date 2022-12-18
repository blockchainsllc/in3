#pragma once
#include "globals.h"

cx_ecfp_private_key_t private_key_at_given_path(cx_curve_t curve_id, unsigned char* loc, int path_len);

cx_ecfp_public_key_t public_key_at_given_path(cx_curve_t curve_id, unsigned char* loc, int path_len);

const bagl_element_t* io_seproxyhal_touch_exit(const bagl_element_t* e);

const bagl_element_t* io_seproxyhal_touch_approve(const bagl_element_t* e);

const bagl_element_t* io_seproxyhal_touch_deny(const bagl_element_t* e);

unsigned short io_exchange_al(unsigned char channel, unsigned short tx_len);

void io_seproxyhal_display(const bagl_element_t* element);

uint8_t curve_to_curve_code(cx_curve_t curve);

cx_curve_t curve_code_to_curve(uint8_t curve_code);

int generate_hash(unsigned char* data, int data_len, unsigned char* out_hash);

uint32_t read_bip32_path(uint32_t bytes, const uint8_t* buf, uint32_t* bip32_path);

void tohex(unsigned char* in, size_t insz, char* out, size_t outsz);
