#!/bin/sh
cd ..
docker run \
  --rm \
  -v $(pwd):/src \
  docker.slock.it/build-images/cmake:gcc7-mingw \
  /bin/bash -c "cd /src; rm -rf build; mkdir build; cd build; cmake -DCMAKE_BUILD_TYPE=MINSIZEREL -DUSE_CURL=true -DLIBCURL_LINKTYPE=static .. && make -j8 in3"
cd scripts
