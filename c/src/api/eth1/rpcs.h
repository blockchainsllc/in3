/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 *
 * Copyright (C) 2018-2022 slock.it GmbH, Blockchains LLC
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

// ::: This is a autogenerated file. Do not edit it manually! :::
// clang-format off

#include "../../core/client/request_internal.h"

// list of availbale rpc-functions
#ifndef __RPC_UTILS_H
#define __RPC_UTILS_H

/**
 * clears the incubed cache (usually found in the .in3-folder)
 *
 * Returns:
 *   - bool : (bool) true indicating the success
 */
static inline in3_ret_t rpc_call_in3_cacheClear(in3_rpc_handle_ctx_t* ctx, bool* _res) {
  d_token_t* res = NULL;
  in3_ret_t  _r  = req_send_sub_request(ctx->req, "in3_cacheClear", "", NULL, &res, NULL);
  if (!_r) *_res = d_int(res);
  return _r;
}
#define FN_IN3_CACHECLEAR "in3_cacheClear"

/**
 * Returns the underlying client version. See [web3_clientversion](https://eth.wiki/json-rpc/API#web3_clientversion) for spec.
 *
 * Returns:
 *   - char* : (string) when connected to the incubed-network, `Incubed/<Version>` will be returned, but in case of a direct enpoint, its's version will be used.
 */
static inline in3_ret_t rpc_call_web3_clientVersion(in3_rpc_handle_ctx_t* ctx, char** _res) {
  d_token_t* res = NULL;
  in3_ret_t  _r  = req_send_sub_request(ctx->req, "web3_clientVersion", "", NULL, &res, NULL);
  if (!_r) *_res = d_string(res);
  return _r;
}
#define FN_WEB3_CLIENTVERSION "web3_clientVersion"

/**
 * Returns Keccak-256 (not the standardized SHA3-256) of the given data.
 *
 * See [web3_sha3](https://eth.wiki/json-rpc/API#web3_sha3) for spec.
 *
 * No proof needed, since the client will execute this locally.
 *
 *
 *
 * Parameters:
 *
 *   - bytes_t data : (bytes) data to hash
 * Returns:
 *   - bytes_t : (bytes) the 32byte hash of the data
 */
static inline in3_ret_t rpc_call_web3_sha3(in3_rpc_handle_ctx_t* ctx, bytes_t* _res, bytes_t data) {
  d_token_t* res      = NULL;
  char*      jpayload = sprintx("\"%B\"", (bytes_t) data);
  in3_ret_t  _r       = req_send_sub_request(ctx->req, "web3_sha3", jpayload, NULL, &res, NULL);
  _free(jpayload);
  if (!_r) *_res = d_bytes(res);
  return _r;
}
#define FN_WEB3_SHA3 "web3_sha3"

/**
 * Returns base58 encoded data
 *
 *
 *
 * Parameters:
 *
 *   - bytes_t data : (bytes) data to encode
 * Returns:
 *   - char* : (string) the encoded data
 */
static inline in3_ret_t rpc_call_in3_base58_encode(in3_rpc_handle_ctx_t* ctx, char** _res, bytes_t data) {
  d_token_t* res      = NULL;
  char*      jpayload = sprintx("\"%B\"", (bytes_t) data);
  in3_ret_t  _r       = req_send_sub_request(ctx->req, "in3_base58_encode", jpayload, NULL, &res, NULL);
  _free(jpayload);
  if (!_r) *_res = d_string(res);
  return _r;
}
#define FN_IN3_BASE58_ENCODE "in3_base58_encode"

/**
 * Returns base58 decoded data
 *
 *
 *
 * Parameters:
 *
 *   - char* data : (string) data to decode
 * Returns:
 *   - bytes_t : (bytes) the decoded bytes
 */
