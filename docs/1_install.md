# Building

While we provide binaries (TODO put link to releases), you can also build from source:

### requirements

- cmake
- curl : curl is used as transport for the comandline-tools
- optional: libsycrypt, which would be used for unlocking keystore files using `scrypt` as kdf method. if it does not exist you can still build, but not decrypt such keys.

Incubed uses cmake for configuring:

```sh
mkdir build && cd build
cmake -DEVM_GAS=true -DCMAKE_BUILD_TYPE=Release .. && make
make install
```

### CMake options

#### CMD

  build the comandline utils

  Type: `BOOL ` ,    Default-Value: `ON`


#### DEBUG

  Turns on Debug output in the code. This would be required if the tests should output additional debug infos.

  Type: `BOOL ` ,    Default-Value: `OFF`


#### EVM_GAS

  if true the gas costs are verified when validating a eth_call. This is a optimization since most calls are only interessted in the result. EVM_GAS would be required if the contract uses gas-dependend op-codes.

  Type: `BOOL ` ,    Default-Value: `ON`


#### EXAMPLES

  build the examples.

  Type: `BOOL ` ,    Default-Value: `OFF`


#### FAST_MATH

  Math optimizations used in the EVM. This will also increase the filesize.

  Type: `BOOL ` ,    Default-Value: `OFF`


#### IN3API

  build the USN-API which offer better interfaces and additional functions on top of the pure verification

  Type: `BOOL ` ,    Default-Value: `ON`


#### JAVA

  build the java-binding (shared-lib and jar-file)

  Type: `BOOL ` ,    Default-Value: `OFF`


#### TEST

  builds the tests and also adds special memory-management, which detects memory leaks, but will cause slower performance

  Type: `BOOL ` ,    Default-Value: `OFF`


#### TRANSPORTS

  builds transports, which may require extra libraries.

  Type: `BOOL ` ,    Default-Value: `ON`


#### WASM

  Includes the WASM-Build. In order to build it you need emscripten as toolchain. Usually you also want to turn off other builds in this case.

  Type: `BOOL ` ,    Default-Value: `OFF`


