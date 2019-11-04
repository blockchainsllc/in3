#!/bin/bash

TARGET_JS="in3.js"

# wrap code in a function in order to not polute the global namespace.
printf "const IN3=(function(){\nvar in3w = {};\n" > $TARGET_JS
# add emscripten gluecode
cat in3w.js >> $TARGET_JS
# add custom code the defines the public export. This code will have access to all the internal functions of the gluecode!
# it should also overwrite the module.exports to use the wrapper-class.
cat "$1/in3.js" >> $TARGET_JS
cat "$1/in3_util.js" >> $TARGET_JS
cat "$1/in3_eth_api.js" >> $TARGET_JS
# we return the default export
echo " return IN3; })();" >> $TARGET_JS

# add a simple demo page.
cp "$1/demo.html" . 

# create package
mkdir -p ../module
cp ../../LICENSE "$1/package.json" $1/README.md ../module/
cp in3.js  ../module/index.js
cp "$1/in3.d.ts"  ../module/index.d.ts