static inline in3_ret_t rpc_call_in3_base58_decode(in3_rpc_handle_ctx_t* ctx, bytes_t* _res, char* data) {
  d_token_t* res      = NULL;
  char*      jpayload = sprintx("\"%S\"", (char*) data);
  in3_ret_t  _r       = req_send_sub_request(ctx->req, "in3_base58_decode", jpayload, NULL, &res, NULL);
  _free(jpayload);
  if (!_r) *_res = d_bytes(res);
  return _r;
}
#define FN_IN3_BASE58_DECODE "in3_base58_decode"

/**
 * Returns base64 encoded data
 *
 *
 *
 * Parameters:
 *
 *   - bytes_t data : (bytes) data to encode
 * Returns:
 *   - char* : (string) the encoded data
 */
static inline in3_ret_t rpc_call_in3_base64_encode(in3_rpc_handle_ctx_t* ctx, char** _res, bytes_t data) {
  d_token_t* res      = NULL;
  char*      jpayload = sprintx("\"%B\"", (bytes_t) data);
  in3_ret_t  _r       = req_send_sub_request(ctx->req, "in3_base64_encode", jpayload, NULL, &res, NULL);
  _free(jpayload);
  if (!_r) *_res = d_string(res);
  return _r;
}
#define FN_IN3_BASE64_ENCODE "in3_base64_encode"

/**
 * Returns base64 decoded data
 *
 *
 *
 * Parameters:
 *
 *   - char* data : (string) data to decode
 * Returns:
 *   - bytes_t : (bytes) the decoded bytes
 */
static inline in3_ret_t rpc_call_in3_base64_decode(in3_rpc_handle_ctx_t* ctx, bytes_t* _res, char* data) {
  d_token_t* res      = NULL;
  char*      jpayload = sprintx("\"%S\"", (char*) data);
  in3_ret_t  _r       = req_send_sub_request(ctx->req, "in3_base64_decode", jpayload, NULL, &res, NULL);
  _free(jpayload);
  if (!_r) *_res = d_bytes(res);
  return _r;
}
#define FN_IN3_BASE64_DECODE "in3_base64_decode"

/**
 * Returns sha-256 of the given data.
 *
 * No proof needed, since the client will execute this locally.
 *
 *
 *
 * Parameters:
 *
 *   - bytes_t data : (bytes) data to hash
 * Returns:
 *   - bytes_t : (bytes) the 32byte hash of the data
 */
static inline in3_ret_t rpc_call_sha256(in3_rpc_handle_ctx_t* ctx, bytes_t* _res, bytes_t data) {
  d_token_t* res      = NULL;
  char*      jpayload = sprintx("\"%B\"", (bytes_t) data);
  in3_ret_t  _r       = req_send_sub_request(ctx->req, "sha256", jpayload, NULL, &res, NULL);
  _free(jpayload);
  if (!_r) *_res = d_bytes(res);
  return _r;
}
#define FN_SHA256 "sha256"

/**
 * sends a simple http-request. This is used internally to fetch data from REST-APIs.
 *
 *
 * Parameters:
 *
 *   - char*      method  : (string) the HTTP-method to use like 'GET', 'POST', 'PUT', 'DELETE',...
 *   - char*      url     : (string) the url of the endpoint
 *   - char*      payload : (string) the payload or null, if it does not apply
 *   - d_token_t* headers : (string) a array of additional headers to send. each header must be a string in the form `Key: Value`
 * Returns:
 *   - d_token_t* : (any) the response will depend on the http-response. If the data are json-data, the json-object will the response.
 */
static inline in3_ret_t rpc_call_in3_http(in3_rpc_handle_ctx_t* ctx, d_token_t** res, char* method, char* url, char* payload, d_token_t* headers) {
  char*     jpayload = sprintx("\"%S\",\"%S\",\"%S\",%j", (char*) method, (char*) url, (char*) payload, (d_token_t*) headers);
  in3_ret_t _r       = req_send_sub_request(ctx->req, "in3_http", jpayload, NULL, res, NULL);
  _free(jpayload);
  return _r;
}
#define FN_IN3_HTTP "in3_http"

