/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/blockchainsllc/in3
 *
 * Copyright (C) 2018-2020 slock.it GmbH, Blockchains LLC
 *
 *
 * COMMERCIAL LICENSE USAGE
 *
 * Licensees holding a valid commercial license may use this file in accordance
 * with the commercial license agreement provided with the Software or, alternatively,
 * in accordance with the terms contained in a written agreement between you and
 * slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further
 * information please contact slock.it at in3@slock.it.
 *
 * Alternatively, this file may be used under the AGPL license as follows:
 *
 * AGPL LICENSE USAGE
 *
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 * [Permissions of this strong copyleft license are conditioned on making available
 * complete source code of licensed works and modifications, which include larger
 * works using a licensed work, under the same license. Copyright and license notices
 * must be preserved. Contributors provide an express grant of patent rights.]
 * You should have received a copy of the GNU Affero General Public License along
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/
#ifndef CRYPTOCELL_HANDLER_H
#define CRYPTOCELL_HANDLER_H
#ifdef __cplusplus
extern "C" {
#endif
#include <cryptocell_signer.h>
#include <hw_unique_key.h>
#include <nrf_cc3xx_platform_defines.h>
#include <nrf_cc3xx_platform_kmu.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/sys/printk.h>

#define SUCCESS (0)
#define ERROR   (-1)
#define IV_LEN  12
#define MAC_LEN 16

#define HUK_KEYSLOT                       HUK_KEYSLOT_MKEK
#define PK_RECOVERY_KEY_SLOT              NRF_KMU_LAST_IDENTTY_KEY_SLOT
#define CRYPTOCELL_SIGNER_PK_SLOT         (NRF_KMU_SLOT_KIDENT)
#define CRYPTOCELL_SIGNER_PUBLIC_KEY_SIZE (65)

#define HUK_KEY_SIZE      32
#define IDENTITY_KEY_SIZE 32

/**
 * Attributes setting information for psa crypto operations
 *
 */
typedef struct
{
  psa_algorithm_t    alg;
  psa_key_usage_t    usage_flag;
  psa_key_type_t     alg_type;
  psa_key_lifetime_t lifetime;
  size_t             bits;
} attr_info_t;

typedef enum {
  KEY_TYPE_SYM,
  KEY_TYPE_RSA,
  KEY_TYPE_ECDSA,
  KEY_TYPE_ECDH
} key_type_t;

/**
 *
 *
 */
typedef struct
{
  uint32_t     kmu_slot;
  uint8_t      key_buf[32];
  attr_info_t* psa_attr;
} cryptocell_key_handle_t;

/**
 * @brief
 *
 * @return int
 */
int crypto_init(void);

/**
 * @brief
 *
 * @param key_handle
 * @return int
 */
int crypto_deinit(psa_key_id_t key_handle);

/**
 * @brief
 *
 * @return int
 */
int platform_init(void);

/**
 * @brief
 *
 * @return int
 */
int write_random_key_kmu(void);

/**
 * @brief Set the psa attr object
 *
 * @param attributes
 * @param psa_attr
 */
static psa_key_attributes_t set_psa_attr(key_type_t key_type);

/**
 * @brief
 *
 * @return psa_status_t
 */
psa_status_t
generate_random_vector(void);

/**
 * @brief
 *
 * @param attributes
 * @param key_label
 * @param label_size
 * @return psa_key_id_t
 */
int derive_ek_sym_aes_enc_dec(uint8_t* key_label, uint32_t label_size, uint8_t* key_out);

#if defined(NRF5340_XXAA_APPLICATION)
/**
 * @brief
 *
 * @param slot
 * @param type
 * @param buff
 * @return int
 */
int write_derived_key_kmu(uint32_t slot, nrf_cc3xx_platform_key_type_t type, nrf_cc3xx_platform_key_buff_t buff);
#endif

/**
 * @brief
 *
 * @param slot
 * @param key
 * @return int
 */
int store_pk_identity_keyslot_kmu(uint32_t slot, uint8_t* key);

/**
 * @brief
 *
 * @param slot
 * @param key
 * @return int
 */
int load_pk_identity_keyslot_kmu(uint32_t slot, uint8_t* key);

/**
 * @brief
 *
 * @param key
 * @return int
 */
int destroy_key(uint8_t* key);

/**
 * @brief
 *
 * @param pk_key_buf
 * @param pub_key_buf
 * @return int
 */
int generate_pk_keypair_ecdsa_sha256(uint8_t* pk_key_buf, uint8_t* pub_key_buf);

/**
 * @brief Set the up cryptocell object
 *
 * @param info
 */
int register_cryptocell_cbk(cryptocell_cbks_t* cbks);

/**
 * @brief
 *
 * @param key_pair_handle
 * @param pub_key
 * @param pub_key_size
 * @return int
 */
static int export_public_key_keypair(psa_key_id_t* key_pair_handle, uint8_t* pub_key, size_t pub_key_size);

/**
 * @brief
 *
 * @param slot
 * @return int
 */
int pk_identity_key_is_stored(uint32_t slot);

#ifdef __cplusplus
}
#endif
#endif // CRYPTOCELL_HANDLER_H