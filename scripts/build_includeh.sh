#!/bin/sh
cd ..
rm -rf include
mkdir -p include/in3
find src -type d >include/in3/.dirs
cd include/in3
xargs mkdir -p <.dirs
cd ../..
while read path; do
  # find headers in path
  find $path -name "*.h" -maxdepth 1 -exec bash -c 'for file in "$1";
    do
    # check if public header
    if grep -q "@PUBLIC_HEADER" "$file"; then
      cp "$file" "include/in3/$file"
      projectdir=`pwd`
      fdir=`dirname "$file"`
      cd "$fdir"
      # echo "$fdir"
      # extract and copy public includes
      grep "#include \"" `basename "$file"` | while read incl
      do
        header=`echo "$incl" | sed -e "s/.*include \"\(.*\)\".*/\1/"`
        cp "$header" "$projectdir/include/in3/$fdir/$header"
      done
    fi
    done' none {} \;
done <include/in3/.dirs
rm include/in3/.dirs
