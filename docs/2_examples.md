# Examples


### creating a incubed instance

creating always follow these steps:

```c
#include <client/client.h> // the core client
#include <eth_full.h>      // the full ethereum verifier containing the EVM
#include <in3_curl.h>      // transport implementation

// register verifiers, in this case a full verifier allowing eth_call
in3_register_eth_full();

// create new client
in3_t* client = in3_new();

// configure storage by using storage-functions from in3_curl, which store the cache in /home/<USER>/.in3
in3_storage_handler_t storage_handler;
storage_handler.get_item = storage_get_item;
storage_handler.set_item = storage_set_item;

client->cacheStorage = &storage_handler;

// configure transport by using curl
client->transport    = send_curl;

// init cache by reading the nodelist from the cache >(if exists)
in3_cache_init(client);

// ready to use ...
```

### calling a function

```c

  // define a address (20byte)
  address_t contract;

  // copy the hexcoded string into this address
  hex2byte_arr("0x845E484b505443814B992Bf0319A5e8F5e407879", -1, contract, 20);

  // ask for the number of servers registered
  json_ctx_t* response = eth_call_fn(client, contract, "totalServers():uint256");

  // handle response
  if (!response) {
    printf("Could not get the response: %s", eth_last_error());
    return;
  }

  // convert the result to a integer
  int number_of_servers = d_int(response->result);

  // don't forget the free the response!
  free_json(response);

  // out put result
  printf("Found %i servers registered : \n", number_of_servers);

  // now we call a function with a complex result...
  for (int i = 0; i < number_of_servers; i++) {

    // get all the details for one server.
    response = eth_call_fn(c, contract, "servers(uint256):(string,address,uint,uint,uint,address)", to_uint256(i));

    // handle error
    if (!response) {
      printf("Could not get the response: %s", eth_last_error());
      return;
    }

    // decode data
    char*    url     = d_get_string_at(response->result, 0); // get the first item of the result (the url)
    bytes_t* owner   = d_get_bytes_at(response->result, 1);  // get the second item of the result (the owner)
    uint64_t deposit = d_get_long_at(response->result, 2);   // get the third item of the result (the deposit)

    // print values
    printf("Server %i : %s owner = ", i, url);
    ba_print(owner->data, owner->len);
    printf(", deposit = %" PRIu64 "\n", deposit);

    // clean up
    free_json(response);
  }

```

