#!/bin/sh
cd ..
docker run \
  --rm \
  -v $(pwd):/src \
  docker.slock.it/build-images/cmake:clang9 \
  /bin/bash -c "cd /src; rm -rf build; mkdir build; cd build; scan-build-9 cmake -DCMAKE_BUILD_TYPE=DEBUG -DIN3_SERVER=true .. && scan-build-9 -v --force-analyze-debug-code make -j8 && cp -r /tmp/scan-build* ."
cd scripts
