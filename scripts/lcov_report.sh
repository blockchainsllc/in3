#!/bin/sh
llvm-profdata merge -o all.profdata -sparse *.profraw **/*.profraw
echo "--instr-profile"
echo "all.profdata"
for f in test/test*
do
 echo "-object=$PWD/$f"
done
echo "-ignore-filename-regex='c/test/*'"
echo "-ignore-filename-regex='src/third-party/*'"
