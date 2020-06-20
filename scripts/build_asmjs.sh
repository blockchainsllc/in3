#!/bin/sh
cd ..; rm -rf build/*
docker run \
  --rm \
  -v $(pwd):$(pwd) \
  docker.slock.it/build-images/cmake:clang11 \
  /bin/bash -c "cd /$(pwd)/build;  emcmake cmake -DWASM=true -DASMJS=true -DWASM_EMMALLOC=true  -DWASM_EMBED=false  -DTRANSPORTS=false -DBUILD_DOC=false -DIN3API=true -DIN3_LIB=false -DCMD=false -DUSE_CURL=false -DCMAKE_BUILD_TYPE=DEBUG .. && make -j8 in3_wasm"

cd scripts
