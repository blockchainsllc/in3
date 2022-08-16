#ifndef IN3_NODE_SELECT_DEF_CFG_H
#define IN3_NODE_SELECT_DEF_CFG_H

#define JSON_TO_BIN(x) x // marker for pre-build stage

#include "../../core/client/client.h"

#ifndef NO_BOOT_NODES_MAINNET
/*
#define BOOT_NODES_MAINNET JSON_TO_BIN("{"                                                                                          \
                                       " \"nodeRegistry\": {"                                                                       \
                                       "   \"contract\": \"0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f\","                           \
                                       "   \"registryId\": \"0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb\"," \
                                       "   \"nodeList\": [{"                                                                        \
                                       "    \"address\": \"0x45d45e6ff99e6c34a235d263965910298985fcfe\","                           \
                                       "    \"url\": \"https://in3-v2.slock.it/mainnet/nd-1\","                                     \
                                       "    \"props\": \"0xFFFF\""                                                                  \
                                       "   }, {"                                                                                    \
                                       "    \"address\": \"0x2e333ec090f1028df0a3c39a918063443be82b2b\","                           \
                                       "    \"url\": \"https://mainnet.incubed.net\","                                     \
                                       "    \"props\": \"0xFFFF\""                                                                  \
                                       "   }, {"                                                                                    \
                                       "    \"address\": \"0x510ee7f6f198e018e3529164da2473a96eeb3dc8\","                           \
                                       "    \"url\": \"https://0001.mainnet.in3.anyblock.tools\","                                     \
                                       "    \"props\": \"0xFFFF\""                                                                  \
                                       "   }, {"                                                                                    \
                                       "    \"address\": \"0xc513a534de5a9d3f413152c41b09bd8116237fc8\","                           \
                                       "    \"url\": \"https://in3-v2.slock.it/mainnet/nd-4\","                                     \
                                       "    \"props\": \"0xFFFF\""                                                                  \
                                       "   }, {"                                                                                    \
                                       "    \"address\": \"0xbcdf4e3e90cc7288b578329efd7bcc90655148d2\","                           \
                                       "    \"url\": \"https://in3-v2.slock.it/mainnet/nd-5\","                                     \
                                       "    \"props\": \"0xFFFF\""                                                                  \
                                       "   }]"                                                                                      \
                                       " }"                                                                                         \
                                       "}")

#endif // NO_BOOT_NODES_MAINNET

#ifdef IN3_STAGING

#ifndef NO_BOOT_NODES_GOERLI
/*
#define BOOT_NODES_GOERLI JSON_TO_BIN("{"                                                                                          \
                                      " \"nodeRegistry\": {"                                                                       \
                                      "   \"contract\": \"0x814fb2203f9848192307092337340dcf791a3fed\","                           \
                                      "   \"registryId\": \"0x0f687341e0823fa5288dc9edd8a00950b35cc7e481ad7eaccaf61e4e04a61e08\"," \
                                      "   \"nodeList\": [{"                                                                        \
                                      "    \"address\": \"0x784bfa9eb182c3a02dbeb5285e3dba92d717e07a\","                           \
                                      "    \"url\": \"https://in3.stage.slock.it/goerli/nd-1\","                                   \
                                      "    \"props\": \"0xFFFF\""                                                                  \
                                      "   }, {"                                                                                    \
                                      "    \"address\": \"0x17cdf9ec6dcae05c5686265638647e54b14b41a2\","                           \
                                      "    \"url\": \"https://in3.stage.slock.it/goerli/nd-2\","                                   \
                                      "    \"props\": \"0xFFFF\""                                                                  \
                                      "   }]"                                                                                      \
                                      " }"                                                                                         \
                                      "}")

#endif // NO_BOOT_NODES_GOERLI

#else /* IN3_STAGING */

