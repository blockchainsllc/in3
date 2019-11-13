#!/bin/sh

cd ../build
cmake -LAH .. |  grep '^[^-]\|^$' | awk -f ../docs/options.awk > ../docs/1_install.md