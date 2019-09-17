#!/bin/sh
DOWNLOAD="$1"
echo "writing to $DOWNLOAD"
cd ..

echo "windows ..."
docker run \
  --rm \
  -v $(pwd):/src \
  docker.slock.it/build-images/cmake:gcc7-mingw \
  /bin/bash -c "cd /src; rm -rf build; mkdir build; cd build; cmake -DCMAKE_BUILD_TYPE=MINSIZEREL -DJAVA=true -DUSE_CURL=true -DLIBCURL_LINKTYPE=static  .. && make in3_jni && make in3"
cp build/lib/libin3_jni.so src/bindings/java/in3/native/in3_jni.dll
cd scripts
cp ../build/bin/in3 "$DOWNLOAD/win/in3.exe"
cd ..

echo "linux ..."
docker run \
  --rm \
  -v $(pwd):/src \
  docker.slock.it/build-images/cmake:gcc8 \
  /bin/bash -c "cd /src; rm -rf build; mkdir build; cd build; cmake -DCMAKE_BUILD_TYPE=MINSIZEREL -DJAVA=true .. && make in3_jni && make in3"
cp build/lib/libin3_jni.so src/bindings/java/in3/native/
cd scripts
cp ../build/bin/in3 "$DOWNLOAD/x64/in3_x64"
cd ..

echo "linux ... x86"
docker run \
  --rm \
  -v $(pwd):/src \
  docker.slock.it/build-images/cmake:gcc8-x86 \
  /bin/bash -c "cd /src; rm -rf build; mkdir build; cd build; cmake -DCMAKE_BUILD_TYPE=MINSIZEREL -DJAVA=true .. && make in3_jni && make in3"
cd scripts
cp ../build/bin/in3 "$DOWNLOAD/x86/in3_x86"
cd ..

echo "arm7 ... "
docker run \
  --rm \
  -v $(pwd):/src \
  docker.slock.it/build-images/cmake:gcc8-armv7 \
  /bin/bash -c "cd /src; rm -rf build; mkdir build; cd build; cmake -DCMAKE_BUILD_TYPE=MINSIZEREL -DUSE_CURL=false .. && make in3"
cd scripts
cp ../build/bin/in3 "$DOWNLOAD/armv7/in3_armv7"
cd ..

echo "arm7hf ... "
docker run \
  --rm \
  -v $(pwd):/src \
  docker.slock.it/build-images/cmake:gcc8-armv7hf \
  /bin/bash -c "cd /src; rm -rf build; mkdir build; cd build; cmake -DCMAKE_BUILD_TYPE=MINSIZEREL -DUSE_CURL=false .. &&  make in3"
cd scripts
cp ../build/bin/in3 "$DOWNLOAD/armv7hf/in3_armv7hf"
cd ..

echo "mac ..."
cd build
rm -rf *
cmake -DCMAKE_BUILD_TYPE=MINSIZEREL  -DIN3_SERVER=true -DJAVA=true .. && make in3_jni && make in3
cp lib/libin3_jni.dylib ../src/bindings/java/in3/native/
cp bin/in3 "$DOWNLOAD/osx/in3_osx"
make in3j
cp lib/in3.jar "$DOWNLOAD/in3.jar"


cd ../scripts
