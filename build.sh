#!/bin/sh

c++ -g -fsanitize=address,undefined -Wall -Wextra -o asmcc asm.cc