/**
 * execute a ENS-lookup
 *
 *
 * Parameters:
 *
 *   - char*    name     : (string) the ens name
 *   - char*    type     : (string) the type of data to resolve. "addr" - the address (default), "resolver", "owner" or "hash"
 *   - uint8_t* registry : (address) the ENS-Registry to use
 * Returns:
 *   - bytes_t : (bytes) depending on the type, the resulting address or hash
 */
static inline in3_ret_t rpc_call_in3_ens(in3_rpc_handle_ctx_t* ctx, bytes_t* _res, char* name, char* type, uint8_t* registry) {
  d_token_t* res      = NULL;
  char*      jpayload = sprintx("\"%S\",\"%S\",\"%B\"", (char*) name, (char*) type, bytes(registry, 20));
  in3_ret_t  _r       = req_send_sub_request(ctx->req, "in3_ens", jpayload, NULL, &res, NULL);
  _free(jpayload);
  if (!_r) *_res = d_bytes(res);
  return _r;
}
#define FN_IN3_ENS "in3_ens"

/**
 * based on the [ABI-encoding](https://solidity.readthedocs.io/en/v0.5.3/abi-spec.html) used by solidity, this function encodes the value given and returns it as hexstring.
 *
 *
 * Parameters:
 *
 *   - char*      signature  : (string) the signature of the function. e.g. `getBalance(uint256)`. The format is the same as used by solidity to create the functionhash. optional you can also add the return type, which in this case is ignored.
 *   - d_token_t* parameters : (any) a array of arguments. the number of arguments must match the arguments in the signature.
 * Returns:
 *   - char* : (string) the ABI-encoded data as hex including the 4 byte function-signature. These data can be used for `eth_call` or to send a transaction.
 */
static inline in3_ret_t rpc_call_in3_abiEncode(in3_rpc_handle_ctx_t* ctx, char** _res, char* signature, d_token_t* parameters) {
  d_token_t* res      = NULL;
  char*      jpayload = sprintx("\"%S\",%j", (char*) signature, (d_token_t*) parameters);
  in3_ret_t  _r       = req_send_sub_request(ctx->req, "in3_abiEncode", jpayload, NULL, &res, NULL);
  _free(jpayload);
  if (!_r) *_res = d_string(res);
  return _r;
}
#define FN_IN3_ABIENCODE "in3_abiEncode"

/**
 * based on the [ABI-encoding](https://solidity.readthedocs.io/en/v0.5.3/abi-spec.html) used by solidity, this function decodes the bytes given and returns it as array of values.
 *
 *
 * Parameters:
 *
 *   - char*   signature : (string) the signature of the function. e.g. `uint256`, `(address,string,uint256)` or `getBalance(address):uint256`. If the complete functionhash is given, only the return-part will be used.
 *   - bytes_t data      : (bytes) the data to decode (usually the result of a eth_call)
 *   - bytes_t topics    : (bytes) in case of an even the topics (concatinated to max 4x32bytes). This is used if indexed.arguments are used.
 * Returns:
 *   - d_token_t* : (any) a array with the values after decodeing.
 */
static inline in3_ret_t rpc_call_in3_abiDecode(in3_rpc_handle_ctx_t* ctx, d_token_t** res, char* signature, bytes_t data, bytes_t topics) {
  char*     jpayload = sprintx("\"%S\",\"%B\",\"%B\"", (char*) signature, (bytes_t) data, (bytes_t) topics);
  in3_ret_t _r       = req_send_sub_request(ctx->req, "in3_abiDecode", jpayload, NULL, res, NULL);
  _free(jpayload);
  return _r;
}
#define FN_IN3_ABIDECODE "in3_abiDecode"

/**
 * rlp decode the data
 *
 *
 * Parameters:
 *
 *   - bytes_t data : (bytes) input data
 * Returns:
 *   - d_token_t* : (any) a array with the values after decodeing. The result is either a hex-string or an array.
 */
