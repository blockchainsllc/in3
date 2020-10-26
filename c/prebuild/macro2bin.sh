#!/bin/bash

HEADERFILE=$1
MACRO=0
MULTILINE=0
DEF=""
NEWDEF=""

while IFS="" read -r p || [ -n "$p" ]; do
  if [[ $p == "#define "*" JSON_TO_BIN("* ]]; then
    NEWDEF=$(echo $p | sed -e 's/#define \(.*\) JSON_TO_BIN\(.*\)/\1/')
    if [[ $p != *"\\" ]]; then
      DEF=${p##*(}
      DEF=$(echo "${p##*(}" | tr -d '[:space:]' | sed -e 's/^"//' -e 's/"$//')
      DEF=${DEF:0:$((${#DEF} - 2))}
      printf '// %s\n' "$p"
      echo -n "${NEWDEF} "
      DEF=$(echo -n "$DEF" | sed 's/\\//g' | json)
      printf "0x${DEF}" | xxd -r -p >"${NEWDEF}"".bin"
      xxd -i -a -C "${NEWDEF}"".bin"
      DEF=""
      NEWDEF=""
      continue
    fi
    MACRO=1
  fi

  if [[ $p == *"\\" ]]; then
    MULTILINE=1
  else
    MULTILINE=0
  fi

  if [[ $MACRO == 1 ]]; then
    printf '// %s\n' "$p"
    if [[ $MULTILINE == 0 ]]; then
      MACRO=0
      temp="$p"
      temp=$(echo "$temp" | tr -d '[:space:]' | sed -e 's/^"//' -e 's/"$//')
      temp=${temp:0:$((${#temp} - 2))}
      DEF="$DEF$temp"
      DEF=$(echo -n "$DEF" | sed 's/\\//g' | json)
      printf "0x${DEF}" | xxd -r -p >"${NEWDEF}"".bin"
      xxd -i -a -C "${NEWDEF}"".bin"
      DEF=""
      NEWDEF=""
    else
      temp=${p##*(}
      temp=$(echo "$temp" | tr -d '[:space:]')
      temp=${temp:0:$((${#temp} - 1))}
      temp=$(echo "$temp" | sed -e 's/^"//' -e 's/"$//')
      DEF="$DEF$temp"
    fi
  else
    printf '%s\n' "$p"
  fi
done <$HEADERFILE

rm -f *.bin
