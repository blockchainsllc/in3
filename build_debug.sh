#!/bin/sh
mkdir -p build
cd build
rm -rf * 
conan install .. --build missing
cmake -DCMAKE_BUILD_TYPE=Debug .. && make
#cmake -GNinja -DBOARD=nrf52840_pca10056 .. && ninja
#cmake -GNinja -DZEPHYR=true -DBOARD=nrf52840_pca10056 .. && ninja
#cmake -DCMAKE_BUILD_TYPE=Debug .. && make
cd ..