#ifndef NO_BOOT_NODES_GOERLI
/*
#define BOOT_NODES_GOERLI JSON_TO_BIN("{"                                                                                          \
                                      " \"nodeRegistry\": {"                                                                       \
                                      "   \"contract\": \"0x5f51e413581dd76759e9eed51e63d14c8d1379c8\","                           \
                                      "   \"registryId\": \"0x67c02e5e272f9d6b4a33716614061dd298283f86351079ef903bf0d4410a44ea\"," \
                                      "   \"nodeList\": [{"                                                                        \
                                      "    \"address\": \"0x45d45e6ff99e6c34a235d263965910298985fcfe\","                           \
                                      "    \"url\": \"https://in3-v2.slock.it/goerli/nd-1\","                                      \
                                      "    \"props\": \"0xFFFF\""                                                                  \
                                      "   }, {"                                                                                    \
                                      "    \"address\": \"0x1fe2e9bf29aa1938859af64c413361227d04059a\","                           \
                                      "    \"url\": \"https://in3-v2.slock.it/goerli/nd-2\","                                      \
                                      "    \"props\": \"0xFFFF\""                                                                  \
                                      "   }, {"                                                                                    \
                                      "    \"address\": \"0x7c1f2b5c1e2fe8300f3e0865818faf43ad22d99d\","                           \
                                      "    \"url\": \"https://in3.jentzschfamily.de\","                                      \
                                      "    \"props\": \"0xFFFF\""                                                                  \
                                      "   }]"                                                                                      \
                                      " }"                                                                                         \
                                      "}")


#endif // NO_BOOT_NODES_GOERLI

#ifndef NO_BOOT_NODES_MUMBAI
/*
#define BOOT_NODES_MUMBAI JSON_TO_BIN("{"                                                                                          \
                                      " \"nodeRegistry\": {"                                                                       \
                                      "   \"contract\": \"0x41a07f9b9bd9c972fa56a20d0e7ed68ec1559f0b\","                           \
                                      "   \"registryId\": \"0xb3e3826c9ef619acac52771e4f3015e5b03ca37d7db0c8615bfd9e47cf94c9b6\"," \
                                      "   \"nodeList\": [{"                                                                        \
                                      "    \"address\": \"0x45d45e6ff99e6c34a235d263965910298985fcfe\","                           \
                                      "    \"url\": \"https://in3-v2.slock.it/mumbai/nd-1","                                      \
                                      "    \"props\": \"0xFFFF\""                                                                  \
                                      "   }, {"                                                                                    \
                                      "    \"address\": \"0x1fe2e9bf29aa1938859af64c413361227d04059a\","                           \
                                      "    \"url\": \"https://in3-v2.slock.it/mumbai/nd-2","                                      \
                                      "    \"props\": \"0xFFFF\""                                                                  \
                                      "   }, {"                                                                                    \
                                      "    \"address\": \"0x0f72a0b5ff2fd40e415138c7bf90e4822597327b\","                           \
                                      "    \"url\": \"https://mumbai1.incubed.net\","                                      \
                                      "    \"props\": \"0xFFFF\""                                                                  \
                                      "   }]"                                                                                      \
                                      " }"                                                                                         \
                                      "}")


#endif // NO_BOOT_NODES_GOERLI

#ifndef NO_BOOT_NODES_IPFS

/*
#define BOOT_NODES_IPFS JSON_TO_BIN("{"                                                                                          \
                                    " \"nodeRegistry\": {"                                                                       \
                                    "   \"contract\": \"0xa93b57289070550c82edb1106e12bb37138948b8\","                           \
                                    "   \"registryId\": \"0xf0162ec6d785ee990e36bad865251f45af0916cf136169540c02b0dd9cb69196\"," \
                                    "   \"nodeList\": [{"                                                                        \
                                    "    \"address\": \"0x45d45e6ff99e6c34a235d263965910298985fcfe\","                           \
                                    "    \"url\": \"https://in3-v2.slock.it/ipfs/nd-1\","                                        \
                                    "    \"props\": \"0xFFFF\""                                                                  \
                                    "   }, {"                                                                                    \
                                    "    \"address\": \"0x1fe2e9bf29aa1938859af64c413361227d04059a\","                           \
                                    "    \"url\": \"https://in3-v2.slock.it/ipfs/nd-2\","                                        \
                                    "    \"props\": \"0xFFFF\""                                                                  \
                                    "   }]"                                                                                      \
                                    " }"                                                                                         \
                                    "}")


#endif // NO_BOOT_NODES_IPFS

#ifndef NO_BOOT_NODES_BTC
/*
#define BOOT_NODES_BTC JSON_TO_BIN("{"                                                                                          \
                                   " \"nodeRegistry\": {"                                                                       \
                                   "   \"contract\": \"0xc2c05fbfe76ee7748ae5f5b61b57a46cc4061c32\","                           \
                                   "   \"registryId\": \"0x53786c93e54c21d9852d093c394eee9df8d714d8f2534cdf92f9c9998c528d19\"," \
                                   "   \"nodeList\": [{"                                                                        \
                                   "    \"address\": \"0x45d45e6ff99e6c34a235d263965910298985fcfe\","                           \
                                   "    \"url\": \"https://in3-v2.slock.it/btc/nd-1\","                                         \
                                   "    \"props\": \"0xFFFF\""                                                                  \
                                   "   }, {"                                                                                    \
                                   "    \"address\": \"0x1fe2e9bf29aa1938859af64c413361227d04059a\","                           \
                                   "    \"url\": \"https://in3-v2.slock.it/btc/nd-2\","                                         \
                                   "    \"props\": \"0xFFFF\""                                                                  \
                                   "   }]"                                                                                      \
                                   " }"                                                                                         \
                                   "}")


#endif // NO_BOOT_NODES_BTC

#ifndef NO_BOOT_NODES_EWC
/*
#define BOOT_NODES_EWC JSON_TO_BIN("{"                                                                                          \
                                   " \"nodeRegistry\": {"                                                                       \
                                   "   \"contract\": \"0x039562872008f7a76674a6e7842804f0ad37cb13\","                           \
                                   "   \"registryId\": \"0x313454c05fc6e5336a3315ed2233da6b831d4cb826d836c3d603f2e2a9f1ed75\"," \
                                   "   \"nodeList\": [{"                                                                        \
                                   "    \"address\": \"0x45d45e6ff99e6c34a235d263965910298985fcfe\","                           \
                                   "    \"url\": \"https://in3-v2.slock.it/ewc/nd-1\","                                         \
                                   "    \"props\": \"0xFFFF\""                                                                  \
                                   "   }, {"                                                                                    \
                                   "    \"address\": \"0x1fe2e9bf29aa1938859af64c413361227d04059a\","                           \
                                   "    \"url\": \"https://in3-v2.slock.it/ewc/nd-2\","                                         \
                                   "    \"props\": \"0xFFFF\""                                                                  \
                                   "   }]"                                                                                      \
                                   " }"                                                                                         \
                                   "}")


#endif // NO_BOOT_NODES_EWC

#ifndef NO_BOOT_NODES_LOCAL
/*
#define BOOT_NODES_LOCAL JSON_TO_BIN("{"                                                                \
                                     " \"nodeRegistry\": {"                                             \
                                     "   \"contract\": \"0xf0fb87f4757c77ea3416afe87f36acaa0496c7e9\"," \
                                     "   \"nodeList\": [{"                                              \
                                     "    \"address\": \"0x784bfa9eb182c3a02dbeb5285e3dba92d717e07a\"," \
                                     "    \"url\": \"http://localhost:8545\","                          \
                                     "    \"props\": \"0xFFFF\""                                        \
                                     "   }]"                                                            \
                                     " }"                                                               \
                                     "}")

#endif // NO_BOOT_NODES_LOCAL

#endif /* IN3_STAGING */

