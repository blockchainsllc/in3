# Building

Incubed uses cmake for configuring.

```sh
mkdir build && cd build
cmake -DEVM_GAS=true -DCMAKE_BUILD_TYPE=Release .. && make
make install
```

### Defines

```
// Build documentation
BUILD_DOC:BOOL=true

// Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel ...
CMAKE_BUILD_TYPE:STRING=Debug


// CLI-utils
CMD:BOOL=ON

// Path to a file.
CURL_INCLUDE_DIR:PATH=/opt/local/include

// Debug
DEBUG:BOOL=OFF

// Evm Gas
EVM_GAS:BOOL=true

// examples
EXAMPLES:BOOL=OFF

// Math optimizations
FAST_MATH:BOOL=OFF

// apis
IN3API:BOOL=ON

// Java
JAVA:BOOL=true


// Link type for curl
LIBCURL_LINKTYPE:BOOL=OFF

// Test
TEST:BOOL=true

// transports
TRANSPORTS:BOOL=ON

// Wasm
WASM:BOOL=OFF 
```