#!/bin/sh
cd ..
rm -rf build_rust
mkdir -p build_rust
cd build_rust
export UPDATE_IN3_BINDINGS=1
cmake -DCMAKE_BUILD_TYPE=MINSIZEREL -DDEV_NO_INTRN_PTR=OFF -DUSE_CURL=false .. && make -j8 && cd ../rust/ && cargo clean && cargo build
cd ../scripts
