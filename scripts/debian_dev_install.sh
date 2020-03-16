#!/bin/bash
### This file is meant to be used by the debian package manager, It moves the binary file to the destination needed by debian

set -ex # tells script to exit in case of a non-zero return value

mkdir -p "$DESTDIR/usr/lib/in3-dev" # create directory in case it doesn't exist, should always exist though
mkdir -p "$DESTDIR/usr/lib/in3" # create directory in case it doesn't exist, should always exist though

#cp build/bin/in3 "$DESTDIR/usr/bin/in3-dev/" # move binary file to location where it can be used
mkdir -p "$DESTDIR/usr/include/in3-dev/"
mkdir -p "$DESTDIR/usr/include/in3/"

cp -r build/lib/* "$DESTDIR/usr/lib/in3/" # moving lib files
cp -r c/include/in3/*.h "$DESTDIR/usr/include/in3/" # moving include files