static inline in3_ret_t rpc_call_in3_rlpDecode(in3_rpc_handle_ctx_t* ctx, d_token_t** res, bytes_t data) {
  char*     jpayload = sprintx("\"%B\"", (bytes_t) data);
  in3_ret_t _r       = req_send_sub_request(ctx->req, "in3_rlpDecode", jpayload, NULL, res, NULL);
  _free(jpayload);
  return _r;
}
#define FN_IN3_RLPDECODE "in3_rlpDecode"

/**
 * decodes a raw transaction and returns the values. The transaction may be a signed or unsigned tx. In case of a signed transaction, the from-address will be calculated along with many other helpful values.
 *
 *
 * Parameters:
 *
 *   - bytes_t data : (bytes) input data
 * Returns:
 *   - d_token_t* : (eth_tx_decoded) the decoded transaction.
 */
static inline in3_ret_t rpc_call_in3_decodeTx(in3_rpc_handle_ctx_t* ctx, d_token_t** res, bytes_t data) {
  char*     jpayload = sprintx("\"%B\"", (bytes_t) data);
  in3_ret_t _r       = req_send_sub_request(ctx->req, "in3_decodeTx", jpayload, NULL, res, NULL);
  _free(jpayload);
  return _r;
}
#define FN_IN3_DECODETX "in3_decodeTx"

/**
 * Will convert an upper or lowercase Ethereum address to a checksum address.  (See [EIP55](https://github.com/ethereum/EIPs/blob/master/EIPS/eip-55.md) )
 *
 *
 * Parameters:
 *
 *   - uint8_t* address    : (address) the address to convert.
 *   - bool     useChainId : (bool) if true, the chainId is integrated as well (See [EIP1191](https://github.com/ethereum/EIPs/issues/1121) )
 * Returns:
 *   - bytes_t : (bytes) the address-string using the upper/lowercase hex characters.
 */
static inline in3_ret_t rpc_call_in3_checksumAddress(in3_rpc_handle_ctx_t* ctx, bytes_t* _res, uint8_t* address, bool useChainId) {
  d_token_t* res      = NULL;
  char*      jpayload = sprintx("\"%B\",%i", bytes(address, 20), (int) useChainId);
  in3_ret_t  _r       = req_send_sub_request(ctx->req, "in3_checksumAddress", jpayload, NULL, &res, NULL);
  _free(jpayload);
  if (!_r) *_res = d_bytes(res);
  return _r;
}
#define FN_IN3_CHECKSUMADDRESS "in3_checksumAddress"

/**
 * converts the given value into wei.
 *
 *
 * Parameters:
 *
 *   - char* value : (string) the value, which may be floating number as string
 *   - char* unit  : (string) the unit of the value, which must be one of `wei`, `kwei`,  `Kwei`,  `babbage`,  `femtoether`,  `mwei`,  `Mwei`,  `lovelace`,  `picoether`,  `gwei`,  `Gwei`,  `shannon`,  `nanoether`,  `nano`,  `szabo`,  `microether`,  `micro`,  `finney`,  `milliether`,  `milli`,  `ether`,  `eth`,  `kether`,  `grand`,  `mether`,  `gether` or  `tether`
 * Returns:
 *   - bytes_t : (uint256) the value in wei as hex.
 */
static inline in3_ret_t rpc_call_in3_toWei(in3_rpc_handle_ctx_t* ctx, bytes_t* _res, char* value, char* unit) {
  d_token_t* res      = NULL;
  char*      jpayload = sprintx("\"%S\",\"%S\"", (char*) value, (char*) unit);
  in3_ret_t  _r       = req_send_sub_request(ctx->req, "in3_toWei", jpayload, NULL, &res, NULL);
  _free(jpayload);
  if (!_r) *_res = d_bytes(res);
  return _r;
}
#define FN_IN3_TOWEI "in3_toWei"

