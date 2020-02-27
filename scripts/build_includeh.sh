#!/bin/bash
cd ../c
rm -rf include
mkdir -p c/include/in3
find c -type d >c/include/in3/.dirs
while read path; do
  # find headers in path
  find $path -name "*.h" -maxdepth 1 -exec bash -c 'for file in "$1";
    do
    # check if public header
    if grep -q "@PUBLIC_HEADER" "$file"; then
      cp "$file" "c/include/in3/`basename $file`"
      projectdir=`pwd`
      fdir=`dirname "$file"`
      cd "$fdir"
      # extract and copy public includes
      gcc -MM `basename $file` | tail -n +1  | while read -r incl;
      do
        headers=`echo "$incl" | sed "s/[\]//g"`
        for header in $headers;
        do
          output="$projectdir/c/include/in3/`basename $header`"
          cp "$header" "$output" 2>/dev/null
          sed -i -E "s/include \"\(.*\)\/\(.*h\)\"/include \"\2\"/" "$output" 2>/dev/null
        done
        rm $projectdir/c/include/in3/*.h-E
      done
    fi
    done' none {} \;
done <c/include/in3/.dirs
rm c/include/in3/.dirs
