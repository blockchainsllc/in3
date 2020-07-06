#!/bin/sh
CWD=$PWD
cd $(dirname $0)/..
CONTAINER=$(cat build/container.txt)
RD=$(pwd)
TARGET=$1
echo "container: $CONTAINER"
if [ -z "$CONTAINER" ]; then
  echo "local_build"
  cd build
  make $TARGET -j8
else                                  
  echo "build $CONTAINER"
  docker run --rm -v $RD:$RD docker.slock.it/build-images/cmake:$CONTAINER \
  /bin/bash -c "cd $RD/build; make $TARGET -j8"
fi

cd $CWD
