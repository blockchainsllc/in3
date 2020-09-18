#ifndef IN3_NODE_SELECT_DEF_CFG_H
#define IN3_NODE_SELECT_DEF_CFG_H

#include "../core/client/client.h"

#define BOOT_NODES_MAINNET "{"                                                                                             \
                           " \"servers\": {"                                                                               \
                           "  \"" STR(CHAIN_ID_MAINNET) "\": {"                                                            \
                                                        "   \"nodeList\": [{"                                              \
                                                        "    \"address\": \"0x45d45e6ff99e6c34a235d263965910298985fcfe\"," \
                                                        "    \"url\": \"https://in3-v2.slock.it/mainnet/nd-1\","           \
                                                        "    \"props\": \"0xFFFF\""                                        \
                                                        "   }, {"                                                          \
                                                        "    \"address\": \"0x1fe2e9bf29aa1938859af64c413361227d04059a\"," \
                                                        "    \"url\": \"https://in3-v2.slock.it/mainnet/nd-2\","           \
                                                        "    \"props\": \"0xFFFF\""                                        \
                                                        "   }, {"                                                          \
                                                        "    \"address\": \"0x0cea2ff03adcfa047e8f54f98d41d9147c3ccd4d\"," \
                                                        "    \"url\": \"https://in3-v2.slock.it/mainnet/nd-3\","           \
                                                        "    \"props\": \"0xFFFF\""                                        \
                                                        "   }, {"                                                          \
                                                        "    \"address\": \"0xccd12a2222995e62eca64426989c2688d828aa47\"," \
                                                        "    \"url\": \"https://in3-v2.slock.it/mainnet/nd-4\","           \
                                                        "    \"props\": \"0xFFFF\""                                        \
                                                        "   }, {"                                                          \
                                                        "    \"address\": \"0x510ee7f6f198e018e3529164da2473a96eeb3dc8\"," \
                                                        "    \"url\": \"https://in3-v2.slock.it/mainnet/nd-5\","           \
                                                        "    \"props\": \"0xFFFF\""                                        \
                                                        "   }]"                                                            \
                                                        "  }"                                                              \
                                                        " }"                                                               \
                                                        "}"

#ifdef IN3_STAGING
#define BOOT_NODES_KOVAN "{"                                                                                           \
                         " \"servers\": {"                                                                             \
                         "  \"" STR(CHAIN_ID_KOVAN) "\": {"                                                            \
                                                    "   \"nodeList\": [{"                                              \
                                                    "    \"address\": \"0x784bfa9eb182c3a02dbeb5285e3dba92d717e07a\"," \
                                                    "    \"url\": \"https://in3.stage.slock.it/kovan/nd-1\","          \
                                                    "    \"props\": \"0xFFFF\""                                        \
                                                    "   }, {"                                                          \
                                                    "    \"address\": \"0x17cdf9ec6dcae05c5686265638647e54b14b41a2\"," \
                                                    "    \"url\": \"https://in3.stage.slock.it/kovan/nd-2\","          \
                                                    "    \"props\": \"0xFFFF\""                                        \
                                                    "   }]"                                                            \
                                                    "  }"                                                              \
                                                    " }"                                                               \
                                                    "}"
#define BOOT_NODES_GOERLI "{"                                                                                            \
                          " \"servers\": {"                                                                              \
                          "  \"" STR(CHAIN_ID_GOERLI) "\": {"                                                            \
                                                      "   \"nodeList\": [{"                                              \
                                                      "    \"address\": \"0x784bfa9eb182c3a02dbeb5285e3dba92d717e07a\"," \
                                                      "    \"url\": \"https://in3.stage.slock.it/goerli/nd-1\","         \
                                                      "    \"props\": \"0xFFFF\""                                        \
                                                      "   }, {"                                                          \
                                                      "    \"address\": \"0x17cdf9ec6dcae05c5686265638647e54b14b41a2\"," \
                                                      "    \"url\": \"https://in3.stage.slock.it/goerli/nd-2\","         \
                                                      "    \"props\": \"0xFFFF\""                                        \
                                                      "   }]"                                                            \
                                                      "  }"                                                              \
                                                      " }"                                                               \
                                                      "}"
#else /* IN3_STAGING */
#define BOOT_NODES_KOVAN "{"                                                                                           \
                         " \"servers\": {"                                                                             \
                         "  \"" STR(CHAIN_ID_KOVAN) "\": {"                                                            \
                                                    "   \"nodeList\": [{"                                              \
                                                    "    \"address\": \"0x45d45e6ff99e6c34a235d263965910298985fcfe\"," \
                                                    "    \"url\": \"https://in3-v2.slock.it/kovan/nd-1\","             \
                                                    "    \"props\": \"0xFFFF\""                                        \
                                                    "   }, {"                                                          \
                                                    "    \"address\": \"0x1fe2e9bf29aa1938859af64c413361227d04059a\"," \
                                                    "    \"url\": \"https://in3-v2.slock.it/kovan/nd-2\","             \
                                                    "    \"props\": \"0xFFFF\""                                        \
                                                    "   }]"                                                            \
                                                    "  }"                                                              \
                                                    " }"                                                               \
                                                    "}"

