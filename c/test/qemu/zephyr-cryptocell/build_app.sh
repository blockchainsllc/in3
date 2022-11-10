#!/bin/sh
rm -rf build
ccache -C
docker container prune -f
docker run \
  -v $PWD:/in3 \
  docker.slock.it/build-images/cmake:nrf-connect-sdk \
  /bin/bash -c "cd in3; mkdir build; cd build; cmake -DCRYPTOCELL=true -DRPC_ONLY=in3_signData -DED25519=false -DBOARD=nrf5340dk_nrf5340_cpuapp -DARM_MBEDTLS_PATH=/ncs/mbedtls/ ../c/test/qemu/zephyr-cryptocell/; make -j8"
