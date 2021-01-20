#!/bin/bash
function build {
  NAME=$1
  OPTIONS=$2
  
  # build
  mkdir -p _build
  cd _build
  emcmake cmake $OPTIONS ..
  make -j8

  # copy to destination
  if [ ! -f "$DST/package.json" ]; then
    cp -r module/* "$DST/"
  else
    cat module/index.js | sed  "s/in3w.wasm/$NAME.wasm/g" > "$DST/$NAME.js"
    cp module/index.d.ts "$DST/$NAME.d.ts"
    [ -f module/in3w.wasm ] && cp module/in3w.wasm "$DST/$NAME.wasm"
  fi

  # clean up
  cd ..
  rm -rf _build

}

# define options
source ~/ws/tools/emsdk/emsdk_env.sh > /dev/null
CWD=$PWD
cd $(dirname $0)/..
DST="$1"
OPTS="-DWASM=true -DTRANSPORTS=false -DIN3_LIB=false -DWASM_EMMALLOC=true  -DBUILD_DOC=false -DUSE_CURL=false -DCMD=false   -DTAG_VERSION=$2"
WASM="-DCMAKE_BUILD_TYPE=MINSIZEREL -DWASM_EMBED=false"
ASMJS="-DCMAKE_BUILD_TYPE=RELEASE -DWASM_EMBED=true -DASMJS=true"
if [ "$3" = "debug" ]; then
  WASM="-DCMAKE_BUILD_TYPE=DEBUG -DWASM_EMBED=false"
  ASMJS="-DCMAKE_BUILD_TYPE=DEBUG -DASMJS=true"
fi

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
