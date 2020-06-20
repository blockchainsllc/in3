#!/bin/sh
cd ..; rm -rf build/*
docker run \
  --rm \
  -v $(pwd):$(pwd) \
  docker.slock.it/build-images/cmake:clang11 \
  /bin/bash -c "cd /$(pwd)/build;  emcmake cmake -DWASM_EMMALLOC=true  -DWASM=true -DASMJS=true -DWASM_EMBED=true   -DCMAKE_BUILD_TYPE=RELEASE .. && make -j8 in3_wasm"
cd scripts
