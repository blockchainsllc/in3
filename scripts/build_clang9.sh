#!/bin/sh
cd ..
docker run \
  --rm \
  -v $(pwd):/src \
  docker.slock.it/build-images/cmake:clang9 \
  /bin/bash -c "cd /src; rm -rf build; mkdir build; cd build; cmake -DCMAKE_BUILD_TYPE=MINSIZEREL -DIN3_SERVER=true .. && cmake --graphviz=graph . && make -j8"
cd scripts
