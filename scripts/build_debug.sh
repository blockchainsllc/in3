#!/bin/sh
cd ..
mkdir -p build
cd build
rm -rf *
export CTEST_OUTPUT_ON_FAILURE=1
cmake -DTEST=true -DBUILD_DOC=true -DJAVA=true -DCMAKE_BUILD_TYPE=Debug -DIN3_STAGING=true .. -DLIBCURL_TYPE=shared && make && make ptest
#cmake -DTEST=true -DEVM_GAS=true -DCMAKE_BUILD_TYPE=Debug .. && make && make test
#cmake -GNinja -DTEST=true -DCMAKE_BUILD_TYPE=Debug .. && ninja && ninja test
#cmake -DTEST=true -DEVM_GAS=true -GNinja -DCMAKE_BUILD_TYPE=Release .. && ninja
cd ../scripts
