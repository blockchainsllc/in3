#!/bin/sh

# C

DOC="../../c/docs/2_examples.md"
README="README.md"

cd ../c/examples

printf "# Examples\n\n" > $DOC
printf "# Examples\n\n" > $README

for f in *.c; 
  do 
    printf "### ${f%%.*}\n\nsource : [in3-c/examples/c/$f](https://github.com/slockit/in3-c/blob/master/c/examples/$f)\n\n" >> $DOC
    cat $f | grep ^/// | sed "s/\/\/\/ //g" >> $DOC
    printf "\n\n\`\`\`c\n" >> $DOC
    cat $f >> $DOC
    printf "\n\`\`\`\n\n" >> $DOC

    printf "\n-  [${f%%.*}](./$f)\n   " >> $README
    cat $f | grep ^/// | sed "s/\/\/\/ //g" >> $README
done

cat ../../c/docs/build_examples.md_ >> $DOC
cat ../../c/docs/build_examples.md_ >> $README
cd ../../scripts



# JAVA

DOC="../../java/docs/2_examples.md"
README="README.md"

cd ../java/examples

printf "# Examples\n\n" > $DOC
printf "# Examples\n\n" > $README

for f in *.java; 
  do 
    printf "### ${f%%.*}\n\nsource : [in3-c/examples/java/$f](https://github.com/slockit/in3-c/blob/master/java/examples/$f)\n\n" >> $DOC
    cat $f | grep ^/// | sed "s/\/\/\/ //g" >> $DOC
    printf "\n\n\`\`\`java\n" >> $DOC
    cat $f >> $DOC
    printf "\n\`\`\`\n\n" >> $DOC

    printf "\n-  [${f%%.*}](./$f)\n   " >> $README
    cat $f | grep ^/// | sed "s/\/\/\/ //g" >> $README
done

cat ../../java/docs/build_examples.md_ >> $DOC
cat ../../java/docs/build_examples.md_ >> $README