static inline bytes_t nodeselect_def_cfg_data(chain_id_t chain_id) {
#ifndef NO_BOOT_NODES_MAINNET
  if (chain_id == CHAIN_ID_MAINNET)
    return bytes((uint8_t*) BOOT_NODES_MAINNET_BIN, BOOT_NODES_MAINNET_BIN_LEN);
#endif // NO_BOOT_NODES_MAINNET

#ifndef NO_BOOT_NODES_GOERLI
  if (chain_id == CHAIN_ID_GOERLI)
    return bytes((uint8_t*) BOOT_NODES_GOERLI_BIN, BOOT_NODES_GOERLI_BIN_LEN);
#endif // NO_BOOT_NODES_GOERLI

#ifndef NO_BOOT_NODES_MUMBAI
  if (chain_id == CHAIN_ID_MUMBAI)
    return bytes((uint8_t*) BOOT_NODES_MUMBAI_BIN, BOOT_NODES_MUMBAI_BIN_LEN);
#endif // NO_BOOT_NODES_MUMBAI

#ifndef NO_BOOT_NODES_IPFS
  if (chain_id == CHAIN_ID_IPFS)
    return bytes((uint8_t*) BOOT_NODES_IPFS_BIN, BOOT_NODES_IPFS_BIN_LEN);
#endif // NO_BOOT_NODES_IPFS

#ifndef NO_BOOT_NODES_BTC
  if (chain_id == CHAIN_ID_BTC)
    return bytes((uint8_t*) BOOT_NODES_BTC_BIN, BOOT_NODES_BTC_BIN_LEN);
#endif // NO_BOOT_NODES_BTC

#ifndef NO_BOOT_NODES_EWC
  if (chain_id == CHAIN_ID_EWC)
    return bytes((uint8_t*) BOOT_NODES_EWC_BIN, BOOT_NODES_EWC_BIN_LEN);
#endif // NO_BOOT_NODES_EWC

#ifndef NO_BOOT_NODES_LOCAL
  if (chain_id == CHAIN_ID_LOCAL)
    return bytes((uint8_t*) BOOT_NODES_LOCAL_BIN, BOOT_NODES_LOCAL_BIN_LEN);
#endif // NO_BOOT_NODES_LOCAL

  return NULL_BYTES;
}

static inline json_ctx_t* nodeselect_def_cfg(chain_id_t chain_id) {
  bytes_t bincfg = nodeselect_def_cfg_data(chain_id);
  return bincfg.data ? parse_binary(&bincfg) : NULL;
}

#endif // IN3_NODE_SELECT_DEF_CFG_H
