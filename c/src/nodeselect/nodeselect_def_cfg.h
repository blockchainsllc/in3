#ifndef IN3_NODE_SELECT_DEF_CFG_H
#define IN3_NODE_SELECT_DEF_CFG_H

#include "../../prebuild/prebuild.h"
#include "../core/client/client.h"

// #define BOOT_NODES_MAINNET JSON_TO_BIN("{"                                                                                          \
//                                        " \"servers\": {"                                                                            \
//                                        "  \"0x1\": {"                                                                               \
//                                        "   \"contract\": \"0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f\","                           \
//                                        "   \"registryId\": \"0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb\"," \
//                                        "   \"nodeList\": [{"                                                                        \
//                                        "    \"address\": \"0x45d45e6ff99e6c34a235d263965910298985fcfe\","                           \
//                                        "    \"url\": \"https://in3-v2.slock.it/mainnet/nd-1\","                                     \
//                                        "    \"props\": \"0xFFFF\""                                                                  \
//                                        "   }, {"                                                                                    \
//                                        "    \"address\": \"0x1fe2e9bf29aa1938859af64c413361227d04059a\","                           \
//                                        "    \"url\": \"https://in3-v2.slock.it/mainnet/nd-2\","                                     \
//                                        "    \"props\": \"0xFFFF\""                                                                  \
//                                        "   }, {"                                                                                    \
//                                        "    \"address\": \"0x0cea2ff03adcfa047e8f54f98d41d9147c3ccd4d\","                           \
//                                        "    \"url\": \"https://in3-v2.slock.it/mainnet/nd-3\","                                     \
//                                        "    \"props\": \"0xFFFF\""                                                                  \
//                                        "   }, {"                                                                                    \
//                                        "    \"address\": \"0xccd12a2222995e62eca64426989c2688d828aa47\","                           \
//                                        "    \"url\": \"https://in3-v2.slock.it/mainnet/nd-4\","                                     \
//                                        "    \"props\": \"0xFFFF\""                                                                  \
//                                        "   }, {"                                                                                    \
//                                        "    \"address\": \"0x510ee7f6f198e018e3529164da2473a96eeb3dc8\","                           \
//                                        "    \"url\": \"https://in3-v2.slock.it/mainnet/nd-5\","                                     \
//                                        "    \"props\": \"0xFFFF\""                                                                  \
//                                        "   }]"                                                                                      \
//                                        "  }"                                                                                        \
//                                        " }"                                                                                         \
//                                        "}")
#define BOOT_NODES_MAINNET "da6130f6613c7963ce1214ac1b824795e1eb1f6e609fe0da9b9af8beaab60f6eb81c2023d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb1aa24563b2f61445d45e6ff99e6c34a235d263965910298985fcfe796b3c2468747470733a2f2f696e332d76322e736c6f636b2e69742f6d61696e6e65742f6e642d3100416ebdffff63b2f6141fe2e9bf29aa1938859af64c413361227d04059a796b3c2468747470733a2f2f696e332d76322e736c6f636b2e69742f6d61696e6e65742f6e642d3200416ebdffff63b2f6140cea2ff03adcfa047e8f54f98d41d9147c3ccd4d796b3c2468747470733a2f2f696e332d76322e736c6f636b2e69742f6d61696e6e65742f6e642d3300416ebdffff63b2f614ccd12a2222995e62eca64426989c2688d828aa47796b3c2468747470733a2f2f696e332d76322e736c6f636b2e69742f6d61696e6e65742f6e642d3400416ebdffff63b2f614510ee7f6f198e018e3529164da2473a96eeb3dc8796b3c2468747470733a2f2f696e332d76322e736c6f636b2e69742f6d61696e6e65742f6e642d3500416ebdffff"

