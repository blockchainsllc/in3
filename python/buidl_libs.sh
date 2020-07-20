#!/bin/bash
function quit {
    echo $1 >&2
    exit
}
cd ../scripts > /dev/null 2>&1 || quit '[e] Error: scripts folder not found.'
echo '[~] Task: BUIDLing release libs'
./build.sh bindings 3>&1 1>/dev/null 2>&3 3>&- | sed 's/^/   - [!] /' || quit '[e] Error: failed building release.'
mkdir -p "../python/in3/libin3/shared"
cp ../build/lib/libin3.dylib ../python/in3/libin3/shared/libin3.x64.dylib || cp ../build/lib/libin3.so ../python/in3/libin3/shared/libin3.x64.so || cp ../build/lib/libin3.dll ../python/in3/libin3/shared/libin3.x64.dll || quit '[e] Error: Failed trying to create ..python/in3/libin3/shared folder.'
echo '   - [.] Finished: Release libs copied to python/in3/libin3/shared'
echo '[~] Task: BUIDLing debug libs'
./build.sh bindings-debug  3>&1 1>/dev/null 2>&3 3>&- | sed 's/^/   - [!] /' || quit '[e] Error: failed building debug.'
cp ../build/lib/libin3.dylib ../python/in3/libin3/shared/libin3.x64d.dylib || cp ../build/lib/libin3.so ../python/in3/libin3/shared/libin3.x64d.so || cp ../build/lib/libin3.dll ../python/in3/libin3/shared/libin3.x64d.dll || quit '[e] Error: Failed trying to create ..python/in3/libin3/shared folder.'
echo '   - [.] Finished: Debug libs copied to python/in3/libin3/shared'
echo '[Ω] Happy Hacking'