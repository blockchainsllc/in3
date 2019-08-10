#!/bin/sh

#!/bin/sh
cd ..

echo "windows ..."
docker run \
  --rm \
  -v $(pwd):/src \
  docker.slock.it/build-images/cmake:gcc7-mingw \
  /bin/bash -c "cd /src; rm -rf build; mkdir build; cd build; cmake -DCMAKE_BUILD_TYPE=MINSIZEREL -DJAVA=true .. && make libin3"
cp build/lib/libin3.so src/bindings/java/in3/native/in3.dll

echo "linux ..."
docker run \
  --rm \
  -v $(pwd):/src \
  docker.slock.it/build-images/cmake:gcc8 \
  /bin/bash -c "cd /src; rm -rf build; mkdir build; cd build; cmake -DCMAKE_BUILD_TYPE=MINSIZEREL -DJAVA=true .. && make libin3"
cp build/lib/libin3.so src/bindings/java/in3/native/libin3.so

echo "mac ..."
cd build
rm -rf *
cmake -DCMAKE_BUILD_TYPE=MINSIZEREL -DJAVA=true .. && make libin3
cd ..
cp build/lib/libin3.dylib src/bindings/java/in3/native/


cd scripts
