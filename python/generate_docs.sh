#!/bin/bash
source ../scripts/update_examples.sh
cd ../python/docs || exit
pydocmd build
printf "# API Reference Python\n\n" > documentation.md
cd _build/pydocmd || exit
cat index.md examples.md  >> ../../documentation.md
printf "## Incubed Modules\n\n" > documentation.md
# shellcheck disable=SC2129
sed 's/# /## /' in3.md | sed 's/### in3$/### Incubed Client/g' >> ../../documentation.md
sed 's/# /## /' eth.md | sed 's/### in3.eth$/### Ethereum API/g' >> ../../documentation.md
sed 's/# /## /' account.md | sed 's/### in3.eth.account$/### Ethereum Account API/g' >> ../../documentation.md
sed 's/# /## /' contract.md | sed 's/### in3.eth.contract$/### Ethereum Smart-Contract API/g' >> ../../documentation.md
sed 's/# /## /' libin3.md | sed 's/### in3.libin3$/### Incubed Library Runtime/g' >> ../../documentation.md
cd ../..
rm compiled_examples.md