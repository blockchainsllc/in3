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

#ifndef _ETH_API__ABI2_H_
#define _ETH_API__ABI2_H_
#include "../../core/client/plugin.h"
#include "../../core/util/bytes.h"
#include "../../core/util/data.h"

/**
 * the type of data structured use in the signature-
 */
typedef enum {
  ABI_TUPLE       = 1, /**< a series of different data fields or a `struct` in solidity */
  ABI_STRING      = 2, /**< a dynamic string of characters (`string` in solidity) */
  ABI_NUMBER      = 3, /**< a number which could be `uint<SIZE>` or `int<SIZE>`or also `fixed<SIZE>x<N>` */
  ABI_BYTES       = 4, /**< a dynamic array of bytes ( `bytes` in solidity) */
  ABI_ADDRESS     = 5, /**< a 20 byte long address ( `address`)*/
  ABI_FIXED_BYTES = 6, /**< a fixed length array of bytes where the length would be 1-32. like  `bytes32` */
  ABI_BOOL        = 8, /**< a boolean value */
  ABI_ARRAY       = 9  /**< a dynamic or fixed size array of elements */
} abi_coder_type_t;

/**
 * a coder configuration for a specific type.
 * Depending on the type the config is stored in the union-structs.
 */
typedef struct signature {
  abi_coder_type_t type;    /**< the type of the coder */
  char*            name;    /**< the name in case of a named tubles*/
  bool             indexed; /**< marks a tuple as being indexed, which is relevant for event decoding */
  union {
    struct {
      struct signature** components; /**< the pointer to an array of ponters to the types */
      int                len;        /**< the number of componeents in the tuple */
    } tuple;                         /**< the config used for coder of type `ABI_TUPLE`*/

    struct {
      struct signature* component; /**< the pointer to the type of array */
      int               len;       /**< the length of an array */
    } array;                       /**< the config used for coder of type `ABI_ARRAY`*/

    struct {
      bool sign; /**< uint or int */
      int  size; /**< size in bits */
      int  n;    /**< if n is set it is a fixed type ( fixed128x18 )*/
    } number;    /**< the config used for coder of type `ABI_NUMBER`*/

    struct {
      int len; /**< the number of bytes for a fixed bytes-type */
    } fixed;   /**< the config used for coder of type `ABI_FIXED_BYTES`*/

  } data;
} abi_coder_t;

/**
 * defines a ABI-signature of a function or data structure
 */
typedef struct {
  abi_coder_t* input;        /**< the tuple-coder for encoding data */
  abi_coder_t* output;       /**< the tuple-coder for decoding data. If NULL, the `input`is used. This optional field is filled if the signaturestring contains a return-definition. */
  uint8_t      fn_hash[4];   /**< the function-hash, which is used for calling a solidity function. */
  bool         return_tuple; /**< if this is true the return-type will be an array representing the decoded value, even if there is only one type returned, other wise it will return a single value in case of an single return type. The parser will set this based on the bracket around the return type. */
} abi_sig_t;

/**
 * frees a previously creates abi signature.
 */
void abi_sig_free(
    abi_sig_t* c /**< the signature */
);

/**
 * parses a ABI signature string and creates the a pointer to the struct, which can be used to encode and decode date
 *
 * The signature string may contain either just a comma separated list of valid solidity types:
 *
 * ```c
 * "address"           // single type
 * "address,uint,bool" // tuple
 * "(uint,int,bool)"   // tuple
 * "bytes32[]"         // dynamic array
 * "uint[4]"           // fixed array
 * "(uint,bool)[]"     // dynamic array of a tuple
 * ```
 *
 * Optionaly a function name can be prefixed, which will be used when creating the functionhash:
 * ```c
 * "execTransaction(bytes,uint,uint)"
 * ```
 *
 * Optionally a return type can be added which is used when decoding the value:
 *
 * ```c
 * "isValidSAignature(bytes,bytes):(bytes4)" // return the single type as array with one value
 * "isValidSAignature(bytes,bytes):bytes4"   // return the single type as single value
 * ```
 *
 */
abi_sig_t* abi_sig_create(
    char*  signature, /**< the abi signature */
    char** error      /**< the a pointer to error, which will hold the error message in case of an error. This does not need to be freed, since those messages are constant strings. */
);

/**
 * internal function to check if the coder is handled dynamicly
 */
bool abi_is_dynamic(
    abi_coder_t* coder /**< the coder to check */
);

/**
 * encodes JSON-data to bytes.
 * The resulting bytes data-field MUST be freed if not NULL!
 */
bytes_t abi_encode(
    abi_sig_t* s,    /**< the signature to use */
    d_token_t* src,  /**< the data as json-token, which can be a single value or a array. All data may not contain any objects, but arrays instead. */
    char**     error /**< the a pointer to error, which will hold the error message in case of an error. This does not need to be freed, since those messages are constant strings. */
);

/**
 * decodes bytes to a JSON-structure.
 * The resulting json_ctx MUST be freed using `json_free` if not NULL.
 */
json_ctx_t* abi_decode(
    abi_sig_t* s,    /**< the signature to use */
    bytes_t    data, /**< the data to decode */
    char**     error /**< the a pointer to error, which will hold the error message in case of an error. This does not need to be freed, since those messages are constant strings. */

);

/**
 * decodes bytes to a JSON-structure.
 * The resulting json_ctx MUST be freed using `json_free` if not NULL.
 */
json_ctx_t* abi_decode_event(
    abi_sig_t* s,      /**< the signature to use */
    bytes_t    topics, /**< the topics to decode */
    bytes_t    data,   /**< the data to decode */
    char**     error   /**< the a pointer to error, which will hold the error message in case of an error. This does not need to be freed, since those messages are constant strings. */

);

/**
 * calls a smart contract function.
 * if successful the result will contain a json-ctx with the decoded result.
 * You must free the result with json_free() after usage!
 */
in3_ret_t abi_call(in3_rpc_handle_ctx_t* ctx, json_ctx_t** result, address_t to, char* sig, ...);

/**
 * generates the bytes to encode the arguments
 */
bytes_t abi_encode_args(in3_rpc_handle_ctx_t* ctx, char* sig, ...);

/**
 * sends a ethcall and encodes the result as json
 */
#define SEND_ETH_CALL(ctx, to, sig, ...)                                                   \
  {                                                                                        \
    json_ctx_t* result = NULL;                                                             \
    TRY_CATCH(abi_call(ctx, &result, to, sig, __VA_ARGS__), if (result) json_free(result)) \
    char* json = d_create_json(result, result->result);                                    \
    json_free(result);                                                                     \
    in3_rpc_handle_with_string(ctx, json);                                                 \
    _free(json);                                                                           \
    return IN3_OK;                                                                         \
  }
/**
 * sends a ethcall and encodes the result as json
 */
#define SEND_ETH_CALL_NO_ARGS(ctx, to, sig)                                   \
  {                                                                           \
    json_ctx_t* result = NULL;                                                \
    TRY_CATCH(abi_call(ctx, &result, to, sig), if (result) json_free(result)) \
    char* json = d_create_json(result, result->result);                       \
    json_free(result);                                                        \
    in3_rpc_handle_with_string(ctx, json);                                    \
    _free(json);                                                              \
    return IN3_OK;                                                            \
  }

#endif // _ETH_API__ABI_H_
