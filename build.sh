#!/bin/sh -ex

CXXFLAGS="-g -fsanitize=address,undefined -Wall -Wextra"

c++ -c $CXXFLAGS arena.cc &
c++ -c $CXXFLAGS asm.cc &
c++ -c $CXXFLAGS amd64.cc &
wait
ar crs libasm.a arena.o asm.o amd64.o
c++ $CXXFLAGS -o test test.cc libasm.a &
c++ -L . -I . $CXXFLAGS -o examples/fib examples/fib.cc libasm.a &
c++ -L . -I . $CXXFLAGS -o examples/link examples/link.cc libasm.a &
wait
./test
