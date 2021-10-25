#!/bin/sh
MAC_FILE=../build/lib/libin3.dylib
LINUX_FILE=../build/lib/libin3.so

CWD=$PWD
cd $(dirname $0)/../build
make
cd ../python
mkdir -p in3/libin3/shared
if test -f "$LINUX_FILE"; then
    cp "$LINUX_FILE" in3/libin3/shared/libin3.x64.so
else 
    cp "$MAC_FILE" in3/libin3/shared/libin3.x64.dylib
fi

pip3 install -r requirements.txt
coverage run -m pytest --pylama --junitxml=report.xml
cd $CWD
