/** @file 
 * incubed main client file.
 * */

#ifndef CLIENT_H
#define CLIENT_H

#include "../util/bytes.h"
#include "../util/data.h"
#include "../util/error.h"
#include "../util/stringbuilder.h"
#include "../util/utils.h"
#include <stdbool.h>
#include <stdint.h>

/** type of the chain */
typedef enum {
  CHAIN_ETH       = 0, /**< Ethereum chain */
  CHAIN_SUBSTRATE = 1, /**< substrate chain */
  CHAIN_IPFS      = 2, /**< ipfs verifiaction */
  CHAIN_BTC       = 3, /**< Bitcoin chain */
  CHAIN_IOTA      = 4, /**< IOTA chain */
  CHAIN_GENERIC   = 5,
} in3_chain_type_t;

/** type of proof */
typedef enum {
  PROOF_NONE     = 0, /**< No Verification */
  PROOF_STANDARD = 1, /**< Standard Verification of the important properties */
  PROOF_FULL     = 2  /**< All field will be validated including uncles */
} in3_proof_t;

/** verification as delivered by the server. This will be part of the in3-request.*/
typedef enum {
  VERIFICATION_NEVER                = 0, /**< No Verifacation */
  VERIFICATION_PROOF                = 1, /**< Includes the proof of the data */
  VERIFICATION_PROOF_WITH_SIGNATURE = 2  /**< Proof + Signmatures */
} in3_verification_t;

/** the configuration as part of each incubed request. 
 * This will be generated based on the client-configuration. 
 */
typedef struct {
  uint64_t           chainId;             /**< the chain to be used. this is holding the integer-value of the hexstring. */
  uint8_t            includeCode;         /**< if true the code needed will always be devlivered.  */
  uint8_t            useFullProof;        /**< this flaqg is set, if the proof is set to "PROOF_FULL" */
  uint8_t            useBinary;           /**< this flaqg is set, the client should use binary-format */
  bytes_t*           verifiedHashes;      /**< a list of blockhashes already verified. The Server will not send any proof for them again . */
  uint16_t           verifiedHashesCount; /**< number of verified blockhashes*/
  uint16_t           latestBlock;         /**< the last blocknumber the nodelistz changed */
  uint16_t           finality;            /**< number of signatures( in percent) needed in order to reach finality. */
  in3_verification_t verification;        /**< Verification-tyüe */
  bytes_t*           clientSignature;     /**< the signature of the client with the client key */
  bytes_t*           signatures;          /**< the addresses of servers requested to sign the blockhash */
  uint8_t            signaturesCount;     /**< number or addresses */

} in3_request_config_t;

/** node-configuration */
typedef struct {
  uint32_t index;   /**< index within the nodelist, also used in the contract as key */
  bytes_t* address; /**< address of the server */

  /* stored deposit */
  uint64_t deposit;

  /* the maximal capacity able to handle*/
  uint32_t capacity;

  /* the properties*/
  uint64_t props;

  /* url of the node*/
  char* url;

} in3_node_t;

typedef struct {
  /** current weight*/
  float weight;

  /* counter for responses */
  uint32_t response_count;

  /* total of all response times */
  uint32_t total_response_time;

  /** if >0 this node is blacklisted  untilk the ts */
  uint64_t blacklistedUntil;

} in3_node_weight_t;

typedef struct {
  /* chainId */
  uint64_t chainId;

  /*! chaintype */
  in3_chain_type_t type;

  /* last blocknumber the nodeList was updated*/
  uint64_t lastBlock;

  /* if true the nodelist should be updated. */
  bool needsUpdate;

  /* number of nodes in the nodeList */
  int nodeListLength;

  /* array of nodes */
  in3_node_t* nodeList;

  /* stats and weights recorded for each node */
  in3_node_weight_t* weights;

  /* array of addresses of nodes that should always part of the nodeList */
  bytes_t** initAddresses;

  /* the address of the registry contract */
  bytes_t* contract;

  /* optional chain specification*/
  json_ctx_t* spec;

} in3_chain_t;

/** storage handler */
typedef bytes_t* (*in3_storage_get_item)(void* cptr, char*);
typedef void (*in3_storage_set_item)(void* cptr, char*, bytes_t*);

typedef struct {
  /* function pointer returning a stored value for the given key.*/
  in3_storage_get_item get_item;

  /* function pointer setting a stored value for the given key.*/
  in3_storage_set_item set_item;

  /* custom object whill will be passed to functions */
  void* cptr;

} in3_storage_handler_t;

