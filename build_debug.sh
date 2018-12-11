#!/bin/sh
mkdir build
cd build
rm -rf * 
cmake -DCMAKE_BUILD_TYPE=Debug .. && make
cd ..
