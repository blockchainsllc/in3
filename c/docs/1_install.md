# Building

While we provide binaries, you can also build from source:

### requirements

- cmake
- curl : curl is used as transport for command-line tools, but you can also compile it without curl (`-DUSE_CURL=false -DCMD=false`), if you want to implement your own transport.

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


#### ASSERTIONS

  includes assertions into the code, which help track errors but may cost time during runtime

Default-Value: `-DASSERTIONS=OFF`


#### BTC

  if true, the bitcoin verifiers will be build

Default-Value: `-DBTC=ON`


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


#### DEV_NO_INTRN_PTR

  (*dev option*) if true the client will NOT include a void pointer (named internal) for use by devs)

Default-Value: `-DDEV_NO_INTRN_PTR=ON`


#### ESP_IDF

  include support for ESP-IDF microcontroller framework

Default-Value: `-DESP_IDF=OFF`


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


#### GCC_ANALYZER

  GCC10 static code analyses

Default-Value: `-DGCC_ANALYZER=OFF`


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


#### LEDGER_NANO

  include support for nano ledger

Default-Value: `-DLEDGER_NANO=OFF`


#### LOGGING

  if set logging and human readable error messages will be inculded in th executable, otherwise only the error code is used. (saves about 19kB)

Default-Value: `-DLOGGING=ON`


#### MULTISIG

  add capapbility to sign with a multig. Currrently only gnosis safe is supported

Default-Value: `-DMULTISIG=OFF`


#### PAY_ETH

  support for direct Eth-Payment

Default-Value: `-DPAY_ETH=OFF`


#### PKG_CONFIG_EXECUTABLE

  pkg-config executable

Default-Value: `-DPKG_CONFIG_EXECUTABLE=/opt/local/bin/pkg-config`


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


#### ZKSYNC

  add RPC-functioin to handle zksync-payments

Default-Value: `-DZKSYNC=OFF`


