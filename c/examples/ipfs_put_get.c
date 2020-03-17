///  using the IPFS module

#include <in3/client.h>   // the core client
#include <in3/in3_init.h> // if included the verifier will automaticly be initialized.
#include <in3/ipfs_api.h> // access ipfs-api
#include <in3/log.h>      // logging functions
#include <stdio.h>

#define LOREM_IPSUM "Lorem ipsum dolor sit amet"
#define return_err(err)                                \
  do {                                                 \
    printf(__FILE__ ":%d::Error %s\n", __LINE__, err); \
    return;                                            \
  } while (0)

static void ipfs_rpc_example(in3_t* c) {
  char *result, *error;
  char  tmp[100];

  in3_ret_t res = in3_client_rpc(
      c,
      "ipfs_put",
      "[\"" LOREM_IPSUM "\", \"utf8\"]",
      &result,
      &error);
  if (res != IN3_OK)
    return_err(in3_errmsg(res));

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
    return_err(in3_errmsg(res));
  res = strcmp(result, "\"" LOREM_IPSUM "\"");
  if (res) return_err("Content mismatch");
}

static void ipfs_api_example(in3_t* c) {
  bytes_t b         = {.data = (uint8_t*) LOREM_IPSUM, .len = strlen(LOREM_IPSUM)};
  char*   multihash = ipfs_put(c, &b);
  if (multihash == NULL)
    return_err("ipfs_put API call error");
  printf("IPFS hash: %s\n", multihash);

  bytes_t* content = ipfs_get(c, multihash);
  free(multihash);
  if (content == NULL)
    return_err("ipfs_get API call error");

  int res = strncmp((char*) content->data, LOREM_IPSUM, content->len);
  b_free(content);
  if (res)
    return_err("Content mismatch");
}

int main() {
  // create new incubed client
  in3_t* c = in3_for_chain(ETH_CHAIN_ID_IPFS);

  // IPFS put/get using raw RPC calls
  ipfs_rpc_example(c);

  // IPFS put/get using API
  ipfs_api_example(c);

  // cleanup client after usage
  in3_free(c);
  return 0;
}
