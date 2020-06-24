#!/bin/sh
cd ..
mkdir -p rust/in3-sys/in3-core
mkdir -p rust/in3-sys/in3-core/c
cp -r c/CMakeLists.txt c/macro.cmake c/compiler.cmake c/docs c/src c/include rust/in3-sys/in3-core/c/
cp CMakeLists.txt rust/in3-sys/in3-core/
export UPDATE_IN3_BINDINGS=1
cd rust && cargo clean && cargo build
#cd rust && cargo build
cd ../scripts
