#!/bin/sh
cd ..
docker run \
  --rm \
  -v $(pwd):/src \
  silkeh/clang:dev \
  /bin/bash -c "cd /src; rm -rf build; mkdir build; cd build; cmake -DIN3API=true -DIN3_LIB=false -DCMD=false -DUSE_CURL=false -DTEST=true -DCODE_COVERAGE=true -DUSE_SEGGER_RTT=false -DTRANSPORTS=false -DCMAKE_BUILD_TYPE=Debug ..  && make -j8 &&  make ccov-all && make ccov-all-report && cat ccov/binaries.list | xargs llvm-cov export -instr-profile ccov/all-merged.profdata -format=lcov  > lcov.info; cp -r ccov/all-merged ../coverage_report"
cd scripts
