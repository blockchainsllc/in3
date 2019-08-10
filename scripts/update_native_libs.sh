#!/bin/sh

#!/bin/sh
cd ..

echo "windows ..."
docker run \
  --rm \
  -v $(pwd):/src \
  docker.slock.it/build-images/cmake:gcc7-mingw \
  /bin/bash -c "cd /src; rm -rf build; mkdir build; cd build; cmake -DCMAKE_BUILD_TYPE=MINSIZEREL -DJAVA=true .. && make in3_jni"
cp build/lib/libin3_jni.so src/bindings/java/in3/native/in3_jni.dll

echo "linux ..."
docker run \
  --rm \
  -v $(pwd):/src \
  docker.slock.it/build-images/cmake:gcc8 \
  /bin/bash -c "cd /src; rm -rf build; mkdir build; cd build; cmake -DCMAKE_BUILD_TYPE=MINSIZEREL -DJAVA=true .. && make in3_jni"
cp build/lib/libin3_jni.so src/bindings/java/in3/native/

echo "mac ..."
cd build
rm -rf *
cmake -DCMAKE_BUILD_TYPE=MINSIZEREL -DJAVA=true .. && make in3_jni
cd ..
cp build/lib/libin3_jni.dylib src/bindings/java/in3/native/


cd scripts
