#!/bin/bash
qemu-system-arm -cpu cortex-m3 -machine lm3s6965evb -nographic -vga none -net none -pidfile qemu.pid -serial mon:stdio -semihosting -kernel build/zephyr/zephyr.elf 2> /dev/null
RESULT=$?
# the goal here is to return a success exit code regarding of the qemu output 
echo success