#ifdef IN3_STAGING
// #define BOOT_NODES_KOVAN JSON_TO_BIN("{"                                                                                          \
//                                      " \"servers\": {"                                                                            \
//                                      "  \"0x2a\": {"                                                                              \
//                                      "   \"contract\": \"0x0604014f2a5fdfafce3f2ec10c77c31d8e15ce6f\","                           \
//                                      "   \"registryId\": \"0xd440f01322c8529892c204d3705ae871c514bafbb2f35907832a07322e0dc868\"," \
//                                      "   \"nodeList\": [{"                                                                        \
//                                      "    \"address\": \"0x784bfa9eb182c3a02dbeb5285e3dba92d717e07a\","                           \
//                                      "    \"url\": \"https://in3.stage.slock.it/kovan/nd-1\","                                    \
//                                      "    \"props\": \"0xFFFF\""                                                                  \
//                                      "   }, {"                                                                                    \
//                                      "    \"address\": \"0x17cdf9ec6dcae05c5686265638647e54b14b41a2\","                           \
//                                      "    \"url\": \"https://in3.stage.slock.it/kovan/nd-2\","                                    \
//                                      "    \"props\": \"0xFFFF\""                                                                  \
//                                      "   }]"                                                                                      \
//                                      "  }"                                                                                        \
//                                      " }"                                                                                         \
//                                      "}")
#define BOOT_NODES_KOVAN "ce6130f661011b63ce12140604014f2a5fdfafce3f2ec10c77c31d8e15ce6f6eb81c20d440f01322c8529892c204d3705ae871c514bafbb2f35907832a07322e0dc8681aa24263b2f614784bfa9eb182c3a02dbeb5285e3dba92d717e07a796b3c2568747470733a2f2f696e332e73746167652e736c6f636b2e69742f6b6f76616e2f6e642d3100416ebdffff63b2f61417cdf9ec6dcae05c5686265638647e54b14b41a2796b3c2568747470733a2f2f696e332e73746167652e736c6f636b2e69742f6b6f76616e2f6e642d3200416ebdffff"

// #define BOOT_NODES_GOERLI JSON_TO_BIN("{"                                                                                          \
//                                       " \"servers\": {"                                                                            \
//                                       "  \"0x5\": {"                                                                               \
//                                       "   \"contract\": \"0x814fb2203f9848192307092337340dcf791a3fed\","                           \
//                                       "   \"registryId\": \"0x0f687341e0823fa5288dc9edd8a00950b35cc7e481ad7eaccaf61e4e04a61e08\"," \
//                                       "   \"nodeList\": [{"                                                                        \
//                                       "    \"address\": \"0x784bfa9eb182c3a02dbeb5285e3dba92d717e07a\","                           \
//                                       "    \"url\": \"https://in3.stage.slock.it/goerli/nd-1\","                                   \
//                                       "    \"props\": \"0xFFFF\""                                                                  \
//                                       "   }, {"                                                                                    \
//                                       "    \"address\": \"0x17cdf9ec6dcae05c5686265638647e54b14b41a2\","                           \
//                                       "    \"url\": \"https://in3.stage.slock.it/goerli/nd-2\","                                   \
//                                       "    \"props\": \"0xFFFF\""                                                                  \
//                                       "   }]"                                                                                      \
//                                       "  }"                                                                                        \
//                                       " }"                                                                                         \
//                                       "}")
#define BOOT_NODES_GOERLI "ce6130f6613c7d63ce1214814fb2203f9848192307092337340dcf791a3fed6eb81c200f687341e0823fa5288dc9edd8a00950b35cc7e481ad7eaccaf61e4e04a61e081aa24263b2f614784bfa9eb182c3a02dbeb5285e3dba92d717e07a796b3c2668747470733a2f2f696e332e73746167652e736c6f636b2e69742f676f65726c692f6e642d3100416ebdffff63b2f61417cdf9ec6dcae05c5686265638647e54b14b41a2796b3c2668747470733a2f2f696e332e73746167652e736c6f636b2e69742f676f65726c692f6e642d3200416ebdffff"

#else /* IN3_STAGING */

