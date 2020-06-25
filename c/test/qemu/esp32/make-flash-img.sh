#!/usr/bin/env bash
set -e
arg_projname=$1
arg_flashimg=$2

if [ -z "$1" -o -z "$2" ]; then
    echo "Usage: make-flash-img.sh app_name flash_img_file"
    exit 1
fi

dd if=/dev/zero bs=1024 count=4096 of=${arg_flashimg}
dd if=build/bootloader/bootloader.bin bs=1 seek=$((0x1000)) of=${arg_flashimg} conv=notrunc
dd if=build/partition_table/partition-table.bin bs=1 seek=$((0x8000)) of=${arg_flashimg} conv=notrunc
dd if=build/${arg_projname}.bin bs=1 seek=$((0x10000)) of=${arg_flashimg} conv=notrunc
