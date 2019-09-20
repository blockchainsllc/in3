#!/bin/bash
### This file is meant to be used by the debian package manager, It moves the binary file to the destination needed by debian

set -ex # tells script to exit in case of a non-zero return value
#mkdir -p "$DESTDIR/usr/bin/in3-dev/" # create directory in case it doesn't exist, should always exist though
mkdir -p "$DESTDIR/usr/lib/in3-dev" # create directory in case it doesn't exist, should always exist though
#cp build/bin/in3 "$DESTDIR/usr/bin/in3-dev/" # move binary file to location where it can be used
mkdir -p "$DESTDIR/usr/local/include/in3-dev/"
cp build/bin/in3 "$DESTDIR/usr/bin/" # move binary file to location where it can be used
cp -r build/lib/*.a "$DESTDIR/usr/lib/in3-dev/" # moving lib files
cp -r include/in3/* "$DESTDIR/usr/local/include/in3-dev/" # moving include files