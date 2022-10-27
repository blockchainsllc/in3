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
#include "../include/cryptocell.h"
#include <hw_unique_key.h>
#include <identity_key.h>
#include <nrf_cc3xx_platform.h>
#include <nrf_cc3xx_platform_ctr_drbg.h>
#include <nrf_cc3xx_platform_identity_key.h>
#include <nrfx.h>
#include <nrfx_nvmc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>

/** initialize the psa crypto library */
int crypto_init(void) {
  psa_status_t status;
  /* Initialize PSA Crypto */
  status = psa_crypto_init();
  if (status != PSA_SUCCESS) {
    printk("psa crypto init failed! (Error: %d)", status);
    return status;
  }
  return status;
}

/** de-initialize the psa crypto library */
int crypto_deinit(psa_key_handle_t key_handle) {
  psa_status_t status;
  /* Destroy the key handle */
  status = psa_destroy_key(key_handle);
  if (status != PSA_SUCCESS) {
    printk("psa_destroy_key failed! (Error: %d)", status);
    return status;
  }
  return status;
}

/** print the hex values */
void hex_dump(uint8_t* buff, uint32_t len) {
  for (int i = 0; i < len; i++) {
    printk("%s%02x", i && ((i % 32) == 0) ? "\n" : "", buff[i]);
  }
  printk("\n");
}

/** nRF platform initialization */
int platform_init(void) {
  int status = SUCCESS;
#if !defined(CONFIG_BUILD_WITH_TFM)
  status = nrf_cc3xx_platform_init();

  if (status != NRF_CC3XX_PLATFORM_SUCCESS) {
    printk("nrf_cc3xx_platform_init returned error: %d\n", status);
    return ERROR;
  }
#endif /* !defined(CONFIG_BUILD_WITH_TFM) */
  return status;
}

/** generate random initialization vector */
psa_status_t generate_random_vector(void) {
  psa_status_t status;
  uint8_t      iv[IV_LEN];
  printk("*** Generating random IV ***\n");
  status = psa_generate_random(iv, IV_LEN);
  if (status != PSA_SUCCESS) {
    printk("psa_generate_random returned error: %d\n", status);
    return status;
  }
  printk("IV:\n");
  hex_dump(iv, IV_LEN);
  printk("\n");
  return status;
}

/** generate and write random hardware unique keys including MKEK to KMU */
int write_random_key_kmu(void) {
  int status;

#if !defined(CONFIG_BUILD_WITH_TFM)
  if (!hw_unique_key_are_any_written()) {
    printk("Writing random keys to KMU\n");
    hw_unique_key_write_random();
    printk("Success!\n");

#if !defined(HUK_HAS_KMU)
    /* Reboot to allow the bootloader to load the key into CryptoCell. */
    sys_reboot(0);
#endif /* !defined(HUK_HAS_KMU) */
    status = SUCCESS;
  }
#endif /* !defined(CONFIG_BUILD_WITH_TFM) */
  return status;
}

/** Set attributes for the storage key */
static psa_key_attributes_t set_psa_attr(key_type_t key_type) {
  /* Configure the key attributes */
  psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
  switch (key_type) {
    case KEY_TYPE_SYM: {
      /* Configure the key attributes for Symmetric key type */
      psa_set_key_usage_flags(&key_attributes,
                              (PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT));
      psa_set_key_algorithm(&key_attributes, PSA_ALG_GCM);
      psa_set_key_type(&key_attributes, PSA_KEY_TYPE_AES);
      psa_set_key_bits(&key_attributes, PSA_BYTES_TO_BITS(HUK_SIZE_BYTES));
    } break;
    case KEY_TYPE_RSA:
      break;
    case KEY_TYPE_ECDSA: {
      /* Configure the key attributes for Curve type secp256r1
       * This key needs to be exported from the volatile storage
       */
      psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_VERIFY_HASH | PSA_KEY_USAGE_EXPORT);
      psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
      psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
      psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_K1));
      psa_set_key_bits(&key_attributes, IDENTITY_KEY_SIZE_BYTES * 8);
    } break;
    case KEY_TYPE_ECDH:
      break;
    default:
      break;
  }
  return key_attributes;
}

/** derive an encryption key out of MKEK HUK */
int derive_ek_sym_aes_enc_dec(uint8_t* key_label, uint32_t label_size, uint8_t* key_out) {
  int          res = SUCCESS;
  psa_status_t status;

  // initialize psa cryptographic library
  status = crypto_init();
  if (status != PSA_SUCCESS) {
    printk("psa crypto initialization failed\n");
    res = ERROR;
    return res;
  }

  // Set PSA attributes for key generation
  psa_key_attributes_t key_attributes = set_psa_attr(KEY_TYPE_SYM);
  psa_key_id_t         key_id_out     = 0;
  static uint8_t       ek_out[HUK_SIZE_BYTES];
  // derive an encryption key using the random HUK available in the MKEK slot
  int result = hw_unique_key_derive_key(HUK_KEYSLOT, NULL, 0,
                                        key_label, label_size, ek_out, sizeof(ek_out));
  if (result != 0) {
    printk("hw_unique_key_derive_key returned error: %d\n", result);
    res = ERROR;
    return res;
  }

  /** Importing the key into PSA crypto. */
  status = psa_import_key(&key_attributes, ek_out, sizeof(ek_out), &key_id_out);
  if (status != PSA_SUCCESS) {
    printk("psa_import_key returned error: %d\n", status);
    res = ERROR;
    return res;
  }
  printk("Encryption Key:\n");
  hex_dump(ek_out, sizeof(ek_out));
  printk("Key handle:\n");
  printk("0x%x\n", key_id_out);
  return res;
}

