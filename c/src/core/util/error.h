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

// @PUBLIC_HEADER
/** @file
 * defines the return-values of a function call.
 * */
#ifndef IN3_ERROR_H
#define IN3_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif
/** depreacted-attribute */
#define DEPRECATED __attribute__((deprecated))

/** JSON-RPC errors */
#define JSON_RPC_ERR_INTERNAL (-32603) /** Internal error (probably offline signer node) */
#define JSON_RPC_ERR_FINALITY (-16001) /** Block is not final so node refused to sign */

/** ERROR types  used as return values.
 * 
 * All values (except IN3_OK) indicate an error.
 * IN3_WAITING may be treated like an error, since we have stop executing until the response has arrived, but it is a valid return value. 
 * 
*/
typedef enum {
  /* On success positive values (impl. defined) upto INT_MAX maybe returned */
  IN3_OK                = 0,   /**< Success */
  IN3_EUNKNOWN          = -1,  /**< Unknown error - usually accompanied with specific error msg */
  IN3_ENOMEM            = -2,  /**< No memory */
  IN3_ENOTSUP           = -3,  /**< Not supported */
  IN3_EINVAL            = -4,  /**< Invalid value */
  IN3_EFIND             = -5,  /**< Not found */
  IN3_ECONFIG           = -6,  /**< Invalid config */
  IN3_ELIMIT            = -7,  /**< Limit reached */
  IN3_EVERS             = -8,  /**< Version mismatch */
  IN3_EINVALDT          = -9,  /**< Data invalid, eg. invalid/incomplete JSON */
  IN3_EPASS             = -10, /**< Wrong password */
  IN3_ERPC              = -11, /**< RPC error (i.e. in3_req_t::error set) */
  IN3_ERPCNRES          = -12, /**< RPC no response */
  IN3_EUSNURL           = -13, /**< USN URL parse error */
  IN3_ETRANS            = -14, /**< Transport error */
  IN3_ERANGE            = -15, /**< Not in range */
  IN3_WAITING           = -16, /**< the process can not be finished since we are waiting for responses */
  IN3_EIGNORE           = -17, /**< Ignorable error */
  IN3_EPAYMENT_REQUIRED = -18, /**< payment required */
  IN3_ENODEVICE         = -19, /**< harware wallet device not connected */
  IN3_EAPDU             = -20, /**< error in hardware wallet communication  */
  IN3_EPLGN_NONE        = -21, /**< no plugin could handle specified action */

  IN3_HTTP_BAD_REQUEST        = -400, /**< Bad Request */
  IN3_HTTP_UNAUTHORIZED       = -401, /**< Unauthorized */
  IN3_HTTP_PAYMENT_REQUIRED   = -402, /**< Unauthorized */
  IN3_HTTP_FORBIDDEN          = -403, /**< Forbidden */
  IN3_HTTP_NOT_FOUND          = -404, /**< not found */
  IN3_HTTP_M_NOT_ALLOWED      = -405, /**< method not allowed */
  IN3_HTTP_NOT_ACCEPTABLE     = -406, /**< Not acceptable */
  IN3_HTTP_PROX_AUTH_REQUIRED = -407, /**< Proxy Authentification required */
  IN3_HTTP_TIMEOUT            = -408, /**< Request timeout */
  IN3_HTTP_CONFLICT           = -409, /**< conflict */
  IN3_HTTP_GONE               = -410, /**< gone */
  IN3_HTTP_INTERNAL_ERROR     = -500, /**< Internal Server Error */
  IN3_HTTP_NOT_IMPLEMENTED    = -501, /**< not implemented */
  IN3_HTTP_BAD_GATEWAY        = -502, /**< Bad Gateway */
  IN3_HTTP_UNAVAILABLE        = -503  /**< service unavailable */

} in3_ret_t;

/** Optional type similar to C++ std::optional
 * Optional types must be defined prior to usage (e.g. DEFINE_OPTIONAL_T(int))
 * Use OPTIONAL_T_UNDEFINED(t) & OPTIONAL_T_VALUE(t, v) for easy initialization (rvalues)
 * Note: Defining optional types for pointers is ill-formed by definition. This is
 * because redundant
 */
#define OPTIONAL_T(t) opt_##t

/** 
 * Optional types must be defined prior to usage (e.g. DEFINE_OPTIONAL_T(int))
 * Use OPTIONAL_T_UNDEFINED(t) & OPTIONAL_T_VALUE(t, v) for easy initialization (rvalues)
 */
#define DEFINE_OPTIONAL_T(t) \
  typedef struct {           \
    t    value;              \
    bool defined;            \
  } OPTIONAL_T(t)

/** 
 * marks a used value as undefined.
 */
#define OPTIONAL_T_UNDEFINED(t) ((OPTIONAL_T(t)){.defined = false})

/** 
 * sets the value of an optional type.
 */
#define OPTIONAL_T_VALUE(t, v) ((OPTIONAL_T(t)){.value = v, .defined = true})

/**
 * converts a error code into a string.
 * These strings are constants and do not need to be freed.
 */
char* in3_errmsg(in3_ret_t err /**< the error code */);
#ifdef __cplusplus
}
#endif

#endif //IN3_ERROR_H
