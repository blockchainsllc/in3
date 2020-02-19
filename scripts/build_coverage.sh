#!/bin/sh
CMAKE_OPTIONS="-DIN3API=true -DIN3_LIB=false -DCMD=false -DUSE_CURL=false -DTEST=true -DCODE_COVERAGE=true -DUSE_SEGGER_RTT=false -DTRANSPORTS=false -DCMAKE_BUILD_TYPE=Debug"
LCOV="../scripts/lcov_report.sh | xargs llvm-cov report && ../scripts/lcov_report.sh | xargs llvm-cov show -show-line-counts-or-regions -output-dir=all-merged -format=html  && ../scripts/lcov_report.sh | xargs llvm-cov export  -format=lcov  > lcov.info"
cd ..
docker run \
  --rm \
  -v $(pwd):/src \
  silkeh/clang:dev \
  /bin/bash -c "cd /src; rm -rf build; mkdir build; cd build; cmake $CMAKE_OPTIONS ..  && make -j8 &&  make ptest && $LCOV"
cd scripts
