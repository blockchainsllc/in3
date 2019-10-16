#!/bin/bash

TARGET_JS="in3.js"

# wrap code in a function in order to not polute the global namespace.
printf "const IN3=(function(){ var wasmdata=\"" > $TARGET_JS
# append the base64 encoded wasm-data in the variable wasmdata
base64 -i in3w.wasm | tr -d '\n' >> $TARGET_JS
# for nodejs we can use Buffer to decode it se the wasmBinary, so the emscripten gluecod will not load it from filesystem.
printf "\";\nvar in3w = {}; if (typeof Buffer !=='undefined')  in3w.wasmBinary=Buffer.from(wasmdata,'base64');\n" >> $TARGET_JS
# for the browser we change the url to embed the data, so fetch will not send a request.
# so we replace the url in the gluecode and append it to the bundle.
sed "s/['\"]+in3w.wasm['\"]+/'data:application\/octet-stream;base64,'+wasmdata/g" in3w.js >> $TARGET_JS
# add custom code the defines the public export. This code will have access to all the internal functions of the gluecode!
# it should also overwrite the module.exports to use the wrapper-class.
cat "$1/in3.js" >> $TARGET_JS
# we return the defualt export
echo " return IN3; })();" >> $TARGET_JS

# add a simple demo page.
cp "$1/demo.html" .

# create package
mkdir -p ../module
cp ../../LICENSE "$1/package.json" $1/README.md ../module/
cp in3.js  ../module/index.js
cp "$1/in3.d.ts"  ../module/index.d.ts
