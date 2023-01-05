#### zkcrypto

The zkcrypto library is used for zksync and developed as a rust library offering the basic crypto functions. 

- src: https://github.com/matter-labs/zksync/tree/master/sdk/zksync-crypto
- License: MIT ( and Apache for wabt )
- Version : v0.2.1

There are 2 ways to use this lib

1. linking the rust code directly ( `-DZKCRYPTO_LIB=true` )
2. using a C-version, which was derrived from the rust. ( `-DZKCRYPTO_LIB=true` ) To generate the C-Code, follow these steps:

```sh
git clone https://github.com/matter-labs/zksync.git
cd zksync/js/zksync-crypto
./buils.sh
wasm2c dist/web_bg.wasm -o $DST/zk-crypto.c 
```

This will generate 2 files :
- `zk-crypto.c `
- `zk-crypto.h`

Those file are copied in this folder.

Since `wasm2c` also generates some dependencies to `wasm-rt` the wasm-rt files are included taken from `wabt`: ( https://github.com/WebAssembly/wabt/tree/master/wasm2c )

```sh
git clone https://github.com/WebAssembly/wabt.git
cd wasm.c
cp wasm-rt.h wasm-rt-impl.h wasm-rt-impl.c $DST/

```

The generated C-Code is a bit slower, since it is based on the wasm and using wabt as a static wasm engine, but works on all platforms.

