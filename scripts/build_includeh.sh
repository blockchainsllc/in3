#!/bin/bash
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
      #echo "$file" $fdir
      #echo "============================"
      # extract and copy public includes
      gcc -MM `basename $file` | tail -n +2  | while read -r incl;
      do
	headers=`echo "$incl" | sed "s/[\]//g"` 
	for header in $headers;
	do
            cp  "$header" "$projectdir/include/in3/$fdir/$header"
	done
      done
    fi
    done' none {} \;
done <include/in3/.dirs
find include -type d -empty -delete
mv include/in3/src/* include/in3/ 
rm include/in3/.dirs 
rm -r include/in3/src