// #define BOOT_NODES_KOVAN JSON_TO_BIN("{"                                                                                          \
//                                      " \"servers\": {"                                                                            \
//                                      "  \"0x2a\": {"                                                                              \
//                                      "   \"contract\": \"0x4c396dcf50ac396e5fdea18163251699b5fcca25\","                           \
//                                      "   \"registryId\": \"0x92eb6ad5ed9068a24c1c85276cd7eb11eda1e8c50b17fbaffaf3e8396df4becf\"," \
//                                      "   \"nodeList\": [{"                                                                        \
//                                      "    \"address\": \"0x45d45e6ff99e6c34a235d263965910298985fcfe\","                           \
//                                      "    \"url\": \"https://in3-v2.slock.it/kovan/nd-1\","                                       \
//                                      "    \"props\": \"0xFFFF\""                                                                  \
//                                      "   }, {"                                                                                    \
//                                      "    \"address\": \"0x1fe2e9bf29aa1938859af64c413361227d04059a\","                           \
//                                      "    \"url\": \"https://in3-v2.slock.it/kovan/nd-2\","                                       \
//                                      "    \"props\": \"0xFFFF\""                                                                  \
//                                      "   }]"                                                                                      \
//                                      "  }"                                                                                        \
//                                      " }"                                                                                         \
//                                      "}")
#define BOOT_NODES_KOVAN "ce6130f661011b63ce12144c396dcf50ac396e5fdea18163251699b5fcca256eb81c2092eb6ad5ed9068a24c1c85276cd7eb11eda1e8c50b17fbaffaf3e8396df4becf1aa24263b2f61445d45e6ff99e6c34a235d263965910298985fcfe796b3c2268747470733a2f2f696e332d76322e736c6f636b2e69742f6b6f76616e2f6e642d3100416ebdffff63b2f6141fe2e9bf29aa1938859af64c413361227d04059a796b3c2268747470733a2f2f696e332d76322e736c6f636b2e69742f6b6f76616e2f6e642d3200416ebdffff"

// #define BOOT_NODES_GOERLI JSON_TO_BIN("{"                                                                                          \
//                                       " \"servers\": {"                                                                            \
//                                       "  \"0x5\": {"                                                                               \
//                                       "   \"contract\": \"0x5f51e413581dd76759e9eed51e63d14c8d1379c8\","                           \
//                                       "   \"registryId\": \"0x67c02e5e272f9d6b4a33716614061dd298283f86351079ef903bf0d4410a44ea\"," \
//                                       "   \"nodeList\": [{"                                                                        \
//                                       "    \"address\": \"0x45d45e6ff99e6c34a235d263965910298985fcfe\","                           \
//                                       "    \"url\": \"https://in3-v2.slock.it/goerli/nd-1\","                                      \
//                                       "    \"props\": \"0xFFFF\""                                                                  \
//                                       "   }, {"                                                                                    \
//                                       "    \"address\": \"0x1fe2e9bf29aa1938859af64c413361227d04059a\","                           \
//                                       "    \"url\": \"https://in3-v2.slock.it/goerli/nd-2\","                                      \
//                                       "    \"props\": \"0xFFFF\""                                                                  \
//                                       "   }]"                                                                                      \
//                                       "  }"                                                                                        \
//                                       " }"                                                                                         \
//                                       "}")
#define BOOT_NODES_GOERLI "ce6130f6613c7d63ce12145f51e413581dd76759e9eed51e63d14c8d1379c86eb81c2067c02e5e272f9d6b4a33716614061dd298283f86351079ef903bf0d4410a44ea1aa24263b2f61445d45e6ff99e6c34a235d263965910298985fcfe796b3c2368747470733a2f2f696e332d76322e736c6f636b2e69742f676f65726c692f6e642d3100416ebdffff63b2f6141fe2e9bf29aa1938859af64c413361227d04059a796b3c2368747470733a2f2f696e332d76322e736c6f636b2e69742f676f65726c692f6e642d3200416ebdffff"

