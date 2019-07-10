#!/bin/sh
docker run \
  --rm \
  -v $(pwd):/src \
  docker.slock.it/build-images/cmake:gcc5 \
  /bin/bash -c "cd /src; rm -rf build; mkdir build; cd build; cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8"
