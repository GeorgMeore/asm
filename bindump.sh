#!/bin/sh

objdump -D -Mintel,x86-64 -b binary -m i386 "$@"
