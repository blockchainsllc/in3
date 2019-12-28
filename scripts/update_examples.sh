#!/bin/sh

DOC="../../docs/2_examples.md"
README="README.md"

cd ../examples/c

printf "# Examples\n\n" > $DOC
printf "# Examples\n\n" > $README

for f in *.c; 
  do 
    printf "### ${f%%.*}\n\nsource : [in3-c/examples/c/$f](https://github.com/slockit/in3-c/blob/master/examples/c/$f)\n\n" >> $DOC
    cat $f | grep ^/// | sed "s/\/\/\/ //g" >> $DOC
    printf "\n\n\`\`\`c\n" >> $DOC
    cat $f >> $DOC
    printf "\n\`\`\`\n\n" >> $DOC

    printf "\n-  [${f%%.*}](./$f)\n   " >> $README
    cat $f | grep ^/// | sed "s/\/\/\/ //g" >> $README
done

cat ../../docs/build_examples.md_ >> $DOC
cat ../../docs/build_examples.md_ >> $README

