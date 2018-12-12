#!/bin/sh
mkdir build
cd build
#rm -rf * 
#conan install .. --build libcurl
cmake -DCMAKE_BUILD_TYPE=Debug .. && make
cd ..
