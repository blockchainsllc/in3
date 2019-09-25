#include <in3/client.h>   // the core client
#include <in3/eth_api.h>  // wrapper for easier use
#include <in3/eth_full.h> // the full ethereum verifier containing the EVM
#include <in3/in3_curl.h> // transport implementation
#include <inttypes.h>
#include <stdio.h>

int main() {

  // register a chain-verifier for full Ethereum-Support in order to verify eth_call
  // this needs to be called only once.
  in3_register_eth_full();

  // use curl as the default for sending out requests
  // this needs to be called only once.
  in3_register_curl();

  // create new incubed client
  in3_t* c = in3_new();

  // define a address (20byte)
  address_t contract;

  // copy the hexcoded string into this address
  hex2byte_arr("0x2736D225f85740f42D17987100dc8d58e9e16252", -1, contract, 20);

  // ask for the number of servers registered
  json_ctx_t* response = eth_call_fn(c, contract, BLKNUM_LATEST(), "totalServers():uint256");
  if (!response) {
    printf("Could not get the response: %s", eth_last_error());
    return -1;
  }

  // convert the response to a uint32_t,
  uint32_t number_of_servers = d_int(response->result);

  // clean up resources
  free_json(response);

  // output
  printf("Found %u servers registered : \n", number_of_servers);

  // read all structs ...
  for (uint32_t i = 0; i < number_of_servers; i++) {
    response = eth_call_fn(c, contract, BLKNUM_LATEST(), "servers(uint256):(string,address,uint,uint,uint,address)", to_uint256(i));
    if (!response) {
      printf("Could not get the response: %s", eth_last_error());
      return -1;
    }

    char*    url     = d_get_string_at(response->result, 0); // get the first item of the result (the url)
    bytes_t* owner   = d_get_bytes_at(response->result, 1);  // get the second item of the result (the owner)
    uint64_t deposit = d_get_long_at(response->result, 2);   // get the third item of the result (the deposit)

    printf("Server %i : %s owner = %02x%02x...", i, url, owner->data[0], owner->data[1]);
    printf(", deposit = %" PRIu64 "\n", deposit);

    // free memory
    free_json(response);
  }

  // clean up
  in3_free(c);
  return 0;
}