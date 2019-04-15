#include <client/client.h> // the core client
#include <eth_api.h>       // wrapper for easier use
#include <eth_full.h>      // the full ethereum verifier containing the EVM
#include <in3_curl.h>      // transport implementation
#include <stdio.h>

int main(int argc, char* argv[]) {

  // register a chain-verifier for full Ethereum-Support
  in3_register_eth_full();

  // create new incubed client
  in3_t* c = in3_new();

  c->evm_flags = 65536;

  // set your config
  c->transport    = send_curl; // use curl to handle the requests
  c->requestCount = 1;         // number of requests to send
  c->chainId      = 0x1;       // use main chain

  // use a ethereum-api instead of pure JSON-RPC-Requests
  eth_block_t* block = eth_getBlockByNumber(c, atoi(argv[1]), true);
  if (!block)
    printf("Could not find the Block: %s", eth_last_error());
  else {
    printf("Number of verified transactions in block: %i", block->tx_count);
    free(block);
  }
}