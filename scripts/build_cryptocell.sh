#!/bin/sh
cd ..
rm -rf build
ccache -C
docker container prune -f
docker run \
  -v $PWD:/in3 \
  docker.slock.it/build-images/cmake:nrf-connect-sdk \
  /bin/bash -c "cd in3; mkdir build; cd build; cmake -DCRYPTOCELL=true -DBOARD=nrf5340dk_nrf5340_cpuapp -DARM_MBEDTLS_PATH=/ncs/mbedtls/ -DCMD=false  -DTRANSPORTS=false -DUSE_CURL=false -DNODESELECT_DEF_WL=false -DUSE_SCRYPT=false -DTHREADSAFE=false -DCMAKE_BUILD_TYPE=Release -DZKSYNC=false -DBASE64=false -DIPFS=false -DMULTISIG=false -DPK_SIGNER=false -DSOL=false -DBTC=false -DIN3API=false ..; make -j8"