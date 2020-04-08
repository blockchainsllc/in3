#!/bin/sh
cd ..
mkdir -p build_rust
cd build_rust
rm -rf *
cmake -DCMAKE_BUILD_TYPE=MINSIZEREL -DDEV_NO_INTRN_PTR=OFF -DUSE_CURL=false .. && make -j8 && cd ../rust/ && cargo build
cd ../scripts
