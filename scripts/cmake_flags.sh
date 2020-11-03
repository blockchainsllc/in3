opts=( "-DLOGGING" "-DUSE_CURL" "-DJAVA" "-DEVM_GAS" "-DTEST" "-DZKSYNC" "-DIN3API" "-DIN3_LIB" "-DCMD" )
len=$((${#opts[@]}))
echo $len
max=$((1<<len))
types=("MINSIZEREL" "release" "debug")
(
for ((t=0;t<3;t++));
do
    for ((mask=0;mask<$max;mask++));
    do
    opt_str=""
    echo "MASK:   "$mask
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
    log_out="${mask}_cmake_test.txt"
    cmake ${opt_str} .. 2>>../$log_out
    # if cmake fails record the error to a file
    if [ $? -eq 1 ]
        then
            echo "cmake failed ${opt_str}"
            echo "test: ${mask} ${opt_str}">>../$log_out
            #break
        else 
            rm -f ../$log_out 
        fi
    #make -j 8
    
    done
done
)



