#!/bin/sh
cd ..
mkdir -p build
cd build
rm -rf *
export CTEST_OUTPUT_ON_FAILURE=1
cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j8
cd ../scripts
