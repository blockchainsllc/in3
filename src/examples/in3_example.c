#include <in3/client.h>   // the core client
#include <in3/eth_api.h>  // wrapper for easier use
#include <in3/eth_full.h> // the full ethereum verifier containing the EVM
#include <in3/evm.h>
#include <in3/signer.h>
#include <in3/in3_curl.h> // transport implementation
#include <inttypes.h>
#include <stdio.h>
#include <time.h>
#if defined(_WIN32) || defined(WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

void call_a_function(in3_t* c) {
  // define a address (20byte)
  address_t contract;
  // copy the hexcoded string into this address
  hex2byte_arr("0x845E484b505443814B992Bf0319A5e8F5e407879", -1, contract, 20);

  // ask for the number oif servers registered
  json_ctx_t* response = eth_call_fn(c, contract, "totalServers():uint256");
  if (!response) {
    printf("Could not get the response: %s", eth_last_error());
    return;
  }

  int number_of_servers = d_int(response->result);
  free_json(response);

  printf("Found %i servers registered : \n", number_of_servers);

  for (int i = 0; i < number_of_servers; i++) {

    response = eth_call_fn(c, contract, "servers(uint256):(string,address,uint,uint,uint,address)", to_uint256(i));
    if (!response) {
      printf("Could not get the response: %s", eth_last_error());
      return;
    }

    char*    url     = d_get_string_at(response->result, 0); // get the first item of the result (the url)
    bytes_t* owner   = d_get_bytes_at(response->result, 1);  // get the second item of the result (the owner)
    uint64_t deposit = d_get_long_at(response->result, 2);   // get the third item of the result (the deposit)

    printf("Server %i : %s owner = ", i, url);
    ba_print(owner->data, owner->len);
    printf(", deposit = %" PRIu64 "\n", deposit);

    free_json(response);
  }
}

void show_transactions_in_block(in3_t* c, uint64_t block_number) {
  // use a ethereum-api instead of pure JSON-RPC-Requests
  eth_block_t* block = eth_getBlockByNumber(c, block_number, true);
  if (!block)
    printf("Could not find the Block: %s\n", eth_last_error());
  else {
    printf("Number of verified transactions in block: %i\n", block->tx_count);
    free(block);
  }
}


int main(int argc, char* argv[]) {

  char* example = argc ? argv[1] : NULL;

  // register a chain-verifier for full Ethereum-Support
  in3_register_eth_full();

  // create new incubed client
  in3_t* c = in3_new();

  // uncomment this for more debug output
  //c->evm_flags = 65536;

  // set your config
  c->transport    = send_curl; // use curl to handle the requests
  c->requestCount = 1;         // number of requests to send
  c->chainId      = 0x44d;     // use tobalaba
  c->evm_flags |= EVM_PROP_DEBUG;

  // example 1 - getBlock
  if (example == NULL || strcmp(example, "eth_getBlock") == 0)
    show_transactions_in_block(c, 11773341);

  // example 2 - call a function
  if (example == NULL || strcmp(example, "eth_call") == 0)
    call_a_function(c);

  // clean up
  in3_free(c);
}
