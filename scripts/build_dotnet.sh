#!/bin/sh
cd ..

CMD="cd $PWD"
CMD="$CMD; mkdir -p dotnet/In3/runtimes/osx-x64/native"
CMD="$CMD; mkdir -p dotnet/In3/runtimes/linux-x64/native"
CMD="$CMD; cp build/lib/libin3.dylib dotnet/In3/runtimes/osx-x64/native/libin3.dylib"
CMD="$CMD; cp build/lib/libin3.dylib dotnet/In3/runtimes/linux-x64/native/libin3.so"
CMD="$CMD; cd dotnet"
CMD="$CMD; mkdir Release"
CMD="$CMD; dotnet build -c Release"
CMD="$CMD; cp -r In3/bin/Release/* Release/"

echo $CMD

docker run \
  --rm \
  -v $PWD:/$PWD \
 docker.slock.it/build-images/dotnet-core-sdk:3.1 \
  /bin/bash -c "$CMD"

cd scripts
