#include "../util/data.h"
#include "../util/mem.h"
#include "cache.h"
#include "client.h"
#include "nodelist.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define KOVAN_SPEC parse_binary_str("\xcc\x62\xb6\x0e.authorityRound\x00\x5c\xdcI\x14\x00\xd6\xcc\x1b\xa9\xcf\x89\xbd.X\x00\x97\x41\xf4\xf7\x32[\xad\xc0\xed\x14\x00\x42\x7f\xea\xe2\x41\x9c\x15\xb8\x9d\x1c!\xaf\x10\xd1\xb6\x65\x0aM=\x14N\xd9\xb0\x8e\x63T\xc7\x0f\xe6\xf8\xcb\x04\x11\xb0\xd3$kBMl\x14\x00 \xeeK\xe0\xe2\x02}v`<\xb7Q\xee\x06\x95\x19\xba\x81\xa1\x14\x00\x10\xf9K)j\x85*\xaa\xc5.\xa6\xc5\xacr\xe0:\xfd\x03-\x14\x00w3\xa1\xfei\xcf?,\xf9\x89\xf8\x1c{L\xac\x16\x93\x38z\x14\x00\xe6\xd2\xb9\x31\xf5Z?\x17\x01\xc7\x38\x9dY*wx\x89xy\x14\x00\xe4\xa1\x06P\xe5\xa6\xd6\x00\x1c\x38\xff\x8e\x64\xf9p\x16\xa1\x64\x5c\x14\x00\xa0\xa2K\x9f\x0e^\xc7\xaaLs\x89\xb8\x30/\xd0\x12\x31\x94\xde", 212)
//#define KOVAN_SPEC NULL
//char* ps = "\xccb\x00\x00.authorityRound\x00\x00\x00I\x14\x00\xd6\xcc\x1b\xa9\xcf\x89\xbd.X\x00\x97A\xf4\xf72[\xad\xc0\xed\x14\x00B\x7f\xea\xe2A\x9c\x15\xb8\x9d\x1c!\xaf\x10\xd1\xb6e\x0aM=\x14N\xd9\xb0\x8ecT\xc7\x0f\xe6\xf8\xcb\x04\x11\xb0\xd3$kBMl\x14\x00 \xeeK\xe0\xe2\x02}v`<\xb7Q\xee\x06\x95\x19\xba\x81\xa1\x14\x00\x10\xf9K)j\x85*\xaa\xc5.\xa6\xc5\xacr\xe0:\xfd\x03-\x14\x00w3\xa1\xfei\xcf?,\xf9\x89\xf8\x1c{L\xac\x16\x938z\x14\x00\xe6\xd2\xb91\xf5Z?\x17\x01\xc78\x9dY*wx\x89xy\x14\x00\xe4\xa1\x06P\xe5\xa6\xd6\x00\x1c8\xff\x8ed\xf9p\x16\xa1d\x5c\x14\x00\xa0\xa2K\x9f\x0e^\xc7\xaaLs\x89\xb80/\xd0\x121\x94\xde";

static void initChain(in3_chain_t* chain, uint64_t chainId, char* contract, int boot_node_count, in3_chain_type_t type, json_ctx_t* spec) {
  chain->chainId        = chainId;
  chain->initAddresses  = NULL;
  chain->lastBlock      = 0;
  chain->contract       = hex2byte_new_bytes(contract, 40);
  chain->needsUpdate    = 0;
  chain->nodeList       = _malloc(sizeof(in3_node_t) * boot_node_count);
  chain->nodeListLength = boot_node_count;
  chain->weights        = _malloc(sizeof(in3_node_weight_t) * boot_node_count);
  chain->type           = type;
  chain->spec           = spec;
}

static void initNode(in3_chain_t* chain, int node_index, char* address, char* url) {
  in3_node_t* node = chain->nodeList + node_index;
  node->address    = hex2byte_new_bytes(address, 40);
  node->index      = node_index;
  node->capacity   = 1;
  node->deposit    = 0;
  node->props      = 0xFF;
  node->url        = _malloc(strlen(url) + 1);
  memcpy(node->url, url, strlen(url) + 1);

  in3_node_weight_t* weight   = chain->weights + node_index;
  weight->blacklistedUntil    = 0;
  weight->response_count      = 0;
  weight->total_response_time = 0;
  weight->weight              = 1;
}

