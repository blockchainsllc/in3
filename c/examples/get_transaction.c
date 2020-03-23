/// checking the transaction data

#include <in3/client.h> // the core client
#include <in3/eth_api.h>
#include <in3/in3_curl.h> // transport implementation
#include <in3/in3_init.h>
#include <in3/utils.h>
#include <stdio.h>

static void get_tx_rpc(in3_t* in3);
static void get_tx_api(in3_t* in3);

int main() {
  // create new incubed client
  in3_t* in3 = in3_for_chain(ETH_CHAIN_ID_MAINNET);

  // get tx using raw RPC call
  get_tx_rpc(in3);

  // get tx using API
  get_tx_api(in3);

  // cleanup client after usage
  in3_free(in3);
}

void get_tx_rpc(in3_t* in3) {
  // prepare 2 pointers for the result.
  char *result, *error;

  // send raw rpc-request, which is then verified
  in3_ret_t res = in3_client_rpc(
      in3,                                                                        //  the configured client
      "eth_getTransactionByHash",                                                 // the rpc-method you want to call.
      "[\"0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab5252287c34bc5d12457eab0e\"]", // the arguments as json-string
      &result,                                                                    // the reference to a pointer which will hold the result
      &error);                                                                    // the pointer which may hold a error message

  // check and print the result or error
  if (res == IN3_OK) {
    printf("Latest tx : \n%s\n", result);
    free(result);
  } else {
    printf("Error verifing the Latest tx : \n%s\n", error);
    free(error);
  }
}

void get_tx_api(in3_t* in3) {
  // the hash of transaction that we want to get
  bytes32_t tx_hash;
  hex_to_bytes("0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab5252287c34bc5d12457eab0e", -1, tx_hash, 32);

  // get the tx by hash
  eth_tx_t* tx = eth_getTransactionByHash(in3, tx_hash);

  // if the result is null there was an error an we can get the latest error message from eth_last_error()
  if (!tx)
    printf("error getting the tx : %s\n", eth_last_error());
  else {
    printf("Transaction #%d of block #%llx", tx->transaction_index, tx->block_number);
    free(tx);
  }
}