#if defined(NRF5340_XXAA_APPLICATION)
/** write derived key to KMU slot */
int write_derived_key_kmu(uint32_t slot, nrf_cc3xx_platform_key_type_t type, nrf_cc3xx_platform_key_buff_t buff) {
  int status;
  status = nrf_cc3xx_platform_kmu_write_key(slot, type, buff);
  if (status != NRF_CC3XX_PLATFORM_SUCCESS) {
    printk("key write to kmu slot returned with error: %d\n", status);
  }
  return status;
}
#endif

/** write the generated identity key to KMU slot */
int store_pk_identity_keyslot_kmu(uint32_t slot, uint8_t* key) {
  int status = ERROR;
  /* MKEK is required to encrypt the key before storing it in the KMU slot*/
  if (!hw_unique_key_is_written(HUK_KEYSLOT_MKEK)) {
    printk("Could not find the MKEK!\n");
    return -ERR_HUK_MISSING;
  }
  if (slot < NRF_KMU_LAST_IDENTTY_KEY_SLOT && slot >= NRF_KMU_SLOT_KIDENT) {
    if (!nrf_cc3xx_platform_identity_key_is_stored(slot)) {
      // TODO: Need to handle the recovery key check before store any new key on the recovery key slot(uses last KMU slot)
      status = nrf_cc3xx_platform_identity_key_store(slot, key);
      if (status == NRF_CC3XX_PLATFORM_SUCCESS) {
        printk("Identity key stored: kmu slot %d\n", slot);
        status = SUCCESS;
      }
      return status;
    }
  }
  return status;
}

/** retrieve the identity key to KMU slot */
int load_pk_identity_keyslot_kmu(uint32_t slot, uint8_t* key) {
  int status = ERROR;
  /* MKEK is required to retrieve the key by decrypting it before reading from the KMU slot*/
  if (!hw_unique_key_is_written(HUK_KEYSLOT_MKEK)) {
    printk("Could not find the MKEK!\n");
    return -ERR_HUK_MISSING;
  }
  if (slot < NRF_KMU_LAST_IDENTTY_KEY_SLOT && slot >= NRF_KMU_SLOT_KIDENT) {
    /* check if any identity key is already stored in the given KMU slot */
    if (nrf_cc3xx_platform_identity_key_is_stored(slot)) {
      /* retreive the identity key from the key slot reserved for identity keys */
      status = nrf_cc3xx_platform_identity_key_retrieve(slot, key);
      if (status == NRF_CC3XX_PLATFORM_SUCCESS) {
        printk("Key loaded: kmu slot %d \n", slot);
        status = SUCCESS;
      }
      printk("Retreived Private Key:\n");
      hex_dump(key, IDENTITY_KEY_SIZE_BYTES);
      return status;
    }
  }
  return status;
}

/** destroy the key from the local memory for security purpose */
int destroy_key(uint8_t* key) {
  if (key != NULL) {
    nrf_cc3xx_platform_identity_key_free(key);
    return SUCCESS;
  }
  return ERROR;
}

/** destroy key handle */
int destroy_key_handle(psa_key_handle_t key_handle) {
  psa_status_t status = psa_destroy_key(key_handle);
  if (status != PSA_SUCCESS) {
    printk("psa_destroy_key failed! Error: %d\n", status);
    return ERROR;
  }
  return SUCCESS;
}

/** generate random private key */
int generate_pk_ecdsa_sha256(uint8_t* key_buf) {
  int          res = SUCCESS;
  psa_status_t status;
  size_t       key_len;

  // initialize psa cryptographic library
  status = crypto_init();
  if (status != PSA_SUCCESS) {
    printk("psa crypto initialization failed\n");
    return ERROR;
  }

  // Set PSA attributes for key generation for the given key type
  psa_key_attributes_t key_attributes = set_psa_attr(KEY_TYPE_ECDSA);
  psa_key_handle_t     key_handle;
  status = psa_generate_key(&key_attributes, &key_handle);
  if (status != PSA_SUCCESS) {
    printk("psa_generate_key failed! Error: %d\n", status);
    return ERROR;
  }

  status = psa_export_key(key_handle, key_buf, IDENTITY_KEY_SIZE_BYTES, &key_len);
  if (status != PSA_SUCCESS) {
    printk("psa_export_key failed! Error: %d\n", status);
    return ERROR;
  }
  printk("Generated Private Key:\n");
  hex_dump(key_buf, IDENTITY_KEY_SIZE_BYTES);
  return res;
}

/** set the cryptocell signer configuration informations */
int register_cryptocell_cbk(cryptocell_cbks_t* cbks) {
  if (cbks) {
    cbks->gen_pk_func = generate_pk_ecdsa_sha256;
    cbks->str_pk_func = store_pk_identity_keyslot_kmu;
    cbks->ld_pk_func  = load_pk_identity_keyslot_kmu;
    cbks->des_pk_func = destroy_key;
    return SUCCESS;
  }
  return ERROR;
}
