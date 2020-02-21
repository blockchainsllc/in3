# Examples


-  [call_a_function](./call_a_function.c)
   This example shows how to call functions on a smart contract eiither directly or using the api to encode the arguments

-  [get_balance](./get_balance.c)
    get the Balance with the API and also as direct RPC-call

-  [get_block](./get_block.c)
    using the basic-module to get and verify a Block with the API and also as direct RPC-call

-  [get_logs](./get_logs.c)
    fetching events and verify them with eth_getLogs

-  [get_transaction](./get_transaction.c)
   checking the transaction data

-  [get_transaction_receipt](./get_transaction_receipt.c)
    validating the result or receipt of an transaction

-  [ipfs_put_get](./ipfs_put_get.c)
    using the IPFS module

-  [send_transaction](./send_transaction.c)
   sending a transaction including signing it with a private key

-  [usn_device](./usn_device.c)
   a example how to watch usn events and act upon it.

-  [usn_rent](./usn_rent.c)
   how to send a rent transaction to a usn contract usinig the usn-api.

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

