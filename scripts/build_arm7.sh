#!/bin/sh
cd ..
docker run \
  --rm \
  -v $(pwd):/src \
  docker.slock.it/build-images/cmake:gcc8-armv7 \
  /bin/bash -c "cd /src; rm -rf build; mkdir build; cd build; cmake -DJAVA=true -DCMAKE_BUILD_TYPE=MinSizeRel -DIN3_SERVER=true .. && make -j8"
cd scripts
