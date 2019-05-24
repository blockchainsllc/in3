#include <client/client.h> // the core client
#include <eth_api.h>       // wrapper for easier use
#include <eth_full.h>      // the full ethereum verifier containing the EVM
#include <in3_curl.h>      // transport implementation
#include <signer.h>        // the full ethereum verifier containing the EVM
#include <stdio.h>
#include <time.h>
#include <usn_api.h> // for usn-specific functions
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
  _free(response);

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
    printf(", deposit = %llu\n", deposit);

    _free(response);
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

void start_charging(in3_t* c) {

  address_t contract;
  bytes32_t private_key, tx_hash;

  // set the contract-address
  hex2byte_arr("0x85Ec283a3Ed4b66dF4da23656d4BF8A507383bca", -1, contract, 20);

  // copy bytes and register private key first ( needs to be called only once per client-struct)
  hex2byte_arr("0x21c15ea1cd68229536fb71f713a538bc3f0f33201db773f4edf49602bf15e5df", -1, private_key, 32);
  eth_set_pk_signer(c, private_key);

  // reserve charging for one hour (and pay for it)
  uint32_t max_charging_seconds = 3600;

  // start charging
  if (usn_rent(c, contract, NULL, "wirelane1@tobalaba", max_charging_seconds, tx_hash) < 0)
    printf("Could not start charging\n");
  else {
    printf("Charging tx successfully sent... tx_hash=");
    ba_print(tx_hash, 32);
    printf("\n");
  }
}

void stop_charging(in3_t* c) {
  address_t contract;
  bytes32_t private_key, tx_hash;

  // set the contract-address
  hex2byte_arr("0x85Ec283a3Ed4b66dF4da23656d4BF8A507383bca", -1, contract, 20);

  // copy bytes and register private key first ( needs to be called only once per client-struct)
  hex2byte_arr("0x21c15ea1cd68229536fb71f713a538bc3f0f33201db773f4edf49602bf15e5df", -1, private_key, 32);
  eth_set_pk_signer(c, private_key);

  // stop charging
  if (usn_return(c, contract, "wirelane1@tobalaba", tx_hash) < 0)
    printf("Could not stop charging\n");
  else {
    printf("Charging tx successfully stopped... tx_hash=");
    ba_print(tx_hash, 32);
    printf("\n");
  }
}

static int handle_booking(usn_event_t* ev) {
  if (ev->type == BOOKING_START)
    printf("UNLOCK device\n");
  else
    printf("LOCK device\n");
  return 0;
}

void watch_for_events(in3_t* c) {

  // interval to check for new event (here 5 seconds)
  unsigned int wait_time = 5;

  // configure a device-config
  usn_device_conf_t usn;

  // set a function which is called whenever a rent event starts or stops
  usn.booking_handler = handle_booking;
  usn.c               = c;
  usn.chain_id        = c->chainId;
  usn.devices         = NULL;
  usn.len_devices     = 0;
  usn.now             = 0;
  // set the contract-address
  hex2byte_arr("0x85Ec283a3Ed4b66dF4da23656d4BF8A507383bca", -1, usn.contract, 20);

  // register the device to listen to
  usn_register_device(&usn, "greyp1@tobalaba");

  printf("\n start watching...\n");

  while (true) {
    printf("checking...\n");
    // update the current timestamp (UNIIX-TS in seconds since 1970)
    usn.now = time(NULL);

    // get the events and return the time to wait before the next check
    unsigned int timeout = usn_update_state(&usn, wait_time) * 1000;
#if defined(_WIN32) || defined(WIN32)
    Sleep(timeout);
#else
    usleep(timeout * 1000); // usleep takes sleep time in us (1 millionth of a second)
#endif
  }
}

int main(int argc, char* argv[]) {

  char* example = argc ? argv[0] : NULL;

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

  // example 1 - getBlock
  if (example == NULL || strcmp(example, "eth_getBlock") == 0)
    show_transactions_in_block(c, 11773341);

  // example 2 - call a function
  if (example == NULL || strcmp(example, "eth_call") == 0)
    call_a_function(c);

  // example 3 - start charging
  if (example == NULL || strcmp(example, "usn_rent") == 0)
    start_charging(c);

  // example 4 - stop charging
  if (example == NULL || strcmp(example, "usn_rent") == 0)
    stop_charging(c);

  // example 5 - watch for events
  if (example == NULL || strcmp(example, "usn_watch") == 0)
    watch_for_events(c);

  // clean up
  in3_free(c);
}