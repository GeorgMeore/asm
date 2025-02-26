#!/bin/sh

[ -f "$1" ] || exit

objcopy -S --rename-section .data=.text --add-symbol _start=.text:0 -I binary -O elf64-x86-64 "$1" "${1%%.*}.o"
ld -o "${1%%.*}.elf" "${1%%.*}.o"
