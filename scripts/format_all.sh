#!/bin/sh
cd .. 
docker run --rm -v $(pwd):$(pwd) docker.slock.it/build-images/cmake:clang13  /bin/bash -c \
"cd $PWD; find c/src java -not -path \"c/src/third-party/*\" \\( -name \"*.c\" -o -name \"*.h\" -o -name \"*.java\" \\) | xargs clang-format -i"

cd scripts
