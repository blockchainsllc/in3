# Examples

### call_a_function

source : [in3-c/c/examples/call_a_function.c](https://github.com/slockit/in3-c/blob/master/c/examples/call_a_function.c)

This example shows how to call functions on a smart contract eiither directly or using the api to encode the arguments


```c
/// This example shows how to call functions on a smart contract eiither directly or using the api to encode the arguments

#include <in3/client.h>   // the core client
#include <in3/eth_api.h>  // functions for direct api-access
#include <in3/in3_init.h> // if included the verifier will automaticly be initialized.
#include <in3/log.h>      // logging functions
#include <inttypes.h>
#include <stdio.h>

static in3_ret_t call_func_rpc(in3_t* c);
static in3_ret_t call_func_api(in3_t* c, address_t contract);

int main() {
  in3_ret_t ret = IN3_OK;

  // Remove log prefix for readability
  in3_log_set_prefix("");

  // create new incubed client
  in3_t* c = in3_for_chain(ETH_CHAIN_ID_MAINNET);

  // define a address (20byte)
  address_t contract;

  // copy the hexcoded string into this address
  hex_to_bytes("0x2736D225f85740f42D17987100dc8d58e9e16252", -1, contract, 20);

  // call function using RPC
  ret = call_func_rpc(c);
  if (ret != IN3_OK) goto END;

  // call function using API
  ret = call_func_api(c, contract);
  if (ret != IN3_OK) goto END;

END:
  // clean up
  in3_free(c);
  return 0;
}

in3_ret_t call_func_rpc(in3_t* c) {
  // prepare 2 pointers for the result.
  char *result, *error;

  // send raw rpc-request, which is then verified
  in3_ret_t res = in3_client_rpc(
      c,                                                                                                //  the configured client
      "eth_call",                                                                                       // the rpc-method you want to call.
      "[{\"to\":\"0x2736d225f85740f42d17987100dc8d58e9e16252\", \"data\":\"0x15625c5e\"}, \"latest\"]", // the signed raw txn, same as the one used in the API example
      &result,                                                                                          // the reference to a pointer which will hold the result
      &error);                                                                                          // the pointer which may hold a error message

  // check and print the result or error
  if (res == IN3_OK) {
    printf("Result: \n%s\n", result);
    free(result);
    return 0;
  } else {
    printf("Error sending tx: \n%s\n", error);
    free(error);
    return IN3_EUNKNOWN;
  }
}

in3_ret_t call_func_api(in3_t* c, address_t contract) {
  // ask for the number of servers registered
  json_ctx_t* response = eth_call_fn(c, contract, BLKNUM_LATEST(), "totalServers():uint256");
  if (!response) {
    printf("Could not get the response: %s", eth_last_error());
    return IN3_EUNKNOWN;
  }

  // convert the response to a uint32_t,
  uint32_t number_of_servers = d_int(response->result);

  // clean up resources
  json_free(response);

  // output
  printf("Found %u servers registered : \n", number_of_servers);

  // read all structs ...
  for (uint32_t i = 0; i < number_of_servers; i++) {
    response = eth_call_fn(c, contract, BLKNUM_LATEST(), "servers(uint256):(string,address,uint,uint,uint,address)", to_uint256(i));
    if (!response) {
      printf("Could not get the response: %s", eth_last_error());
      return IN3_EUNKNOWN;
    }

    char*    url     = d_get_string_at(response->result, 0); // get the first item of the result (the url)
    bytes_t* owner   = d_get_bytes_at(response->result, 1);  // get the second item of the result (the owner)
    uint64_t deposit = d_get_long_at(response->result, 2);   // get the third item of the result (the deposit)

    printf("Server %i : %s owner = %02x%02x...", i, url, owner->data[0], owner->data[1]);
    printf(", deposit = %" PRIu64 "\n", deposit);

    // free memory
    json_free(response);
  }
  return 0;
}

```

### get_balance

source : [in3-c/c/examples/get_balance.c](https://github.com/slockit/in3-c/blob/master/c/examples/get_balance.c)

 get the Balance with the API and also as direct RPC-call


```c
///  get the Balance with the API and also as direct RPC-call

#include <in3/client.h>   // the core client
#include <in3/eth_api.h>  // functions for direct api-access
#include <in3/in3_init.h> // if included the verifier will automaticly be initialized.
#include <in3/log.h>      // logging functions
#include <in3/utils.h>
#include <stdio.h>

static void get_balance_rpc(in3_t* in3);
static void get_balance_api(in3_t* in3);

int main() {
  // create new incubed client
  in3_t* in3 = in3_for_chain(ETH_CHAIN_ID_MAINNET);

  // get balance using raw RPC call
  get_balance_rpc(in3);

  // get balance using API
  get_balance_api(in3);

  // cleanup client after usage
  in3_free(in3);
}

void get_balance_rpc(in3_t* in3) {
  // prepare 2 pointers for the result.
  char *result, *error;

  // send raw rpc-request, which is then verified
  in3_ret_t res = in3_client_rpc(
      in3,                                                            //  the configured client
      "eth_getBalance",                                               // the rpc-method you want to call.
      "[\"0xc94770007dda54cF92009BFF0dE90c06F603a09f\", \"latest\"]", // the arguments as json-string
      &result,                                                        // the reference to a pointer whill hold the result
      &error);                                                        // the pointer which may hold a error message

  // check and print the result or error
  if (res == IN3_OK) {
    printf("Balance: \n%s\n", result);
    free(result);
  } else {
    printf("Error getting balance: \n%s\n", error);
    free(error);
  }
}

void get_balance_api(in3_t* in3) {
  // the address of account whose balance we want to get
  address_t account;
  hex_to_bytes("0xc94770007dda54cF92009BFF0dE90c06F603a09f", -1, account, 20);

  // get balance of account
  long double balance = as_double(eth_getBalance(in3, account, BLKNUM_EARLIEST()));

  // if the result is null there was an error an we can get the latest error message from eth_lat_error()
  balance ? printf("Balance: %Lf\n", balance) : printf("error getting the balance : %s\n", eth_last_error());
}

```

### get_block

source : [in3-c/c/examples/get_block.c](https://github.com/slockit/in3-c/blob/master/c/examples/get_block.c)

 using the basic-module to get and verify a Block with the API and also as direct RPC-call


```c
///  using the basic-module to get and verify a Block with the API and also as direct RPC-call

#include <in3/client.h>   // the core client
#include <in3/eth_api.h>  // functions for direct api-access
#include <in3/in3_init.h> // if included the verifier will automaticly be initialized.
#include <in3/log.h>      // logging functions

#include <inttypes.h>
#include <stdio.h>

static void get_block_rpc(in3_t* in3);
static void get_block_api(in3_t* in3);

int main() {
  // create new incubed client
  in3_t* in3 = in3_for_chain(ETH_CHAIN_ID_MAINNET);

  // get block using raw RPC call
  get_block_rpc(in3);

  // get block using API
  get_block_api(in3);

  // cleanup client after usage
  in3_free(in3);
}

void get_block_rpc(in3_t* in3) {
  // prepare 2 pointers for the result.
  char *result, *error;

  // send raw rpc-request, which is then verified
  in3_ret_t res = in3_client_rpc(
      in3,                    //  the configured client
      "eth_getBlockByNumber", // the rpc-method you want to call.
      "[\"latest\",true]",    // the arguments as json-string
      &result,                // the reference to a pointer whill hold the result
      &error);                // the pointer which may hold a error message

  // check and print the result or error
  if (res == IN3_OK) {
    printf("Latest block : \n%s\n", result);
    free(result);
  } else {
    printf("Error verifing the Latest block : \n%s\n", error);
    free(error);
  }
}

void get_block_api(in3_t* in3) {
  // get the block without the transaction details
  eth_block_t* block = eth_getBlockByNumber(in3, BLKNUM(8432424), false);

  // if the result is null there was an error an we can get the latest error message from eth_lat_error()
  if (!block)
    printf("error getting the block : %s\n", eth_last_error());
  else {
    printf("Number of transactions in Block #%llu: %d\n", block->number, block->tx_count);
    free(block);
  }
}

```

### get_logs

source : [in3-c/c/examples/get_logs.c](https://github.com/slockit/in3-c/blob/master/c/examples/get_logs.c)

 fetching events and verify them with eth_getLogs


```c
///  fetching events and verify them with eth_getLogs

#include <in3/client.h>   // the core client
#include <in3/eth_api.h>  // functions for direct api-access
#include <in3/in3_init.h> // if included the verifier will automaticly be initialized.
#include <in3/log.h>      // logging functions
#include <inttypes.h>
#include <stdio.h>

static void get_logs_rpc(in3_t* in3);
static void get_logs_api(in3_t* in3);

int main() {
  // create new incubed client
  in3_t* in3    = in3_for_chain(ETH_CHAIN_ID_MAINNET);
  in3->chain_id = ETH_CHAIN_ID_KOVAN;

  // get logs using raw RPC call
  get_logs_rpc(in3);

  // get logs using API
  get_logs_api(in3);

  // cleanup client after usage
  in3_free(in3);
}

void get_logs_rpc(in3_t* in3) {
  // prepare 2 pointers for the result.
  char *result, *error;

  // send raw rpc-request, which is then verified
  in3_ret_t res = in3_client_rpc(
      in3,           //  the configured client
      "eth_getLogs", // the rpc-method you want to call.
      "[{}]",        // the arguments as json-string
      &result,       // the reference to a pointer whill hold the result
      &error);       // the pointer which may hold a error message

  // check and print the result or error
  if (res == IN3_OK) {
    printf("Logs : \n%s\n", result);
    free(result);
  } else {
    printf("Error getting logs : \n%s\n", error);
    free(error);
  }
}

void get_logs_api(in3_t* in3) {
  // Create filter options
  char b[30];
  sprintf(b, "{\"fromBlock\":\"0x%" PRIx64 "\"}", eth_blockNumber(in3) - 2);
  json_ctx_t* jopt = parse_json(b);

  // Create new filter with options
  size_t fid = eth_newFilter(in3, jopt);

  // Get logs
  eth_log_t* logs = NULL;
  in3_ret_t  ret  = eth_getFilterLogs(in3, fid, &logs);
  if (ret != IN3_OK) {
    printf("eth_getFilterLogs() failed [%d]\n", ret);
    return;
  }

  // print result
  while (logs) {
    eth_log_t* l = logs;
    printf("--------------------------------------------------------------------------------\n");
    printf("\tremoved: %s\n", l->removed ? "true" : "false");
    printf("\tlogId: %lu\n", l->log_index);
    printf("\tTxId: %lu\n", l->transaction_index);
    printf("\thash: ");
    ba_print(l->block_hash, 32);
    printf("\n\tnum: %" PRIu64 "\n", l->block_number);
    printf("\taddress: ");
    ba_print(l->address, 20);
    printf("\n\tdata: ");
    b_print(&l->data);
    printf("\ttopics[%lu]: ", l->topic_count);
    for (size_t i = 0; i < l->topic_count; i++) {
      printf("\n\t");
      ba_print(l->topics[i], 32);
    }
    printf("\n");
    logs = logs->next;
    free(l->data.data);
    free(l->topics);
    free(l);
  }
  eth_uninstallFilter(in3, fid);
  json_free(jopt);
}

```

### get_transaction

source : [in3-c/c/examples/get_transaction.c](https://github.com/slockit/in3-c/blob/master/c/examples/get_transaction.c)

checking the transaction data


```c
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

```

### get_transaction_receipt

source : [in3-c/c/examples/get_transaction_receipt.c](https://github.com/slockit/in3-c/blob/master/c/examples/get_transaction_receipt.c)

 validating the result or receipt of an transaction


```c
///  validating the result or receipt of an transaction

#include <in3/client.h>   // the core client
#include <in3/eth_api.h>  // functions for direct api-access
#include <in3/in3_init.h> // if included the verifier will automaticly be initialized.
#include <in3/log.h>      // logging functions
#include <in3/utils.h>
#include <inttypes.h>
#include <stdio.h>

static void get_tx_receipt_rpc(in3_t* in3);
static void get_tx_receipt_api(in3_t* in3);

int main() {
  // create new incubed client
  in3_t* in3 = in3_for_chain(ETH_CHAIN_ID_MAINNET);

  // get tx receipt using raw RPC call
  get_tx_receipt_rpc(in3);

  // get tx receipt using API
  get_tx_receipt_api(in3);

  // cleanup client after usage
  in3_free(in3);
}

void get_tx_receipt_rpc(in3_t* in3) {
  // prepare 2 pointers for the result.
  char *result, *error;

  // send raw rpc-request, which is then verified
  in3_ret_t res = in3_client_rpc(
      in3,                                                                        //  the configured client
      "eth_getTransactionReceipt",                                                // the rpc-method you want to call.
      "[\"0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab5252287c34bc5d12457eab0e\"]", // the arguments as json-string
      &result,                                                                    // the reference to a pointer which will hold the result
      &error);                                                                    // the pointer which may hold a error message

  // check and print the result or error
  if (res == IN3_OK) {
    printf("Transaction receipt: \n%s\n", result);
    free(result);
  } else {
    printf("Error verifing the tx receipt: \n%s\n", error);
    free(error);
  }
}

void get_tx_receipt_api(in3_t* in3) {
  // the hash of transaction whose receipt we want to get
  bytes32_t tx_hash;
  hex_to_bytes("0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab5252287c34bc5d12457eab0e", -1, tx_hash, 32);

  // get the tx receipt by hash
  eth_tx_receipt_t* txr = eth_getTransactionReceipt(in3, tx_hash);

  // if the result is null there was an error an we can get the latest error message from eth_last_error()
  if (!txr)
    printf("error getting the tx : %s\n", eth_last_error());
  else {
    printf("Transaction #%d of block #%llx, gas used = %" PRIu64 ", status = %s\n", txr->transaction_index, txr->block_number, txr->gas_used, txr->status ? "success" : "failed");
    eth_tx_receipt_free(txr);
  }
}

```

### ipfs_put_get

source : [in3-c/c/examples/ipfs_put_get.c](https://github.com/slockit/in3-c/blob/master/c/examples/ipfs_put_get.c)

 using the IPFS module


```c
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

```

### send_transaction

source : [in3-c/c/examples/send_transaction.c](https://github.com/slockit/in3-c/blob/master/c/examples/send_transaction.c)

sending a transaction including signing it with a private key


```c
/// sending a transaction including signing it with a private key

#include <in3/client.h>   // the core client
#include <in3/eth_api.h>  // functions for direct api-access
#include <in3/in3_init.h> // if included the verifier will automaticly be initialized.
#include <in3/log.h>      // logging functions
#include <in3/signer.h>   // default signer implementation
#include <in3/utils.h>
#include <stdio.h>

// fixme: This is only for the sake of demo. Do NOT store private keys as plaintext.
#define ETH_PRIVATE_KEY "0x8da4ef21b864d2cc526dbdb2a120bd2874c36c9d0a1fb7f8c63d7f7a8b41de8f"

static void send_tx_rpc(in3_t* in3);
static void send_tx_api(in3_t* in3);

int main() {
  // create new incubed client
  in3_t* in3 = in3_for_chain(ETH_CHAIN_ID_MAINNET);

  // convert the hexstring to bytes
  bytes32_t pk;
  hex_to_bytes(ETH_PRIVATE_KEY, -1, pk, 32);

  // create a simple signer with this key
  eth_set_pk_signer(in3, pk);

  // send tx using raw RPC call
  send_tx_rpc(in3);

  // send tx using API
  send_tx_api(in3);

  // cleanup client after usage
  in3_free(in3);
}

void send_tx_rpc(in3_t* in3) {
  // prepare 2 pointers for the result.
  char *result, *error;

  // send raw rpc-request, which is then verified
  in3_ret_t res = in3_client_rpc(
      in3,                      //  the configured client
      "eth_sendRawTransaction", // the rpc-method you want to call.
      "[\"0xf892808609184e72a0008296c094d46e8dd67c5d32be8058bb8eb970870f0724456"
      "7849184e72aa9d46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8eb9"
      "70870f07244567526a06f0103fccdcae0d6b265f8c38ee42f4a722c1cb36230fe8da40315acc3051"
      "9a8a06252a68b26a5575f76a65ac08a7f684bc37b0c98d9e715d73ddce696b58f2c72\"]", // the signed raw txn, same as the one used in the API example
      &result,                                                                    // the reference to a pointer which will hold the result
      &error);                                                                    // the pointer which may hold a error message

  // check and print the result or error
  if (res == IN3_OK) {
    printf("Result: \n%s\n", result);
    free(result);
  } else {
    printf("Error sending tx: \n%s\n", error);
    free(error);
  }
}

void send_tx_api(in3_t* in3) {
  // prepare parameters
  address_t to, from;
  hex_to_bytes("0x63FaC9201494f0bd17B9892B9fae4d52fe3BD377", -1, from, 20);
  hex_to_bytes("0xd46e8dd67c5d32be8058bb8eb970870f07244567", -1, to, 20);

  bytes_t* data = hex_to_new_bytes("d46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8eb970870f072445675", 82);

  // send the tx
  bytes_t* tx_hash = eth_sendTransaction(in3, from, to, OPTIONAL_T_VALUE(uint64_t, 0x96c0), OPTIONAL_T_VALUE(uint64_t, 0x9184e72a000), OPTIONAL_T_VALUE(uint256_t, to_uint256(0x9184e72a)), OPTIONAL_T_VALUE(bytes_t, *data), OPTIONAL_T_UNDEFINED(uint64_t));

  // if the result is null there was an error and we can get the latest error message from eth_last_error()
  if (!tx_hash)
    printf("error sending the tx : %s\n", eth_last_error());
  else {
    printf("Transaction hash: ");
    b_print(tx_hash);
    b_free(tx_hash);
  }
  b_free(data);
}

```

### usn_device

source : [in3-c/c/examples/usn_device.c](https://github.com/slockit/in3-c/blob/master/c/examples/usn_device.c)

a example how to watch usn events and act upon it.


```c
/// a example how to watch usn events and act upon it.

#include <in3/client.h>   // the core client
#include <in3/eth_api.h>  // functions for direct api-access
#include <in3/in3_init.h> // if included the verifier will automaticly be initialized.
#include <in3/log.h>      // logging functions
#include <in3/signer.h>   // signer-api
#include <in3/usn_api.h>
#include <in3/utils.h>
#include <inttypes.h>
#include <stdio.h>
#include <time.h>
#if defined(_WIN32) || defined(WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

static int handle_booking(usn_event_t* ev) {
  printf("\n%s Booking timestamp=%" PRIu64 "\n", ev->type == BOOKING_START ? "START" : "STOP", ev->ts);
  return 0;
}

int main(int argc, char* argv[]) {
  // create new incubed client
  in3_t* c = in3_for_chain(ETH_CHAIN_ID_MAINNET);

  // switch to goerli
  c->chain_id = 0x5;

  // setting up a usn-device-config
  usn_device_conf_t usn;
  usn.booking_handler    = handle_booking;                                          // this is the handler, which is called for each rent/return or start/stop
  usn.c                  = c;                                                       // the incubed client
  usn.chain_id           = c->chain_id;                                             // the chain_id
  usn.devices            = NULL;                                                    // this will contain the list of devices supported
  usn.len_devices        = 0;                                                       // and length of this list
  usn.now                = 0;                                                       // the current timestamp
  unsigned int wait_time = 5;                                                       // the time to wait between the internval
  hex_to_bytes("0x85Ec283a3Ed4b66dF4da23656d4BF8A507383bca", -1, usn.contract, 20); // address of the usn-contract, which we copy from hex

  // register a usn-device
  usn_register_device(&usn, "office@slockit");

  // now we run en endless loop which simply wait for events on the chain.
  printf("\n start watching...\n");
  while (true) {
    usn.now              = time(NULL);                               // update the timestamp, since this is running on embedded devices, this may be depend on the hardware.
    unsigned int timeout = usn_update_state(&usn, wait_time) * 1000; // this will now check for new events and trigger the handle_booking if so.

    // sleep
#if defined(_WIN32) || defined(WIN32)
    Sleep(timeout);
#else
    nanosleep((const struct timespec[]){{0, timeout * 1000000L}}, NULL);
#endif
  }

  // clean up
  in3_free(c);
  return 0;
}
```

### usn_rent

source : [in3-c/c/examples/usn_rent.c](https://github.com/slockit/in3-c/blob/master/c/examples/usn_rent.c)

how to send a rent transaction to a usn contract usinig the usn-api.


```c
/// how to send a rent transaction to a usn contract usinig the usn-api.

#include <in3/api_utils.h>
#include <in3/eth_api.h>  // functions for direct api-access
#include <in3/in3_init.h> // if included the verifier will automaticly be initialized.
#include <in3/signer.h>   // signer-api
#include <in3/usn_api.h>  // api for renting
#include <in3/utils.h>
#include <inttypes.h>
#include <stdio.h>

/**
 * takes the keystore json-data and a password and assigns the decrypted key as signer.
 */
void unlock_key(in3_t* c, char* json_data, char* passwd) {
  // parse the json
  json_ctx_t* key_data = parse_json(json_data);
  if (!key_data) {
    perror("key is not parseable!\n");
    exit(EXIT_FAILURE);
  }

  // decrypt the key
  uint8_t* pk = malloc(32);
  if (decrypt_key(key_data->result, passwd, pk) != IN3_OK) {
    perror("wrong password!\n");
    exit(EXIT_FAILURE);
  }

  // free json
  json_free(key_data);

  // create a signer with this key
  eth_set_pk_signer(c, pk);
}

int main(int argc, char* argv[]) {
  // create new incubed client
  in3_t* c = in3_for_chain(ETH_CHAIN_ID_GOERLI);

  // address of the usn-contract, which we copy from hex
  address_t contract;
  hex_to_bytes("0x85Ec283a3Ed4b66dF4da23656d4BF8A507383bca", -1, contract, 20);

  // read the key from args - I know this is not safe, but this is just a example.
  if (argc < 3) {
    perror("you need to provide a json-key and password to rent it");
    exit(EXIT_FAILURE);
  }
  char* key_data = argv[1];
  char* passwd   = argv[2];
  unlock_key(c, key_data, passwd);

  // rent it for one hour.
  uint32_t renting_seconds = 3600;

  // allocate 32 bytes for the resulting tx hash
  bytes32_t tx_hash;

  // start charging
  if (usn_rent(c, contract, NULL, "office@slockit", renting_seconds, tx_hash))
    printf("Could not start charging\n");
  else {
    printf("Charging tx successfully sent... tx_hash=0x");
    for (int i = 0; i < 32; i++) printf("%02x", tx_hash[i]);
    printf("\n");

    if (argc == 4) // just to include it : if you want to stop earlier, you can call
      usn_return(c, contract, "office@slockit", tx_hash);
  }

  // clean up
  in3_free(c);
  return 0;
}
```


### Building 

In order to run those examples, you only need a c-compiler (gcc or clang) and curl installed.

```
./build.sh
```

will build all examples in this directory.
You can build them individually by executing:

```
gcc -o get_block_api get_block_api.c -lin3 -lcurl
```

