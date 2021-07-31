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
   OPTS="-DLOGGING=true -DRECORDER=true -DSENTRY=true -DJAVA=false -DSENTRY_BACKEND=crashpad -DBUILD_SHARED_LIBS=false"
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
   BUILDTYPE=DEBUGG
   #TEST=true
fi
if [ "$BUILDTYPE" = "release" ]; then
   BUILDTYPE=MINSIZEREL
fi
if [ "$BUILDTYPE" = "debug" ]; then
   BUILDTYPE=DEBUG
   TEST=true
fi
[ -z "$VERSION" ] && VERSION=$CI_COMMIT_TAG
[ -z "$VERSION" ] && VERSION=3.1.0-debug

MIN_OPTS="-DUSE_SCRYPT=false -DETH_BASIC=true -DETH_FULL=false -DTHREADSAFE=false -DMULTISIG=false -DLOGGING=false -DCMAKE_EXPORT_COMPILE_COMMANDS=true -DTAG_VERSION=$VERSION -DZKCRYPTO_LIB=true -DTEST=$TEST -DBUILD_DOC=$TEST -DJAVA=$TEST -DZKSYNC=false -DIN3_SERVER=false -DBTC=false -DIPFS=false -DCMAKE_BUILD_TYPE=$BUILDTYPE $OPTS "
FULL_OPTS="-DUSE_SCRYPT=true -DETH_BASIC=true -DETH_FULL=true -DTHREADSAFE=true -DMULTISIG=true -DLOGGING=true -DCMAKE_EXPORT_COMPILE_COMMANDS=true -DTAG_VERSION=$VERSION -DZKCRYPTO_LIB=true -DTEST=$TEST -DBUILD_DOC=$TEST -DJAVA=$TEST -DZKSYNC=true -DIN3_SERVER=true -DBTC=true -DIPFS=true -DCMAKE_BUILD_TYPE=$BUILDTYPE $OPTS "

OPTS=$FULL_OPTS

if [ "$CONTAINER" = "--help" ]; then
   echo "usage $0 <TARGET> <DEBUG|MINSIZEREL|RELEASE|debug|release> "
   echo "  <TARGET> could be one of the following:"
   echo "     - android-clang8-armv8"
   echo "     - centos"
   echo "     - android"
   echo "     - clang13"
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
  cmake $OPTS .. && make -j 8
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
elif [ "$CONTAINER" = "android" ]; then
  CONTAINER=cangol/android-gradle
  echo $CONTAINER > build/container.txt
  CMD="cd $RD;export GRADLE_USER_HOME=$RD/.gradle"
  CMD="$CMD;wget https://services.gradle.org/distributions/gradle-6.8.1-bin.zip && unzip gradle-6.8.1-bin.zip && export PATH=$RD/gradle-6.8.1/bin:/usr/local/bin:/android-sdk-linux/tools:/android-sdk-linux/platform-tools:/usr/local/gradle-5.4.1/bin:/usr/local/openjdk-8/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
  CMD="$CMD;git clone https://github.com/slockit/in3-example-android.git;cd in3-example-android"
  CMD="$CMD;ln -s ../ in3"
  CMD="$CMD;gradle --stacktrace build"
  echo $CMD
  docker run  --rm -v $RD:$RD  $CONTAINER /bin/bash -c "$CMD"
elif [ "$CONTAINER" = "wasm_local" ]; then
  cd build
#  source ~/ws/tools/emsdk/emsdk_env.sh > /dev/null
  emcmake cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=true -DWASM=true -DASMJS=false -DWASM_EMMALLOC=true -DIPFS=false -DZKSYNC=true -DWASM_EMBED=false -DCMAKE_BUILD_TYPE=$BUILDTYPE .. 
  make -j8 in3_wasm
elif [ "$CONTAINER" = "wasm" ]; then
  CONTAINER=docker.slock.it/build-images/cmake:clang13
  echo $CONTAINER > build/container.txt
  docker run --rm -v $RD:$RD $CONTAINER /bin/bash -c "cd $RD/build; emcmake cmake -DWASM=true -DASMJS=false -DBTC_PRE_BPI34=false -DWASM_EMMALLOC=true -DZKSYNC=true -DWASM_EMBED=false -DCMAKE_BUILD_TYPE=$BUILDTYPE ..  && make -j8"
elif [ "$CONTAINER" = "asmjs_local" ]; then
  cd build
  source ~/ws/tools/emsdk/emsdk_env.sh > /dev/null
  emcmake cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=true -DWASM=true -DASMJS=true -DZKSYNC=false -DBTC=true -DBTC_PRE_BPI34=false -DIPFS=true -DWASM_EMMALLOC=true  -DCMAKE_BUILD_TYPE=$BUILDTYPE .. 
  make -j8 in3_wasm
elif [ "$CONTAINER" = "asmjs" ]; then
  CONTAINER=docker.slock.it/build-images/cmake:clang13
  echo $CONTAINER > build/container.txt
  docker run --rm -v $RD:$RD $CONTAINER /bin/bash -c "cd $RD/build; emcmake cmake -DWASM=true -DASMJS=true -DWASM_EMMALLOC=true -DCMAKE_BUILD_TYPE=$BUILDTYPE ..  && make -j8"
else                                  
  echo "build $CONTAINER"
  echo docker.slock.it/build-images/cmake:$CONTAINER > build/container.txt
  docker run --rm -v $RD:$RD docker.slock.it/build-images/cmake:$CONTAINER /bin/bash -c "cd $RD/build; cmake $OPTS ..  && make -j8"
fi

cd $CWD
