#!/bin/sh
cd ../c/src/third-party/hidapi/
./bootstrap
./configure 
make 
make install