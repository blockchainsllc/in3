#!/bin/bash
cd ../c
rm -rf include
mkdir -p include/in3
find src -type d >include/in3/.dirs
while read path; do
  # find headers in path
  find $path -name "*.h" -maxdepth 1 -exec bash -c 'for file in "$1";
    do
    # check if public header
    if grep -q "@PUBLIC_HEADER" "$file"; then
      cp "$file" "include/in3/`basename $file`"
      projectdir=`pwd`
      fdir=`dirname "$file"`
      cd "$fdir"
      # extract and copy public includes
      gcc -MM `basename $file` | tail -n +1  | while read -r incl;
      do
        headers=`echo "$incl" | sed "s/[\]//g"`
        for header in $headers;
        do
          output="$projectdir/include/in3/`basename $header`"
          cp "$header" "$output" 2>/dev/null
          sed -i -E "s/include \"\(.*\)\/\(.*h\)\"/include \"\2\"/" "$output" 2>/dev/null
        done
        rm $projectdir/include/in3/*.h-E
      done
    fi
    done' none {} \;
done <include/in3/.dirs
rm include/in3/.dirs

# create rust binding header
cat <<EOF >../c/include/in3.rs.h
// AUTO-GENERATED FILE
// See scripts/build_includeh.sh
#include "../src/core/client/context_internal.h"
#include "../signer/pk-signer/signer-priv.h"
#include "../signer/pk-signer/signer.h"
#include "in3/bytes.h"
#include "in3/client.h"
#include "in3/context.h"
#include "in3/error.h"
#include "in3/eth_api.h"
#include "in3/in3_curl.h"
#include "in3/in3_init.h"
EOF