#define BOOT_NODES_GOERLI "{"                                                                                            \
                          " \"servers\": {"                                                                              \
                          "  \"" STR(CHAIN_ID_GOERLI) "\": {"                                                            \
                                                      "   \"nodeList\": [{"                                              \
                                                      "    \"address\": \"0x45d45e6ff99e6c34a235d263965910298985fcfe\"," \
                                                      "    \"url\": \"https://in3-v2.slock.it/goerli/nd-1\","            \
                                                      "    \"props\": \"0xFFFF\""                                        \
                                                      "   }, {"                                                          \
                                                      "    \"address\": \"0x1fe2e9bf29aa1938859af64c413361227d04059a\"," \
                                                      "    \"url\": \"https://in3-v2.slock.it/goerli/nd-2\","            \
                                                      "    \"props\": \"0xFFFF\""                                        \
                                                      "   }]"                                                            \
                                                      "  }"                                                              \
                                                      " }"                                                               \
                                                      "}"

#define BOOT_NODES_IPFS "{"                                                                                          \
                        " \"servers\": {"                                                                            \
                        "  \"" STR(CHAIN_ID_IPFS) "\": {"                                                            \
                                                  "   \"nodeList\": [{"                                              \
                                                  "    \"address\": \"0x45d45e6ff99e6c34a235d263965910298985fcfe\"," \
                                                  "    \"url\": \"https://in3-v2.slock.it/ipfs/nd-1\","              \
                                                  "    \"props\": \"0xFFFF\""                                        \
                                                  "   }, {"                                                          \
                                                  "    \"address\": \"0x1fe2e9bf29aa1938859af64c413361227d04059a\"," \
                                                  "    \"url\": \"https://in3-v2.slock.it/ipfs/nd-2\","              \
                                                  "    \"props\": \"0xFFFF\""                                        \
                                                  "   }]"                                                            \
                                                  "  }"                                                              \
                                                  " }"                                                               \
                                                  "}"

#define BOOT_NODES_BTC "{"                                                                                         \
                       " \"servers\": {"                                                                           \
                       "  \"" STR(CHAIN_ID_BTC) "\": {"                                                            \
                                                "   \"nodeList\": [{"                                              \
                                                "    \"address\": \"0x45d45e6ff99e6c34a235d263965910298985fcfe\"," \
                                                "    \"url\": \"https://in3-v2.slock.it/btc/nd-1\","               \
                                                "    \"props\": \"0xFFFF\""                                        \
                                                "   }, {"                                                          \
                                                "    \"address\": \"0x1fe2e9bf29aa1938859af64c413361227d04059a\"," \
                                                "    \"url\": \"https://in3-v2.slock.it/btc/nd-2\","               \
                                                "    \"props\": \"0xFFFF\""                                        \
                                                "   }]"                                                            \
                                                "  }"                                                              \
                                                " }"                                                               \
                                                "}"

#define BOOT_NODES_EWC "{"                                                                                         \
                       " \"servers\": {"                                                                           \
                       "  \"" STR(CHAIN_ID_EWC) "\": {"                                                            \
                                                "   \"nodeList\": [{"                                              \
                                                "    \"address\": \"0x45d45e6ff99e6c34a235d263965910298985fcfe\"," \
                                                "    \"url\": \"https://in3-v2.slock.it/ewc/nd-1\","               \
                                                "    \"props\": \"0xFFFF\""                                        \
                                                "   }, {"                                                          \
                                                "    \"address\": \"0x1fe2e9bf29aa1938859af64c413361227d04059a\"," \
                                                "    \"url\": \"https://in3-v2.slock.it/ewc/nd-2\","               \
                                                "    \"props\": \"0xFFFF\""                                        \
                                                "   }]"                                                            \
                                                "  }"                                                              \
                                                " }"                                                               \
                                                "}"

#define BOOT_NODES_LOCAL "{"                                                                                           \
                         " \"servers\": {"                                                                             \
                         "  \"" STR(CHAIN_ID_LOCAL) "\": {"                                                            \
                                                    "   \"nodeList\": [{"                                              \
                                                    "    \"address\": \"0x784bfa9eb182c3a02dbeb5285e3dba92d717e07a\"," \
                                                    "    \"url\": \"http://localhost:8545\","                          \
                                                    "    \"props\": \"0xFFFF\""                                        \
                                                    "   }]"                                                            \
                                                    "  }"                                                              \
                                                    " }"                                                               \
                                                    "}"
#endif /* IN3_STAGING */

static inline json_ctx_t* nodeselect_def_cfg(chain_id_t chain_id) {
  json_ctx_t* json = NULL;
  if (chain_id == CHAIN_ID_MAINNET)
    json = parse_json(BOOT_NODES_MAINNET);
  else if (chain_id == CHAIN_ID_KOVAN)
    json = parse_json(BOOT_NODES_KOVAN);
  else if (chain_id == CHAIN_ID_GOERLI)
    json = parse_json(BOOT_NODES_GOERLI);
  else if (chain_id == CHAIN_ID_IPFS)
    json = parse_json(BOOT_NODES_IPFS);
  else if (chain_id == CHAIN_ID_BTC)
    json = parse_json(BOOT_NODES_BTC);
  else if (chain_id == CHAIN_ID_EWC)
    json = parse_json(BOOT_NODES_EWC);
  else if (chain_id == CHAIN_ID_LOCAL)
    json = parse_json(BOOT_NODES_LOCAL);
  return json;
}

#endif //IN3_NODE_SELECT_DEF_CFG_H
