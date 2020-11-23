## zksync crypto lib

The library is taken from zksync : https://github.com/matter-labs/zksync/tree/master/js/zksync-crypto

### build it

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

### LICENSE

zksync : MIT License
wabt : Apache License




