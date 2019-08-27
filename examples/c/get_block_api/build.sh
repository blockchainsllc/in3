#!/bin/sh

# build
gcc -o in3_example example.c -L../../../build/lib/ -I../../../include -lin3_bundle -lcurl

# run
./in3_example