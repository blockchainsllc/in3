#!/bin/sh
cd ../build
rm -rf *
emcmake cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=true -DWASM=true -DASMJS=false -DWASM_EMMALLOC=true  -DWASM_EMBED=false  -DCMAKE_BUILD_TYPE=DEBUG .. 
make -j8 in3_wasm
cd ../scripts
