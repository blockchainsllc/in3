#!/bin/bash
cd ..
mkdir -p build
cd build
rm -rf *
key="$1"
case $key in
    -s|--size)
    in3size="$2"
    case $in3size in
        nano)
        echo "nano"
        cmake -DCMAKE_BUILD_TYPE=MINSIZEREL -DIN3_MATH_LITE=true -DIN3_LIB=false -DIN3_NANO=true -DETH_BASIC=false -DETH_FULL=false -DIN3_SERVER=true -DPOA=false .. && make -j8
        ;;
        basic)
        echo "basic"
        cmake -DCMAKE_BUILD_TYPE=MINSIZEREL -DIN3_MATH_LITE=true -DIN3_LIB=false -DIN3_NANO=false -DETH_BASIC=true -DETH_FULL=false -DIN3_SERVER=true -DPOA=false .. && make -j8
        ;;
        full)
        echo "full"
        cmake -DCMAKE_BUILD_TYPE=MINSIZEREL -DIN3_MATH_LITE=true -DETH_FULL=true -DIN3_SERVER=true -DPOA=false .. && make -j8
        ;;
    esac
    ;;
    -h|--help)
        echo 'Usage: %s <options> ... 
        -s, --size         compilation size for in3 . (nano, basic, full)
        -h, --help         usage help'
    ;;
esac
cd ../scripts