/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 *
 * Copyright (C) 2019 Blockchains, LLC
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
 * 
 * If you cannot meet the requirements of AGPL, 
 * you should contact us to inquire about a commercial license.
 *******************************************************************************/

// @PUBLIC_HEADER
/** @file
 * defines the return-values of a function call.
 * */
#ifndef IN3_ERROR_H
#define IN3_ERROR_H

/** ERROR types  used as return values.
 * 
 * All values (except IN3_OK) indicate an error. 
 * 
*/
typedef enum {
  /* On success positive values (impl. defined) upto INT_MAX maybe returned */
  IN3_OK       = 0,   /**< Success */
  IN3_EUNKNOWN = -1,  /**< Unknown error - usually accompanied with specific error msg */
  IN3_ENOMEM   = -2,  /**< No memory */
  IN3_ENOTSUP  = -3,  /**< Not supported */
  IN3_EINVAL   = -4,  /**< Invalid value */
  IN3_EFIND    = -5,  /**< Not found */
  IN3_ECONFIG  = -6,  /**< Invalid config */
  IN3_ELIMIT   = -7,  /**< Limit reached */
  IN3_EVERS    = -8,  /**< Version mismatch */
  IN3_EINVALDT = -9,  /**< Data invalid, eg. invalid/incomplete JSON */
  IN3_EPASS    = -10, /**< Wrong password */
  IN3_ERPC     = -11, /**< RPC error (i.e. in3_ctx_t::error set) */
  IN3_ERPCNRES = -12, /**< RPC no response */
  IN3_EUSNURL  = -13, /**< USN URL parse error */
  IN3_ETRANS   = -14, /**< Transport error */
} in3_ret_t;

#endif //IN3_ERROR_H
