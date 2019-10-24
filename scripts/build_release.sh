#!/bin/sh
cd ..
mkdir -p build
cd build
rm -rf *
cmake -DCMAKE_BUILD_TYPE=MINSIZEREL -DIN3_SERVER=true -DPOA=false .. && make -j8
cd ../scripts
