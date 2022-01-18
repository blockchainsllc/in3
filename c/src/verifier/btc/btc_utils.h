#ifndef _BTC_UTILS_H
#define _BTC_UTILS_H

#include "../../core/util/bytes.h"
#include "../../core/util/error.h"
#include <stdint.h>

in3_ret_t rev_memcpy(uint8_t* dst, uint8_t* src, uint32_t len);
in3_ret_t append_bytes(bytes_t* dst, const bytes_t* src);

#endif