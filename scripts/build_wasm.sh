#!/bin/sh
cd ..
docker run \
  --rm \
  -v $(pwd):/src \
  -u emscripten \
  trzeci/emscripten \
  /bin/bash -c "rm -rf build; mkdir build; cd build; emconfigure cmake -DWASM=true -DTRANSPORTS=false -DBUILD_DOC=false -DIN3API=false -DCMD=false -DEXAMPLES=false .. && make -j8"
cd scripts
