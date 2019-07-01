#include "../util/data.h"
#include "../util/log.h"
#include "../util/mem.h"
#include "cache.h"
#include "client.h"
#include "nodelist.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define KOVAN_SPEC parse_json("[{\"block\":0,\"engine\":\"authorityRound\",\"list\":[\"0x00D6Cc1BA9cf89BD2e58009741f4F7325BAdc0ED\",\"0x00427feae2419c15b89d1c21af10d1b6650a4d3d\",\"0x4Ed9B08e6354C70fE6F8CB0411b0d3246b424d6c\",\"0x0020ee4Be0e2027d76603cB751eE069519bA81A1\",\"0x0010f94b296a852aaac52ea6c5ac72e03afd032d\",\"0x007733a1FE69CF3f2CF989F81C7b4cAc1693387A\",\"0x00E6d2b931F55a3f1701c7389d592a7778897879\",\"0x00e4a10650e5a6D6001C38ff8E64F97016a1645c\",\"0x00a0a24b9f0e5ec7aa4c7389b8302fd0123194de\"]},{\"block\":10960440,\"engine\":\"authorityRound\",\"list\":[\"0x00D6Cc1BA9cf89BD2e58009741f4F7325BAdc0ED\",\"0x0010f94b296a852aaac52ea6c5ac72e03afd032d\",\"0x00a0a24b9f0e5ec7aa4c7389b8302fd0123194de\"]},{\"block\":10960500,\"engine\":\"authorityRound\",\"contract\":\"0xaE71807C1B0a093cB1547b682DC78316D945c9B8\",\"list\":[\"0xfaadface3fbd81ce37b0e19c0b65ff4234148132\",\"0x596e8221a30bfe6e7eff67fee664a01c73ba3c56\",\"0xa4df255ecf08bbf2c28055c65225c9a9847abd94\",\"0x03801efb0efe2a25ede5dd3a003ae880c0292e4d\",\"0xd05f7478c6aa10781258c5cc8b4f385fc8fa989c\"],\"requiresFinality\":true,\"bypassFinality\":10960502}]")
#define TOBALABA_SPEC parse_json("[{\"engine\":\"authorityRound\",\"block\":0,\"validatorContract\":\"0x1000000000000000000000000000000000000005\",\"validatorList\":[\"0x4ba15b56452521c0826a35a6f2022e1210fc519b\"]}]")

static void initChain(in3_chain_t* chain, uint64_t chainId, char* contract, int boot_node_count, in3_chain_type_t type, json_ctx_t* spec) {
  chain->chainId        = chainId;
  chain->initAddresses  = NULL;
  chain->lastBlock      = 0;
  chain->contract       = hex2byte_new_bytes(contract, 40);
  chain->needsUpdate    = 1;
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
  c->filters            = NULL;

  // mainnet
  initChain(c->chains, 0x01, "2736D225f85740f42D17987100dc8d58e9e16252", 2, CHAIN_ETH, NULL);
  initNode(c->chains, 0, "8f354b72856e516f1e931c97d1ed3bf1709f38c9", "https://in3.slock.it/mainnet/nd-3");
  initNode(c->chains, 1, "243D5BB48A47bEd0F6A89B61E4660540E856A33D", "https://in3.slock.it/mainnet/nd-5");

  // tobalaba
  initChain(c->chains + 1, 0x044d, "845E484b505443814B992Bf0319A5e8F5e407879", 2, CHAIN_ETH, TOBALABA_SPEC);
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

  if (a->filters != NULL) {
    in3_filter_t* f = NULL;
    for (size_t j = 0; j < a->filters->count; j++) {
      f = a->filters->array[j];
      if (f) f->release(f);
    }
    _free(a->filters->array);
    _free(a->filters);
  }
  _free(a);
}

in3_t* in3_new() {
  // initialize random with the timestamp as seed
  _srand(_time());

  // create new client
  in3_t* c = _calloc(1, sizeof(in3_t));
  in3_client_init(c);

#ifndef TEST
  in3_log_set_quiet(1);
#endif
  return c;
}
