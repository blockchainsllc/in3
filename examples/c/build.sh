#!/bin/sh
IN3=../..
IN3_LIB=$IN3/build/lib/

# build
for f in *.c; 
  do gcc -o "${f%%.*}" $f -L$IN3_LIB -I$IN3/include -lin3_bundle -lcurl
done

