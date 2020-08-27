#!/bin/sh
CWD=$PWD
BUILDTYPE=$2
CONTAINER=$1
TEST=false

if [ "$CONTAINER" = "debug" ]; then
   BUILDTYPE=debug
   CONTAINER=""
   TEST=true
fi
if [ "$CONTAINER" = "sentry" ]; then
   BUILDTYPE=debug
   CONTAINER=""
   TEST=true
   OPTS="-DLOGGING=true -DIN3SENTRY=true -DJAVA=false -DSENTRY_BACKEND=inproc -DBUILD_SHARED_LIBS=false"
fi
if [ "$CONTAINER" = "release" ]; then
   BUILDTYPE=release
   CONTAINER=""
fi
if [ "$CONTAINER" = "bindings" ]; then
   BUILDTYPE=release
   CONTAINER=""
#   OPTS="-DUSE_CURL=false"
fi
if [ "$CONTAINER" = "bindings-debug" ]; then
   BUILDTYPE=debug
   CONTAINER=""
   OPTS="-DUSE_CURL=false"
fi
if [ -z "$BUILDTYPE" ]; then
   BUILDTYPE=DEBUG
   TEST=true
fi
if [ "$BUILDTYPE" = "release" ]; then
   BUILDTYPE=MINSIZEREL
fi
if [ "$BUILDTYPE" = "debug" ]; then
   BUILDTYPE=DEBUG
   TEST=true
fi
OPTS="-DCMAKE_EXPORT_COMPILE_COMMANDS=true -DTEST=$TEST -DBUILD_DOC=$TEST -DJAVA=$TEST -DCMAKE_BUILD_TYPE=$BUILDTYPE $OPTS "

if [ "$CONTAINER" = "--help" ]; then
   echo "usage $0 <TARGET> <DEBUG|MINSIZEREL|RELEASE|debug|release> "
   echo "  <TARGET> could be one of the following:"
   echo "     - android-clang8-armv8"
   echo "     - centos"
   echo "     - clang11"
   echo "     - clang10"
   echo "     - clang9"
   echo "     - clang50"
   echo "     - gcc10"
   echo "     - gcc8-x86"
   echo "     - gcc8"
   echo "     - gcc8-armv7"
   echo "     - gcc8-armv7hf"
   echo "     - gcc7-mingw"
   echo "     - gcc5"
   echo "     - gcc-legacy"
   echo "     - wasm"
   echo "     - wasm_local"
   echo "     - asmjs"
   echo "     - asmjs_local"
   echo "     - win"
   echo "     - cortexm3"
   echo "     - esp"
   exit
fi


cd $(dirname $0)/..
RD=$(pwd)
mkdir build || echo "using existng build-folder ..."
rm -rf build/*
echo "container: $CONTAINER"
if [ -z "$CONTAINER" ]; then
  echo "local_build"
  touch build/container.txt
  cd build
  cmake $OPTS .. && make -j8
elif [ "$CONTAINER" = "win" ]; then
  CONTAINER=docker.slock.it/build-images/cmake:gcc7-mingw
  echo $CONTAINER > build/container.txt
  docker run  --rm -v $RD:$RD  $CONTAINER \
    /bin/bash -c "cd $RD/build; cmake -DCMAKE_BUILD_TYPE=MINSIZEREL -DJAVA=false -DLEDGER_NANO=false -DUSE_CURL=false -DUSE_WINHTTP=true .. && make -j8"
elif [ "$CONTAINER" = "cortexm3" ]; then
  CONTAINER=docker.io/zephyrprojectrtos/zephyr-build:v0.12
  echo $CONTAINER > build/container.txt
  docker run  --rm -v $RD:$RD  $CONTAINER \
    /bin/bash -c "cd $RD;west init -m https://github.com/zephyrproject-rtos/zephyr --mr v2.0.0;export ZEPHYR_BASE=/builds/in3/c/in3-core;source /builds/in3/c/in3-core/zephyr/zephyr-env.sh;west build -b qemu_cortex_m3 c/test/qemu/zephyr-arm3"
elif [ "$CONTAINER" = "esp" ]; then
  CONTAINER=docker.slock.it/build-images/cmake:esp
  echo $CONTAINER > build/container.txt
  docker run  --rm -v $RD:$RD  $CONTAINER \
    /bin/bash -c "cd $RD;cp -avi scripts/qemu_xtensa.sh /opt/qemu;cd c/test/qemu/esp32;idf.py build;./make-flash-img.sh in3-espidf flash_image.bin"
elif [ "$CONTAINER" = "wasm_local" ]; then
  cd build
  source ~/ws/tools/emsdk/emsdk_env.sh > /dev/null
  emcmake cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=true -DWASM=true -DASMJS=false -DWASM_EMMALLOC=true  -DWASM_EMBED=false -DCMAKE_BUILD_TYPE=$BUILDTYPE .. 
  make -j8 in3_wasm
elif [ "$CONTAINER" = "wasm" ]; then
  CONTAINER=docker.slock.it/build-images/cmake:clang11
  echo $CONTAINER > build/container.txt
  docker run --rm -v $RD:$RD $CONTAINER /bin/bash -c "cd $RD/build; emcmake cmake -DWASM=true -DASMJS=false -DWASM_EMMALLOC=true  -DWASM_EMBED=false -DCMAKE_BUILD_TYPE=$BUILDTYPE ..  && make -j8"
elif [ "$CONTAINER" = "asmjs_local" ]; then
  cd build
  source ~/ws/tools/emsdk/emsdk_env.sh > /dev/null
  emcmake cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=true -DWASM=true -DASMJS=true -DWASM_EMMALLOC=true  -DCMAKE_BUILD_TYPE=$BUILDTYPE .. 
  make -j8 in3_wasm
elif [ "$CONTAINER" = "asmjs" ]; then
  CONTAINER=docker.slock.it/build-images/cmake:clang11
  echo $CONTAINER > build/container.txt
  docker run --rm -v $RD:$RD $CONTAINER /bin/bash -c "cd $RD/build; emcmake cmake -DWASM=true -DASMJS=true -DWASM_EMMALLOC=true -DCMAKE_BUILD_TYPE=$BUILDTYPE ..  && make -j8"
else                                  
  echo "build $CONTAINER"
  echo docker.slock.it/build-images/cmake:$CONTAINER > build/container.txt
  docker run --rm -v $RD:$RD docker.slock.it/build-images/cmake:$CONTAINER /bin/bash -c "cd $RD/build; cmake $OPTS ..  && make -j8"
fi

cd $CWD
