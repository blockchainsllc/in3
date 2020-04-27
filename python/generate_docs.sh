#!/bin/bash
source ../scripts/update_examples.sh
cd ../python/docs || exit
pydocmd build
printf "# API Reference Python\n\n" > documentation.md
cd _build/pydocmd || exit
cat index.md examples.md  >> ../../documentation.md
sed 's/# /## /' in3.md | sed 's/## in3$/## Penis/g' >> ../../documentation.md
#sed 's/## in3$/## Client\n/g' eth.md >> ../../documentation.md
#sed 's/## in3$/## Client\n/g' libin3.md >> ../../documentation.md
#cat in3.md eth.md libin3.md | sed 's/# /## /'  >> ../../documentation.md



#      - Client: in3.md
#      - Ethereum API: eth.md
#- Libin3 Runtime: libin3.md
