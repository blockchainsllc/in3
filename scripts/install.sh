#!/bin/bash
set -ex
mkdir -p "$DESTDIR/usr/bin"
cp build/bin/in3 "$DESTDIR/usr/bin/in3"

