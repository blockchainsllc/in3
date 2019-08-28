# Examples for C

- [get_block_rpc](./get_block_rpc.c)
   getting a block as raw verfified JSON-RPC-Result using the miniimal verification.

-  [get_block_api](./get_block_api.c)
   getting a block as verified structured data using the Eth API

-  [call_a_function](./call_a_function.c)
   This example shows how to call functions on a smart contract

-  [usn_device](./usn_device.c)
   a example how to watch usn events and act upon it.

-  [usn_rent](./usn_rent.c)
   how to send a rent transaction to a usn contract usinig the usn-api.


## Building 

In order to run those examples, you only need a c-compiler (gcc or clang) and curl installed.

```
./build.sh
```

will build all examples in this directory.
You can build them individually by executing:

```
gcc -o get_block_api get_block_api.c -L$IN3_LIB -I$IN3_INCLUDE -lin3_bundle -lcurl
```

