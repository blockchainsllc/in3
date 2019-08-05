#!/bin/sh
cd ..
docker run \
  -v $(pwd)/src:/src \
  docker.slock.it/build-images/zephyr:arm-0_10_0__v1_14_0 \
  bash -c "source /zephyr/zephyr-env.sh; cd /src/sample; rm -rf build; mkdir build; cd build; cmake -DBOARD=nrf52840_pca10056 -DCMAKE_BUILD_TYPE=Release ..; make -j8"
cd scripts
