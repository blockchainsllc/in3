#!/bin/sh
#VALGRIND_OPTS="-v -q --track-origins=yes --read-var-info=yes --xtree-memory=full --num-callers=50 --error-exitcode=1 --leak-check=full --track-fds=yes --main-stacksize=800  --show-leak-kinds=definite --suppressions=suppress.valgrind"
VALGRIND_OPTS="-v -q --track-origins=yes  --xtree-memory=full --num-callers=50 --error-exitcode=1 --leak-check=full --track-fds=yes --main-stacksize=5000  --show-leak-kinds=definite --suppressions=suppress.valgrind"
cd ..
if [ ! -f build/suppress.valgrind ]; then
    rm -rf build/*
    printf "{\n  ignore_libcrypto_conditional_jump_errors\n  Memcheck:Leak\n  ...\n  obj:*/libcrypto.so.*\n}\n"  > build/suppress.valgrind
    docker run \
      --rm \
      -v $(pwd):$(pwd) \
      docker.slock.it/build-images/cmake:valgrind \
      /bin/bash -c "cd /$(pwd)/build;  cmake -DCMAKE_BUILD_TYPE=Release -DTEST=true  .."
fi

# build
docker run --rm -v $(pwd):$(pwd)  docker.slock.it/build-images/cmake:valgrind  /bin/bash -c "cd /$(pwd)/build;  make -j8"

rm -rf build/test/test*.kcg
rm -rf build/test/test*.out
rm -rf build/test/test*.txt
clear
# run tests
for f in build/test/test*; do 
  docker run --rm -v $(pwd):$(pwd)  docker.slock.it/build-images/cmake:valgrind  /bin/bash -c "cd $(pwd)/build; valgrind $VALGRIND_OPTS --xtree-memory-file=$(pwd)/$f.kcg $(pwd)/$f"
done

cd scripts
