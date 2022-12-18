#!/bin/sh

# we check if incubed is installed, if not we try to use the local incubed-directory.
if [ ! -d /usr/local/include/in3 ]; then

  echo "Using local incubed build..."
  if [ ! -d ../../build/lib ]; then
     # not installed yet, so let'sa build it locally
     mkdir -p ../../build
     cd ../../build
     cmake .. && make 
     cd ../examples
  fi

  BUILDARGS="-L../../build/lib/  -I../../include/ -lin3 -lm"

  # if you want to staticly link, uncomment the next lines

  # do we need to add zk_crypto?
  #  if [ -f ../../build/rust/zkcrypto/release/libzk_crypto.a ]; then
  #   ZKCRYPTO="../../build/rust/zkcrypto/release/libzk_crypto.a -lm -ldl"
  #  fi

  # set the library path to use the local
  #  BUILDARGS="-v -L../../build/lib/  -I../../include/ ../../build/lib/libin3.a $ZKCRYPTO -ltransport_curl -lcurl -pthread"
else
  BUILDARGS="-lin3 -lm"
fi
# now build the examples build
for f in *.c; 
  do
    if [ "$f" = ledger_sign.c ]; then # skipping ledger_sign compilation as it requires specific dependencies 
      continue
    fi
    gcc -std=c99 -o "${f%%.*}" $f $BUILDARGS -D_POSIX_C_SOURCE=199309L
done

