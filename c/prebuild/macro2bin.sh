#!/bin/bash

HEADERFILE=$1
MACRO=0
MULTILINE=0
DEF=""
NEWDEF=""

while IFS="" read -r p || [ -n "$p" ]; do
  if [[ $p == "#define "*" JSON_TO_BIN("* ]]; then
    # macro start
    NEWDEF=$(echo $p | sed -e 's/#define \(.*\) JSON_TO_BIN\(.*\)/\1/')
    if [[ $p != *"\\" ]]; then
      # one-liner
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

    # use old-style comments for multi-liners
    printf '/*\n'
    MACRO=1
  fi

  # detect multi-line comment
  if [[ $p == *"\\" ]]; then
    MULTILINE=1
  else
    MULTILINE=0
  fi

  # handle multi-line macros
  if [[ $MACRO == 1 ]]; then
    printf '%s\n' "$p"
    if [[ $MULTILINE == 0 ]]; then
      # multi-line macro ends
      printf '*/\n'
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
      # macro lines
      temp=${p##*(}
      temp=$(echo "$temp" | tr -d '[:space:]')
      temp=${temp:0:$((${#temp} - 1))}
      temp=$(echo "$temp" | sed -e 's/^"//' -e 's/"$//')
      DEF="$DEF$temp"
    fi
  else
    # reproduce non-macro lines as is
    printf '%s\n' "$p"
  fi
done <$HEADERFILE

rm -f *.bin
