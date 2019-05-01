#!/bin/bash
if [ -d build ]; then
	cd build
	ninja
	cd ..
else
	echo build missing !!!
fi
echo -n "press any key..."
read
