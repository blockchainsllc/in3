#!/bin/sh

cd ../build
cmake -LAH .. |  grep '^[^-]\|^$' | awk -f ../c/docs/options.awk > ../c/docs/1_install.md