#!/bin/sh
IN3=../..
IN3_LIB=$IN3/build/lib/
IN3_INCLUDE=$IN3/include

# build
for f in *.c; 
  do gcc -o "${f%%.*}" $f -L$IN3_LIB -I$IN3_INCLUDE -lin3_bundle -lcurl
done

