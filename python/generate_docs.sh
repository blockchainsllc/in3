#!/bin/bash
source ../scripts/update_examples.sh
cd ../python/docs || exit

pydocmd build
printf "# API Reference Python\n\n" > documentation.md

cd _build/pydocmd || exit
cat index.md examples.md  >> ../../documentation.md

# shellcheck disable=SC2129
sed 's/# /## /' in3.md |
sed 's/^## in3$/## Incubed Modules/g' |
sed 's/^### in3$//g' >> ../../documentation.md

sed 's/# /## /' account.md |
sed 's/^## in3.eth.account$//g' >> ../../documentation.md

sed 's/# /## /' contract.md |
sed 's/^## in3.eth.contract$//g' >> ../../documentation.md

sed 's/# /## /' eth.md |
sed 's/^## in3.eth$//g' |
sed 's/^## in3.eth.model$//g' |
sed 's/^### in3.eth.model$/### Ethereum Objects/g' >> ../../documentation.md

sed 's/# /## /' libin3.md |
sed 's/^## in3.libin3$/## Library Runtime/g' |
sed 's/^### in3.libin3.lib_loader$/### Library Loader/g' >> ../../documentation.md

cd ../..
rm compiled_examples.md