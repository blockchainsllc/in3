#!/bin/sh
mkdir -p build
cd build
rm -rf * 
conan install .. --build missing -o libcurl:shared=True
#conan install .. --build missing -o libcurl:shared=False
cmake -GNinja -DCMAKE_BUILD_TYPE=Debug .. && ninja
#cmake -GNinja -DCMAKE_BUILD_TYPE=Release .. && ninja
cd ..
