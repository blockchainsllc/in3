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
*/
const unsigned char DEFAULT_CFG_JSON_BIN[] = {
    0xd0, 0x6f, 0xf7, 0x9c, 0x81, 0x24, 0x40, 0xa1, 0x32, 0x27, 0xa0, 0x88,
    0x08, 0xa0, 0x20, 0xd1, 0x80, 0xb6, 0xc5, 0x81, 0x66, 0xca, 0xa7, 0xbd,
    0x8f, 0x80, 0x40, 0x61, 0x81, 0x56, 0xcc, 0x80, 0x02, 0x8c, 0x80, 0x67,
    0xdb, 0x80, 0x0c, 0x76, 0xa5, 0xbc, 0xfb, 0xbd, 0x27, 0x10, 0x4e, 0xe4,
    0x28, 0x73, 0x74, 0x61, 0x6e, 0x64, 0x61, 0x72, 0x64, 0x00};
const unsigned int DEFAULT_CFG_JSON_BIN_LEN = 58;
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
*/
const unsigned char DEV_CFG_JSON_BIN[] = {
    0xd0, 0x6f, 0xf7, 0x9c, 0x81, 0x24, 0x40, 0xa1, 0x32, 0x27, 0xa0, 0x88,
    0x08, 0xa0, 0x20, 0xd1, 0x80, 0xb6, 0xc5, 0x81, 0x66, 0xca, 0xa7, 0xbd,
    0x8f, 0x80, 0x40, 0x61, 0x81, 0x56, 0xcc, 0x80, 0x02, 0x8c, 0x80, 0x67,
    0xdb, 0x80, 0x0c, 0x76, 0xa5, 0xbc, 0xfb, 0xbd, 0x27, 0x10, 0x4e, 0xe4,
    0x28, 0x73, 0x74, 0x61, 0x6e, 0x64, 0x61, 0x72, 0x64, 0x00};
const unsigned int DEV_CFG_JSON_BIN_LEN = 58;
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
*/
const unsigned char TEST_CFG_JSON_BIN[] = {
    0xd0, 0x6f, 0xf7, 0x9c, 0x81, 0x24, 0x40, 0xa1, 0x32, 0x27, 0xa0, 0x88,
    0x08, 0xa0, 0x20, 0xd1, 0x80, 0xb6, 0xc5, 0x81, 0x66, 0xca, 0xa7, 0xbd,
    0x8f, 0x80, 0x40, 0x61, 0x81, 0x56, 0xcc, 0x80, 0x02, 0x8c, 0x80, 0x67,
    0xdb, 0x80, 0x0c, 0x76, 0xa5, 0xbc, 0xfb, 0xbd, 0x27, 0x10, 0x4e, 0xe4,
    0x28, 0x73, 0x74, 0x61, 0x6e, 0x64, 0x61, 0x72, 0x64, 0x00};
const unsigned int TEST_CFG_JSON_BIN_LEN = 58;
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
*/
const unsigned char PROD_CFG_JSON_BIN[] = {
    0xd0, 0x6f, 0xf7, 0x9c, 0x81, 0x24, 0x40, 0xa1, 0x32, 0x27, 0xa0, 0x88,
    0x08, 0xa0, 0x20, 0xd1, 0x80, 0xb6, 0xc5, 0x81, 0x66, 0xca, 0xa7, 0xbd,
    0x8f, 0x80, 0x40, 0x61, 0x81, 0x56, 0xcc, 0x80, 0x02, 0x8c, 0x80, 0x67,
    0xdb, 0x80, 0x0c, 0x76, 0xa5, 0xbc, 0xfb, 0xbd, 0x27, 0x10, 0x4e, 0xe4,
    0x28, 0x73, 0x74, 0x61, 0x6e, 0x64, 0x61, 0x72, 0x64, 0x00};
const unsigned int PROD_CFG_JSON_BIN_LEN = 58;

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
