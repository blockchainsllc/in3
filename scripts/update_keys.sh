#!/bin/sh
DST=../src/cmd/tools/used_keys.h
echo "static char* USED_KEYS[] = {" > $DST
cat ../src/core/client/keys.h | grep "key(" | sed 's/.*key("\(.*\)")/    "\1",/' >> $DST
echo "    0};" >> $DST
