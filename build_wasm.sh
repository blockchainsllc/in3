#!/bin/sh
mkdir -p build
rm -rf build/* 


docker run \
  --rm \
  -v $(pwd):/src \
  -u emscripten \
  trzeci/emscripten \
  /bin/bash -c "cd build; emconfigure cmake -DWASM=true .. && make"



#cmake -DCMAKE_TOOLCHAIN_FILE=<EmscriptenRoot>/cmake/Modules/Platform/Emscripten.cmake -DTEST=true -DEVM_GAS=true -DCMAKE_BUILD_TYPE=Debug .. -DLIBCURL_TYPE=shared && make && make ptest
#cmake -DTEST=true -DEVM_GAS=true -DCMAKE_BUILD_TYPE=Debug .. && make && make test
#cmake -GNinja -DTEST=true -DCMAKE_BUILD_TYPE=Debug .. && ninja && ninja test
#cmake -DTEST=true -DEVM_GAS=true -GNinja -DCMAKE_BUILD_TYPE=Release .. && ninja
