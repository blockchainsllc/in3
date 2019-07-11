#!/bin/sh
cd ..
rm -rfv include
mkdir -p include/in3
find src -type d > include/in3/.dirs
cd include/in3
xargs mkdir -p < .dirs
cd ../../
find src -name "*.h" > include/in3/.headers
cd include/in3
xargs touch < .headers

