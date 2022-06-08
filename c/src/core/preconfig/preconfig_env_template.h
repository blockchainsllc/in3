#ifndef PRE_CONFIG_TEMPLATE_H
#define PRE_CONFIG_TEMPLATE_H

#define JSON_TO_BIN(x) x // marker for pre-build stage

#include "../client/client.h"

#ifdef IN3_PRE_CFG
/*
#define DEFAULT_CFG_JSON JSON_TO_BIN("{"                        \
                                     "\"autoUpdateList\":true," \
                                     "\"chainId\":1,"           \
                                     "\"signatureCount\":0,"    \
                                     "\"finality\":0,"          \
                                     "\"includeCode\":false,"   \
                                     "\"bootWeights\":true,"    \
                                     "\"maxAttempts\":7,"       \
                                     "\"keepIn3\":false,"       \
                                     "\"stats\":true,"          \
                                     "\"useBinary\":false,"     \
                                     "\"experimental\":false,"  \
                                     "\"useHttp\":false,"       \
                                     "\"maxVerifiedHashes\":5," \
                                     "\"timeout\":10000,"       \
                                     "\"proof\":\"standard\""   \
                                     "}")
/*
#define DEV_CFG_JSON JSON_TO_BIN("{"                        \
                                 "\"autoUpdateList\":true," \
                                 "\"chainId\":1,"           \
                                 "\"signatureCount\":0,"    \
                                 "\"finality\":0,"          \
                                 "\"includeCode\":false,"   \
                                 "\"bootWeights\":true,"    \
                                 "\"maxAttempts\":7,"       \
                                 "\"keepIn3\":false,"       \
                                 "\"stats\":true,"          \
                                 "\"useBinary\":false,"     \
                                 "\"experimental\":false,"  \
                                 "\"useHttp\":false,"       \
                                 "\"maxVerifiedHashes\":5," \
                                 "\"timeout\":10000,"       \
                                 "\"proof\":\"standard\""   \
                                 "}")
/*
#define TEST_CFG_JSON JSON_TO_BIN("{"                        \
                                  "\"autoUpdateList\":true," \
                                  "\"chainId\":1,"           \
                                  "\"signatureCount\":0,"    \
                                  "\"finality\":0,"          \
                                  "\"includeCode\":false,"   \
                                  "\"bootWeights\":true,"    \
                                  "\"maxAttempts\":7,"       \
                                  "\"keepIn3\":false,"       \
                                  "\"stats\":true,"          \
                                  "\"useBinary\":false,"     \
                                  "\"experimental\":false,"  \
                                  "\"useHttp\":false,"       \
                                  "\"maxVerifiedHashes\":5," \
                                  "\"timeout\":10000,"       \
                                  "\"proof\":\"standard\""   \
                                  "}")
/*
#define PROD_CFG_JSON JSON_TO_BIN("{"                        \
                                  "\"autoUpdateList\":true," \
                                  "\"chainId\":1,"           \
                                  "\"signatureCount\":0,"    \
                                  "\"finality\":0,"          \
                                  "\"includeCode\":false,"   \
                                  "\"bootWeights\":true,"    \
                                  "\"maxAttempts\":7,"       \
                                  "\"keepIn3\":false,"       \
                                  "\"stats\":true,"          \
                                  "\"useBinary\":false,"     \
                                  "\"experimental\":false,"  \
                                  "\"useHttp\":false,"       \
                                  "\"maxVerifiedHashes\":5," \
                                  "\"timeout\":10000,"       \
                                  "\"proof\":\"standard\""   \
                                  "}")

static inline json_ctx_t*
in3_get_preconfig(char* env) {
  if (strcmp(env, "default") == 0) {
    bytes_t bincfg = bytes((unsigned char*) DEFAULT_CFG_JSON_BIN, DEFAULT_CFG_JSON_BIN_LEN);
    return parse_binary(&bincfg);
  }
  else if (strcmp(env, "dev") == 0) {
    bytes_t bincfg = bytes((unsigned char*) DEV_CFG_JSON_BIN, DEV_CFG_JSON_BIN_LEN);
    return parse_binary(&bincfg);
  }
  else if (strcmp(env, "test") == 0) {
    bytes_t bincfg = bytes((unsigned char*) TEST_CFG_JSON_BIN, TEST_CFG_JSON_BIN_LEN);
    return parse_binary(&bincfg);
  }
  else if (strcmp(env, "prod") == 0) {
    bytes_t bincfg = bytes((unsigned char*) PROD_CFG_JSON_BIN, PROD_CFG_JSON_BIN_LEN);
    return parse_binary(&bincfg);
  }
  return NULL;
}
#endif // IN3_PRE_CFG
#endif // PRE_CONFIG_TEMPLATE_H