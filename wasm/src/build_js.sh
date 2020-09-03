#!/bin/bash
function replace_var {
  echo "REPLACE $1 with $2"
  sed -i "s/$(echo $1 | sed -e 's/\([[\/.*]\|\]\)/\\&/g')/$(echo $2 | sed -e 's/[\/&]/\\&/g')/g" $3
}

TARGET_JS="in3.js"

# wrap code in a function in order to not polute the global namespace.
printf "const IN3=(function(){\nvar in3w = {};\n" > $TARGET_JS
# add emscripten gluecode
cat in3w.js | sed "s/uncaughtException/ue/g" >> $TARGET_JS
# add custom code the defines the public export. This code will have access to all the internal functions of the gluecode!
# it should also overwrite the module.exports to use the wrapper-class.
cat "$1/in3.js" >> $TARGET_JS
cat "$1/in3_util.js" >> $TARGET_JS

__CONFIG__=""
typedefs=""
__API__=""
for m in $3 $4 $5 $6 $7 $8 $9
do
    cat "$1/modules/$m.js" >> $TARGET_JS
    if test -f "$1/modules/$m.d.ts"; then
      if grep -q "${m}_config" "$1/modules/$m.d.ts"; then
          __CONFIG__="$__CONFIG__\n   /** config for $m */\n    $m?:${m}_config"
      fi
      conf=`grep API  $1/modules/$m.d.ts | grep public | grep "$m:"`
      [[ ! -z "$conf" ]] && __API__="$__API__\n   /** $m API */\n    $conf"
      typedefs="$typedefs $1/modules/$m.d.ts"
    fi
done


#cat "$1/in3_ipfs_api.js" >> $TARGET_JS
#cat "$1/in3_btc_api.js" >> $TARGET_JS

# we return the default export
echo " return IN3; })();" >> $TARGET_JS
#echo "//# sourceMappingURL=index.js.map" >> $TARGET_JS

# create package
mkdir -p ../module
cp ../../LICENSE.AGPL "$1/package.json" $1/../README.md ../module/
cp in3.js  ../module/index.js

cat "$1/in3.d.ts" | awk -v "r=$__CONFIG__" '{gsub(/__CONFIG__/,r)}1' | awk -v "r=$__API__" '{gsub(/__API__/,r)}1'  > ../module/index.d.ts
for f in $typedefs; do 
  cat $f >>  ../module/index.d.ts
done


if [ -e in3w.wasm ]
 then cp in3w.wasm  ../module/
fi
if [ $2 == "true" ]
 then
   cat "$1/package.json" | sed  's/wasm/asmjs/g' > ../module/package.json
   cat "$1/../README.md" | sed  's/wasm/asmjs/g' > ../module/README.md
fi
if [ -d "$1/../test/in3" ] 
  then 
     rm -rf "$1/../test/in3"
fi
cp -r ../module "$1/../test/in3"