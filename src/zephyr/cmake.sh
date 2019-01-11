#!/bin/bash
# remove build folder if it exists
if [ -d build ]; then
	rm -r build
fi
# create new one and move to
mkdir build
cd build       
# call cmake for USB board
#cmake -GNinja -DBOARD=nrf52840_pca10059 ..
# call cmake for SDK board
cmake -GNinja -DBOARD=nrf52840_pca10056 ..
echo -n "press any key..."
read
