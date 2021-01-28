#!/bin/bash

# This script parses the input file looking for macros that use the `JSON_TO_BIN` marker.
# It then converts the JSON string contained in such macros to their equivalent binary data, 
# and outputs the commented input followed by 2 definitions (i.e. one for bin data array
# and one for length).
# 
# Example - 
#
# // header.h (before)
# #define BOOT_NODES_LOCAL JSON_TO_BIN("{"                                                                \
#                                      " \"nodeRegistry\": {"                                             \
#                                      "   \"contract\": \"0xf0fb87f4757c77ea3416afe87f36acaa0496c7e9\"," \
#                                      "   \"nodeList\": [{"                                              \
#                                      "    \"address\": \"0x784bfa9eb182c3a02dbeb5285e3dba92d717e07a\"," \
#                                      "    \"url\": \"http://localhost:8545\","                          \
#                                      "    \"props\": \"0xFFFF\""                                        \
#                                      "   }]"                                                            \
#                                      " }"                                                               \
#                                      "}")
#
# // header.h (after)
# /*
# #define BOOT_NODES_LOCAL JSON_TO_BIN("{"                                                                \
#                                      " \"nodeRegistry\": {"                                             \
#                                      "   \"contract\": \"0xf0fb87f4757c77ea3416afe87f36acaa0496c7e9\"," \
#                                      "   \"nodeList\": [{"                                              \
#                                      "    \"address\": \"0x784bfa9eb182c3a02dbeb5285e3dba92d717e07a\"," \
#                                      "    \"url\": \"http://localhost:8545\","                          \
#                                      "    \"props\": \"0xFFFF\""                                        \
#                                      "   }]"                                                            \
#                                      " }"                                                               \
#                                      "}")
# */
# unsigned char BOOT_NODES_LOCAL_BIN[] = {
#     0xc8, 0x61, 0x5f, 0x35, 0x62, 0xce, 0x12, 0x14, 0xf0, 0xfb, 0x87, 0xf4,
#     0x75, 0x7c, 0x77, 0xea, 0x34, 0x16, 0xaf, 0xe8, 0x7f, 0x36, 0xac, 0xaa,
#     0x04, 0x96, 0xc7, 0xe9, 0x1a, 0xa2, 0x41, 0x63, 0xb2, 0xf6, 0x14, 0x78,
#     0x4b, 0xfa, 0x9e, 0xb1, 0x82, 0xc3, 0xa0, 0x2d, 0xbe, 0xb5, 0x28, 0x5e,
#     0x3d, 0xba, 0x92, 0xd7, 0x17, 0xe0, 0x7a, 0x79, 0x6b, 0x35, 0x68, 0x74,
#     0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f,
#     0x73, 0x74, 0x3a, 0x38, 0x35, 0x34, 0x35, 0x00, 0x41, 0x6e, 0xbd, 0xff,
#     0xff};
# unsigned int BOOT_NODES_LOCAL_BIN_LEN = 85;

if [ $# -ne 1 ]; then
  echo "Usage: ./macro2bin.sh <input-file.{c,h}>"
  exit 1
fi

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
