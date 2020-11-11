#!/bin/bash
opt=$1
#cmd args
enable_make=0
case $opt in
    -m|--make)
    enable_make=1
    ;;
    -h|--help)
        echo 'Usage: %s <options> ... 
        -m, --make         enable make compiling of the cmake project, (this will take a long time 2 days aprox)
        -h, --help         test common in3 cmake definitions combinations. current flags: -DLOGGING -DUSE_CURL -DJAVA -DEVM_GAS -DTEST -DZKSYNC -DIN3API -DIN3_LIB -DCMD"'
        exit 0
    ;;
esac

opts=( "-DLOGGING" "-DUSE_CURL" "-DJAVA" "-DEVM_GAS" "-DTEST" "-DZKSYNC" "-DIN3API" "-DIN3_LIB" "-DCMD" )
len=$((${#opts[@]}))
max=$((1<<len))
types=("MINSIZEREL" "release" "debug")
(
for ((t=0;t<3;t++));
do
    for ((mask=0;mask<$max;mask++));
    do
        opt_str=""
        for ((i=0;i<$len;i++));
        do

            if ((((1<<i)&mask)>0));
            then
                opt_str="${opt_str} ${opts[i]}=true"
            else
                opt_str="${opt_str} ${opts[i]}=false"
            fi
        done
        opt_str="${opt_str} -DCMAKE_BUILD_TYPE=${types[t]}"
        cd ..
        mkdir build || echo "using existng build-folder ..."
        rm -rf build/*
        cd build
        log_out="${types[t]}_${mask}_cmake_test.txt"
        echo "Current mask: " $mask "Current cmake definitions state: " ${opt_str}
        cmake ${opt_str} .. 2>>../$log_out
        # if cmake fails record the error to a file
        if [ $? -eq 1 ]
            then
                echo "cmake failed ${opt_str}"
                echo "test: ${mask} ${opt_str}">../"err_$log_out"
        fi
        if [ $enable_make -eq 1 ]
            then 
            make -j 8
        fi
    
    done
done
)



