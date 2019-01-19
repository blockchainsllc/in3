#!/bin/sh
mkdir -p build
cd build
rm -rf * 
conan install .. --build missing -o libcurl:shared=True
#conan install .. --build missing -o libcurl:shared=False
export CTEST_OUTPUT_ON_FAILURE=1 
cmake -DTEST=true -DEVM_GAS=true -DCMAKE_BUILD_TYPE=Debug .. && make && make test
#cmake -GNinja -DTEST=true -DCMAKE_BUILD_TYPE=Debug .. && ninja && ninja test
#cmake -GNinja -DCMAKE_BUILD_TYPE=Release .. && ninja
cd ..
