#!/bin/sh
mkdir -p build
cd build
rm -rf * 
conan install .. --build missing
cmake -DCMAKE_BUILD_TYPE=Debug .. && make
cd ..
