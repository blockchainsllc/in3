#!/bin/sh
cd ..
mkdir -p build
cd build
rm -rf *
cat <<EOF >../c/include/in3.rs.h
// AUTO-GENERATED FILE
// See scripts/build_rust.sh
#include "../src/core/client/context_internal.h"
#include "in3/bytes.h"
#include "in3/client.h"
#include "in3/context.h"
#include "in3/error.h"
#include "in3/eth_api.h"
#include "in3/in3_init.h"
EOF
cmake -DCMAKE_BUILD_TYPE=MINSIZEREL -DUSE_CURL=false .. && make -j8 && cd ../rust/ && cargo build --examples --tests
cd ../scripts
