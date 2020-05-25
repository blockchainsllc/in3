#!/bin/sh
cd ..
docker run \
  --rm \
  -v $(pwd):/src \
  docker.slock.it/build-images/cmake:gcc10 \
  /bin/bash -c "cd /src; rm -rf build; mkdir build; cd build; cmake -DGCC_ANALYZER=true -DCMAKE_BUILD_TYPE=DEBUG -DDEBUG=true .. && make -j8"
cd scripts