static void in3_client_init(in3_t* c) {
  c->evm_flags          = 0;
  c->autoUpdateList     = 1;
  c->cacheStorage       = NULL;
  c->signer             = NULL;
  c->cacheTimeout       = 0;
  c->use_binary         = 0;
  c->use_http           = 0;
  c->includeCode        = 0;
  c->chainId            = 0x01; // mainnet
  c->key                = NULL;
  c->finality           = 0;
  c->max_attempts       = 3;
  c->maxBlockCache      = 0;
  c->maxCodeCache       = 0;
  c->minDeposit         = 0;
  c->nodeLimit          = 0;
  c->proof              = PROOF_STANDARD;
  c->replaceLatestBlock = 0;
  c->requestCount       = 1;
  c->chainsCount        = 5;
  c->chains             = _malloc(sizeof(in3_chain_t) * c->chainsCount);

  // mainnet
  initChain(c->chains, 0x01, "2736D225f85740f42D17987100dc8d58e9e16252", 2, CHAIN_ETH, NULL);
  initNode(c->chains, 0, "8f354b72856e516f1e931c97d1ed3bf1709f38c9", "https://in3.slock.it/mainnet/nd-3");
  initNode(c->chains, 1, "243D5BB48A47bEd0F6A89B61E4660540E856A33D", "https://in3.slock.it/mainnet/nd-5");

  // tobalaba
  initChain(c->chains + 1, 0x044d, "845E484b505443814B992Bf0319A5e8F5e407879", 2, CHAIN_ETH, NULL);
  initNode(c->chains + 1, 0, "8f354b72856e516f1e931c97d1ed3bf1709f38c9", "https://in3.slock.it/tobalaba/nd-3");
  initNode(c->chains + 1, 1, "784bfa9eb182c3a02dbeb5285e3dba92d717e07a", "https://in3.slock.it/tobalaba/nd-1");

  // evan
  initChain(c->chains + 2, 0x04b1, "85613723dB1Bc29f332A37EeF10b61F8a4225c7e", 2, CHAIN_ETH, NULL);
  initNode(c->chains + 2, 0, "eaC4B82273e828878fD765D993800891bA2E3475", "http://52.47.61.24:8500");
  initNode(c->chains + 2, 1, "243D5BB48A47bEd0F6A89B61E4660540E856A33A", "https://in3.slock.it/evan/nd-5");

  // kovan
  initChain(c->chains + 3, 0x2a, "27a37a1210df14f7e058393d026e2fb53b7cf8c1", 2, CHAIN_ETH, KOVAN_SPEC);
  initNode(c->chains + 3, 0, "8f354b72856e516f1e931c97d1ed3bf1709f38c9", "https://in3.slock.it/kovan/nd-3");
  initNode(c->chains + 3, 1, "243D5BB48A47bEd0F6A89B61E4660540E856A33D", "https://in3.slock.it/kovan/nd-5");

  // ipfs
  initChain(c->chains + 4, 0x7d0, "f0fb87f4757c77ea3416afe87f36acaa0496c7e9", 2, CHAIN_IPFS, NULL);
  initNode(c->chains + 4, 0, "784bfa9eb182c3a02dbeb5285e3dba92d717e07a", "https://in3.slock.it/ipfs/nd-1");
  initNode(c->chains + 4, 1, "243D5BB48A47bEd0F6A89B61E4660540E856A33D", "https://in3.slock.it/ipfs/nd-5");
}

/* frees the data */
void in3_free(in3_t* a) {
  int i;
  for (i = 0; i < a->chainsCount; i++) {
    in3_nodelist_clear(a->chains + i);
    b_free(a->chains[i].contract);
    free_json(a->chains[i].spec);
  }
  if (a->signer) _free(a->signer);
  _free(a->chains);
  _free(a);
}

in3_t* in3_new() {
  // initialize random with the timestamp as seed
  _srand(_time());

  // create new client
  in3_t* c = _calloc(1, sizeof(in3_t));
  in3_client_init(c);
  return c;
}
