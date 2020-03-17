#!/bin/sh

# we check if incubed is installed, if not we try to use the local incubed-directory.
if [ ! -d /usr/local/include/in3 ]; then

  echo "Using local incubed build..."
  if [ ! -d ../../build/lib ]; then
     # not installed yet, so let'sa build it locally
     mkdir -p ../../build
     cd ../../build
     cmake .. && make 
     cd ../c/examples
  fi

  # set the library path to use the local
  BUILDARGS="-L../../build/lib/ -I../../c/include"
fi

# now build the examples build
for f in *.c; 
  do gcc -std=c99 -o "${f%%.*}" $f $BUILDARGS -lin3 -D_POSIX_C_SOURCE=199309L
done

