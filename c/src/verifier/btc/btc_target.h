#ifndef _BTC_TARGET_H
#define _BTC_TARGET_H

#include "../../core/client/verifier.h"
#include "../../core/util/bytes.h"
#include "../../core/util/data.h"
#include "../../core/util/error.h"
#include <stdint.h>

static inline uint32_t btc_get_dap(uint32_t block_number) {
  return block_number / 2016;
}

typedef struct btc_target_conf {
  bytes_t  data;
  uint32_t max_daps;
  uint32_t max_diff;
} btc_target_conf_t;

/**
 * @brief  checks if the target is within a range
 * @note   
 * @param  vc: 
 * @param  old_target: 
 * @param  new_target: 
 * @retval 
 */
in3_ret_t btc_new_target_check(in3_vctx_t* vc, bytes32_t old_target, bytes32_t new_target);

void btc_set_target(in3_vctx_t* vc, uint32_t dap, uint8_t* difficulty);

uint32_t btc_get_closest_target(in3_vctx_t* vc, uint32_t dap, uint8_t* difficulty);

in3_ret_t btc_vc_set_config(in3_t* c, d_token_t* conf, in3_chain_t* chain);

btc_target_conf_t* btc_get_conf(in3_t* c, in3_chain_t* chain);

void btc_vc_free(in3_t* c, in3_chain_t* chain);

btc_target_conf_t* btc_get_config(in3_vctx_t* vc);

in3_ret_t btc_check_target(in3_vctx_t* vc, uint32_t block_number, bytes32_t block_target, bytes_t final);

#endif