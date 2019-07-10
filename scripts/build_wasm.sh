#!/bin/sh
mkdir -p build
rm -rf build/* 


docker run \
  --rm \
  -v $(pwd):/src \
  -u emscripten \
  trzeci/emscripten \
  /bin/bash -c "cd build; emconfigure cmake -DWASM=true -DTRANSPORTS=false -DBUILD_DOC=false -DIN3API=false -DCMD=false .. && make -j8"
