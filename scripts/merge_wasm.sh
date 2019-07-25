#!/bin/bash

TARGET_JS="in3ws.js"

printf "var wasmdata=\"" > $TARGET_JS
base64 -i in3w.wasm | tr -d '\n' >> $TARGET_JS
printf "\";\nvar in3w = {}; if (typeof Buffer !=='undefined')  in3w.wasmBinary=Buffer.from(wasmdata,'base64');\n" >> $TARGET_JS
sed 's/in3w.wasm/data:application\/octet-stream;base64,"+wasmdata+"/g' in3w.js >> $TARGET_JS
cat "$1/in3.js" >> $TARGET_JS
cp "$1/demo.html" .
