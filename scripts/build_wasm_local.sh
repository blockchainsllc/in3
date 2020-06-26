#!/bin/sh
source ~/ws/tools/emsdk/emsdk_env.sh > /dev/null
cd ../build
rm -rf *
cp ../scripts/cmake_modules/CMakeGraphVizOptions.cmake .
emcmake cmake --graphviz=in3.dot -DCMAKE_EXPORT_COMPILE_COMMANDS=true -DWASM=true -DASMJS=false -DWASM_EMMALLOC=true  -DWASM_EMBED=false  -DCMAKE_BUILD_TYPE=DEBUG .. 
dot -Tsvg -o in3.svg in3.dot 
make -j8 in3_wasm
cd ../scripts

# M "command": "/Users/simon/ws/tools/emsdk/upstream/emscripten/emcc -DBTC -DDEV_NO_INTRN_PTR -DERR_MSG -DETH_API -DETH_FULL -DEVM_GAS -DIN3_MATH_LITE -DIN3_VERSION=\\\"2.2.2-local\\\" -DIN3_VERSION_MAJOR=2 -DIN3_VERSION_MINOR=2 -DIN3_VERSION_PATCH=2-local -DIPFS -DLOG_USE_COLOR -DUSE_PRECOMPUTED_CP=1  -DNDEBUG -Os   -o CMakeFiles/in3w.dir/wasm.c.o   -c /Users/simon/ws/in3/c/in3-core/wasm/src/wasm.c",
# R "command": "/Users/simon/ws/tools/emsdk/upstream/emscripten/emcc -DBTC -DDEV_NO_INTRN_PTR -DERR_MSG -DETH_API -DETH_FULL -DEVM_GAS -DIN3_MATH_LITE -DIN3_VERSION=\\\"2.2.2-local\\\" -DIN3_VERSION_MAJOR=2 -DIN3_VERSION_MINOR=2 -DIN3_VERSION_PATCH=2-local -DIPFS -DLOG_USE_COLOR -DUSE_PRECOMPUTED_CP=1  -DNDEBUG -O2   -o CMakeFiles/in3w.dir/wasm.c.o   -c /Users/simon/ws/in3/c/in3-core/wasm/src/wasm.c",
