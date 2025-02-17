#!/bin/sh

cc -g -fsanitize=address,undefined -Wall -Wextra -o asmc asm.c
c++ -g -fsanitize=address,undefined -Wall -Wextra -o asmcc asm.cc
