#!/bin/bash
function quit {
    echo $1 >&2
    exit
}
cd ../scripts > /dev/null 2>&1 || quit '[e] Error: scripts folder not found.'
#echo '[~] Task: BUIDLing release libs'
#./build_release.sh 3>&1 1>/dev/null 2>&3 3>&- | sed 's/^/   - [!] /' || quit '[e] Error: failed building release.'
#cp ../build/lib/libin3.dylib ../python/in3/libin3/shared/libin3.x64.dylib || cp ../build/lib/libin3.so ../python/in3/libin3/shared/libin3.x64.so || cp ../build/lib/libin3.dll ../python/in3/libin3/shared/libin3.x64.dll
#echo '   - [.] Finished: Release libs copied to python/in3/libin3/shared'
echo '[~] Task: BUIDLing debug libs'
./build_debug.sh  3>&1 1>/dev/null 2>&3 3>&- | sed 's/^/   - [!] /' || quit '[e] Error: failed building debug.'
cp ../build/lib/libin3.dylib ../python/in3/libin3/shared/libin3.x64d.dylib || cp ../build/lib/libin3.so ../python/in3/libin3/shared/libin3.x64d.so || cp ../build/lib/libin3.dll ../python/in3/libin3/shared/libin3.x64d.dll
echo '   - [.] Finished: Debug libs copied to python/in3/libin3/shared'
echo '[Ω] Happy Hacking'