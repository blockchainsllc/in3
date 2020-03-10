#!/bin/sh
cd .. 
docker run --rm -v $(pwd):$(pwd) docker.slock.it/build-images/cmake:clang10  /bin/bash -c \
"cd $PWD; find c/src/core/ c/src/cmd/ c/src/api/ c/src/verifier/ c/src/transport/ java \\( -name \"*.c\" -o -name \"*.h\" -o -name \"*.java\" \\) | xargs clang-format -i"

cd scripts
