#!/bin/sh
cd ..
mkdir -p build
cd build
rm -rf *
export CTEST_OUTPUT_ON_FAILURE=1
cmake -DCMAKE_BUILD_TYPE=Debug -DIN3_SERVER=true -DPOA=false -DJAVA=true -DUSE_SEGGER_RTT=false -DIN3_LIB=true -DTEST=true -DCMAKE_EXPORT_COMPILE_COMMANDS=true .. && make -j8
cd ../scripts
