#include <stdint.h>  
#include "../util/bytes.h"
#include "../util/utils.h"
#include "../util/stringbuilder.h"
#include <stdbool.h>

#ifndef CLIENT_H
#define CLIENT_H

enum in3err {
	/*  */
	IN3_ERR_INVALID_JSON = -1,
    IN3_ERR_BUFFER_TOO_SMALL = -2,
    IN3_ERR_REQUEST_INVALID = -3,
    IN3_ERR_CHAIN_NOT_FOUND = -4,
    IN3_ERR_NO_NODES_FOUND = -5,
    IN3_ERR_CONFIG_ERROR =  -6,
    IN3_ERR_MAX_ATTEMPTS = -7,
};


typedef enum {
	PROOF_NONE = 0,
	PROOF_STANDARD = 1,
	PROOF_FULL = 2
} in3_proof_t;

typedef enum {
	VERIFICATION_NEVER = 0,
	VERIFICATION_PROOF = 1,
	VERIFICATION_PROOF_WITH_SIGNATURE = 2
} in3_verification_t;

typedef struct {
    /* function pointer returning a stored value for the given key.*/
//    uint32_t (verify)(bytes_t* data);

    //TODO define verifier
    
} in3_verifier_t;

typedef struct {
   uint64_t chainId;
   uint8_t includeCode;
   uint8_t useFullProof;
   bytes_t* verifiedHashes;
   uint16_t verifiedHashesCount;
   uint16_t latestBlock;
   uint16_t finality;
   in3_verification_t verification;
   bytes_t* clientSignature;

   bytes_t* signatures;
   uint8_t signaturesCount;

} in3_request_config_t;


typedef struct {
    /* the index within the contract */
    uint32_t index;

    /* url of the node*/
    char* url;

    /* the address of node */
    bytes_t* address; 

    /* stored deposit */
    uint64_t deposit;

    /* the maximal capacity able to handle*/
    uint32_t capacity;

    /* the properties*/
    uint64_t props;
    
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

   /* array of addresses of nodes that should always part of the nodeList */
   bytes_t** initAddresses;

   /* last blocknumber the nodeList was updated*/
   uint64_t lastBlock;

   /* the address of the registry contract */
   bytes_t* contract; 

   /* if true the nodelist should be updated. */
   bool needsUpdate;

   /* array of nodes */
   in3_node_t* nodeList;

   /* number of nodes in the nodeList */
   int nodeListLength;

   /* stats and weights recorded for each node */
   in3_node_weight_t* weights;

    
} in3_chain_t;



typedef bytes_t* (*in3_storage_get_item)(char *);
typedef void (*in3_storage_set_item)(char *, bytes_t*);

typedef struct {
    /* function pointer returning a stored value for the given key.*/
    in3_storage_get_item get_item;
     
    /* function pointer setting a stored value for the given key.*/
    in3_storage_set_item set_item;

} in3_storage_handler_t;

typedef struct {
    sb_t error;
    sb_t result;
} in3_response_t;


typedef int (*in3_transport_send)(char** urls,  int urls_len, char* payload, in3_response_t* results);


typedef struct {
    /* number of seconds requests can be cached. */
    uint32_t cacheTimeout;

    /* the limit of nodes to store in the client. */
    uint16_t nodeLimit;

    /* the client key to sign requests */
    bytes_t* key; 

    /* number of max bytes used to cache the code in memory */
    uint32_t maxCodeCache;

    /* number of number of blocks cached  in memory */
    uint32_t maxBlockCache;

    /* the type of proof used */
    in3_proof_t proof;

    /* the number of request send when getting a first answer */
    uint8_t requestCount; 

    /* the number of signatures used to proof the blockhash. */
    uint8_t signatureCount;

    /* min stake of the server. Only nodes owning at least this amount will be chosen. */
    uint64_t minDeposit;

    /* if specified, the blocknumber *latest* will be replaced by blockNumber- specified value */
    uint16_t replaceLatestBlock;

    /* the number of signatures in percent required for the request*/
    uint16_t finality;

    /* the max number of attempts before giving up*/
    uint16_t max_attempts;

    /* specifies the number of milliseconds before the request times out. increasing may be helpful if the device uses a slow connection. */
    uint32_t timeout;  

    /* servers to filter for the given chain. The chain-id based on EIP-155.*/
    uint64_t chainId; 

    /* if true the nodelist will be automaticly updated if the lastBlock is newer */
    uint8_t autoUpdateList; 

    /* a cache handler offering 2 functions ( setItem(string,string), getItem(string) ) */
    in3_storage_handler_t* cacheStorage;

    /* the transporthandler sending requests */
    in3_transport_send transport;

    /* chain spec and nodeList definitions*/
    in3_chain_t* servers;

    /* number of configured chains */
    uint16_t serversCount;


} in3;



/* allocates a new byte array with 0 filled */
in3 *in3_new();

/* sends a request and stores the result in the provided buffer */
int in3_client_send(in3* c,char* req, char* result, int buf_size, char* error);

/* sends a request and stores the result in the provided buffer */
int in3_client_rpc(in3* c, char* method, char* params ,char* result, int buf_size, char* error);

/* frees the references of the client */
void in3_free(in3 *a);


#endif
