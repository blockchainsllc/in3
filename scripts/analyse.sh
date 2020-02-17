#!/bin/sh
cd ..
docker run \
  --rm \
  -v $(pwd):/src \
  docker.slock.it/build-images/cmake:clang9 \
  /bin/bash -c "cd /src; rm -rf build; mkdir build; cd build; scan-build-9 cmake -DEVM_GAS=true -DCMAKE_BUILD_TYPE=DEBUG -DIN3_SERVER=true .. &&  scan-build-9 --exclude ../c/src/third-party --force-analyze-debug-code -o report  make -j8"
cd scripts
