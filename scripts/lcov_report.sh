#!/bin/sh
llvm-profdata-16 merge -o all.profdata -sparse *.profraw **/*.profraw
echo "--instr-profile"
echo "all.profdata"
for f in test/test*
do
 echo "-object=$PWD/$f"
done
echo "-ignore-filename-regex='test/*'"
echo "-ignore-filename-regex='rpcs.h'"
echo "-ignore-filename-regex='src/third-party/*'"
echo "-ignore-filename-regex='src/transport/*'"
echo "-ignore-filename-regex='src/tools/*'"
echo "-ignore-filename-regex='src/verifier/sol/*'"
echo "-ignore-filename-regex='src/signer/cryptocell/*'"
echo "-ignore-filename-regex='src/signer/ledger-nano/*'"