// #define BOOT_NODES_IPFS JSON_TO_BIN("{"                                                                                          \
//                                     " \"servers\": {"                                                                            \
//                                     "  \"0x7d0\": {"                                                                             \
//                                     "   \"contract\": \"0xa93b57289070550c82edb1106e12bb37138948b8\","                           \
//                                     "   \"registryId\": \"0xf0162ec6d785ee990e36bad865251f45af0916cf136169540c02b0dd9cb69196\"," \
//                                     "   \"nodeList\": [{"                                                                        \
//                                     "    \"address\": \"0x45d45e6ff99e6c34a235d263965910298985fcfe\","                           \
//                                     "    \"url\": \"https://in3-v2.slock.it/ipfs/nd-1\","                                        \
//                                     "    \"props\": \"0xFFFF\""                                                                  \
//                                     "   }, {"                                                                                    \
//                                     "    \"address\": \"0x1fe2e9bf29aa1938859af64c413361227d04059a\","                           \
//                                     "    \"url\": \"https://in3-v2.slock.it/ipfs/nd-2\","                                        \
//                                     "    \"props\": \"0xFFFF\""                                                                  \
//                                     "   }]"                                                                                      \
//                                     "  }"                                                                                        \
//                                     " }"                                                                                         \
//                                     "}")
#define BOOT_NODES_IPFS "ce6130f661ce2b63ce1214a93b57289070550c82edb1106e12bb37138948b86eb81c20f0162ec6d785ee990e36bad865251f45af0916cf136169540c02b0dd9cb691961aa24263b2f61445d45e6ff99e6c34a235d263965910298985fcfe796b3c2168747470733a2f2f696e332d76322e736c6f636b2e69742f697066732f6e642d3100416ebdffff63b2f6141fe2e9bf29aa1938859af64c413361227d04059a796b3c2168747470733a2f2f696e332d76322e736c6f636b2e69742f697066732f6e642d3200416ebdffff"

// #define BOOT_NODES_BTC JSON_TO_BIN("{"                                                                                          \
//                                    " \"servers\": {"                                                                            \
//                                    "  \"0x99\": {"                                                                              \
//                                    "   \"contract\": \"0xc2c05fbfe76ee7748ae5f5b61b57a46cc4061c32\","                           \
//                                    "   \"registryId\": \"0x53786c93e54c21d9852d093c394eee9df8d714d8f2534cdf92f9c9998c528d19\"," \
//                                    "   \"nodeList\": [{"                                                                        \
//                                    "    \"address\": \"0x45d45e6ff99e6c34a235d263965910298985fcfe\","                           \
//                                    "    \"url\": \"https://in3-v2.slock.it/btc/nd-1\","                                         \
//                                    "    \"props\": \"0xFFFF\""                                                                  \
//                                    "   }, {"                                                                                    \
//                                    "    \"address\": \"0x1fe2e9bf29aa1938859af64c413361227d04059a\","                           \
//                                    "    \"url\": \"https://in3-v2.slock.it/btc/nd-2\","                                         \
//                                    "    \"props\": \"0xFFFF\""                                                                  \
//                                    "   }]"                                                                                      \
//                                    "  }"                                                                                        \
//                                    " }"                                                                                         \
//                                    "}")
#define BOOT_NODES_BTC "ce6130f66104c863ce1214c2c05fbfe76ee7748ae5f5b61b57a46cc4061c326eb81c2053786c93e54c21d9852d093c394eee9df8d714d8f2534cdf92f9c9998c528d191aa24263b2f61445d45e6ff99e6c34a235d263965910298985fcfe796b3c2068747470733a2f2f696e332d76322e736c6f636b2e69742f6274632f6e642d3100416ebdffff63b2f6141fe2e9bf29aa1938859af64c413361227d04059a796b3c2068747470733a2f2f696e332d76322e736c6f636b2e69742f6274632f6e642d3200416ebdffff"

