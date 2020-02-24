#!/bin/sh

# C

DOC="../../c/docs/2_examples.md"
README="README.md"

cd ../c/examples

printf "# Examples\n\n" > $DOC
printf "# Examples\n\n" > $README

for f in *.c; 
  do 
    printf "### ${f%%.*}\n\nsource : [in3-c/c/examples/$f](https://github.com/slockit/in3-c/blob/master/c/examples/$f)\n\n" >> $DOC
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
    printf "### ${f%%.*}\n\nsource : [in3-c/java/examples/$f](https://github.com/slockit/in3-c/blob/master/java/examples/$f)\n\n" >> $DOC
    cat $f | grep ^/// | sed "s/\/\/\/ //g" >> $DOC
    printf "\n\n\`\`\`java\n" >> $DOC
    cat $f >> $DOC
    printf "\n\`\`\`\n\n" >> $DOC

    printf "\n-  [${f%%.*}](./$f)\n   " >> $README
    cat $f | grep ^/// | sed "s/\/\/\/ //g" >> $README
done

cat ../../java/docs/build_examples.md_ >> $DOC
cat ../../java/docs/build_examples.md_ >> $README
cd ../../scripts



# PYTHON

DOC="../../python/docs/2_examples.md"
README="README.md"

cd ../python/examples

printf "## Examples\n\n" > $DOC
printf "# Examples\n\n" > $README

for f in *.py; 
  do 
    printf "### ${f%%.*}\n\nsource : [in3-c/python/examples/$f](https://github.com/slockit/in3-c/blob/master/python/examples/$f)\n\n" >> $DOC
    cat $f | grep ^/// | sed "s/### //g" >> $DOC
    printf "\n\n\`\`\`python\n" >> $DOC
    cat $f >> $DOC
    printf "\n\`\`\`\n\n" >> $DOC

    printf "\n-  [${f%%.*}](./$f)\n   " >> $README
    cat $f | grep ^/// | sed "s/### //g" >> $README
done

cat ../../python/docs/build_examples.md_ >> $DOC
cat ../../python/docs/build_examples.md_ >> $README
cd ../../scripts


# WASM

DOC="../../wasm/docs/2_examples.md"
README="README.md"

cd ../wasm/examples

printf "## Examples\n\n" > $DOC
printf "# Examples\n\n" > $README

for f in *.js *.ts; 
  do 
    printf "### ${f%%.*}\n\nsource : [in3-c/wasm/examples/$f](https://github.com/slockit/in3-c/blob/master/wasm/examples/$f)\n\n" >> $DOC
    cat $f | grep ^/// | sed "s/\/\/\/ //g" >> $DOC
    printf "\n\n\`\`\`js\n" >> $DOC
    cat $f >> $DOC
    printf "\n\`\`\`\n\n" >> $DOC

    printf "\n-  [${f%%.*}](./$f)\n   " >> $README
    cat $f | grep ^/// | sed "s/\/\/\/ //g" >> $README
done
for f in *.html; 
  do 
    printf "### ${f%%.*}\n\nsource : [in3-c/wasm/examples/$f](https://github.com/slockit/in3-c/blob/master/wasm/examples/$f)\n\n" >> $DOC
    cat $f | grep "^<!--" | sed "s/<!-- \(.*\)-->/\\1/g" >> $DOC
    printf "\n\n\`\`\`html\n" >> $DOC
    cat $f >> $DOC
    printf "\n\`\`\`\n\n" >> $DOC

    printf "\n-  [${f%%.*}](./$f)\n   " >> $README
    cat $f | grep "^<!--" | sed "s/<!-- \(.*\)-->/\\1/g" >> $README
done

cat ../../wasm/docs/build_examples.md_ >> $DOC
cat ../../wasm/docs/build_examples.md_ >> $README

