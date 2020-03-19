# Building

While we provide binaries, you can also build from source:

### requirements

- cmake
- curl : curl is used as transport for command-line tools.
- optional: libsycrypt, which would be used for unlocking keystore files using `scrypt` as kdf method. if it does not exist you can still build, but not decrypt such keys.   
  for osx `brew install libscrypt` and for debian `sudo apt-get install libscrypt-dev`

Incubed uses cmake for configuring:

```sh
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release .. && make
make install
```

### CMake options

When configuring cmake, you can set a lot of different incubed specific like `cmake -DEVM_GAS=false ..`.

#### ASMJS

  compiles the code as asm.js.

Default-Value: `-DASMJS=OFF`


#### BTC

  if true, the bitcoin verifiers will be build

Default-Value: `-DBTC=OFF`


#### BUILD_DOC

  generates the documenation with doxygen.

Default-Value: `-DBUILD_DOC=OFF`


#### CMD

  build the comandline utils

Default-Value: `-DCMD=ON`


#### CODE_COVERAGE

  Builds targets with code coverage instrumentation. (Requires GCC or Clang)

Default-Value: `-DCODE_COVERAGE=OFF`


#### COLOR

  Enable color codes for debug

Default-Value: `-DCOLOR=ON`


#### ERR_MSG

  if set human readable error messages will be inculded in th executable, otherwise only the error code is used. (saves about 19kB)

Default-Value: `-DERR_MSG=ON`


#### ETH_BASIC

  build basic eth verification.(all rpc-calls except eth_call)

Default-Value: `-DETH_BASIC=ON`


#### ETH_FULL

  build full eth verification.(including eth_call)

Default-Value: `-DETH_FULL=ON`


#### ETH_NANO

  build minimal eth verification.(eth_getTransactionReceipt)

Default-Value: `-DETH_NANO=ON`


#### EVM_GAS

  if true the gas costs are verified when validating a eth_call. This is a optimization since most calls are only interessted in the result. EVM_GAS would be required if the contract uses gas-dependend op-codes.

Default-Value: `-DEVM_GAS=ON`


#### FAST_MATH

  Math optimizations used in the EVM. This will also increase the filesize.

Default-Value: `-DFAST_MATH=OFF`


#### IN3API

  build the USN-API which offer better interfaces and additional functions on top of the pure verification

Default-Value: `-DIN3API=ON`


#### IN3_LIB

  if true a shared anmd static library with all in3-modules will be build.

Default-Value: `-DIN3_LIB=ON`


#### IN3_SERVER

  support for proxy server as part of the cmd-tool, which allows to start the cmd-tool with the -p option and listens to the given port for rpc-requests

Default-Value: `-DIN3_SERVER=OFF`


#### IN3_STAGING

  if true, the client will use the staging-network instead of the live ones

Default-Value: `-DIN3_STAGING=OFF`


#### IPFS

  build IPFS verification

Default-Value: `-DIPFS=ON`


#### JAVA

  build the java-binding (shared-lib and jar-file)

Default-Value: `-DJAVA=OFF`


#### PKG_CONFIG_EXECUTABLE

  pkg-config executable

Default-Value: `-DPKG_CONFIG_EXECUTABLE=/usr/local/bin/pkg-config`


#### POA

  support POA verification including validatorlist updates

Default-Value: `-DPOA=OFF`


#### SEGGER_RTT

  Use the segger real time transfer terminal as the logging mechanism

Default-Value: `-DSEGGER_RTT=OFF`


#### TAG_VERSION

  the tagged version, which should be used

Default-Value: `-DTAG_VERSION=OFF`


#### TEST

  builds the tests and also adds special memory-management, which detects memory leaks, but will cause slower performance

Default-Value: `-DTEST=OFF`


#### TRANSPORTS

  builds transports, which may require extra libraries.

Default-Value: `-DTRANSPORTS=ON`


#### USE_CURL

  if true the curl transport will be built (with a dependency to libcurl)

Default-Value: `-DUSE_CURL=ON`


#### USE_PRECOMPUTED_EC

  if true the secp256k1 curve uses precompiled tables to boost performance. turning this off makes ecrecover slower, but saves about 37kb.

Default-Value: `-DUSE_PRECOMPUTED_EC=ON`


#### USE_SCRYPT

  integrate scrypt into the build in order to allow decrypt_key for scrypt encoded keys.

Default-Value: `-DUSE_SCRYPT=ON`


#### WASM

  Includes the WASM-Build. In order to build it you need emscripten as toolchain. Usually you also want to turn off other builds in this case.

Default-Value: `-DWASM=OFF`


#### WASM_EMBED

  embedds the wasm as base64-encoded into the js-file

Default-Value: `-DWASM_EMBED=ON`


#### WASM_EMMALLOC

  use ther smaller EMSCRIPTEN Malloc, which reduces the size about 10k, but may be a bit slower

Default-Value: `-DWASM_EMMALLOC=ON`


#### WASM_SYNC

  intiaializes the WASM synchronisly, which allows to require and use it the same function, but this will not be supported by chrome (4k limit)

Default-Value: `-DWASM_SYNC=OFF`


