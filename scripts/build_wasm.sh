#!/bin/sh
cd ..; rm -rf build/*
docker run \
  --rm \
  -v $(pwd):/src \
  -u emscripten \
  trzeci/emscripten:sdk-tag-1.38.32-64bit \
  /bin/bash -c "cd build;  emconfigure cmake -DWASM=true -DTRANSPORTS=false -DBUILD_DOC=false -DIN3API=false -DCMD=false -DEXAMPLES=false -DCMAKE_BUILD_TYPE=MINSIZEREL .. && make -j8 in3w"

cd build/bin
../../scripts/merge_wasm.sh ../../src/bindings/wasm
cd ../../scripts
