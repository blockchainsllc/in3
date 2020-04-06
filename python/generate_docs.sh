#!/bin/bash

cd docs
pydocmd build
printf "# API Reference Python\n\n" > documentation.md
cd _build/pydocmd
cat index.md examples.md  >> ../../documentation.md
cat in3.md eth.md libin3.md | sed 's/# /## /'  >> ../../documentation.md
cd ..
source ../scripts/update_examples.sh