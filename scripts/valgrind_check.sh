#!/bin/sh
VALGRIND_OPTS="-v -q --track-origins=yes  --xtree-memory=full --num-callers=50 --error-exitcode=1 --leak-check=full --track-fds=yes --main-stacksize=4000  --show-leak-kinds=definite --suppressions=suppress.valgrind"
cd ..
if [ ! -f build/suppress.valgrind ]; then
    rm -rf build/*
    printf "{\n  ignore_libcrypto_conditional_jump_errors\n  Memcheck:Leak\n  ...\n  obj:*/libcrypto.so.*\n}\n"  > build/suppress.valgrind
    docker run \
      --rm \
      -v $(pwd):$(pwd) \
      docker.slock.it/build-images/cmake:valgrind \
      /bin/bash -c "cd /$(pwd)/build;  cmake -DCMAKE_BUILD_TYPE=Debug -DZKSYNC=true -DTEST=true  .."
fi

# build
docker run --rm -v $(pwd):$(pwd)  docker.slock.it/build-images/cmake:valgrind  /bin/bash -c "cd /$(pwd)/build;  make -j8; valgrind $VALGRIND_OPTS --xtree-memory-file=$(pwd)/valgrind.kcg $(pwd)/build/bin/in3 -fi $(pwd)/$1"


cd scripts
