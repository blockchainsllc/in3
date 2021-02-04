#!/bin/sh
CWD=$PWD
cd $(dirname $0)/../build
make
cd ../python
mkdir -p in3/libin3/shared
cp ../build/lib/libin3.dylib in3/libin3/shared/libin3.x64.dylib
pip3 install -r requirements.txt
coverage run -m pytest --pylama --junitxml=report.xml
cd $CWD
