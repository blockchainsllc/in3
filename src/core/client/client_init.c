#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "client.h"
#include "cache.h"
#include "nodelist.h"
#include <time.h>
#include "../util/mem.h"


static void initChain(in3_chain_t* chain, uint64_t chainId, char* contract, int boot_node_count, in3_chain_type_t type) {
    chain->chainId       = chainId;
    chain->initAddresses = NULL;
    chain->lastBlock     = 0;
    chain->contract      = hex2byte_new_bytes(contract,40);
    chain->needsUpdate   = 1;
    chain->nodeList      = _malloc(sizeof(in3_node_t)*boot_node_count);
    chain->nodeListLength= boot_node_count;
    chain->weights       = _malloc(sizeof(in3_node_weight_t)*boot_node_count);
    chain->type          = type;
}

static void initNode(in3_chain_t* chain, int node_index, char* address, char* url) {
    in3_node_t* node = chain->nodeList+node_index;
    node->address  = hex2byte_new_bytes(address,40);
    node->index    = node_index;
    node->capacity = 1;
    node->deposit  = 0;
    node->props    = 0xFF;
    node->url      = _malloc( strlen(url)+1 );
    memcpy(node->url,url,strlen(url)+1 );

    in3_node_weight_t* weight = chain->weights + node_index;
    weight->blacklistedUntil    = 0;
    weight->response_count      = 0;
    weight->total_response_time = 0;
    weight->weight              = 1;
}

static void in3_client_init(in3_t* c) {

    c->autoUpdateList=1;
    c->cacheStorage=NULL;
    c->cacheTimeout = 0;
    c->chainId      = 0x01; // mainnet
    c->key          = NULL;
    c->finality     = 0;
    c->max_attempts = 3;
    c->maxBlockCache= 0;
    c->maxCodeCache = 0;
    c->minDeposit   = 0;
    c->nodeLimit    = 0;
    c->proof        = PROOF_STANDARD;
    c->replaceLatestBlock = 0;
    c->requestCount = 1;
    c->serversCount = 5;
    c->servers      = _malloc(sizeof(in3_chain_t) * c->serversCount);

    // mainnet
    initChain(c->servers, 0x01, "2736D225f85740f42D17987100dc8d58e9e16252" , 2 , CHAIN_ETH);
    initNode( c->servers, 0, "8f354b72856e516f1e931c97d1ed3bf1709f38c9","https://in3.slock.it/mainnet/nd-3");
    initNode( c->servers, 1, "243D5BB48A47bEd0F6A89B61E4660540E856A33D","https://in3.slock.it/mainnet/nd-5");

    // tobalaba
    initChain(c->servers+1, 0x044d, "845E484b505443814B992Bf0319A5e8F5e407879" , 2 ,  CHAIN_ETH);
    initNode( c->servers+1, 0, "8f354b72856e516f1e931c97d1ed3bf1709f38c9","https://in3.slock.it/tobalaba/nd-3");
    initNode( c->servers+1, 1, "784bfa9eb182c3a02dbeb5285e3dba92d717e07a","https://in3.slock.it/tobalaba/nd-1");

    // evan
    initChain(c->servers+2, 0x04b1, "85613723dB1Bc29f332A37EeF10b61F8a4225c7e" , 2 ,  CHAIN_ETH);
    initNode( c->servers+2, 0, "eaC4B82273e828878fD765D993800891bA2E3475","http://52.47.61.24:8500");
    initNode( c->servers+2, 1, "243D5BB48A47bEd0F6A89B61E4660540E856A33A","https://in3.slock.it/evan/nd-5");

    // kovan
    initChain(c->servers+3, 0x2a, "27a37a1210df14f7e058393d026e2fb53b7cf8c1" , 2 , CHAIN_ETH);
    initNode( c->servers+3, 0, "8f354b72856e516f1e931c97d1ed3bf1709f38c9","https://in3.slock.it/kovan/nd-3");
    initNode( c->servers+3, 1, "243D5BB48A47bEd0F6A89B61E4660540E856A33D","https://in3.slock.it/kovan/nd-5");

    // ipfs
    initChain(c->servers+4, 0x7d0, "f0fb87f4757c77ea3416afe87f36acaa0496c7e9" , 2 , CHAIN_IPFS);
    initNode( c->servers+4, 0, "784bfa9eb182c3a02dbeb5285e3dba92d717e07a","https://in3.slock.it/ipfs/nd-1");
    initNode( c->servers+4, 1, "243D5BB48A47bEd0F6A89B61E4660540E856A33D","https://in3.slock.it/ipfs/nd-5");
}


/* frees the data */
void in3_free(in3_t* a) {
    int i;
    for (i=0;i<a->serversCount;i++) {
        in3_nodelist_clear(a->servers+i);
        b_free(a->servers[i].contract);
    }
    _free(a->servers);
    _free(a);
}

in3_t *in3_new() {
    // initialize random with the timestamp as seed
    _srand ( _time() );

     // create new client
	in3_t *c = _calloc(1, sizeof(in3_t));
    in3_client_init(c);
	return c;
}
