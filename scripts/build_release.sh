#!/bin/sh
cd ..
mkdir -p build
cd build
rm -rf *
export CTEST_OUTPUT_ON_FAILURE=1
cmake -DCMAKE_BUILD_TYPE=MINSIZEREL -DIN3_SERVER=true .. && make && make install
#cmake -DTEST=true -DEVM_GAS=true -DCMAKE_BUILD_TYPE=Debug .. && make && make test
#cmake -GNinja -DTEST=true -DCMAKE_BUILD_TYPE=Debug .. && ninja && ninja test
#cmake -DTEST=true -DEVM_GAS=true -GNinja -DCMAKE_BUILD_TYPE=Release .. && ninja
cd ../scripts
