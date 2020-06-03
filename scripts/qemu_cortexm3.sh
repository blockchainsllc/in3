#!/bin/bash
# qemu invocation: execute in3 for cortex m3 get receipt and process the out to test_out.txt
qemu-system-arm -cpu cortex-m3 -machine lm3s6965evb -nographic -vga none -net none -pidfile qemu.pid -serial mon:stdio -semihosting -kernel build/zephyr/zephyr.elf 2> /dev/null > test_out.txt
# search for the result of the test
grep -i PASSED ./test_out.txt
out="$?"
if [ "$out" -eq 1 ] ; then 
    echo "TEST FAILED"
    exit 1
else
    echo "TEST SUCCESS!"
    exit 0
fi


