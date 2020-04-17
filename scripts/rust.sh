#!/bin/sh
cd ..
docker run \
  --rm \
  -it \
  -v $(pwd):/in3-core \
  docker.slock.it/build-images/cmake:rust
cd scripts