/**
 * returns the internal transactions send during execution of the tx. Currently this only works with geth with activated `debug` module (supporting `debug_traceTransaction` ). Also when running a pruned node, the state of transaction is only available for limited time ( mostly 5 min)
 *
 *
 * Parameters:
 *
 *   - bytes_t tx_hash : (bytes32) the transactionhash
 * Returns:
 *   - d_token_t* : (eth_internal_tx) an array of internal transactions
 */
static inline in3_ret_t rpc_call_in3_get_internal_tx(in3_rpc_handle_ctx_t* ctx, d_token_t** res, bytes_t tx_hash) {
  char*     jpayload = sprintx("\"%B\"", (bytes_t) tx_hash);
  in3_ret_t _r       = req_send_sub_request(ctx->req, "in3_get_internal_tx", jpayload, NULL, res, NULL);
  _free(jpayload);
  return _r;
}
#define FN_IN3_GET_INTERNAL_TX "in3_get_internal_tx"

/**
 * converts a given uint (also as hex) with a wei-value into a specified unit.
 *
 *
 * Parameters:
 *
 *   - bytes_t value  : (uint256) the value in wei
 *   - char*   unit   : (string) the unit of the target value, which must be one of `wei`, `kwei`,  `Kwei`,  `babbage`,  `femtoether`,  `mwei`,  `Mwei`,  `lovelace`,  `picoether`,  `gwei`,  `Gwei`,  `shannon`,  `nanoether`,  `nano`,  `szabo`,  `microether`,  `micro`,  `finney`,  `milliether`,  `milli`,  `ether`,  `eth`,  `kether`,  `grand`,  `mether`,  `gether` or  `tether`
 *   - bytes_t digits : (int) fix number of digits after the comma. If left out, only as many as needed will be included.
 * Returns:
 *   - d_token_t* : (double) the value as string.
 */
static inline in3_ret_t rpc_call_in3_fromWei(in3_rpc_handle_ctx_t* ctx, d_token_t** res, bytes_t value, char* unit, bytes_t digits) {
  char*     jpayload = sprintx("\"%B\",\"%S\",\"%B\"", (bytes_t) value, (char*) unit, (bytes_t) digits);
  in3_ret_t _r       = req_send_sub_request(ctx->req, "in3_fromWei", jpayload, NULL, res, NULL);
  _free(jpayload);
  return _r;
}
#define FN_IN3_FROMWEI "in3_fromWei"

/**
 * calculates the address of a contract about to deploy. The address depends on the senders nonce.
 *
 *
 * Parameters:
 *
 *   - uint8_t* sender : (address) the sender of the transaction
 *   - uint64_t nonce  : (uint64) the nonce of the sender during deployment
 * Returns:
 *   - uint8_t* : (address) the address of the deployed contract
 */
static inline in3_ret_t rpc_call_in3_calcDeployAddress(in3_rpc_handle_ctx_t* ctx, uint8_t** _res, uint8_t* sender, uint64_t nonce) {
  d_token_t* res      = NULL;
  char*      jpayload = sprintx("\"%B\",\"%U\"", bytes(sender, 20), (uint64_t) nonce);
  in3_ret_t  _r       = req_send_sub_request(ctx->req, "in3_calcDeployAddress", jpayload, NULL, &res, NULL);
  _free(jpayload);
  if (!_r) *_res = d_bytes(res).data;
  return _r;
}
#define FN_IN3_CALCDEPLOYADDRESS "in3_calcDeployAddress"

/**
 * Returns the current network id.
 *
 * Returns:
 *   - uint64_t : (uint64) the network id
 */
static inline in3_ret_t rpc_call_net_version(in3_rpc_handle_ctx_t* ctx, uint64_t* _res) {
  d_token_t* res = NULL;
  in3_ret_t  _r  = req_send_sub_request(ctx->req, "net_version", "", NULL, &res, NULL);
  if (!_r) *_res = d_long(res);
  return _r;
}
#define FN_NET_VERSION "net_version"

#endif