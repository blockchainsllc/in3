#include <stdint.h>  
#include "../util/bytes.h"
#include "../util/utils.h"
#include "context.h"
#include <stdbool.h>

#ifndef CLIENT_H
#define CLIENT_H

enum in3err {
	/*  */
	IN3_ERR_INVALID_JSON = -1,

    IN3_ERR_BUFFER_TOO_SMALL = -2,

    IN3_ERR_CHAIN_NOT_FOUND = -3,
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
//    u_int32_t (verify)(bytes_t* data);

    //TODO define verifier
    
} in3_verifier_t;

typedef struct {
   u_int64_t chainId;
   u_int8_t includeCode;
   u_int8_t useFullProof;
   bytes_t** verifiedHashes;
   u_int16_t verifiedHashesCount;
   u_int16_t latestBlock;
   u_int16_t finality;
   in3_verification_t verification;
   bytes_t* clientSignature;

   bytes_t** signatures;
   u_int8_t signaturesCount;

} in3_request_config_t;

typedef struct {
   char* url;
   char* data;
   in3_request_config_t in3;
} in3_request_t;
typedef struct {
    /* the index within the contract */
    u_int32_t index;

    /* url of the node*/
    char* url;

    /* the address of node */
    bytes_t* address; 

    /* stored deposit */
    u_int64_t deposit;

    /* the maximal capacity able to handle*/
    u_int32_t capacity;

    /* the properties*/
    u_int64_t props;
    
} in3_node_t;

typedef struct {
    /** current weight*/
    float weight;

    /* counter for responses */
    u_int32_t response_count;

    /* total of all response times */
    u_int32_t total_response_time;

    /** if >0 this node is blacklisted  untilk the ts */
    u_int64_t blacklistedUntil;

} in3_node_weight_t;

typedef struct {
   /* chainId */
   u_int64_t chainId; 

   /* array of addresses of nodes that should always part of the nodeList */
   bytes_t** initAddresses;

   /* last blocknumber the nodeList was updated*/
   u_int64_t lastBlock;

   /* the address of the registry contract */
   bytes_t* contract; 

   /* if true the nodelist should be updated. */
   bool needsUpdate;

   /* array of nodes */
   in3_node_t* nodeList;

   /* number of nodes in the nodeList */
   u_int32_t nodeListLength;

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
    char* error;
    char* result;
} in3_response_t;


typedef in3_response_t* (*in3_transport_send)(in3_request_t* ctx, int request_count);


typedef struct {
    /* number of seconds requests can be cached. */
    u_int32_t cacheTimeout;

    /* the limit of nodes to store in the client. */
    u_int16_t nodeLimit;

    /* the client key to sign requests */
    bytes_t* key; 

    /* number of max bytes used to cache the code in memory */
    u_int32_t maxCodeCache;

    /* number of number of blocks cached  in memory */
    u_int32_t maxBlockCache;

    /* the type of proof used */
    in3_proof_t proof;

    /* the number of request send when getting a first answer */
    u_int8_t requestCount; 

    /* the number of signatures used to proof the blockhash. */
    u_int8_t signatureCount;

    /* min stake of the server. Only nodes owning at least this amount will be chosen. */
    u_int64_t minDeposit;

    /* if specified, the blocknumber *latest* will be replaced by blockNumber- specified value */
    u_int16_t replaceLatestBlock;

    /* specifies the number of milliseconds before the request times out. increasing may be helpful if the device uses a slow connection. */
    u_int32_t timeout;  

    /* servers to filter for the given chain. The chain-id based on EIP-155.*/
    u_int64_t chainId; 

    /* if true the nodelist will be automaticly updated if the lastBlock is newer */
    u_int8_t autoUpdateList; 

    /* a cache handler offering 2 functions ( setItem(string,string), getItem(string) ) */
    in3_storage_handler_t* cacheStorage;

    /* the transporthandler sending requests */
    in3_transport_send transport;

    /* chain spec and nodeList definitions*/
    in3_chain_t* servers;

    /* number of configured chains */
    u_int16_t serversCount;


} in3;



/* allocates a new byte array with 0 filled */
in3 *in3_new();

/* sends a request and stores the result in the provided buffer */
int in3_client_send(in3* c,char* req, char* result, int buf_size);

/* sends a request and stores the result in the provided buffer */
int in3_client_rpc(in3* c, char* method, char* params ,char* result, int buf_size);

/* rreturns the nodelist or even updates it. */
int get_node_list(in3* c, u_int64_t chain, bool update,  in3_node_t** nodeList, u_int32_t* nodeListLength );

/* frees the references of the client */
void in3_free(in3 *a);


#endif
