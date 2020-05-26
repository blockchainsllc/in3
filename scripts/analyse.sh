#!/bin/sh
cd ..
docker run \
  --rm \
  -v $(pwd):$(pwd) \
  docker.slock.it/build-images/cmake:clang11 \
  /bin/bash -c "cd $(pwd); rm -rf build; mkdir build; cd build; scan-build-11 cmake -DEVM_GAS=true -DCMAKE_BUILD_TYPE=DEBUG -DIN3_SERVER=true .. &&  scan-build-11 --exclude ../c/src/third-party --force-analyze-debug-code -o report  make -j8"
cd scripts
