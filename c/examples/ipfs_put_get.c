///  using the IPFS module

#include <in3/client.h>   // the core client
#include <in3/in3_curl.h> // transport implementation
#include <in3/ipfs.h>     // IPFS verifier
#include <stdio.h>

#define LOREM_IPSUM "Lorem ipsum dolor sit amet"

int main() {
  in3_register_ipfs();
  in3_register_curl();

  in3_t* c = in3_for_chain(ETH_CHAIN_ID_IPFS);
  char * result, *error;
  char   tmp[100];

  in3_ret_t res = in3_client_rpc(
      c,
      "ipfs_put",
      "[\"" LOREM_IPSUM "\", \"utf8\"]",
      &result,
      &error);
  if (res != IN3_OK)
    return -1;

  printf("IPFS hash: %s\n", result);
  sprintf(tmp, "[%s, \"utf8\"]", result);
  free(result);
  result = NULL;

  res = in3_client_rpc(
      c,
      "ipfs_get",
      tmp,
      &result,
      &error);
  if (res != IN3_OK)
    return -1;

  return strcmp(result, "\"" LOREM_IPSUM "\"");
}