#include <stdint.h>  
#include "../util/bytes.h"

#ifndef CLIENT_H
#define CLIENT_H


typedef enum {
	PROOF_NONE = 0,
	PROOF_STANDARD = 1,
	PROOF_FULL = 2
} proof_t;

typedef struct {
    /* number of seconds requests can be cached. */
    u_int32_t cacheTimeout;

    /* the limit of nodes to store in the client. */
    u_int16_t nodeLimit;

    /* the limit of nodes to store in the client. */
    u_int16_t nodeLimit;

    /* the client key to sign requests */
    bytes_t key; 

    /* number of max bytes used to cache the code in memory */
    u_int32_t maxCodeCache;

    /* number of number of blocks cached  in memory */
    u_int32_t maxBlockCache;

    /* the type of proof used */
    proof_t proof;

    /* the number of request send when getting a first answer */
    u_int8_t requestCount; 

    /* the number of signatures used to proof the blockhash. */
    u_int8_t signatureCount;

    /* min stake of the server. Only nodes owning at least this amount will be chosen. */
    u_int64_t minDeposit;

    /* if specified, the blocknumber *latest* will be replaced by blockNumber- specified value */
    u_int16_t replaceLatestBlock;

    /* he number in percent needed in order reach finality (% of signature of the validators) */
    u_int8_t requestCount; 
  
    /* specifies the number of milliseconds before the request times out. increasing may be helpful if the device uses a slow connection. */
    u_int32_t timeout;  

    /* servers to filter for the given chain. The chain-id based on EIP-155.*/
    bytes_t chainId; 

    /* if true the nodelist will be automaticly updated if the lastBlock is newer */
    u_int8_t autoUpdateList; 

    /* a cache handler offering 2 functions ( setItem(string,string), getItem(string) ) */
    in3_storage_handler_t* cacheStorage;

    /* chain spec and nodeList definitions*/
    in3_chain_t* servers;


} in3;

typedef struct {
    /* function pointer returning a stored value for the given key.*/
    u_int32_t (verify)(bytes_t* data);





   

    
} in3_verifier_t;

typedef struct {
   /* chainId */
   bytes_t chainId; 

   /* array of addresses of nodes that should always part of the nodeList */
   bytes_t** initAddresses;

   /* last blocknumber the nodeList was updated*/
   u_int64_t lastBlock;

   /* the address of the registry contract */
   bytes_t contract; 

   /* if true the nodelist should be updated. */
   u_int8_t needsUpdate;

   /* array of nodes */
   in3_node_t* nodeList;

   /* number of nodes in the nodeList */
   u_int32_t nodeListLength;

   /* stats and weights recorded for each node */
   in3_node_weight_t* weights;

    
} in3_chain_t;

typedef struct {
    /* the index within the contract */
    u_int32_t index;

    /* the address of node */
    bytes_t address; 

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
    /* function pointer returning a stored value for the given key.*/
    bytes_t* (getItem)(bytes_t* key);
     
    /* function pointer setting a stored value for the given key.*/
    void (setItem)(bytes_t* key, bytes_t* value);

} in3_storage_handler_t;





/* allocates a new byte array with 0 filled */
bytes_t *b_new(char *data, int len);

/* printsa the bytes as hey to stdout */
void b_print(bytes_t *a);

/* compares 2 byte arrays and returns 1 for equal and 0 for not equal*/
int b_cmp(bytes_t *a, bytes_t *b);

/* frees the data */
void b_free(bytes_t *a);

/* clones a byte array*/
bytes_t* b_dup(bytes_t *a);

#endif