// #define BOOT_NODES_EWC JSON_TO_BIN("{"                                                                                          \
//                                    " \"servers\": {"                                                                            \
//                                    "  \"0xf6\": {"                                                                              \
//                                    "   \"contract\": \"0x039562872008f7a76674a6e7842804f0ad37cb13\","                           \
//                                    "   \"registryId\": \"0x313454c05fc6e5336a3315ed2233da6b831d4cb826d836c3d603f2e2a9f1ed75\"," \
//                                    "   \"nodeList\": [{"                                                                        \
//                                    "    \"address\": \"0x45d45e6ff99e6c34a235d263965910298985fcfe\","                           \
//                                    "    \"url\": \"https://in3-v2.slock.it/ewc/nd-1\","                                         \
//                                    "    \"props\": \"0xFFFF\""                                                                  \
//                                    "   }, {"                                                                                    \
//                                    "    \"address\": \"0x1fe2e9bf29aa1938859af64c413361227d04059a\","                           \
//                                    "    \"url\": \"https://in3-v2.slock.it/ewc/nd-2\","                                         \
//                                    "    \"props\": \"0xFFFF\""                                                                  \
//                                    "   }]"                                                                                      \
//                                    "  }"                                                                                        \
//                                    " }"                                                                                         \
//                                    "}")
#define BOOT_NODES_EWC "ce6130f6612b1863ce1214039562872008f7a76674a6e7842804f0ad37cb136eb81c20313454c05fc6e5336a3315ed2233da6b831d4cb826d836c3d603f2e2a9f1ed751aa24263b2f61445d45e6ff99e6c34a235d263965910298985fcfe796b3c2068747470733a2f2f696e332d76322e736c6f636b2e69742f6577632f6e642d3100416ebdffff63b2f6141fe2e9bf29aa1938859af64c413361227d04059a796b3c2068747470733a2f2f696e332d76322e736c6f636b2e69742f6577632f6e642d3200416ebdffff"

// #define BOOT_NODES_LOCAL JSON_TO_BIN("{"                                                                \
//                                      " \"servers\": {"                                                  \
//                                      "  \"0x11\": {"                                                    \
//                                      "   \"contract\": \"0xf0fb87f4757c77ea3416afe87f36acaa0496c7e9\"," \
//                                      "   \"nodeList\": [{"                                              \
//                                      "    \"address\": \"0x784bfa9eb182c3a02dbeb5285e3dba92d717e07a\"," \
//                                      "    \"url\": \"http://localhost:8545\","                          \
//                                      "    \"props\": \"0xFFFF\""                                        \
//                                      "   }]"                                                            \
//                                      "  }"                                                              \
//                                      " }"                                                               \
//                                      "}")
#define BOOT_NODES_LOCAL "c96130f66100c862ce1214f0fb87f4757c77ea3416afe87f36acaa0496c7e91aa24163b2f614784bfa9eb182c3a02dbeb5285e3dba92d717e07a796b35687474703a2f2f6c6f63616c686f73743a3835343500416ebdffff"

#endif /* IN3_STAGING */

static inline json_ctx_t* nodeselect_def_cfg(chain_id_t chain_id) {
  char* bincfg = NULL;

  if (chain_id == CHAIN_ID_MAINNET)
    bincfg = BOOT_NODES_MAINNET;
  else if (chain_id == CHAIN_ID_KOVAN)
    bincfg = BOOT_NODES_KOVAN;
  else if (chain_id == CHAIN_ID_GOERLI)
    bincfg = BOOT_NODES_GOERLI;
  else if (chain_id == CHAIN_ID_IPFS)
    bincfg = BOOT_NODES_IPFS;
  else if (chain_id == CHAIN_ID_BTC)
    bincfg = BOOT_NODES_BTC;
  else if (chain_id == CHAIN_ID_EWC)
    bincfg = BOOT_NODES_EWC;
  else if (chain_id == CHAIN_ID_LOCAL)
    bincfg = BOOT_NODES_LOCAL;
  else
    return NULL;

  bytes_t* cfg = hex_to_new_bytes(bincfg, strlen(bincfg));
  return parse_binary(cfg);
}

#endif //IN3_NODE_SELECT_DEF_CFG_H