#define IN3_SIGN_ERR_REJECTED -1
#define IN3_SIGN_ERR_ACCOUNT_NOT_FOUND -2
#define IN3_SIGN_ERR_INVALID_MESSAGE -3
#define IN3_SIGN_ERR_GENERAL_ERROR -4
#define IN3_DEBUG 65536
/** type of the requested signature */
typedef enum {
  SIGN_EC_RAW  = 0, /**< sign the data directly */
  SIGN_EC_HASH = 1, /**< hash and sign the data */
} d_signature_type_t;

/** 
 * signing function.
 * 
 * signs the given data and write the signature to dst.
 * the return value must be the number of bytes written to dst.
 * In case of an error a negativ value must be returned. It should be one of the IN3_SIGN_ERR... values.
 * 
*/
typedef int (*in3_sign)(void* wallet, d_signature_type_t type, bytes_t message, bytes_t account, uint8_t* dst);

typedef struct {
  /* function pointer returning a stored value for the given key.*/
  in3_sign sign;

  /* custom object whill will be passed to functions */
  void* wallet;

} in3_signer_t;

/** response-object. 
 * 
 * if the error has a length>0 the response will be rejected
 */
typedef struct {
  sb_t error;  /**< a stringbuilder to add any errors! */
  sb_t result; /**< a stringbuilder to add the result */
} in3_response_t;

/** the transport function to be implemented by the transport provider.
 */
typedef in3_error_t (*in3_transport_send)(char** urls, int urls_len, char* payload, in3_response_t* results);

typedef enum {
  FILTER_EVENT   = 0, /**< Event filter */
  FILTER_BLOCK   = 1, /**< Block filter */
  FILTER_PENDING = 2, /**< Pending filter (Unsupported) */
} in3_filter_type_t;

typedef struct in3_filter_t_ {
  /** filter type: (event, block or pending) */
  in3_filter_type_t type;

  /** associated filter options */
  char* options;

  /** block no. when filter was created OR eth_getFilterChanges was called */
  uint64_t last_block;

  /** method to release owned resources */
  void (*release)(struct in3_filter_t_* f);
} in3_filter_t;

typedef struct in3_filter_handler_t_ {
  in3_filter_t** array; /** array of filters */
  size_t         count; /** counter for filters */
} in3_filter_handler_t;

typedef struct {
  /** number of seconds requests can be cached. */
  uint32_t cacheTimeout;

  /** the limit of nodes to store in the client. */
  uint16_t nodeLimit;

  /** the client key to sign requests */
  bytes_t* key;

  /** number of max bytes used to cache the code in memory */
  uint32_t maxCodeCache;

  /** number of number of blocks cached  in memory */
  uint32_t maxBlockCache;

  /** the type of proof used */
  in3_proof_t proof;

  /** the number of request send when getting a first answer */
  uint8_t requestCount;

  /** the number of signatures used to proof the blockhash. */
  uint8_t signatureCount;

  /** min stake of the server. Only nodes owning at least this amount will be chosen. */
  uint64_t minDeposit;

  /** if specified, the blocknumber *latest* will be replaced by blockNumber- specified value */
  uint16_t replaceLatestBlock;

  /** the number of signatures in percent required for the request*/
  uint16_t finality;

  /** the max number of attempts before giving up*/
  uint16_t max_attempts;

  /** specifies the number of milliseconds before the request times out. increasing may be helpful if the device uses a slow connection. */
  uint32_t timeout;

  /** servers to filter for the given chain. The chain-id based on EIP-155.*/
  uint64_t chainId;

  /** if true the nodelist will be automaticly updated if the lastBlock is newer */
  uint8_t autoUpdateList;

  /** a cache handler offering 2 functions ( setItem(string,string), getItem(string) ) */
  in3_storage_handler_t* cacheStorage;

  /** signer-struct managing a wallet */
  in3_signer_t* signer;

  /** the transporthandler sending requests */
  in3_transport_send transport;

  /** includes the code when sending eth_call-requests */
  uint8_t includeCode;

  /** if true the client will use binary format*/
  uint8_t use_binary;

  /** if true the client will try to use http instead of https*/
  uint8_t use_http;

  /** chain spec and nodeList definitions*/
  in3_chain_t* chains;

  /** number of configured chains */
  uint16_t chainsCount;

  /** flags for the evm (EIPs) */
  uint32_t evm_flags;

  /** filter handler */
  in3_filter_handler_t* filters;

} in3_t;

/** allocates a new byte array with 0 filled */
in3_t* in3_new();

/** sends a request and stores the result in the provided buffer */
in3_error_t in3_client_rpc(in3_t* c, char* method, char* params, char** result, char** error);

/** frees the references of the client */
void in3_free(in3_t* a);

#endif
