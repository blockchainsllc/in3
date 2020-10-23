#!/bin/bash

HEADERFILE=$1
MACRO=0
MULTILINE=0
DEF=""
NEWDEF=""

while IFS="" read -r p || [ -n "$p" ]; do
  if [[ $p == "#define "*" JSON_TO_BIN("* ]]; then
    NEWDEF="${p%JSON_TO_BIN*}"
    if [[ $p != *"\\" ]]; then
      DEF=${p##*(}
      DEF=$(echo "${p##*(}" | tr -d '[:space:]' | sed -e 's/^"//' -e 's/"$//')
      DEF=${DEF:0:$((${#DEF} - 2))}
      printf '// %s\n' "$p"
      echo -n "$NEWDEF"
      DEF=$(echo -n "$DEF" | sed 's/\\//g' | json)
      DEF=${DEF:0:$((${#DEF} - 1))}
      echo '"'"${DEF}"'"'
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
      echo -n "$NEWDEF"
      DEF=$(echo -n "$DEF" | sed 's/\\//g' | json)
      echo '"'"${DEF}"'"'
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
