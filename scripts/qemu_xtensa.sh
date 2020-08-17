#!/bin/bash

xtensa_run() {
    # kill xtensa processes 
    pids=$(ps aux | grep "[q]emu-system-xtensa" |awk '{print $2}')
    for pi in $pids; do kill -9  $pi; done
    # run qemu with timeout 
    timeout --foreground 100s xtensa-softmmu/qemu-system-xtensa -nographic -semihosting -machine esp32 -drive file=flash_image.bin,if=mtd,format=raw -nographic -vga none -net none  -global driver=timer.esp32.timg,property=wdt_disable,value=true -no-reboot | tee test.txt
    # search for the result of the test
    if [ -f ./test.txt ]; then
        grep "IN3 TEST PASSED" ./test.txt
        out="$?"
        if [ "$out" -eq 0 ] ; then 
            return 0
        else
            return 1
        fi
    else    
            return 1
    fi
}

retry() {
    local -r -i max_attempts="$1"; shift
    local -i attempt_num=0
    echo $max_attempts
    until [ ${attempt_num} -ge ${max_attempts} ]
    do
        xtensa_run
        local res=$?
        if [ "$res" -eq 0 ] ; then
            echo "TEST SUCCESS"
            exit 0
        else 
            echo "TEST FAILED... RETRYING"
        fi 
        sleep $(( attempt_num++ ))
        
    done
}
retry 3