#!/bin/sh
OPTS="-enable-checker alpha.security.ArrayBoundV2"
OPTS="$OPTS -enable-checker alpha.core.CastSize"
#OPTS="$OPTS -enable-checker alpha.core.CastToStruct"
#OPTS="$OPTS -enable-checker alpha.core.PointerArithm"
OPTS="$OPTS -enable-checker alpha.core.PointerSub"
OPTS="$OPTS -enable-checker alpha.core.TestAfterDivZero"

cd ..
docker run \
  --rm \
  -v $(pwd):$(pwd) \
  docker.slock.it/build-images/cmake:clang11 \
  /bin/bash -c "cd $(pwd); rm -rf build; mkdir build; cd build; scan-build-11 $OPTS cmake -DEVM_GAS=true -DCMAKE_BUILD_TYPE=DEBUG -DIN3_SERVER=true .. &&  scan-build-11 $OPTS --exclude ../c/src/third-party --force-analyze-debug-code -o report  make -j8"
cd scripts
