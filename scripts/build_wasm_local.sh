#!/bin/sh
cd ../build
rm -rf *
emcmake cmake -DWASM=true -DASMJS=false -DWASM_EMMALLOC=true  -DWASM_EMBED=false  -DTRANSPORTS=false -DBUILD_DOC=false -DIN3API=true -DIN3_LIB=false -DCMD=false -DUSE_CURL=false -DCMAKE_BUILD_TYPE=DEBUG .. 
make -j8 in3_wasm
cd ../scripts
