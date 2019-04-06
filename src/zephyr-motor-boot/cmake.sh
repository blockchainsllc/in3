#!/bin/bash
# remove build folder if it exists
if [ -d build ]; then
	rm -r build
fi
# create new one and move to
mkdir build
cd build       
# make (look at CMakeLists.txt for options)
cmake -GNinja ..
# read the results...
echo -n "press any key..."
read
