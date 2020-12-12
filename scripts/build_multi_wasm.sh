#!/bin/bash

function build {
  NAME=$1
  OPTIONS=$2
  DST=$(pwd)/"in3_"$NAME 
  # build
  rm -rf $DST
  mkdir -p $DST
  rm -rf wasm_build
  mkdir -p wasm_build
  mkdir -p _build
  cd _build
  emcmake cmake $OPTIONS ..
  make -j8

  # copy to destination
  if [ ! -f "$DST/package.json" ]; then
    cp -rv module/* "$DST/"
  else
    cat module/index.js | sed  "s/in3w.wasm/$NAME.wasm/g" > "$DST/$NAME.js"
    cp -v module/index.d.ts "$DST/$NAME.d.ts"
    [ -f module/in3w.wasm ] && cp -v module/in3w.wasm "$DST/$NAME.wasm"
  fi

  # clean up
  cd ..
  cp -vr _build wasm_build
  rm -rf _build

}

CWD=$PWD
cd $(dirname $0)/..
OPTS="-DWASM=true -DTRANSPORTS=false -DIN3_LIB=false -DWASM_EMMALLOC=true  -DBUILD_DOC=false -DUSE_CURL=false -DCMD=false   -DTAG_VERSION=$2"
WASM="-DCMAKE_BUILD_TYPE=MINSIZEREL -DWASM_EMBED=false"
ASMJS="-DCMAKE_BUILD_TYPE=RELEASE -DWASM_EMBED=true -DASMJS=true"
if [ "$3" = "debug" ]; then
  WASM="-DCMAKE_BUILD_TYPE=DEBUG -DWASM_EMBED=false"
  ASMJS="-DCMAKE_BUILD_TYPE=DEBUG -DASMJS=true"
fi

# targets
opt=$1
case $opt in
    -i|--index)
      build index       "$OPTS $ASMJS -DBTC=true  -DZKSYNC=false -DIPFS=true" 
    ;;
    -w|--wasm)
      build wasm        "$OPTS $WASM  -DBTC=true  -DZKSYNC=false -DIPFS=true" 
    ;;
    -zw|--zksync-wasm)
      build zksync-wasm "$OPTS $WASM  -DBTC=false -DZKSYNC=true"
    ;;
    -z|--zksync)
      build zksync      "$OPTS $ASMJS -DBTC=false -DZKSYNC=true" 
    ;;
    -b|--btc-wasm)
      build btc-wasm    "$OPTS $WASM  -DBTC=true  -DZKSYNC=false -DIPFS=false -DETH_BASIC=false -DETH_FULL=false -DUSE_SCRYPT=false" 
    ;;
    -b|--btc)
      build btc         "$OPTS $ASMJS -DBTC=true  -DZKSYNC=false -DIPFS=false -DETH_BASIC=false -DETH_FULL=false -DUSE_SCRYPT=false" 
    ;;
    -mw|--min-wasm)
      build min-wasm    "$OPTS $WASM  -DBTC=false -DZKSYNC=false -DIPFS=false -DETH_BASIC=false -DETH_FULL=false -DUSE_SCRYPT=false -DIN3API=false" 
    ;;
    -mi|--min)
      build min         "$OPTS $ASMJS -DBTC=false -DZKSYNC=false -DIPFS=false -DETH_BASIC=false -DETH_FULL=false -DUSE_SCRYPT=false -DIN3API=false" 
    ;;
    -a|--all)
      # targets
      build index       "$OPTS $ASMJS -DBTC=true  -DZKSYNC=false -DIPFS=true" 
      build wasm        "$OPTS $WASM  -DBTC=true  -DZKSYNC=false -DIPFS=true" 
      build zksync-wasm "$OPTS $WASM  -DBTC=false -DZKSYNC=true"
      build zksync      "$OPTS $ASMJS -DBTC=false -DZKSYNC=true" 
      build btc-wasm    "$OPTS $WASM  -DBTC=true  -DZKSYNC=false -DIPFS=false -DETH_BASIC=false -DETH_FULL=false -DUSE_SCRYPT=false" 
      build btc         "$OPTS $ASMJS -DBTC=true  -DZKSYNC=false -DIPFS=false -DETH_BASIC=false -DETH_FULL=false -DUSE_SCRYPT=false" 
      build min-wasm    "$OPTS $WASM  -DBTC=false -DZKSYNC=false -DIPFS=false -DETH_BASIC=false -DETH_FULL=false -DUSE_SCRYPT=false -DIN3API=false" 
      build min         "$OPTS $ASMJS -DBTC=false -DZKSYNC=false -DIPFS=false -DETH_BASIC=false -DETH_FULL=false -DUSE_SCRYPT=false -DIN3API=false" 

      # go back to where we came from
      cd 
      cp -r build/$DST wasm/test/in3
      cd $CWD
    ;;
    -m|--help)
        echo 'Usage: %s <options> build multiple wasm enabled feature for in3... 
    -i,--index        build index       "$OPTS $ASMJS -DBTC=true  -DZKSYNC=false -DIPFS=true" 
    -w,--wasm         build wasm        "$OPTS $WASM  -DBTC=true  -DZKSYNC=false -DIPFS=true" 
    -zw,--zksync-wasm build zksync-wasm "$OPTS $WASM  -DBTC=false -DZKSYNC=true"
    -z,--zksync       build zksync      "$OPTS $ASMJS -DBTC=false -DZKSYNC=true" 
    -b,--btc-wasm     build btc-wasm    "$OPTS $WASM  -DBTC=true  -DZKSYNC=false -DIPFS=false -DETH_BASIC=false -DETH_FULL=false -DUSE_SCRYPT=false" 
    -b,--btc          build btc         "$OPTS $ASMJS -DBTC=true  -DZKSYNC=false -DIPFS=false -DETH_BASIC=false -DETH_FULL=false -DUSE_SCRYPT=false" 
    -mw,--min-wasm    build min-wasm    "$OPTS $WASM  -DBTC=false -DZKSYNC=false -DIPFS=false -DETH_BASIC=false -DETH_FULL=false -DUSE_SCRYPT=false -DIN3API=false" 
    -mi,--min         build min         "$OPTS $ASMJS -DBTC=false -DZKSYNC=false -DIPFS=false -DETH_BASIC=false -DETH_FULL=false -DUSE_SCRYPT=false -DIN3API=false" 
    -a,--all build all configurations'
        exit 0
    ;;
esac

