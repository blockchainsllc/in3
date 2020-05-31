#!/bin/bash
xtensa-softmmu/qemu-system-xtensa -nographic -semihosting -machine esp32 -drive file=flash_image.bin,if=mtd,format=raw -nographic -vga none -net none  -global driver=timer.esp32.timg,property=wdt_disable,value=true -no-reboot 2> /dev/null 
# search for the result of the test
grep "IN3 TEST PASSED" ./test_out.txt
out="$?"
if [ "$out" -eq 1 ] ; then 
    echo "TEST FAILED"
    exit 1
else
    echo "TEST SUCCESS!"
    exit 0
fi
