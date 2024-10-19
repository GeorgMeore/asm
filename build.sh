#!/bin/sh

gcc -g -fsanitize=address,undefined -Wall -Wextra -o asm asm.c
