#!/bin/sh
mkdir -p build
rm -rf build/* 


docker run \
  --rm \
  -v $(pwd):/src \
  -u emscripten \
  trzeci/emscripten \
  /bin/bash -c "rm -rf build; mkdir build; cd build; emconfigure cmake -DWASM=true .. && make -j8"
