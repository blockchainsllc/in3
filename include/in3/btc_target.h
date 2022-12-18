#ifndef _BTC_TARGET_H
#define _BTC_TARGET_H

#include "plugin.h"
#include "bytes.h"
#include "data.h"
#include "error.h"
#include <stdint.h>

#define PRE_BIP34_DISTANCE 200
#define BIP34_START        227835

/**
 * calculates the dap (Difficulty Adjustment Period) from the blocknumber
 */
static inline uint32_t btc_get_dap(uint32_t block_number) {
  return block_number / 2016;
}

/**
 * the security- configuration which is stored within the chain_t - object.
 */
typedef struct btc_target_conf {
  bytes_t       data;
  uint_fast16_t max_daps;
  uint_fast16_t max_diff;
  uint_fast16_t dap_limit;
  chain_id_t    chain_id;
  bool          is_testnet;
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

/**
 *  sets a target in the cache
 */
void btc_set_target(btc_target_conf_t* tc, in3_vctx_t* vc, uint32_t dap, uint8_t* difficulty);

uint32_t btc_get_closest_target(btc_target_conf_t* tc, uint32_t dap, uint8_t* difficulty);

in3_ret_t btc_check_conf(in3_req_t* req, btc_target_conf_t* conf);

in3_ret_t btc_check_target(btc_target_conf_t* tc, in3_vctx_t* vc, uint32_t block_number, bytes32_t block_target, bytes_t final, bytes_t header);

#endif
