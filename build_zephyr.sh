#!/bin/sh
cd src/zephyr
mkdir build
cd build
export ZEPHYR_BASE=/Users/simon/ws/in3/c/in3-c
rm -rf *
cmake -GNinja -DZEPHYR=true -DBOARD=nrf52840_pca10056 ..
ninja
