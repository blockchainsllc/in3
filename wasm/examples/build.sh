#!/bin/sh
if [ -d ../../build/module ]; then
  npm i typescript web3 bignumber.js
  cp -r ../../build/module node_modules/in3
else
  # build ...
  npm i typescript web3 bignumber.js
fi

node_modules/.bin/tsc --pretty -t ES2019 -m commonjs --outDir build *.ts
