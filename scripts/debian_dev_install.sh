#!/bin/bash
### This file is meant to be used by the debian package manager, It moves the binary file to the destination needed by debian

set -ex # tells script to exit in case of a non-zero return value
#mkdir -p "$DESTDIR/usr/bin" # create directory in case it doesn't exist, should always exist though
mkdir -p "$DESTDIR/usr/lib/in3" # create directory to move lib files
#mv build/bin/in3 build/bin/in3-dev
#cp build/bin/in3-dev "$DESTDIR/usr/bin/in3-dev" # move binary file to location where it can be used
cp -r build/lib/* "$DESTDIR/usr/lib/in3/" # moving archive files to libarary
