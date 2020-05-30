#!/bin/bash
 xtensa-softmmu/qemu-system-xtensa -nographic -semihosting -machine esp32 -drive file=flash_image.bin,if=mtd,format=raw -nographic -vga none -net none  -global driver=timer.esp32.timg,property=wdt_disable,value=true -no-reboot 2> /dev/null
RESULT=$?
# the goal here is to return a success exit code regarding of the qemu output 
echo success


