# Examples for C

- [get_block_api](./get_block_rpc.c)
   getting a block as raw verfified JSON-RPC-Result

-  [get_block_api](./get_block_api.c)
   getting a block as verified structured dat 


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

